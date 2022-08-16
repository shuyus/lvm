#ifndef lualimits_h
#define lualimits_h


#include <limits.h>
#include <stddef.h>

#include "lua.h"

typedef unsigned char lu_byte;
typedef LUAI_UINT32 lu_int32;
typedef LUAI_UMEM lu_mem;
typedef LUAI_MEM l_mem;

/* 字符串池参数 */
#define STRCACHE_M 53
#define STRCACHE_N 2

/* string table 起始长度，必须为2的幂 */
#define MINSTRTABLESIZE 128


#define BUFSIZE 512  //LoadF buffsize


#define MAX_SIZET	((size_t)(~(size_t)0)) /*  size_t max */  
#define MAX_SIZE	(sizeof(size_t) < sizeof(lua_Integer) ? MAX_SIZET \
                          : (size_t)(LUA_MAXINTEGER))  /*  Lua 语言可见最大值 必须在lua_Integer范围内  */   
#define MAX_LUMEM	((lu_mem)(~(lu_mem)0))
#define MAX_LMEM	((l_mem)(MAX_LUMEM >> 1))

#define MAX_INT		INT_MAX 


// assert
#if defined(lua_assert)
#define check_exp(c,e)		(lua_assert(c), (e))
/* to avoid problems with conditions too long */
#define lua_longassert(c)	((c) ? (void)0 : lua_assert(0))
#else
#define lua_assert(c)		((void)0)
#define check_exp(c,e)		(e)
#define lua_longassert(c)	((void)0)
#endif

#if !defined(luai_apicheck)
#define luai_apicheck(l,e)	lua_assert(e)
#endif

#define api_check(l,e,msg)	luai_apicheck(l,(e) && msg)

//cast
#ifndef cast
    #define cast(t, exp)	((t)(exp))
#endif

#define cast_void(i)	cast(void, (i))
#define cast_byte(i)	cast(lu_byte, (i))
#define cast_num(i)	cast(lua_Number, (i))
#define cast_int(i)	cast(int, (i))
#define cast_uchar(i)	cast(unsigned char, (i))

#endif