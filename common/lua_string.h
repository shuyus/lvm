#ifndef luastring_h
#define luastring_h

#include "lua_state.h"
#include "../vm/lua_gc.h"

#define sizelstring(l) (sizeof(TString) + (l + 1) * sizeof(char))
#define getstr(ts) ((ts)->data)
#define tslen(s)	((s)->tt == LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)  /* 从'TString *s' 获取实际字符串长度. */
#define oslen(o)	tslen(tsvalue(o)) /* 从'TValue *o' 获取实际字符串长度 */
#define isreserved(s)	((s)->tt == LUA_TSHRSTR && (s)->extra > 0)  /*检测一个TString是否是关键字*/

#define luaS_newliteral(L, s) luaS_newlstr(L, s, strlen(s))

LUAI_FUNC void luaS_init(struct lua_State* L);
LUAI_FUNC int luaS_resize(struct lua_State* L, unsigned int nsize); 
LUAI_FUNC struct TString* luaS_newlstr(struct lua_State* L, const char* str, unsigned int l);
LUAI_FUNC struct TString* luaS_new(struct lua_State* L, const char* str, unsigned int l);
LUAI_FUNC void luaS_remove(struct lua_State* L, struct TString* ts); 

LUAI_FUNC void luaS_clearcache(struct lua_State* L);
LUAI_FUNC int luaS_eqshrstr(struct lua_State* L, struct TString* a, struct TString* b);
LUAI_FUNC int luaS_eqlngstr(struct lua_State* L, struct TString* a, struct TString* b);

LUAI_FUNC unsigned int luaS_hash(struct lua_State* L, const char* str, unsigned int l, unsigned int h);
LUAI_FUNC unsigned int luaS_hashlongstr(struct lua_State* L, struct TString* ts);
LUAI_FUNC struct TString* luaS_createlongstr(struct lua_State* L, const char* str, size_t l);

#endif 
