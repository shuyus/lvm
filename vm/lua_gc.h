#ifndef luagc_h
#define luagc_h

#include "../common/lua_state.h"

// GCState
#define GCSpause        0
#define GCSpropagate    1
#define GCSatomic       2
#define GCSinsideatomic 3
#define GCSsweepallgc   4
#define GCSsweepend     5


// color
#define WHITE0BIT       0
#define WHITE1BIT       1
#define BLACKBIT        2
/* 非黑非白即是灰 */


// gc attr
#define STEPMULADJ 200
#define GCSTEPMUL 200 
#define GCSTEPSIZE 2048  /* 每次触发luaC_step函数，只会处理至多debt+GCSTEPSIZE个字节的数据 */
#define GCPAUSE 100
#define LUA_GCSTEPMUL 200


/* 颜色处理宏 */
#define bitmask(b) (1<<(b))
#define bit2mask(b1, b2) (bitmask(b1) | bitmask(b2))
 
#define resetbits(x, m) ((x) &= cast(lu_byte, ~(m)))
#define setbits(x, m) ((x) |= (m))
#define testbits(x, m) ((x) & (m))

#define resetbit(x, b) resetbits(x, bitmask(b)) 
#define l_setbit(x, b) setbits(x, bitmask(b))
#define testbit(x, b) testbits(x, bitmask(b))


#define WHITEBITS bit2mask(WHITE0BIT, WHITE1BIT)


/* 颜色转换 */
#define white2gray(o) resetbits((o)->marked, WHITEBITS)
#define gray2black(o) l_setbit((o)->marked, BLACKBIT)
#define black2gray(o) resetbit((o)->marked, BLACKBIT)


#define luaC_white(g) ((g)->currentwhite & WHITEBITS)
#define otherwhite(g) ((g)->currentwhite ^ WHITEBITS)


#define makewhite(o) \
	(o)->marked &= cast(lu_byte, ~(bitmask(BLACKBIT) | WHITEBITS)); \
	(o)->marked |= luaC_white(g)

/* 颜色判断 */
/* o是GCObject* */
#define iswhite(o) testbits((o)->marked, WHITEBITS)
#define isgray(o)  (!testbits((o)->marked, bitmask(BLACKBIT) | WHITEBITS))
#define isblack(o) testbit((o)->marked, BLACKBIT)

#define isdeadm(ow, m) (!(((m) ^ WHITEBITS) & (ow)))   /* 当ow 和 m 为同一种白，返回1  */
#define isdead(g, o) isdeadm(otherwhite(g), (o)->marked)
#define changewhite(o) ((o)->marked ^= WHITEBITS)


#define markobject(L, o) if (iswhite(o)) { reallymarkobject(L, obj2gco(o)); }
#define markvalue(L, o)  if (iscollectable(o) && iswhite(gcvalue(o))) { reallymarkobject(L, gcvalue(o)); }  //TValue*  
#define linkgclist(gco, prev) { (gco)->gclist = (prev); (prev) = obj2gco(gco); }

// try trigger gc
#define luaC_condgc(pre, L, pos) if (G(L)->GCdebt > 0) { pre; luaC_step(L); pos; } 
#define luaC_checkgc(L) luaC_condgc((void)0, L, (void)0)
#define luaC_barrierback(L, t, o) \
    (isblack(t) && iscollectable(o) && iswhite(gcvalue(o))) ? luaC_barrierback_(L, t, o) : cast(void, 0)

LUAI_FUNC struct GCObject* luaC_newobj(struct lua_State* L, lu_byte tt_, size_t size);
LUAI_FUNC void luaC_step(struct lua_State* L);
LUAI_FUNC void reallymarkobject(struct lua_State* L, struct GCObject* gc);
LUAI_FUNC void luaC_barrierback_(struct lua_State* L, struct Table* t, const TValue* o);
LUAI_FUNC void luaC_freeallobjects(struct lua_State* L);
LUAI_FUNC void luaC_fix(struct lua_State* L, struct GCObject* o); // GCObject can not collect


#endif