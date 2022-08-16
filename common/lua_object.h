#ifndef luaobj_h
#define luaobj_h


#include "lua.h"
#include "lua_limits.h"

typedef struct UpVal UpVal;

// 0 | 0 0 | 0 0 0 0

/* Variant tags for functions */
#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))  /* Lua closure */
#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))  /* light C function */
#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))  /* C closure */

/* Variant tags for strings */
#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))  /* short strings */
#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))  /* long strings */

/* Variant tags for numbers */
#define LUA_TNUMFLT	(LUA_TNUMBER | (0 << 4))  /* float numbers */
#define LUA_TNUMINT	(LUA_TNUMBER | (1 << 4))  /* integer numbers */

/* Bit mark for collectable types */
#define BIT_ISCOLLECTABLE	(1 << 6)

/* mark a tag as collectable */
#define ctb(t)			((t) | BIT_ISCOLLECTABLE)

#define luaO_nilobject (&luaO_nilobject_)
#define MAXSHORTSTR 40
#define MAXUPVAL 255
#define MAXLOCALVAR 200

#define dummynode (&dummynode_)
#define twoto(lsize) (1 << lsize)
#define lmod(hash, size) check_exp((size) & (size - 1) == 0, (hash) & (size - 1))

#define lua_numeq(a, b) ((a) == (b))
#define lua_numisnan(a) (!lua_numeq(a, a))
#define lua_numbertointeger(n, p) \
    ((n) >= cast(lua_Number, INT_MIN)) && \
    ((n) <= cast(lua_Number, INT_MAX)) && \
    ((*(p) = cast(lua_Integer, n)), 1)

#ifdef _WIN32
#define l_sprintf sprintf_s
#define l_fopen fopen_s
#else
#define l_sprintf(buf, maxsize, fmt, args...) sprintf(buf, fmt, args) 
#define l_fopen(h, name, mode) (*h = fopen(name, mode))
#endif



#define CommonHeader struct GCObject* next; lu_byte tt_; lu_byte marked 

// Closure
#define ClosureHeader CommonHeader; int nupvalues; struct GCObject* gclist

typedef struct GCObject {
    CommonHeader;
} GCObject;

typedef union lua_Value {
    struct GCObject* gc;
    void* p;
    int b;
    lua_Integer i;
    lua_Number n;
    lua_CFunction f;
} Value;

typedef struct lua_TValue {
    Value value_;
    int tt_;
} TValue;

LUAI_DATA const TValue luaO_nilobject_;  /* 全局唯一的nil */

#define novariant(tt)	((tt) & 0x0F) 

#define val_(o)		((o)->value_)
#define rttype(o)	((o)->tt_)
#define ttype(o)	(rttype(o) & 0x3F) /* 去掉垃圾收集位 */
#define ttnov(o)	(novariant(rttype(o))) 



#define checktag(o,t)		(rttype(o) == (t))
#define checktype(o,t)		(ttnov(o) == (t))

//o -> TValue*
/* GCObject的tt_不带清理位 TValue的tt_带清理位 */


#define ttisnumber(o)		checktype((o), LUA_TNUMBER)
#define ttisfloat(o)		checktag((o), LUA_TNUMFLT)
#define ttisinteger(o)		checktag((o), LUA_TNUMINT)
#define ttisnil(o)		checktag((o), LUA_TNIL)
#define ttisboolean(o)		checktag((o), LUA_TBOOLEAN)
#define ttislightuserdata(o)	checktag((o), LUA_TLIGHTUSERDATA)
#define ttisstring(o)		checktype((o), LUA_TSTRING)
#define ttisshrstr(o)	checktag((o), ctb(LUA_TSHRSTR))
#define ttislngstr(o)	checktag((o), ctb(LUA_TLNGSTR))
#define ttistable(o)		checktag((o), ctb(LUA_TTABLE))
#define ttisfunction(o)		checktype((o), LUA_TFUNCTION)
#define ttisclosure(o)		((rttype(o) & 0x1F) == LUA_TFUNCTION)   //CClosure LuaClosure
#define ttisCclosure(o)		checktag((o), ctb(LUA_TCCL))
#define ttisLclosure(o)		checktag((o), ctb(LUA_TLCL))
#define ttislcf(o)		checktag((o), LUA_TLCF) //CLightFunc
#define ttisfulluserdata(o)	checktag((o), ctb(LUA_TUSERDATA))
#define ttisthread(o)		checktag((o), ctb(LUA_TTHREAD))
#define ttisdeadkey(o)		(ttype(o) == LUA_TDEADKEY)



