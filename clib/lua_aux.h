#ifndef luaaux_h
#define luaaux_h 

#include "../common/lua.h"

typedef struct lua_Reg {
	const char* name;
	lua_CFunction func;
}lua_Reg;

LUALIB_API struct lua_State* luaL_newstate();
LUALIB_API void luaL_close(struct lua_State* L);

LUALIB_API void luaL_openlibs(struct lua_State* L);  // open standard libraries
LUALIB_API int luaL_requiref(struct lua_State* L, const char* name, lua_CFunction func, int glb); // try to load module into _LOADED table


LUALIB_API void luaL_pushinteger(struct lua_State* L, int integer);
LUALIB_API void luaL_pushnumber(struct lua_State* L, double number);
LUALIB_API void luaL_pushlightuserdata(struct lua_State* L, void* userdata);
LUALIB_API void luaL_pushnil(struct lua_State* L);
LUALIB_API void luaL_pushcfunction(struct lua_State* L, lua_CFunction f);
LUALIB_API void luaL_pushboolean(struct lua_State* L, bool boolean);
LUALIB_API void luaL_pushstring(struct lua_State* L, const char* str);
LUALIB_API int luaL_pcall(struct lua_State* L, int narg, int nresult);

LUALIB_API bool luaL_checkinteger(struct lua_State* L, int idx);
LUALIB_API lua_Integer luaL_tointeger(struct lua_State* L, int idx);
LUALIB_API lua_Number luaL_tonumber(struct lua_State* L, int idx);
LUALIB_API void* luaL_touserdata(struct lua_State* L, int idx);
LUALIB_API bool luaL_toboolean(struct lua_State* L, int idx);
LUALIB_API char* luaL_tostring(struct lua_State* L, int index);
LUALIB_API int luaL_isnil(struct lua_State* L, int idx);

LUALIB_API void luaL_pop(struct lua_State* L);
LUALIB_API int luaL_stacksize(struct lua_State* L);

LUALIB_API int luaL_createtable(struct lua_State* L); 
LUALIB_API int luaL_settable(struct lua_State* L, int idx);
LUALIB_API int luaL_gettable(struct lua_State* L, int idx);
LUALIB_API int luaL_getglobal(struct lua_State* L);
LUALIB_API int luaL_getsubtable(struct lua_State* L, int idx, const char* name); // try to get a sub table by name and push into the stack

// load source and compile, if load success, then it returns 1, otherwise it returns 0 
LUALIB_API int luaL_loadfile(struct lua_State* L, const char* filename);

#endif
