#ifndef luazio_h_
#define luazio_h_

#include "../common/lua.h"
#include "../common/lua_object.h"

typedef char* (*lua_Reader)(struct lua_State* L, void* data, size_t* size);

#define MIN_BUFF_SIZE 32 
#define zget(z) (((z)->n--) > 0 ? (*(z)->p++) : luaZ_fill(z))   /* 从zio内获取一个字符，若zio全部被读取了，重新填充 */
#define luaZ_resetbuffer(ls) (ls->buff->n = 0)
#define luaZ_buffersize(ls) (ls->buff->size)

typedef struct LoadF {
    FILE* f;			/* 读取的文件句柄 */
    char buff[BUFSIZE];  /* loadF内部的缓冲区 */
	int n;		       /* 缓冲未使用字节数 */
} LoadF;

typedef struct Zio {
	lua_Reader reader;		
	int n;					/* 还有多少个未被处理的字符，初始值是LoadF的n值 */
	char* p;				/* 指向LoadF结构的buff数组的指针，每处理一个字符，它会自增 */
	void* data;				/* 内含文件句柄的结构，一般是LoadF */
	struct lua_State* L;
} Zio;

LUAI_FUNC void luaZ_init(struct lua_State* L, Zio* zio, lua_Reader reader, void* data);

// if fill success, then it will return next character in ASCII table, or it will return -1
LUAI_FUNC int luaZ_fill(Zio* z);	

#endif