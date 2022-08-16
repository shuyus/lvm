#include "lua_prefix.h"

#include "lua_mem.h"
#include "../vm/lua_do.h"

#define MINARRAYSIZE 4

/* *size为块数量 element为块容量 limit为扩容后块最高数量 */
void* luaM_growaux_(struct lua_State* L, void* ptr, int* size, int element, int limit) {
	void* newblock = NULL;
	int block_size = *size;
	if (block_size > limit / 2) {
		if (block_size > limit) {
			LUA_ERROR(L, "luaM_growaux_ size too big");
			luaD_throw(L, LUA_ERRMEM);
		}

		block_size = limit;
	} else {
		block_size *= 2;
	}

	if (block_size <= MINARRAYSIZE) {
		block_size = MINARRAYSIZE;
	}

	newblock = luaM_realloc(L, ptr, *size * element, block_size * element);
	*size = block_size;
	return newblock;
}


void* luaM_realloc(struct lua_State* L, void* ptr, size_t osize, size_t nsize) {
    struct global_State* g = G(L);
    int oldsize = ptr ? osize : 0;

    void* ret = (*g->frealloc)(g->ud, ptr, oldsize, nsize);
    if (ret == NULL && nsize > 0) { //addon
        luaD_throw(L, LUA_ERRMEM);
    }

    g->GCdebt = g->GCdebt - oldsize + nsize;
    return ret;
}
