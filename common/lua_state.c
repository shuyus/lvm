#include "lua_prefix.h"

#include "lua_state.h"
#include "lua_mem.h"
#include "lua_string.h"
#include "lua_table.h"
#include "../vm/lua_gc.h"
#include "../vm/lua_vm.h"
#include "../compiler/lua_lexer.h"

#define fromstate(L) (cast(LX*, cast(lu_byte*, (L)) - offsetof(LX, l)))

typedef struct LX {
    lu_byte extra_[LUA_EXTRASPACE];
    lua_State l;
} LX;

typedef struct LG {
    LX l;
    global_State g;
} LG;

#define addbuff(b, t, p) \
    { memcpy(b, t, sizeof(t)); p += sizeof(t); }

static unsigned int makeseed(struct lua_State* L) {
    char buff[4 * sizeof(size_t)];
    unsigned int h = (unsigned int)time(NULL);
    int p = 0;

    addbuff(buff, L, p);
    addbuff(buff, &h, p);
    addbuff(buff, luaO_nilobject, p);
    addbuff(buff, &lua_newstate, p);

    return luaS_hash(L, buff, p, h);
}

static void init_registry(struct lua_State* L) {
    struct global_State* g = G(L);

    struct Table* t = luaH_new(L);
    sethvalue(&g->l_registry, t);
    luaH_resize(L, t, 2, 0);

    setgcovalue(&t->array[LUA_MAINTHREADIDX], obj2gco(g->mainthread));
    setgcovalue(&t->array[LUA_GLOBALTBLIDX], obj2gco(luaH_new(L)));
}

static void stack_init(struct lua_State* L) {
    L->stack = (StkId)luaM_realloc(L, NULL, 0, LUA_STACKSIZE * sizeof(TValue));
    L->stack_size = LUA_STACKSIZE;
    L->stack_last = L->stack + LUA_STACKSIZE - LUA_EXTRASTACK;
    L->next_s = L->previous_s = NULL;
    L->status = LUA_OK;
    L->errorjmp = NULL;
    L->top = L->stack;
    L->errorfunc = 0;

    int i;
    for (i = 0; i < L->stack_size; i++) {
        setnilvalue(L->stack + i);
    }
    L->top++;

    L->ci = &L->base_ci;
    L->ci->func = L->stack;
    L->ci->top = L->stack + LUA_MINSTACK;
    L->ci->previous = L->ci->next = NULL;
}

struct lua_State* lua_newstate(lua_Alloc alloc, void* ud) {
    struct global_State* g;
    struct lua_State* L;
    
    struct LG* lg = (struct LG*)(*alloc)(ud, NULL, LUA_TTHREAD, sizeof(struct LG));
    if (!lg) {
        return NULL;
    }
    g = &lg->g;
    g->ud = ud;
    g->frealloc = alloc;
    g->panic = NULL;
    
    L = &lg->l.l;
	L->nci = 0;
    L->tt_ = LUA_TTHREAD;
    G(L) = g;
    g->mainthread = L;

    L->ncalls=0;

    g->seed = makeseed(L);

    // gc init
    g->gcstate = GCSpause;
    g->currentwhite = bitmask(WHITE0BIT);
    g->totalbytes = sizeof(LG);
    g->allgc = NULL;
    g->sweepgc = NULL;
    g->gray = NULL;
    g->grayagain = NULL;
    g->fixgc = NULL;
    g->GCdebt = 0;
    g->GCmemtrav = 0;
    g->GCestimate = 0;
    g->GCstepmul = LUA_GCSTEPMUL;
    

    

    L->marked = luaC_white(g);
    L->gclist = NULL;

    stack_init(L);
    luaS_init(L);
    init_registry(L);

    luaX_init(L);
    return L;
}


static void free_stack(struct lua_State* L) {
    global_State* g = G(L);
    luaM_free(L, L->stack, sizeof(TValue));
    L->stack = L->stack_last = L->top = NULL;
    L->stack_size = 0;
}

void lua_close(struct lua_State* L) {
    struct global_State* g = G(L);
    struct lua_State* L1 = g->mainthread; // only mainthread can be close
	luaC_freeallobjects(L);
    
    struct CallInfo* base_ci = &L1->base_ci;
    struct CallInfo* ci = base_ci->next;

    while(ci) {
        struct CallInfo* next = ci->next;
        struct CallInfo* free_ci = ci;

        luaM_free(L, free_ci, sizeof(struct CallInfo));
        ci = next;
    }


    free_stack(L1);    
    (*g->frealloc)(g->ud, fromstate(L1), sizeof(LG), 0);
}
