#include "lua_debug.h"
#include "stdarg.h"
#include "../common/lua_table.h"
#include "../common/lua.h"

char* OpCodeDesc[] = {
    "OP_MOVE",
    "OP_LOADK",
    "OP_GETUPVAL",
    "OP_CALL",
    "OP_RETURN",
    "OP_GETTABUP",
    "OP_GETTABLE",
    "OP_SELF",
    "OP_TEST",
    "OP_TESTSET",
    "OP_JUMP",
    "OP_UNM",
    "OP_LEN",
    "OP_BNOT",
    "OP_NOT",
    "OP_ADD",
    "OP_SUB",
    "OP_MUL",
    "OP_DIV",
    "OP_IDIV",
    "OP_MOD",
    "OP_POW",
    "OP_BAND",
    "OP_BOR",
    "OP_BXOR",
    "OP_SHL",
    "OP_SHR",
    "OP_CONCAT",
    "OP_EQ",
    "OP_LT",
    "OP_LE",
    "OP_LOADBOOL",
    "OP_LOADNIL",
    "OP_SETUPVAL",
    "OP_SETTABUP",
    "OP_NEWTABLE",
    "OP_SETLIST",
    "OP_SETTABLE",
    "NUM_OPCODES"
};

TValue* luaL_index2addr(struct lua_State* L, int idx) {
    if (idx >= 0) {
        lua_assert(L->ci->func + idx < L->ci->top);
        return L->ci->func + idx;
    } else {
        lua_assert(L->top + idx > L->ci->func);
        return L->top + idx;
    }
}

void luaH_TableInfo(struct lua_State* L,int idx){
    TValue* o = luaL_index2addr(L, idx);
    Table* h =hvalue(o);
    printf("%p table info arraysize %d lsizenode %d\n",h,h->arraysize,h->lsizenode);
}

void luaO_Print(const TValue * t){
    switch (ttype(t))
    {

    case LUA_TNIL: {
        printf("%p | nil  \n", t);
        break; }
    case LUA_TNUMINT: {
        lua_Integer i = ivalue(t);
        printf("%p | Int : %lld \n", t, i);
        break;
    }
    case LUA_TNUMFLT: {
        lua_Number f = fltvalue(t);
        printf("%p | Float : %f \n", t, f);
        break;
    }
    case LUA_TSHRSTR: {
        TString* sts = tsvalue(t);
        printf("%p | SSTR Hash: %ld : %s  ||\n", t->value_.gc, sts->hash, sts->data);
        break;
    }
    case LUA_TLNGSTR: {
        TString* lts = tsvalue(t);
        printf("%p | LSTR : %s  ||\n", t->value_.gc, lts->data);
        break;
    }
    case LUA_TBOOLEAN: {
        lua_Integer b = ivalue(t);
        printf("%p | bool \n", t);
        break;
    }
    case LUA_TTABLE: {
        Table* tbl = hvalue(t);
        printf("%p | Table | lsizenode:%d \n", t->value_.gc, tbl->lsizenode);
        break;
    }
    case LUA_TLCF: {
        lua_CFunction fnc = lcfvalue(t);
        printf("%p | light C Function \n", t);
        break;
    }
    case LUA_TLCL: {
        LClosure* lcl = lclvalue(t);
        printf("%p | LClosure \n", t);
        break;
    }
    case LUA_TCCL: {
        CClosure* ccl = cclvlaue(t);
        printf("%p |  CClosure \n", t);
        break;
    }
    default:
        printf("%p | unkonwn type: %d\n",t,ttype(t));
        break;
    }
}

void luaO_PrintGC(const GCObject* t){
    switch (t->tt_)
    {

    case LUA_TSHRSTR: {
        TString* sts = gco2ts(t);
        printf("%p | SSTR Hash: %d : %s  ||\n", t, sts->hash, sts->data);
        break;
    }
    case LUA_TLNGSTR: {
        TString* lts = gco2ts(t);
        printf("%p | LSTR : %s  ||\n", t, lts->data);
        break;
    }
    case LUA_TTABLE: {
        Table* tbl = gco2h(t);
        printf("%p | Table | lsizenode:%d \n", t, tbl->lsizenode);
        break;
    }
    
    default:
        printf("unkonwn GC\n");
        break;
    }
}

void luaH_DumpTable(lua_State * L, Table * t){
    printf("========DumpTable address:%p ==============\n",t);
    printf("arraysize: %d lsizenode: %d\n",t->arraysize,t->lsizenode);
    printf("Array:\n");
    for(unsigned int i=0; i<t->arraysize;i++){
        printf("\t  i:%d v:\t",i+1);
        luaO_Print(t->array+i);
    }

    printf("Hash:\n");
    for(int k=0; k<twoto(t->lsizenode);k++){
        printf("\t key:  ");
        luaO_Print(getkey(t->node+k));
        printf("\t val:  ");
        luaO_Print(getval(t->node+k));
    }
    printf("========DumpTable   OVER  ==============\n");
}

void luaO_printProto(Proto* p) {
    //printf("==============localval=====================\n");
    //
    //printf("==============upval=====================\n");
    //for (int i = 0; i < p->sizeupvalues; i++) {
    //    p->upvalues[i];
    //}
    printf("==============常量表k=====================\n");
    for (int i = 0; i < p->sizek; i++) {
        TValue * k = &p->k[i];
        luaO_Print(k);
    }

}

void luaK_printInstruction(const Instruction* i) {
    printf("=============指令=========================\n");
    bool is_return = false;
   // printf("%p %s", OpCodeDesc[4], OpCodeDesc[4]);
    while (!is_return) {
        char* desc = GET_OPCODE_DESC(*i);

        printf("%s ", desc);
        is_return = IS_RETURN_DESC(desc);
        i++;
    }
    printf("\n");
}


void Debug_Printf(int n,...){
    if(n<1) return;
    __asm
    {
        push eax
        push ebx
        push ecx

        mov ecx, n
        mov eax, 4
        mul ecx
        mov ecx, eax   ; //size of bytes
        lea eax, [n]
        add eax, ecx   ; // address of the last slot
        
        mov ecx, n
        mov ebx, esp

    _loo:
        push [eax]
        sub eax, 4
        dec ecx
        jnz _loo

        call printf
        
        mov esp, ebx

        pop ecx
        pop ebx
        pop eax
    }
}