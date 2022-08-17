#ifndef luastate_h
#define luastate_h 

#include "lua_object.h"

#define LUA_EXTRASPACE sizeof(void*) 
#define G(L) ((L)->l_G)

#define LUA_MAINTHREADIDX 0
#define LUA_GLOBALTBLIDX 1

#define LUA_REGISTRYINDEX (-LUA_MAXSTACK - 1)

typedef TValue* StkId;

struct CallInfo {
    StkId func; /* 被调用函数在栈中的位置 */
    StkId top;  /* 被调用函数的栈顶位置 */
    struct { /* 只有执行LClosure才会用到这个结构 */
		StkId base; /* 指向当前函数帧的栈底，即栈中对应LClosure的下一个位置 */
		const Instruction* savedpc; /* 初始化时指向cl->p->code */
	} l;
    int nresult; /* 有多少个返回值 */
    int callstatus; /* 调用状态 */
    struct CallInfo* next; /* 下一个调用 */
    struct CallInfo* previous; /* 上一个调用 */
};

struct stringtable {
    struct TString** hash;
    unsigned int nuse; /* 当前有多少个经过内部化处理的短字符串 */
    unsigned int size; /* 这个stringtable结构内部的hash数组的大小有多大 */
};

struct lua_State {
    CommonHeader;
    StkId stack; /* 绝对栈底 */
    StkId stack_last; /* 从这里开始，栈不能被使用 */
    StkId top; /* 指向当前栈帧的栈顶，也就是下一块可用槽 */
    StkId base;   /* 当前栈帧的底 */
    int stack_size; 
    struct lua_longjmp* errorjmp;
    lu_byte status;
    struct lua_State* next_s;
    struct lua_State* previous_s;
    struct CallInfo base_ci;  
    struct CallInfo* ci; /*  指向当前调用函数的Callinfo  */
    struct global_State* l_G;
    ptrdiff_t errorfunc; /* 错误函数位于栈的哪个位置 */
    int ncalls;
    int nci;
    struct GCObject* gclist; /* 当lua_State对象在gray链表时，它指向gray链表中的下一个灰色对象的地址，
                       当lua_State对象在grayagain链表时，它指向grayagain链表中，下一个灰色对象的地址，
                       其实就是相当于gray或者grayagain链表中，数据对象的next指针 */
};

typedef struct global_State {
    struct lua_State* mainthread;
    lua_Alloc frealloc;
    void* ud; 
    lua_CFunction panic;

    //strtable
    struct stringtable strt;
    TString* strcache[STRCACHE_M][STRCACHE_N];
    unsigned int seed;              
    TString* memerrmsg;


    TValue l_registry;  // table new add

    //gc
    lu_byte gcstate;
    lu_byte currentwhite;
    struct GCObject* allgc;    /* 单向链表头，所有gc对象都要放入到这个链表中，由GCObject::next串联 */ 
    struct GCObject** sweepgc; /* 用于记录当前sweep的进度 */
    struct GCObject* gray;    /* gc对象，首次从白色被标记为灰色的时候，会被放入这个列表，放入这个列表的gc对象，是准备被propagate的对象 */
    struct GCObject* grayagain;
    struct GCObject* fixgc;   /* 不能被GC回收的对象链表 */
    lu_mem totalbytes;  /* 记录开辟内存字节大小的变量之一，真实的开辟内存大小是totalbytes+GCdebt */
    l_mem GCdebt;                   
    lu_mem GCmemtrav;    /* 每次进行gc操作时，所遍历的对象字节大小之和，单位是byte，当其值大于单步执行的内存上限时，gc终止 */  
    lu_mem GCestimate;   /* 在sweep阶段结束时，会被重新计算，本质是totalbytes+GCdebt */          
    int GCstepmul;       /* 一个和GC单次处理多少字节相关的参数 */
} global_State;


union GCUnion {
    struct GCObject gc;
    struct lua_State th;
    struct TString ts;
    struct Table h;
    union Closure cl;
	struct Proto p;
};


#define obj2gco(o) check_exp(novariant((o)->tt) < LUA_TDEADKEY,(&cast(union GCUnion*, o)->gc))  //TValue* -> GCUnion* -> GCObject*
#define gco2th(o)  check_exp((o)->tt_ == LUA_TTHREAD, &cast(union GCUnion*, o)->th)
#define gco2ts(o) check_exp((o)->tt_ == LUA_TSHRSTR || (o)->tt_ == LUA_TLNGSTR, &cast(union GCUnion*, o)->ts)
#define gco2h(o) check_exp((o)->tt_ == LUA_TTABLE, &cast(union GCUnion*, o)->h)
#define gco2lclosure(o) check_exp((o)->tt_ == LUA_TLCL, &cast(union GCUnion*, o)->cl.l)
#define gco2cclosure(o) check_exp((o)->tt_ == LUA_TCCL, &cast(union GCUnion*, o)->cl.c)
#define gco2proto(o) check_exp((o)->tt_ == LUA_TPROTO, &cast(union GCUnion*, o)->p)
#define gcvalue(o) ((o)->value_.gc)


#endif 
