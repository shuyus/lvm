#ifndef luatable_h
#define luatable_h

#include "lua_state.h"
#define MAXABITS (sizeof(int) * CHAR_BIT - 1)

#define isdummy(t) ((t)->lastfree == NULL)
#define getkey(n) (cast(const TValue*, &(n)->key.tvk)) 
#define getwkey(n) (cast(TValue*, &(n)->key.tvk))
#define getval(n) (cast(TValue*, &(n)->value))
#define getnode(t, i) (&(t)->node[i])

#define hashint(key, t) getnode((t), lmod(key, twoto((t)->lsizenode))) 
#define hashstr(ts, t) getnode((t), lmod(ts->hash, twoto((t)->lsizenode)))
#define hashboolean(key, t) getnode((t), lmod(key, twoto((t)->lsizenode)))
#define hashpointer(p, t) getnode((t), lmod(point2uint(p), twoto((t)->lsizenode)))

LUAI_FUNC struct Table* luaH_new(struct lua_State* L);
LUAI_FUNC void luaH_free(struct lua_State* L, struct Table* t);

LUAI_FUNC const TValue* luaH_getint(struct lua_State* L, struct Table* t, lua_Integer key);
LUAI_FUNC int luaH_setint(struct lua_State* L, struct Table* t, int key, const TValue* value);

LUAI_FUNC const TValue* luaH_getshrstr(struct lua_State* L, struct Table* t, struct TString* key);
LUAI_FUNC const TValue* luaH_getstr(struct lua_State* L, struct Table* t, struct TString* key);

LUAI_FUNC const TValue* luaH_get(struct lua_State* L, struct Table* t, const TValue* key);
LUAI_FUNC TValue* luaH_set(struct lua_State* L, struct Table* t, const TValue* key);

LUAI_FUNC int luaH_resize(struct lua_State* L, struct Table* t, unsigned int asize, unsigned int hsize);
LUAI_FUNC TValue* luaH_newkey(struct lua_State* L, struct Table* t, const TValue* key);


LUAI_FUNC int luaH_next(struct lua_State* L, struct Table* t, TValue* key);
LUAI_FUNC int luaH_getn(struct lua_State* L, struct Table* t);

#endif