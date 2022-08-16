#include "lua_object.h"
#include "lua_table.h"
#include "lua_mem.h"
#include "lua_string.h"
#include "../vm/lua_gc.h"
#include "../vm/lua_vm.h"
#include "../vm/lua_do.h"


#define MAXASIZE (1u << MAXABITS)

static int l_hashfloat(lua_Number n) {
    int i = 0;
    lua_Integer ni = 0;

    n = frexp(n, &i) * -cast(lua_Number, INT_MIN);
    if (!lua_numbertointeger(n, &ni)) {
        lua_assert(lua_numisnan(n) || fabs(n) == cast(lua_Number, HUGE_VAL));
        return 0;
    } else {
        unsigned int u = cast(unsigned int, ni) + cast(unsigned int, i);
        return u <= INT_MAX ? u : ~u;
    }
}


/* 根据key得到哈希表位置 */
static Node* mainposition(struct lua_State* L, struct Table* t, const TValue* key) {
    switch(ttype(key)) {
        case LUA_TNUMINT: return hashint(key->value_.i, t); 
        case LUA_TNUMFLT: return hashint(l_hashfloat(key->value_.n), t);
        case LUA_TBOOLEAN: return hashboolean(key->value_.b, t);
        case LUA_TSHRSTR: return hashstr(gco2ts(gcvalue(key)), t);
        case LUA_TLNGSTR: {
            struct TString* ts = gco2ts(gcvalue(key));
            luaS_hashlongstr(L, ts);
            return hashstr(ts, t);
        };
        case LUA_TLIGHTUSERDATA: {
            return hashpointer(key->value_.p, t);
        };
        case LUA_TLCF: {
            return hashpointer(key->value_.f, t);
        };
        default: {
            lua_assert(!ttisdeadkey(gcvalue(key))); 
            return hashpointer(gcvalue(key), t);
        };  
    } 
}

static const TValue* getgeneric(struct lua_State* L, struct Table* t, const TValue* key) {

    Node* n = mainposition(L, t, key); 

    for (;;) {
        const TValue* k = getkey(n);

        if (luaV_eqobject(L, k, key)) {
            return getval(n);
        } else {
            int next = n->key.nk.next;

            if (next == 0) {
                break;
            }
            n += next;
        }
    } 

    return luaO_nilobject;
}

/* 根据size 新建一个哈希表，不是在原地扩容 */ 
static void setnodesize(struct lua_State* L, struct Table* t, int size) {
    if (size == 0) {
        t->lsizenode = 0;
        t->node = cast(Node*, dummynode);
        t->lastfree = NULL;
    } else {
        int lsize = luaO_ceillog2(size);
        t->lsizenode = (unsigned int)lsize;
        if (t->lsizenode > (sizeof(int) * CHAR_BIT - 1)) {
            luaD_throw(L, LUA_ERRRUN);
        }

        int node_size = twoto(lsize);
        t->node = (Node*)luaM_newvector(L, node_size, Node);
        t->lastfree = &t->node[node_size]; // it is not a bug, at the beginning, lastfree point to the address which next to last node
        
        for (int i = 0; i < node_size; i++) {
            Node* n = &t->node[i];
            TKey* k = &n->key;
            k->nk.next = 0;
            setnilvalue(getwkey(n));
            setnilvalue(getval(n));
        }
    }
}

struct Table* luaH_new(struct lua_State* L) {
    struct GCObject* o = luaC_newobj(L, LUA_TTABLE, sizeof(struct Table));
    struct Table* t = gco2h(o);
    t->array = NULL;
    t->arraysize = 0;
    t->node = NULL;
    t->lsizenode = 0;
    t->lastfree = NULL;
    t->gclist = NULL;
    
    setnodesize(L, t, 0);
    return t;
}

void luaH_free(struct lua_State* L, struct Table* t) {
    if (t->arraysize > 0) {
        luaM_free(L, t->array, t->arraysize * sizeof(TValue));
    }

    if (!isdummy(t)) {
        luaM_free(L, t->node, twoto(t->lsizenode) * sizeof(struct Node));
    }

    luaM_free(L, t, sizeof(struct Table));
}

