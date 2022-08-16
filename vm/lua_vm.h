#ifndef luavm_h
#define luavm_h

#include "../common/lua_state.h"
#include "../common/lua_table.h"

#define luaV_fastget(L, t, k, get, slot) \
    (!ttistable(t) ? (slot = NULL, 0) : \
     (slot = get(L, hvalue(t), k), !ttisnil(slot)))

#define luaV_gettable(L, t, k, v) { const TValue* slot = NULL; \
    if (luaV_fastget(L, t, k, luaH_get, slot)) { setobj(v, cast(TValue*, slot));  } \
    else luaV_finishget(L, t, v, slot); }


#define luaV_fastset(L, t, k, v, get, slot) \
    (!ttistable(t) ? (slot = NULL, 0) : \
     (slot = cast(TValue*, get(L, hvalue(t), k)), \
        (ttisnil(slot) ? (0) : \
            (setobj_exp(slot, v), \
            luaC_barrierback(L, hvalue(t), slot), 1))))

#define luaV_settable(L, t, k, v) \
    { TValue* slot = NULL; \
    if (!luaV_fastset(L, t, k, v, luaH_get, slot)) \
        luaV_finishset(L, t, k, v, slot);}

LUAI_FUNC void luaV_finishget(struct lua_State* L, TValue* t, StkId val, const TValue* slot);
LUAI_FUNC void luaV_finishset(struct lua_State* L, TValue* t, const TValue* key, StkId val, const TValue* slot);
LUAI_FUNC int luaV_eqobject(struct lua_State* L, const TValue* a, const TValue* b);

LUAI_FUNC void luaV_execute(struct lua_State* L);
LUAI_FUNC int luaV_tonumber(struct lua_State* L, const TValue* v, lua_Number* n);
LUAI_FUNC int luaV_tointeger(struct lua_State* L, const TValue* v, lua_Integer* i);

#endif