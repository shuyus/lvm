#ifndef lua_h
#define lua_h

#define sy_printf printf

#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>


#include "lua_config.h"


typedef struct lua_State lua_State;
typedef int (*lua_CFunction) (lua_State *L);
typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);
typedef int Instruction;

/* error code */
#define LUA_OK 0
#define LUA_ERRERR 1
#define LUA_ERRMEM 2
#define LUA_ERRRUN 3 
#define LUA_ERRLEXER 4
#define LUA_ERRPARSER 5

/*
** basic type tag
*/
// #define LUA_TNONE		(-1)  LUA原生

#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8


#define LUA_TPROTO 9
#define LUA_TNONE 10
#define LUA_NUMS LUA_TNONE 
#define LUA_TDEADKEY (LUA_NUMS + 1)


typedef LUA_NUMBER lua_Number;
typedef LUA_INTEGER lua_Integer;
typedef LUA_UNSIGNED lua_Unsigned;

/* stack infomation */
#define LUA_MINSTACK 20
#define LUA_STACKSIZE (2 * LUA_MINSTACK)
#define LUA_EXTRASTACK 5
#define LUA_MAXSTACK 15000
#define LUA_ERRORSTACK 200
#define LUA_MULRET -1
#define LUA_MAXCALLS 200



//
#define LUA_OPT_UMN		1
#define LUA_OPT_LEN		2
#define LUA_OPT_BNOT	3
#define LUA_OPT_NOT		4

#define	LUA_OPT_ADD		5
#define	LUA_OPT_SUB		6
#define	LUA_OPT_MUL		7
#define	LUA_OPT_DIV		8
#define LUA_OPT_IDIV    9
#define	LUA_OPT_MOD		10
#define	LUA_OPT_POW		11
#define	LUA_OPT_BAND    12
#define	LUA_OPT_BOR		13
#define	LUA_OPT_BXOR	14
#define	LUA_OPT_SHL		15
#define	LUA_OPT_SHR		16
#define	LUA_OPT_CONCAT  17
#define	LUA_OPT_GREATER 18
#define	LUA_OPT_LESS	19
#define	LUA_OPT_EQ		20
#define	LUA_OPT_GREATEQ 21
#define	LUA_OPT_LESSEQ  22
#define	LUA_OPT_NOTEQ	23
#define	LUA_OPT_AND		24
#define	LUA_OPT_OR		25



// 
#define LUA_ERROR(L, s) printf("LUA ERROR:%s", s)
#define point2uint(p) ((unsigned int)((size_t)p & UINT_MAX))

//IO
#define lua_writestring(s) fwrite((s), sizeof(char), strlen((s)), stdout)
#define lua_writeline()    (lua_writestring("\n"), fflush(stdout))


//-----------------API  

LUA_API lua_State* lua_newstate(lua_Alloc alloc, void* ud);
LUA_API void lua_close(lua_State* L);


//push functions (C -> stack)
LUA_API void lua_pushcfunction(lua_State* L, lua_CFunction f);
LUA_API void lua_pushinteger(lua_State* L, lua_Integer integer);
LUA_API void lua_pushnumber(lua_State* L, double number);
LUA_API void lua_pushboolean(lua_State* L, bool b);
LUA_API void lua_pushnil(lua_State* L);
LUA_API void lua_pushlightuserdata(lua_State* L, void* p);
LUA_API void lua_pushstring(lua_State* L, const char* str);
LUA_API void lua_pushCclosure(lua_State* L, lua_CFunction f, int nup);

LUA_API int lua_pushvalue(lua_State* L, int idx);	// push the value at the stack indexed by idx onto the top
LUA_API int lua_pushglobaltable(lua_State* L);// get _G and push it into the stack
LUA_API int lua_createtable(lua_State* L);


//to
LUA_API lua_Integer lua_tointegerx(lua_State* L, int idx, int* isnum);
LUA_API lua_Number lua_tonumberx(lua_State* L, int idx, int* isnum);
LUA_API bool lua_toboolean(lua_State* L, int idx);
LUA_API char* lua_tostring(lua_State* L, int idx);
LUA_API struct Table* lua_totable(lua_State* L, int idx);

LUA_API int lua_isnil(lua_State* L, int idx);

LUA_API void lua_settop(lua_State* L, int idx);
LUA_API int lua_gettop(lua_State* L);
LUA_API void lua_pop(lua_State* L);



LUA_API int lua_settable(lua_State* L, int idx);// t[k]=v, the table t is index by idx argument, and k, v are the values beyond t, it will trigger metamethod
LUA_API int lua_setfield(lua_State* L, int idx, const char* k); // set t[k]=v, v is the top value

LUA_API int lua_gettable(lua_State* L, int idx);
LUA_API int lua_getglobal(lua_State* L, const char* name);





#endif