#ifndef luadebug_h
#define luadebug_h

#include "../common/lua_state.h"
#include "../common/lua_object.h"
#include "../vm/lua_opcodes.h"


#define IS_RETURN_DESC(c) ((c)==OpCodeDesc[4])
#define GET_OPCODE_DESC(i) (OpCodeDesc[GET_OPCODE(i)])

extern char* OpCodeDesc[];
extern TValue* luaL_index2addr(struct lua_State* L, int idx);
extern void luaH_DumpTable(struct lua_State* L, Table* t);
extern void luaO_Print(const TValue* t);
extern void luaK_printInstruction(const Instruction* i);

#endif // !luadebug_h

