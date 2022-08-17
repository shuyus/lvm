#ifndef lua_parser_h
#define lua_parser_h

#include "../common/lua_object.h"
#include "../compiler/lua_zio.h"

#define UNOPR_PRIORITY 12

typedef enum UnOpr {
	UNOPR_MINUS, //-
	UNOPR_LEN,	 //#
	UNOPR_BNOT,  //~
	UNOPR_NOT,   //not

	NOUNOPR,
} UnOpr;

typedef enum BinOpr {
	BINOPR_ADD = 0,
	BINOPR_SUB,
	BINOPR_MUL,
	BINOPR_DIV,
	BINOPR_IDIV,
	BINOPR_MOD,
	BINOPR_POW,
	BINOPR_BAND,
	BINOPR_BOR,
	BINOPR_BXOR,
	BINOPR_SHL,
	BINOPR_SHR,
	BINOPR_CONCAT,
	BINOPR_GREATER,
	BINOPR_LESS,
	BINOPR_EQ,
	BINOPR_GREATEQ,
	BINOPR_LESSEQ,
	BINOPR_NOTEQ,
	BINOPR_AND,
	BINOPR_OR,

	NOBINOPR,
} BinOpr;

typedef enum expkind {
	VVOID,          // 表达式是空的，也就是void
	VNIL,           // 表达式是nil类型
	VFLT,           // 表达式是浮点类型
	VINT,           // 表达式是整型类型
	VTRUE,          // 表达式是TRUE
	VFALSE,         // 表达式是FALSE
	VCALL,          // 表达式是函数调用，expdesc中的info字段，表示的是instruction pc
					// 也就是它指向Proto code列表的哪个指令
	VLOCAL,         // 表达式是local变量，expdesc的info字段，表示该local变量，在栈中的位置
	VUPVAL,         // 表达式是Upvalue，expdesc的info字段，表示Upvalue数组的索引
	VINDEXED,       // 表示索引类型，当exp是这种类型时，expdesc的ind域被使用
	VK,             // 表达式是常量类型，expdesc的info字段表示，这个常量是常量表k中的哪个值
	VJMP,
	VRELOCATE,      // 表达式可以把结果放到任意的寄存器上，expdesc的info表示的是instruction pc
	VNONRELOC,      // 表达式已经在某个寄存器上了，expdesc的info字段，表示该寄存器的位置
} expkind;

typedef struct expdesc {
	expkind k;				// expkind
	union {
		int info;			// exp是TK_STRING，info是常量表k的索引值，k是VK
		lua_Integer i;		// for VINT
		lua_Number r;		// for VFLT

		struct {
			int t;		/*  table或者是UpVal的索引  */
			int vt;		/*  标识上一个字段't'是VUPVAL 还是VLOCAL  */
			int idx;	/*  常量表k或者是寄存器的索引，这个索引指向的值就是被取出值的key,不论t是Upvalue还是table的索引，它取出的值一般是一个table  */
		} ind;  //for VINDEXED 
	} u;
	int t;				// patch list of 'exit when true'
	int f;				// patch list of 'exit when false'
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
	int jpc;			// the position of the first test instruction
	int last_target;
	int nk;				// next constant array index to save const value
	int nups;			// next upvalues array index to save upval  /* 下一个可用的p->upvalues的下标 */
	int nlocvars;		// the number of local values
	int nactvars;		// the number of activate values
	int np;				// the number of protos
	int freereg;		// free register index
} FuncState;

typedef struct LH_assign {
	struct LH_assign* prev;
	expdesc	   v;
} LH_assign;

LUAI_FUNC LClosure* luaY_parser(struct lua_State* L, Zio* zio, MBuffer* buffer, Dyndata* dyd, const char* name);
#endif