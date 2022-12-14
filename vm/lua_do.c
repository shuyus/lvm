#include "../common/lua_prefix.h"

#include "lua_do.h"
#include "lua_gc.h"
#include "lua_func.h"
#include "../common/lua_mem.h"
#include "../compiler/lua_parser.h"
#include "../vm/lua_vm.h"

#define LUA_TRY(L, c, a) if (_setjmp((c)->b) == 0) { a } 

#ifdef _WIN32 
#define LUA_THROW(c) longjmp((c)->b, 1) 
#else 
#define LUA_THROW(c) _longjmp((c)->b, 1) 
#endif 

struct lua_longjmp {
    struct lua_longjmp* previous;
    jmp_buf b;
    int status;
};

void seterrobj(struct lua_State* L, int error) {
    lua_pushinteger(L, error);
}

void luaD_checkstack(struct lua_State* L, int need) {
    luaC_checkgc(L);
    if (L->top + need > L->stack_last) {
        luaD_growstack(L, need);
    }
}

/* 栈扩容 */
void luaD_growstack(struct lua_State* L, int size) {
    if (L->stack_size > LUA_MAXSTACK) {
        luaD_throw(L, LUA_ERRERR);
    }

    int stack_size = L->stack_size * 2;
    int need_size = cast(int, L->top - L->stack) + size + LUA_EXTRASTACK;
    if (stack_size < need_size) {
        stack_size = need_size;
    }
    
    if (stack_size > LUA_MAXSTACK) {
        stack_size = LUA_MAXSTACK + LUA_ERRORSTACK;
        LUA_ERROR(L, "stack overflow");
    } 

    TValue* old_stack = L->stack;
    L->stack = luaM_realloc(L, L->stack, L->stack_size, stack_size * sizeof(TValue));
    L->stack_size = stack_size;
    L->stack_last = L->stack + stack_size - LUA_EXTRASTACK;
    int top_diff = cast(int, L->top - old_stack);
    L->top = restorestack(L, top_diff);

    struct CallInfo* ci;
    ci = &L->base_ci;
    while(ci) {
        int func_diff = cast(int, ci->func - old_stack);
        int top_diff = cast(int, ci->top - old_stack);
        ci->func = restorestack(L, func_diff);
        ci->top = restorestack(L, top_diff);
        ci = ci->next;
    }
}

void luaD_throw(struct lua_State* L, int error) {
    struct global_State* g = G(L);
    if (L->errorjmp) {
        L->errorjmp->status = error;
        LUA_THROW(L->errorjmp);
    } else {
        if (g->panic) {
            (*g->panic)(L);
        }
        abort();
    }
}

int luaD_rawrunprotected(struct lua_State* L, Pfunc f, void* ud) {
    int old_ncalls = L->ncalls;
    struct lua_longjmp lj;
    lj.previous = L->errorjmp;
    lj.status = LUA_OK;
    L->errorjmp = &lj;

    LUA_TRY(
        L,
        L->errorjmp,
        (*f)(L, ud);
    )

    L->errorjmp = lj.previous;
    L->ncalls = old_ncalls;
    return lj.status;
}

/* 调用栈切到下一个栈帧 */
static struct CallInfo* next_ci(struct lua_State* L, StkId func, int nresult) {
    struct CallInfo* ci;
	if (L->ci->next) {
        ci = L->ci->next;
    } else {
        ci = luaM_realloc(L, NULL, 0, sizeof(struct CallInfo));
        ci->next = NULL;
        L->nci++;
    }
    ci->previous = L->ci;
    L->ci->next = ci;
    ci->nresult = nresult;
    ci->callstatus = LUA_OK;
    ci->func = func;
    ci->top = L->top + LUA_MINSTACK;
    L->ci = ci;

    return ci;
}

// prepare for function call. 
// if we call a c function, just directly call it
// if we call a lua function, just prepare for call it
/*  nresult从api参数处传递至此  */
int luaD_precall(struct lua_State* L, StkId func, int nresult) {
	lua_CFunction f;
	ptrdiff_t func_diff;

    switch(ttype(func)) {
        case LUA_TCCL: {
			struct GCObject* gco = gcvalue(func);
			CClosure* cc = gco2cclosure(gco);
			f = cc->f;
			goto cfunc;
		} break;
        case LUA_TLCF: {
            f = func->value_.f;
        cfunc:
            func_diff = savestack(L, func);
            luaD_checkstack(L, LUA_MINSTACK);
            func = restorestack(L, func_diff);

            next_ci(L, func, nresult);                        
            int n = (*f)(L);
            lua_assert(L->ci->func + n <= L->ci->top);
            luaD_poscall(L, L->top - n, n);
            return 1; 
        } break;
        case LUA_TLCL: {
			struct GCObject* gco = gcvalue(func);
			LClosure* cl = gco2lclosure(gco);
			int fsize = cl->p->maxstacksize;

			func_diff = savestack(L, func);
			luaD_checkstack(L, fsize);
			func = restorestack(L, func_diff);
			int n = L->top - func - 1; /* 实际参数数量通过func和top的差计算出 */
			for (int i = n; i < cl->p->nparam; i++) {
				setnilvalue(L->top++); /* 缺少的参数补nil */
			}

			next_ci(L, func, nresult);
			L->ci->func = func;
			L->ci->l.base = func + 1; /* base是func上一个位置 */
			L->top = L->ci->top = L->ci->l.base + fsize;
			L->ci->l.savedpc = cl->p->code;
			L->ci->callstatus |= CIST_LUA;
		} break;
		default: {
			LUA_ERROR(L, "attempt to call a unsupport value.");
			luaD_throw(L, LUA_ERRERR);
		} break;
    }

    return 0;
}

