
#include "../clib/lua_aux.h"
#include "lua_base.h"

const lua_Reg reg[] = {
	{ "_G", luaB_openbase },
	{ NULL, NULL },
};

void luaL_openlibs(struct lua_State* L) {
	for (int i = 0; i < sizeof(reg) / sizeof(reg[0]); i++) {
		lua_Reg r = reg[i];
		if (r.name != NULL && r.func != NULL) {
			luaL_requiref(L, r.name, r.func, 1);
		}
	}
}