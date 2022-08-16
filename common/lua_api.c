#include "lua_prefix.h"

#include "lua.h"
#include "lua_mem.h"
#include "lua_object.h"
#include "lua_state.h"
#include "lua_string.h"
#include "lua_table.h"
#include "lua_api.h"
#include "../vm/lua_vm.h"
#include "../vm/lua_func.h"

TValue* index2addr(struct lua_State* L, int idx) {
    if (idx >= 0) {
        lua_assert(L->ci->func + idx < L->ci->top);
        return L->ci->func + idx;
    } else if (idx == LUA_REGISTRYINDEX) {
		return &G(L)->l_registry;
	} else {
        lua_assert(L->top + idx > L->ci->func);
        return L->top + idx;
    }
}

void lua_pushcfunction(struct lua_State* L, lua_CFunction f) {
    setfvalue(L->top, f);
    increase_top(L); 
}

void lua_pushinteger(struct lua_State* L, lua_Integer integer) {
    setivalue(L->top, integer);
    increase_top(L);
}

void lua_pushnumber(struct lua_State* L, double number) {
    setfltvalue(L->top, number);
    increase_top(L);
}

void lua_pushboolean(struct lua_State* L, bool b) {
    setbvalue(L->top, b);
    increase_top(L);
}

void lua_pushnil(struct lua_State* L) {
    setnilvalue(L->top);
    increase_top(L);
}

void lua_pushlightuserdata(struct lua_State* L, void* p) {
    setpvalue(L->top, p);
    increase_top(L);
}

void lua_pushstring(struct lua_State* L, const char* str) {
    luaC_checkgc(L);
    unsigned int l = strlen(str);
    struct TString* ts = luaS_newlstr(L, str, l);
    settsvalue(L->top, ts);
    increase_top(L);
}

void lua_pushCclosure(struct lua_State* L, lua_CFunction f, int nup) {
	luaC_checkgc(L);
	CClosure* cc = luaF_newCclosure(L, f, nup);
	setcclvalue(L->top, cc);
	increase_top(L);
}

/* 复制一份到栈底 */
int lua_pushvalue(struct lua_State* L, int idx) {
	TValue* o = index2addr(L, idx);
	setobj(L->top, o);
	increase_top(L);
	return LUA_OK;
}

int lua_pushglobaltable(struct lua_State* L) {
    struct global_State* g = G(L);
    struct Table* registry = gco2h(gcvalue(&g->l_registry));
    struct Table* t = gco2h(gcvalue(&registry->array[LUA_GLOBALTBLIDX]));
    setgcovalue(L->top, obj2gco(t));
    increase_top(L);
    return LUA_OK;
}

lua_Integer lua_tointegerx(struct lua_State* L, int idx, int* isnum) {
    lua_Integer ret = 0;
    TValue* addr = index2addr(L, idx); 
    if (ttisinteger(addr)) {
        ret = addr->value_.i;
        *isnum = 1;
    } else {
        *isnum = 0;
        LUA_ERROR(L, "can not convert to integer!\n");
    }

    return ret;
}

lua_Number lua_tonumberx(struct lua_State* L, int idx, int* isnum) {
    lua_Number ret = 0.0f;
    TValue* addr = index2addr(L, idx);
    if (ttisfloat(addr)) {
        *isnum = 1;
        ret = addr->value_.n;
    } else {
        LUA_ERROR(L, "can not convert to number!");
        *isnum = 0;
    }
    return ret;
}

bool lua_toboolean(struct lua_State* L, int idx) {
    TValue* addr = index2addr(L, idx);
    return !(ttisnil(addr) || addr->value_.b == 0);
}

char* lua_tostring(struct lua_State* L, int idx) {
    TValue* addr = index2addr(L, idx);
    if (!ttisstring(addr)) {
        return NULL; 
    }

    struct TString* ts = gco2ts(addr->value_.gc);
    return getstr(ts);
}

struct Table* lua_totable(struct lua_State* L, int idx){
    TValue* o = index2addr(L, idx);
	if (!ttistable(o)) {
		return NULL;
	}
	struct Table* t = gco2h(gcvalue(o));
	return t;
}

int lua_isnil(struct lua_State* L, int idx) {
    TValue* addr = index2addr(L, idx);
    return ttisnil(addr);
}

int lua_gettop(struct lua_State* L) {
    return cast(int, L->top - (L->ci->func + 1));
}

void lua_settop(struct lua_State* L, int idx) {
    StkId func = L->ci->func;
    if (idx >=0) {
        lua_assert(idx <= L->stack_last - (func + 1));
        while(L->top < (func + 1) + idx) {
            setnilvalue(L->top++);
        }
        L->top = func + 1 +idx;
    } else {
        lua_assert(L->top + idx > func);
        L->top = L->top + idx;
    }
}

void lua_pop(struct lua_State* L) {
    lua_settop(L, -1);
}



int lua_createtable(struct lua_State* L) {
    luaC_checkgc(L);
    struct Table* tbl = luaH_new(L);
    sethvalue(L->top, tbl);
    // struct GCObject* gco = obj2gco(tbl);
    // setgcovalue(L->top, gco);
    increase_top(L);
    return LUA_OK;
}  

/* 2:0 设置表键值，top-1为值，top-2为键 */
int lua_settable(struct lua_State* L, int idx) {
    TValue* o = index2addr(L, idx);
	if (!ttistable(o)) {
		return LUA_ERRERR;
	}

    struct Table* t = gco2h(gcvalue(o));
    luaV_settable(L, o, L->top - 2, L->top - 1);
    L->top = L->top - 2;
    return LUA_OK;
}

/* 1:0 设置idx位置的表 键为字符串k的值为 top-1 */
int lua_setfield(struct lua_State* L, int idx, const char* k)
{
	setobj(L->top, L->top - 1);
	increase_top(L);

	TString* s = luaS_newliteral(L, k);
	struct GCObject* gco = obj2gco(s);
	setgcovalue(L->top - 2, gco);

	return lua_settable(L, idx);
}

int lua_gettable(struct lua_State* L, int idx) {
    TValue* o = index2addr(L, idx);
    luaV_gettable(L, o, L->top - 1, L->top - 1);
    return LUA_OK;
}


int lua_getglobal(struct lua_State* L, const char* name) {
	struct GCObject* gco = gcvalue(&G(L)->l_registry);
	struct Table* t = gco2h(gco);
	TValue* _go = &t->array[LUA_GLOBALTBLIDX];
	struct Table* _G = gco2h(gcvalue(_go));
	TValue* o = (TValue*)luaH_getstr(L, _G, luaS_newliteral(L, name));

	setobj(L->top, o);
	increase_top(L);

	return LUA_OK;
}


int lua_remove(struct lua_State* L, int idx) {
	TValue* o = index2addr(L, idx);
	for (; o != L->top; o++) {
		TValue* next = o + 1;
		setobj(o, next);
	}
	L->top--;
	return LUA_OK;
}

int lua_insert(struct lua_State* L, int idx, TValue* v) {
	TValue* o = index2addr(L, idx);
	TValue* top = L->top;
	for (; top > o; top--) {
		TValue* prev = top - 1;
		setobj(top, prev);
	}
	increase_top(L);
	setobj(o, v);

	return LUA_OK;
}