/* nresult是函数真实返回的数量 */
int luaD_poscall(struct lua_State* L, StkId first_result, int nresult) {
    StkId func = L->ci->func;
    int nwant = L->ci->nresult;

    switch(nwant) {
        case 0: {
            L->top = L->ci->func;
        } break;
        case 1: {
            if (nresult == 0) {
                first_result->value_.p = NULL;
                setnilvalue(first_result);
            }
            setobj(func, first_result);
            first_result->value_.p = NULL;
            setnilvalue(first_result);

            L->top = func + nwant;
        } break;
        case LUA_MULRET: {
            int nres = cast(int, L->top - first_result);
            int i;
            for (i = 0; i < nres; i++) {
                StkId current = first_result + i;
                setobj(func + i, current);
                current->value_.p = NULL;
                setnilvalue(current);
            }
            L->top = func + nres;
        } break;
        default: {
            if (nwant > nresult) {
                int i;
                for (i = 0; i < nwant; i++) {
                    if (i < nresult) {
                        StkId current = first_result + i;
                        setobj(func + i, current);
                        current->value_.p = NULL;
                        setnilvalue(current);
                    }
                    else {
                        StkId stack = func + i;
                        setnilvalue(stack);
                    }
                }
                L->top = func + nwant;
            } else {
                int i;
                for (i = 0; i < nresult; i++) {
                    if (i < nwant) {
                        StkId current = first_result + i;
                        setobj(func + i, current);
                        current->value_.p = NULL;
                        setnilvalue(current);
                    }
                    else {
                        StkId stack = func + i;
                        stack->value_.p = NULL;
                        setnilvalue(stack);
                    }
                }
                L->top = func + nresult;
            } 
        } break;
    }

    struct CallInfo* ci = L->ci;
    L->ci = ci->previous; 

    return LUA_OK;
}

int luaD_call(struct lua_State* L, StkId func, int nresult) {
    if (++L->ncalls > LUA_MAXCALLS) {
        luaD_throw(L, 0);
    }
   
    if (!luaD_precall(L, func, nresult)) {
        luaV_execute(L);
    }
    
    L->ncalls--;
    return LUA_OK;
}

static void reset_unuse_stack(struct lua_State* L, ptrdiff_t old_top) {
    StkId top = restorestack(L, old_top);
    for (; top < L->top; top++) {
        setnilvalue(top);
    }
}

int luaD_pcall(struct lua_State* L, Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef) {
    int status;
    struct CallInfo* old_ci = L->ci;
    ptrdiff_t old_errorfunc = L->errorfunc;    
    
    status = luaD_rawrunprotected(L, f, ud);
    if (status != LUA_OK) {
        reset_unuse_stack(L, oldtop);
        L->ci = old_ci;
        L->top = restorestack(L, oldtop);
        seterrobj(L, status);
    }
    
    L->errorfunc = old_errorfunc; 
    return status;
}

// skip utf-8 BOM
static int skipBOM(LoadF* lf) {
	const char* bom = "\xEF\xBB\xBF";

	do {
		int c = (int)getc(lf->f);
		if (c == EOF || c != *bom) {
			return c;
		}

		lf->buff[lf->n++] = c;
		bom++;
	} while (*bom != '\0');
	
	lf->n = 0;
	return getc(lf->f);
}

// skip first line comment, if it exist
static int skipcommnet(LoadF* lf, int* c) {
	*c = skipBOM(lf);
	if (*c == '#') { // Unix exec file?
		do {
			*c = getc(lf->f);
		} while (*c != '\n' && *c != EOF);
		return 1;
	}
	return 0;
}



// a struct for tranfer param for f_parser
typedef struct SParser {
	MBuffer buffer;
	Dyndata dyd; 
	Zio* z; 
	char* filename; 
} SParser;

static int f_parser(struct lua_State* L, void* ud) {
	SParser* p = (SParser*)ud;
	LClosure* cl = luaY_parser(L, p->z, &p->buffer, &p->dyd, p->filename);
	if (cl) {
		luaF_initupvals(L, cl);
	}

	return LUA_OK;
}

int luaD_protectedparser(struct lua_State* L, Zio* z, const char* filename) {
	SParser p;
	p.filename = (char*)filename;
	p.z = z;
	p.dyd.actvar.arr = NULL;
	p.buffer.buffer = NULL;
	p.dyd.actvar.size = p.buffer.size = 0;
	p.dyd.actvar.n = p.buffer.n = 0;

	int status = luaD_pcall(L, f_parser, (void*)(&p), savestack(L, L->top), L->errorfunc);
	if (status != LUA_OK) {
		LUA_ERROR(L, "luaD_protectedparser call f_parser failure\n");
		return LUA_ERRERR;
	}

	luaM_free(L, p.dyd.actvar.arr, p.dyd.actvar.size);
	luaM_free(L, p.buffer.buffer, p.buffer.size);
	return LUA_OK;
}

/* 从文件 */
int luaD_load(struct lua_State* L, lua_Reader reader, void* data, const char* filename) {
	LoadF* lf = (LoadF*)data;

	int c = 0;
	skipcommnet(lf, &c);
	if (c == EOF) {
		LUA_ERROR(L, "EOF error!");
		return LUA_ERRERR;
	}

	lf->buff[lf->n++] = c;

	Zio z;
	luaZ_init(L, &z, reader, data);

	if (luaD_protectedparser(L, &z, filename) != LUA_OK) {
		LUA_ERROR(L, "protected parser error!\n");
		return LUA_ERRERR;
	}

	return LUA_OK;
}