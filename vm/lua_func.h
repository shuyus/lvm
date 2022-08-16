#ifndef luafunc_h
#define luafunc_h

#include "../common/lua_object.h"

#define sizeofLClosure(n) (sizeof(LClosure) + sizeof(TValue*) * ((n) - 1))
#define sizeofCClosure(n) (sizeof(CClosure) + sizeof(TValue*) * ((n) - 1))
#define upisopen(up) (up->v != &up->u.value)

struct UpVal {
	TValue* v;  // point to stack or its own value (when open)
	int refcount;
	union {
		struct UpVal* next; // next open upvalue
		TValue value;		// its value (when closed)
	} u;
};

LUAI_FUNC Proto* luaF_newproto(struct lua_State* L);
LUAI_FUNC void luaF_freeproto(struct lua_State* L, Proto* f);
LUAI_FUNC lu_mem luaF_sizeproto(struct lua_State* L, Proto* f);

LUAI_FUNC LClosure* luaF_newLclosure(struct lua_State* L, int nup);
LUAI_FUNC void luaF_freeLclosure(struct lua_State* L, LClosure* cl);

LUAI_FUNC CClosure* luaF_newCclosure(struct lua_State* L, lua_CFunction func, int nup);
LUAI_FUNC void luaF_freeCclosure(struct lua_State* L, CClosure* cc);

LUAI_FUNC void luaF_initupvals(struct lua_State* L, LClosure* cl);

#endif