#ifndef lua_parser_h
#define lua_parser_h

#include "../common/lua_object.h"
#include "../compiler/lua_zio.h"

typedef enum expkind {
	VVOID,          // 表达式是空的，也就是void
	VNIL,           // 表达式是nil类型
	VFLT,           // 表达式是浮点类型
	VINT,           // 表达式是整型类型
	VTRUE,          // 表达式是TRUE
	VFALSE,         // 表达式是FALSE
	VINDEXED,       // 表示索引类型，当exp是这种类型时，expdesc的ind域被使用
	VCALL,          // 表达式是函数调用，expdesc中的info字段，表示的是instruction pc
					// 也就是它指向Proto code列表的哪个指令
	VLOCAL,         // 表达式是local变量，expdesc的info字段，表示该local变量，在栈中的位置
	VUPVAL,         // 表达式是Upvalue，expdesc的info字段，表示Upvalue数组的索引
	VK,             // 表达式是常量类型，expdesc的info字段表示，这个常量是常量表k中的哪个值
	VRELOCATE,      // 表达式可以把结果放到任意的寄存器上，expdesc的info表示的是instruction pc
	VNONRELOC,      // 表达式已经在某个寄存器上了，expdesc的info字段，表示该寄存器的位置
} expkind;

typedef struct expdesc {
	expkind k;				// expkind
	union {
		int info;			
		lua_Integer i;		// for VINT
		lua_Number r;		// for VFLT

		struct {
			int t;		/*  table或者是UpVal的索引  */
			int vt;		/*  标识上一个字段't'是VUPVAL 还是VLOCAL  */
			int idx;	/*  常量表k或者是寄存器的索引，这个索引指向的值就是被取出值得key,不论t是Upvalue还是table的索引，它取出的值一般是一个table  */
		} ind;  //for VINDEXED 
	} u;
} expdesc;

// Token cache
typedef struct MBuffer {
	char* buffer;
	int n; /* 当前buffer中字节数 */
	int size;
} MBuffer;

typedef struct Dyndata {
	struct {
		short* arr; /* 记录local变量的信息，index为local变量所在的寄存器，而值则是Proto结构中locvars列表的索引，用于查找local变量名 */
		int n; /* 下一个local变量要存储的索引 */
		int size; /* array列表的大小 */
	} actvar;
} Dyndata;

typedef struct FuncState {
	int firstlocal;
	struct FuncState* prev;
	struct LexState* ls;
	Proto* p;
	int pc;				// next code array index to save instruction
	int nk;				// next constant array index to save const value
	int nups;			// next upvalues array index to save upval  /* 下一个可用的p->upvalues的下标 */
	int nlocvars;		// the number of local values
	int nactvars;		// the number of activate values
	int np;				// the number of protos
	int freereg;		// free register index
} FuncState;

LUAI_FUNC LClosure* luaY_parser(struct lua_State* L, Zio* zio, MBuffer* buffer, Dyndata* dyd, const char* name);
#endif