const TValue* luaH_getint(struct lua_State* L, struct Table* t, lua_Integer key) {
    // 1 <= key <= arraysize
    if (cast(unsigned int, key) - 1 < t->arraysize) {
        return cast(const TValue*, &t->array[key - 1]);        
    } else {
        struct Node* n = hashint(key, t);
        while(true) {
            const TKey* k = &n->key;
            if (ttisinteger(&k->tvk) && k->tvk.value_.i == key) {
                return cast(const TValue*, getval(n));
            } else {
                int next = k->nk.next;
                if (next == 0) {
                    break;
                }
                n += next; 
            }
        }
    }

    return luaO_nilobject;
}

int luaH_setint(struct lua_State* L, struct Table* t, int key, const TValue* value) {
    const TValue* p = luaH_getint(L, t, key);
    TValue* cell = NULL;
    if (p != luaO_nilobject) {
        cell = cast(TValue*, p);
    } else {
        TValue k;
        setivalue(&k, key);
        cell = luaH_newkey(L, t, &k); 
    }

    setobj(cell,value);
    
    // cell->value_ = value->value_;
    // cell->tt_ = value->tt_;
    return 1;
}

const TValue* luaH_getshrstr(struct lua_State* L, struct Table* t, struct TString* key) {
    lua_assert(key->tt_ == LUA_SHRSTR);
    struct Node* n = hashstr(key, t);
    for (;;) {
        TKey* k = &n->key;
        if (ttisshrstr(&k->tvk) && luaS_eqshrstr(L, gco2ts(gcvalue(&k->tvk)), key)) {
            return getval(n);
        } else {
            int next = k->nk.next;
            if (next == 0) {
                break;
            }
            n += next;
        }
    }
    return luaO_nilobject;
}

const TValue* luaH_getstr(struct lua_State* L, struct Table* t, struct TString* key) {
    if (key->tt_ == LUA_TSHRSTR) {
        return luaH_getshrstr(L, t, key);
    } else {
        TValue k;
        setgcovalue(&k, obj2gco(key));
        return getgeneric(L, t, &k);
    }
}
/* 根据key寻找一个位置，数组内或哈希表，找不到返回全局nil */
const TValue* luaH_get(struct lua_State* L, struct Table* t, const TValue* key) {
    switch(ttype(key)) {
        case LUA_TNIL:   return luaO_nilobject;
        case LUA_TNUMINT: return luaH_getint(L, t, key->value_.i); 
        case LUA_TNUMFLT: return luaH_getint(L, t, l_hashfloat(key->value_.n));
        case LUA_TSHRSTR: return luaH_getshrstr(L, t, gco2ts(gcvalue(key)));
        case LUA_TLNGSTR: return luaH_getstr(L, t, gco2ts(gcvalue(key)));
        default:{
            return getgeneric(L, t, key);   
        };
    }
}

/* 根据key寻找一个位置，数组内或哈希表，没有则在哈希表新建key */
TValue* luaH_set(struct lua_State* L, struct Table* t, const TValue* key) {
    const TValue* p = luaH_get(L, t, key);
    if (p != luaO_nilobject) {
        return cast(TValue*, p); 
    } else {
        return luaH_newkey(L, t, key);
    }
}

typedef struct Auxnode {
    struct Table* t;
    unsigned int size;
} Auxnode;

static int aux_set_node_size(struct lua_State* L, void* ud) {
    struct Auxnode* n = cast(struct Auxnode*, ud);
    setnodesize(L, n->t, n->size);
    return 0;
}

/* hsize是哈希表容量，根据其进行0初始化 */
int luaH_resize(struct lua_State* L, struct Table* t, unsigned int asize, unsigned int hsize) {
    unsigned int old_asize = t->arraysize;
    Node* old_node = t->node;
    unsigned int old_node_size = isdummy(t) ? 0 : twoto(t->lsizenode);

    if (asize > t->arraysize) {
        luaM_reallocvector(L, t->array, t->arraysize, asize, TValue); /* 数组是原地扩容 */
        t->arraysize = asize;

        for (unsigned int i = old_asize; i < asize; i ++) {
            setnilvalue(&t->array[i]);
        }
    }

    Auxnode auxnode;
    auxnode.t = t;
    auxnode.size = hsize;
    if (luaD_rawrunprotected(L, &aux_set_node_size, &auxnode) != LUA_OK) {
        luaM_reallocvector(L, t->array, t->arraysize, old_asize, TValue);
        luaD_throw(L, LUA_ERRRUN);
    }


    // shrink array /* 收缩数组 */
    if (asize < old_asize) {
        for (unsigned int i = asize; i < old_asize; i++) {
            if (!ttisnil(&t->array[i])) {
                luaH_setint(L, t, i + 1, &t->array[i]);
            }
        }
        luaM_reallocvector(L, t->array, old_asize, asize, TValue);
        t->arraysize = asize;
    }

    for (unsigned int i = 0; i < old_node_size; i++) {
        Node* n = &old_node[i];
        if (!ttisnil(getval(n))) {
            setobj(luaH_set(L, t, getkey(n)), getval(n)); /* 这里会把哈希表内的符合数组范围的 数字键节点 迁移到数组内 */
        }
    }

    if (old_node_size > 0) {
        luaM_free(L, old_node, old_node_size * sizeof(Node));
    }

    return 1;
}

