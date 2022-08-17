#include "lua_vm.h"
#include "../common/lua_string.h"
#include "lua_do.h"
#include "lua_gc.h"
#include "lua_opcodes.h"
#include "lua_func.h"


/* slot == NULL意味着t不是table类型 */
void luaV_finishget(struct lua_State* L, TValue* t, StkId val, const TValue* slot) {
    if (slot == NULL) {
        luaD_throw(L, LUA_ERRRUN);
    } else {
        setnilvalue(val);
    }
}

void luaV_finishset(struct lua_State* L, TValue* t, const TValue* key, StkId val, const TValue* slot) {   
    Table* h = hvalue(t);
    if (slot == NULL) {
        luaD_throw(L, LUA_ERRRUN);
    }

    if (slot == luaO_nilobject) {
        slot = luaH_newkey(L, h, key);
    }

    setobj(cast(TValue*, slot), val);
    luaC_barrierback(L, h, slot);

}

int luaV_eqobject(struct lua_State* L, const TValue* a, const TValue* b) {
    if ((ttisfloat(a) && lua_numisnan(a->value_.n)) || 
            (ttisfloat(b) && lua_numisnan(b->value_.n))) {
        return 0;
    }

    if (a->tt_ != b->tt_) {
        /* 只有数值，在类型不同的情况下可能相等 */
        if (ttisnumber(a) && ttisnumber(b)) {
            double fa = ttisinteger(a) ? a->value_.i : a->value_.n;
            double fb = ttisinteger(b) ? b->value_.i : b->value_.n;
            return fa == fb;
        } else {
            return 0;
        }
    }    

    switch(ttype(a)) {
        case LUA_TNIL: return 1;
        case LUA_TNUMFLT: return a->value_.n == b->value_.n;
        case LUA_TNUMINT: return a->value_.i == b->value_.i;
        case LUA_TSHRSTR: return luaS_eqshrstr(L, gco2ts(gcvalue(a)), gco2ts(gcvalue(b)));
        case LUA_TLNGSTR: return luaS_eqlngstr(L, gco2ts(gcvalue(a)), gco2ts(gcvalue(b)));
        case LUA_TBOOLEAN: return a->value_.b == b->value_.b; 
        case LUA_TLIGHTUSERDATA: return a->value_.p == b->value_.p; 
        case LUA_TLCF: return a->value_.f == b->value_.f;
        default: {
            return gcvalue(a) == gcvalue(b);
        } break;
    } 

    return 0;
}


// implement of instructions
static void newframe(struct lua_State* L);

static void op_loadk(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int bx = GET_ARG_Bx(i);
	setobj(ra, &cl->p->k[bx]);
}

static void op_gettabup(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	TValue* upval = cl->upvals[b]->v;
	struct Table* t = gco2h(gcvalue(upval));
	int arg_c = GET_ARG_C(i);
	//luaGG_DumpTable(L, t);
	if (ISK(arg_c)) {
		int index = arg_c - MAININDEXRK - 1;
		TValue* key = &cl->p->k[index];
		TValue* value = (TValue*)luaH_get(L, t, key);
		setobj(ra, value);
	}
	else {
		TValue* value = L->ci->l.base + arg_c;
		setobj(ra, value);
	}
}

/* CFunction，CClosure和LClosure都可以 */
static void op_call(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int narg = GET_ARG_B(i);
	int nresult = GET_ARG_C(i) - 1;
	if (narg > 0) {
		L->top = ra + narg;
	}

	if (luaD_precall(L, ra, nresult)) { // c function
		if (nresult >= 0) {
			L->top = L->ci->top;
		}
	} else {  // closure
		newframe(L);
	}
}

static void op_return(struct lua_State* L, LClosure* cl, StkId ra, Instruction i) {
	int b = GET_ARG_B(i);
	luaD_poscall(L, ra, b ? (b - 1) : (int)(L->top - ra));
	if (L->ci->callstatus & CIST_LUA) {
		L->top = L->ci->top;
		lua_assert(GET_OPCODE(*(L->ci->savedpc - 1)) == OP_CALL);
	}
}

void luaV_execute(struct lua_State* L) {
	luaK_printInstruction(L->ci->l.savedpc);
	L->ci->callstatus |= CIST_FRESH;
	newframe(L);
}

static Instruction vmfetch(struct lua_State* L) {
	Instruction i = *(L->ci->l.savedpc++);
	return i;
}

/* 获取R(A)代表的栈位置 */
static StkId vmdecode(struct lua_State* L, Instruction i) {
	StkId ra = L->ci->l.base + GET_ARG_A(i);
	return ra;
}

static bool vmexecute(struct lua_State* L, StkId ra, Instruction i) {
	bool is_loop = true;
	struct GCObject* gco = gcvalue(L->ci->func);
	LClosure* cl = gco2lclosure(gco);

	switch (GET_OPCODE(i)) {
	case OP_GETTABUP: {
		op_gettabup(L, cl, ra, i);
	} break;
	case OP_LOADK: {
		op_loadk(L, cl, ra, i);
	} break;
	case OP_CALL: {
		op_call(L, cl, ra, i);
	} break;
	case OP_RETURN: {
		op_return(L, cl, ra, i);
		is_loop = false;
	} break;
	default:break;
	}

	return is_loop;
}

static void newframe(struct lua_State* L) {
	bool is_loop = true;
	while (is_loop) {
		Instruction i = vmfetch(L);
		StkId ra = vmdecode(L, i);
		is_loop = vmexecute(L, ra, i);
	}
}

/* v类型是integer或float，把v的实际数值赋予i指向的lua_Number */
int luaV_tonumber(struct lua_State* L, const TValue* v, lua_Number* n) {
	int result = 0;
	if (ttisinteger(v)) {
		*n = (lua_Number)v->value_.i;
		result = 1;
	} else if (ttisfloat(v)) {
		*n = v->value_.n;
		result = 1;
	}
	return result;
}

/* v类型是integer或float，把v的实际数值赋予i指向的lua_Integer */
int luaV_tointeger(struct lua_State* L, const TValue* v, lua_Integer* i) {
	int result = 0;
	if (ttisinteger(v)) {
		*i = v->value_.i;
		result = 1;
	} else if (ttisfloat(v)) {
		*i = (lua_Integer)v->value_.n;
		result = 1;
	}
	return result;
}
