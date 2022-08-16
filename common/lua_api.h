#ifndef luaapi_h
#define luaapi_h

#define increase_top(L)   {L->top++; api_check(L, L->top <= L->ci->top, \
		        "stack overflow");}

LUAI_FUNC TValue* index2addr(struct lua_State* L, int idx);

LUAI_FUNC int lua_remove(lua_State* L, int idx);		// remove the value indexed by 'idx', and all the values beyond it will be moved down
LUAI_FUNC int lua_insert(lua_State* L, int idx,  TValue* v); // insert o into the position of stack indexed by idx, and all the values beyond it will be moved up

#endif