#define ivalue(o)	check_exp(ttisinteger(o), val_(o).i)
#define fltvalue(o)	check_exp(ttisfloat(o), val_(o).n)
#define lcfvalue(o)	check_exp(ttislcf(o), val_(o).f)
#define lclvalue(o) check_exp(ttisLclosure(o), gco2lclosure(val_(o).gc))
#define cclvlaue(o) check_exp(ttisCclosure(o), gco2cclosure(val_(o).gc))

#define nvalue(o)	check_exp(ttisnumber(o), \
	(ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))
#define tsvalue(o) check_exp(ttisstring(o), gco2ts(val_(o).gc))
#define hvalue(o) check_exp(ttistable(o), gco2h(val_(o).gc))


#define settt_(o,t)	((o)->tt_=(t))
#define setnilvalue(obj) (settt_(obj, LUA_TNIL))
#define setivalue(obj,x) \
  { TValue *io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT); }
#define setfltvalue(obj,x) \
  { TValue *io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT); }
#define setbvalue(obj,x) \
  { TValue *io=(obj); val_(io).b=(x); settt_(io, LUA_TBOOLEAN); }
#define setpvalue(obj,x) \
  { TValue *io=(obj); val_(io).p=(x); settt_(io, LUA_TLIGHTUSERDATA); }



#define setfvalue(obj,x) \
{ TValue *io=(obj); val_(io).f=(x); settt_(io, LUA_TLCF); }
#define setlclvalue(obj,x) \
  { TValue *io=(obj); LClosure *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TLCL)); }
#define setcclvalue(obj,x) \
  { TValue *io=(obj); CClosure *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TCCL)); }

#define setgcovalue(obj,x) \
  { TValue *io = (obj); GCObject *i_g=(x); \
    val_(io).gc = i_g; settt_(io, ctb(i_g->tt_)); } ////setgco

#define settsvalue(obj,x) \
  { TValue *io=(obj); TString *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io,ctb(x->tt_)); } 

#define sethvalue(obj,x) \
  { TValue *io = (obj); Table *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTABLE)); }

#define setthvalue(obj,x) \
  { TValue *io = (obj); lua_State *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTHREAD)); }




#define setobj(target,value) \
  { TValue *io=(target); TValue* vo=(value); val_(io)=val_(vo);settt_(io,rttype(vo)); }

#define setobj_exp(o1,o2)  (val_(o1)=val_(o2) , settt_(o1,rttype(o2)))



// #define iscollectable(o) \
//     ((o)->tt_ == LUA_TTHREAD || (o)->tt_ == LUA_TSHRSTR || (o)->tt_ == LUA_TLNGSTR || (o)->tt_ == LUA_TTABLE)

#define iscollectable(o)	(rttype(o) & BIT_ISCOLLECTABLE)
#define setcollectable(o) (((o)->tt_) &= BIT_ISCOLLECTABLE)

typedef struct TString {
    CommonHeader;
    unsigned int hash;          
    unsigned short extra; 
    /* TString为长字符串时：当extra=0表示该字符串未进行hash运算；当extra=1时表示该字符串已经进行过hash运算。
       TString为短字符串时：当extra=0时表示它需要被gc托管；否则表示该字符串不会被gc回收。 */     
    unsigned short shrlen;
    union {
        struct TString* hnext; 
        size_t lnglen;
    } u;
    char data[0];
} TString;

typedef union TKey {
    struct {
        Value value_;  //8
        int tt_;  //4
        int next;//4
    } nk;
    TValue tvk;  //16
} TKey;

typedef struct Node {
    TKey key;
    TValue value;
} Node;

const Node dummynode_;

typedef struct Table {
    CommonHeader;
    TValue* array;
    unsigned int arraysize;
    Node* node;
    unsigned int lsizenode; 
    Node* lastfree;
    struct GCObject* gclist;
} Table;


// compiler and vm structs
typedef struct LocVar {
    TString* varname;
    int startpc;
    int endpc;
} LocVar;

typedef struct Upvaldesc {
    int in_stack;
    int idx;
    TString* name;
} Upvaldesc;

typedef struct Proto {
    CommonHeader;
    int is_vararg;
    int nparam;
    Instruction* code;  // array of opcodes
    int sizecode;
    TValue* k;
    int sizek;
    LocVar* locvars;
    int sizelocvar;
    Upvaldesc* upvalues;
    int sizeupvalues;
    struct Proto** p;
    int sizep;
    TString* source;
    struct GCObject* gclist;
	int maxstacksize;
} Proto;

typedef struct LClosure {
    ClosureHeader;
    Proto* p;
    UpVal* upvals[1];
} LClosure;

typedef struct CClosure {
	ClosureHeader;
	lua_CFunction f;
	UpVal* upvals[1];
} CClosure;

typedef union Closure {
	LClosure l;
	CClosure c;
} Closure;


LUAI_FUNC int luaO_ceillog2(int value);


#endif