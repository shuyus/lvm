#ifndef luaconfig_h
#define luaconfig_h

#include <limits.h>
#include <stddef.h>


#define LUAI_UINT32	unsigned int
#define LUAI_INT32	int
#define LUAI_MAXINT32	INT_MAX
#define LUAI_MININT32	INT_MIN

#define LUAI_UACNUMBER	double

#define LUAI_UMEM	size_t
#define LUAI_MEM	ptrdiff_t


#define LUAI_USER_ALIGNMENT_T	union { double u; void *s; long l; }


#define LUAI_FUNC	extern
#define LUAI_DATA	extern
#define LUAI_DDEC	LUAI_FUNC
#define LUAI_DDEF

//--------------------

#define LUA_INTEGER		long long
#define LUA_UNSIGNED unsigned LUA_INTEGER 

#define LUA_INTEGER_FORMAT "%lld"

#define LUA_MAXINTEGER		LLONG_MAX
#define LUA_MININTEGER		LLONG_MIN

#define LUA_NUMBER	double
#define LUA_NUMBER_FORMAT "%lf"

#define LUA_ENV "_ENV"
#define LUA_LOADED "_LOADED"

//----------

#if defined(LUA_BUILD_AS_DLL)	
    #if defined(LUA_CORE) || defined(LUA_LIB)	
        #define LUA_API __declspec(dllexport)
    #else					
        #define LUA_API __declspec(dllimport)
    #endif					
#else				
    #define LUA_API		extern
#endif	

#define LUALIB_API	LUA_API


#endif