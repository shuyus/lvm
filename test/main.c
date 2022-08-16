#include "../clib/lua_aux.h"
#include "../common/lua_table.h"
#include "../vm/lua_gc.h"
#include "../vm/lua_vm.h"
#include "../common/lua_string.h"
#include "../debugg/sy_debug.h"

int panic(lua_State *L){
    printf("abort~!!!!\n");
	return 0;
}



void main() {
	struct lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	int ok = luaL_loadfile(L, "C:\\Users\\shuyu\\Desktop\\step\\compiler_vc_2\\test\\t.lua");
	if (ok == LUA_OK) {
		luaL_pcall(L, 0, 0);
	}

	luaL_close(L);
}


