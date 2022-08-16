#ifndef luado_h
#define luado_h 

#include "../common/lua_state.h"
#include "../compiler/lua_zio.h"

#define savestack(L, o) ((o) - (L)->stack)
#define restorestack(L, o) ((L)->stack + (o)) 

#define CIST_LUA 1
#define CIST_FRESH (1 << 1)

typedef int (*Pfunc)(struct lua_State* L, void* ud);

LUAI_FUNC void seterrobj(struct lua_State* L, int error);
LUAI_FUNC void luaD_checkstack(struct lua_State* L, int need);
LUAI_FUNC void luaD_growstack(struct lua_State* L, int size);
LUAI_FUNC void luaD_throw(struct lua_State* L, int error);

LUAI_FUNC int luaD_rawrunprotected(struct lua_State* L, Pfunc f, void* ud);
LUAI_FUNC int luaD_precall(struct lua_State* L, StkId func, int nresult);
LUAI_FUNC int luaD_poscall(struct lua_State* L, StkId first_result, int nresult);
LUAI_FUNC int luaD_call(struct lua_State* L, StkId func, int nresult);
LUAI_FUNC int luaD_pcall(struct lua_State* L, Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef);

LUAI_FUNC int luaD_load(struct lua_State* L, lua_Reader reader, void* data, const char* filename); // load lua source from file, and parser it
LUAI_FUNC int luaD_protectedparser(struct lua_State* L, Zio* z, const char* filename);


#endif 