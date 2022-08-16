#ifndef sydebug_h
#define sydebug_h

#include "../common/lua_state.h"
#include "../common/lua_object.h"

extern TValue* luaL_index2addr(struct lua_State* L, int idx);
extern void luaGG_DumpTable(struct lua_State * L, Table * t);
extern void luaGG_Print(const TValue * t);
#endif

// setgco setgcovalue