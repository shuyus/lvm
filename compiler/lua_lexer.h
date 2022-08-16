#ifndef lualexer_h
#define lualexer_h

#include "../common/lua.h"
#include "../common/lua_object.h"
#include "lua_zio.h"

#define FIRST_REVERSED 257

// 1~256 should reserve for ASCII character token
enum RESERVED {
	/* terminal token donated by reserved word */
	TK_LOCAL = FIRST_REVERSED,
	TK_NIL,
	TK_TRUE,
	TK_FALSE,
	TK_END,
	TK_THEN,
	TK_IF,
	TK_ELSEIF,
	TK_NOT,
	TK_AND,
	TK_OR,
	TK_FUNCTION,

	/* other token */
	TK_STRING,
	TK_NAME,
	TK_FLOAT,
	TK_INT,
	TK_NOTEQUAL,
	TK_EQUAL,
	TK_GREATEREQUAL,
	TK_LESSEQUAL,
	TK_SHL,
	TK_SHR,
	TK_MOD,
	TK_DOT,
	TK_VARARG,
	TK_CONCAT,
	TK_EOS,
};

#define NUM_RESERVED (TK_FUNCTION - FIRST_REVERSED + 1)  //12

extern const char* luaX_tokens[NUM_RESERVED];

typedef union Seminfo {
    lua_Number r;
    lua_Integer i;
    TString* s; 
} Seminfo; /* 记录词法分析过程中数字，字符串对应Token的具体值 */

typedef struct Token {
    int token;          // token enum value
    Seminfo seminfo;    // token info
} Token;

typedef struct LexState {
	Zio* zio;		/* zio实例 */
	int current;	/* 当前读出来的字符的ASCII码 */
	struct MBuffer* buff;	/* 当当前的token被识别为TK_FLOAT、TK_INT、TK_NAME或者TK_STRING类型时，
                               这个buff会临时存放被读出来的字符，等token识别完毕后，在赋值到Token
                               结构的seminfo变量中 */
	Token t;		/* 当前token */
	Token lookahead;
	int linenumber; 
	struct Dyndata* dyd; /* 语法分析过程中，存放local变量信息的结构 */
	struct FuncState* fs; /* 语法分析器数据实例 */
	lua_State* L;
	TString* source;
	TString* env;  /* 全局表的字符串表示，一般是 _ENV */
	struct Table* h;  /* 常量缓存表，用于缓存lua代码中的常量，加快编译时的常量查找 */
} LexState;

LUAI_FUNC void luaX_init(struct lua_State* L);
LUAI_FUNC void luaX_setinput(struct lua_State* L, LexState* ls, Zio* z, struct MBuffer* buffer, struct Dyndata* dyd, TString* source, TString* env);
LUAI_FUNC int luaX_next(struct lua_State* L, LexState* ls);
LUAI_FUNC void luaX_syntaxerror(struct lua_State* L, LexState* ls, const char* error_text);
LUAI_FUNC int luaX_lookahead(struct lua_State* L, LexState* ls);

#endif