static Node* getlastfree(struct Table* t) {
    if (t->lastfree == NULL) {
        return NULL;
    }

    while(true) {
        if (t->lastfree == t->node) {  //???
            break;
        }
        t->lastfree--;
        if (ttisnil(getval(t->lastfree))) {
            return t->lastfree;
        }
    }

    return NULL;
}

/***   
 * nums[i]统计array 2 ^ (i - 1) ~ 2 ^ i区间内，不为nil的元素个数
 * 同时他也统计node列表中，key为lua_Integer类型，值不为nil的节点个数，
 * 会计算出node的key位于哪个区间中，并统计   
 ***/
static int numsarray(struct Table* t, int* nums) {
    unsigned int totaluse = 0;
    unsigned int idx = 0;
    for (unsigned int i = 0, twotoi = 1; twotoi <= t->arraysize; i++, twotoi *= 2) {
        for (; idx < twotoi; idx++) {
            if (!ttisnil(&t->array[idx])) {
                totaluse++;
                nums[i]++;
            }
        }
    } 
    return totaluse;
}

/* 遍历node数组，统计值不是nil的节点数量，同时计算哈希表内键为整数的落区间 */
static int numshash(struct Table* t, int* nums) {
    int totaluse = 0;
    for (int i = 0; i < twoto(t->lsizenode); i++) {
        Node* n = &t->node[i];
        if (!ttisnil(getval(n))) {
            totaluse++;
            if (ttisinteger(getkey(n)) && getkey(n)->value_.i > 0) {
                lua_Integer ikey = getkey(n)->value_.i;
                int temp = luaO_ceillog2(ikey);

                if (temp < MAXABITS + 1 && temp >= 0)
                    nums[temp] ++;
            }
        }
    }
    return totaluse;
}

static int compute_array_size(struct Table* t, int* nums, int* array_used_num) {
    unsigned int array_size = 0;
    unsigned int sum_array_used = 0;
    unsigned int sum_int_keys = 0;  /* 数组元素数量+哈希表键为整数元素数量 */
    for (unsigned int i = 0, twotoi = 1; i < MAXABITS + 1; i++, twotoi *= 2) {
        sum_int_keys += nums[i];
        if (sum_int_keys > twotoi / 2) {
            array_size = twotoi;
            sum_array_used = sum_int_keys;
        }
    }
    *array_used_num = sum_array_used;
    return array_size;
}

//** nums[i] = number of keys 'k' where 2^(i - 1) < k <= 2^i 
static void rehash(struct lua_State* L, struct Table* t, const TValue* key) {
    int nums[MAXABITS + 1]; /* nums[i]代表key位于 [2^(i-1) , 2^i) 之间元素数 */
    for (int i = 0; i < MAXABITS + 1; i++) {
        nums[i] = 0;
    }

    int totaluse = 0;
    int array_used_num = 0;  //
    
    totaluse = numsarray(t, nums);
    totaluse += numshash(t, nums);

    totaluse++;
    if (ttisinteger(key) && key->value_.i > 0) {
        int temp = luaO_ceillog2(key->value_.i);
        if (temp < MAXABITS - 1 && temp >= 0)
            nums[temp]++;
    }

    int asize = compute_array_size(t, nums, &array_used_num);
    luaH_resize(L, t, asize, totaluse - array_used_num);//???
}

/* 试图在哈希表内加入一个新键，返回这个键对应Node的value
** 如果key是整数，这时哈希表没有空间，就会rehash，rehash后获取的空间就可能在数组了
 */
