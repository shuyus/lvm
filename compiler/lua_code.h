#ifndef luacode_h_
#define luacode_h_

#include "../common/lua.h"
#include "lua_lexer.h"
#include "lua_parser.h"

#define get_instruction(fs, e) (fs->p->code[e->u.info])
#define NO_JUMP -1
#define NO_REG MAXARG_A

#define luaK_codeAsBx(fs, c, a, sbx) luaK_codeABx(fs, c, a, sbx + LUA_IBIAS)

/*
** Ensures final expression result is either in a register or it is
** a constant.
*/
LUAI_FUNC void luaK_exp2val(FuncState* fs, expdesc* e);
LUAI_FUNC int luaK_exp2RK(FuncState* fs, expdesc* e);   // discharge expression to constant table or specific register
LUAI_FUNC int luaK_stringK(FuncState* fs, TString* key); // generate an expression to index constant in constants vector
LUAI_FUNC int luaK_nilK(FuncState* fs);
LUAI_FUNC int luaK_floatK(FuncState* fs, lua_Number r);
LUAI_FUNC int luaK_intK(FuncState* fs, lua_Integer i);
LUAI_FUNC int luaK_boolK(FuncState* fs, int v);
LUAI_FUNC int luaK_ret(FuncState* fs, int first, int nret);

LUAI_FUNC void luaK_indexed(FuncState* fs, expdesc* e, expdesc* key); // generate an expression to index a key which is in a local table or a upvalue table

LUAI_FUNC int luaK_codeABC(FuncState* fs, int opcode, int a, int b, int c);
LUAI_FUNC int luaK_codeABx(FuncState* fs, int opcode, int a, int bx);

LUAI_FUNC void luaK_dischargevars(FuncState* fs, expdesc* e);
LUAI_FUNC int luaK_exp2nextreg(FuncState* fs, expdesc* e);    // discharge expression to next register 
LUAI_FUNC int luaK_exp2anyreg(FuncState* fs, expdesc * e); 

LUAI_FUNC void luaK_prefix(FuncState* fs, int op, expdesc* e);

#endif
