#include "lua_object.h"

const TValue luaO_nilobject_ = { {NULL}, LUA_TNIL };


/* 得出的n满足2^n >= value */
int luaO_ceillog2(int value) {
    int x = 0;
    for (; value > (int)pow(2, x); x++);
    return x;
}