TValue* luaH_newkey(struct lua_State* L, struct Table* t, const TValue* key) {
    if (ttisnil(key)) {
        luaD_throw(L, LUA_ERRRUN);
    }

    TValue k;
    if (ttisfloat(key)) {
        if (lua_numisnan(key->value_.n)) {
            luaD_throw(L, LUA_ERRRUN);
        }

        setivalue(&k,l_hashfloat(key->value_.n));

        // k.value_.i = l_hashfloat(key->value_.n);
        // k.tt_ = LUA_TNUMINT;
        key = &k;
    }
    Node* main_node = mainposition(L, t, key); /* main_node指向键对应的Node */

    if (!ttisnil(getval(main_node)) || isdummy(t)) { /* 键对应的Node有值了 */
        Node* lastfree = getlastfree(t); 

        if (lastfree == NULL) {
            rehash(L, t, key); /* 哈希完毕必须保证有合法位置 */
            return luaH_set(L, t, key);
        }

        Node* other_node = mainposition(L, t, getkey(main_node));
        if (other_node != main_node) { /* 表示目前main_node指向的节点没有位于其mainpos */
            
            /* 找到main_node的前继节点，最后由other_node指向 */
            while(other_node + other_node->key.nk.next != main_node)
                other_node += other_node->key.nk.next;
            
            /* main_node的键值迁移到lastfree，main_node设为空 */
            other_node->key.nk.next = lastfree - other_node;
            setobj(getwkey(lastfree), getwkey(main_node));
            setobj(getval(lastfree), getval(main_node));

            main_node->key.nk.next = 0;
            setnilvalue(getval(main_node));
        } else {
			if (main_node->key.nk.next != 0) {
				Node* next = main_node + main_node->key.nk.next;
				lastfree->key.nk.next = next - lastfree;
			}
            main_node->key.nk.next = lastfree - main_node; /* main_node指向节点的后继节点设为last_free指向的节点 */
            main_node = lastfree; /* 更改指向，下面通过main_node指针统一赋值 */
        }
    }
    

    setobj(getwkey(main_node), cast(TValue*, key));
    luaC_barrierback(L, t, key);
    lua_assert(ttisnil(getval(main_node)));
    return getval(main_node);
}

static unsigned int arrayindex(struct Table* t, const TValue* key) {
    if (ttisinteger(key)) {
        lua_Integer k = key->value_.i;
        if ((k > 0) && (lua_Unsigned)k < MAXASIZE) {
            return cast(unsigned int, k);
        }
    }
    return 0;
}
/* 如果key对应的值在数组内，返回数组下标(1开始);在哈希表内则返回相对于哈希数组首部的偏移量 */
static unsigned int findindex(struct lua_State* L, struct Table* t, const TValue* key) {
    unsigned int i = arrayindex(t, key);
    if (i != 0 && i <= t->arraysize) {
        return i;
    } else {
        Node* n = mainposition(L, t, key);
        while(true) {
		    // lua允许在遍历的过程中，为table的域赋nil值，这样可能在迭代过程中
            // 遇到gc的情况，将刚刚遍历过的key设置为dead key，如果不处理这种情况
            // 那么迭代将会跳过下一个key，进入下下个key，导致遍历不到下一个key
            if (luaV_eqobject(L, getkey(n), key) || 
                    (ttisdeadkey(getkey(n)) && iscollectable(key) && (gcvalue(getkey(n)) == gcvalue(key)))) {
                i = n - getnode(t, 0);
                return (i + 1) + t->arraysize;
            }

            if (n->key.nk.next == 0) {
                luaD_throw(L, LUA_ERRRUN);
            }
            n += n->key.nk.next;
        }
    }
    return 0;
}

int luaH_next(struct lua_State* L, struct Table* t, TValue* key) {
    unsigned int i = findindex(L, t, key);
    for (; i < t->arraysize; i++) {
        if (!ttisnil(&t->array[i])) {
            setivalue(key, i + 1);
            setobj(key + 1, &t->array[i]);
            return 1;
        }
    }
    
    for (i -= t->arraysize; i < twoto(t->lsizenode); i++) {
        Node* n = getnode(t, i);
        if (!ttisnil(getval(n))) {
            setobj(key, getwkey(n));
            setobj(key + 1, getval(n));
            return 1;
        }
    }
    return 0;
}

/* 统计数组中不为nil的元素个数 */
int luaH_getn(struct lua_State* L, struct Table* t) {
    int n = 0;
    for (unsigned int i = 0; i < t->arraysize; i++) {
        if (!ttisnil(&t->array[i])) {
            n++;
        } else {
            return n;
        }
    }
    return n;
}