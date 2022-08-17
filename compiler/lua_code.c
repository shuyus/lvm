#include "lua_code.h"
#include "../common/lua_object.h"
#include "../common/lua_state.h"
#include "../common/lua_table.h"
#include "../common/lua_mem.h"
#include "../common/lua_string.h"
#include "../vm/lua_gc.h"
#include "../vm/lua_opcodes.h"
#include "../vm/lua_vm.h"

#define MAININDEX 255

static void freereg(FuncState* fs, int reg);
static void freeexp(FuncState* fs, expdesc* e);
static int exp2reg(FuncState* fs, expdesc* e, int reg);


static int addk(FuncState* fs, TValue* v) {
	LexState* ls = fs->ls;
	Proto* p = fs->p;
	TValue* idx = luaH_set(ls->L, ls->h, v);  /* 常量缓存表内查一下v有没有对应的k */
	if (!ttisnil(idx)) {
		int k = idx->value_.i;
		if (k < fs->nk && luaV_eqobject(ls->L, &p->k[k], v)) {
			return k;
		}
	}

	int k = fs->nk;
	luaM_growvector(ls->L, p->k, fs->nk, p->sizek, TValue, INT_MAX);

	for (int i = k; i < p->sizek; i++) {
		setnilvalue(&p->k[i]);
	}

	setobj(&p->k[k], v);
	setivalue(idx, k);

	fs->nk++;
	return k;
}

static int tonumeral(expdesc* e, TValue* v) {
	int ret = 0;

	switch (e->k) {
	case VINT: {
		if (v) {
			setivalue(v, e->u.i);
		}
		ret = 1;
	} break;
	case VFLT: {
		if (v) {
			setfltvalue(v, e->u.r);
		}
		ret = 1;
	} break;
	default:break;
	}

	return ret;
}

static int validop(FuncState* fs, const TValue* v1, const TValue* v2, int op) {
	switch (op) {
	case LUA_OPT_BAND: case LUA_OPT_BOR: case LUA_OPT_BXOR: case LUA_OPT_BNOT:
	case LUA_OPT_IDIV: case LUA_OPT_SHL: case LUA_OPT_SHR: case LUA_OPT_MOD: {
		if (!ttisinteger(v1) || !ttisinteger(v2))
			return 0;

		if (op == LUA_OPT_IDIV && v2->value_.i == 0) {
			return 0;
		}
	} return 1;
	case LUA_OPT_UMN: case LUA_OPT_DIV: case LUA_OPT_ADD: case LUA_OPT_SUB:
	case LUA_OPT_MUL: case LUA_OPT_POW: {
		lua_State* L = fs->ls->L;
		lua_Number n1, n2;
		if (!luaV_tonumber(L, v1, &n1) || !luaV_tonumber(L, v2, &n2)) {
			return 0;
		}

		if (op == LUA_OPT_DIV && n2 == 0.0f) {
			return 0;
		}
	} return 1;
	default:return 1;
	}
}

static int constfolding(FuncState* fs, int op, expdesc* e1, expdesc* e2) {
	TValue v1, v2;
	if (!(tonumeral(e1, &v1) && tonumeral(e2, &v2) && validop(fs, &v1, &v2, op))) {
		return 0;
	}

	luaO_arith(fs->ls->L, op, &v1, &v2);
	if (ttisinteger(&v1)) {
		e1->k = VINT;
		e1->u.i = v1.value_.i;
	} else {
		e1->k = VFLT;
		e1->u.r = v1.value_.n;
	}

	return 1;
}

/* 前缀# */
static void codeunexpval(FuncState* fs, int op, expdesc* e) {
	luaK_exp2anyreg(fs, e);
	freeexp(fs, e);

	int opcode;

	switch (op) {
	case UNOPR_MINUS: {
		opcode = OP_UNM;
	} break;
	case UNOPR_BNOT: {
		opcode = OP_BNOT;
	} break;
	case UNOPR_LEN: {
		opcode = OP_LEN;
	} break;
	default:lua_assert(0);
	}

	e->u.info = luaK_codeABC(fs, opcode, 0, e->u.info, 0);
	e->k = VRELOCATE;
}

//static void codenot(FuncState* fs, expdesc* e) {
//	switch (e->k) {
//	case VFALSE: case VNIL: {
//		e->k = VTRUE;
//	} return;
//	case VTRUE: case VINT: case VFLT: {
//		e->k = VFALSE;
//	} return;
//	default:break;
//	}
//
//	discharge2anyreg(fs, e);
//	freeexp(fs, e);
//	lua_assert(e->k == VNONRELOC);
//	e->u.info = luaK_codeABC(fs, OP_NOT, 0, e->u.info, 0);
//	e->k = VRELOCATE;
//}



/* 前缀 - ~ # not  */
//void luaK_prefix(FuncState* fs, int op, expdesc* e) {
//	expdesc ef;
//	ef.k = VINT; ef.u.info = 0; ef.t = ef.f = NO_JUMP;
//
//	switch (op) {
//	case UNOPR_MINUS: case UNOPR_BNOT: {
//		if (constfolding(fs, LUA_OPT_UMN + op, e, &ef)) {
//			break;
//		}
//	}
//	case UNOPR_LEN: {
//		codeunexpval(fs, op, e);
//	} break;
//	case UNOPR_NOT: {
//		codenot(fs, e);
//	} break;
//	default:lua_assert(0);
//	}
//}

int luaK_exp2RK(FuncState* fs, expdesc* e) {
	switch (e->k) {
	case VNIL: e->u.info = luaK_nilK(fs); goto vk;
	case VFLT: e->u.info = luaK_floatK(fs, e->u.r); goto vk;
	case VINT: e->u.info = luaK_intK(fs, e->u.i); goto vk;
	case VTRUE: case VFALSE: e->u.info = luaK_boolK(fs, e->k == VTRUE); goto vk;
	case VK: {
	vk:
		e->k = VK;
		if (e->u.info <= MAININDEX) {
			return RKMASK(e->u.info);
		}
	} break;
	default:break;
	}

	return luaK_exp2anyreg(fs, e);
}

int luaK_stringK(FuncState* fs, TString* key) {
	TValue value;
	setgcovalue(&value, obj2gco(key));
	return addk(fs, &value);
}

int luaK_nilK(FuncState* fs) {
	TString* nil = luaS_newliteral(fs->ls->L, "nil");
	TValue value;
	setgcovalue(&value, obj2gco(nil));
	return addk(fs, &value);
}

int luaK_floatK(FuncState* fs, lua_Number r) {
	TValue value;
	setfltvalue(&value, (float)r);
	return addk(fs, &value);
}

int luaK_intK(FuncState* fs, lua_Integer i) {
	TValue value;
	setivalue(&value, i);
	return addk(fs, &value);
}

int luaK_boolK(FuncState* fs, int v) {
	TValue value;
	setbvalue(&value, v != 0);
	return addk(fs, &value);
}

int luaK_ret(FuncState* fs, int first, int nret) {
	luaK_codeABC(fs, OP_RETURN, first, nret + 1, 0);
	return LUA_OK;
}

void luaK_indexed(FuncState* fs, expdesc* e, expdesc* key) {
	luaO_printProto(fs->p);
	e->u.ind.t = e->u.info;
	e->u.ind.idx = luaK_exp2RK(fs, key);
	e->u.ind.vt = e->k == VLOCAL ? VLOCAL : VUPVAL;
	e->k = VINDEXED;
}

int luaK_codeABC(FuncState* fs, int opcode, int a, int b, int c) {
	luaM_growvector(fs->ls->L, fs->p->code, fs->pc, fs->p->sizecode, Instruction, INT_MAX);
	Instruction i = (b << POS_B) | (c << POS_C) | (a << POS_A) | opcode;
	fs->p->code[fs->pc] = i;

	return fs->pc++;
}

int luaK_codeABx(FuncState* fs, int opcode, int a, int bx) {
	luaM_growvector(fs->ls->L, fs->p->code, fs->pc, fs->p->sizecode, Instruction, INT_MAX);
	Instruction i = (bx << POS_C) | (a << POS_A) | opcode;
	fs->p->code[fs->pc] = i;

	return fs->pc++;
}

void luaK_dischargevars(FuncState* fs, expdesc* e) {
	switch (e->k) {
	case VLOCAL: {
		e->k = VNONRELOC;
	} break;
	case VUPVAL: {
		e->u.info = luaK_codeABC(fs, OP_GETUPVAL, 0, e->u.info, 0);
		e->k = VRELOCATE;
	} break;
	case VINDEXED: { /* 这里还没有填充A */
		if (e->u.ind.vt == VLOCAL) {
			e->u.info = luaK_codeABC(fs, OP_GETTABLE, 0, e->u.ind.t, e->u.ind.idx);
		} else { // VUPVAL
			e->u.info = luaK_codeABC(fs, OP_GETTABUP, 0, e->u.ind.t, e->u.ind.idx);
		}
		e->k = VRELOCATE;
	} break;
	case VCALL: {
		e->k = VRELOCATE;
		e->u.info = GET_ARG_A(fs->p->code[e->u.info]);
	} break;
	default:break;
	}
}

static void freereg(FuncState* fs, int reg) {
	if (!ISK(reg) && reg > fs->nactvars) {
		fs->freereg--;
		lua_assert(fs->freereg == reg);
	}
}

static void freeexp(FuncState* fs, expdesc* e) {
	if (e->k == VNONRELOC) {
		freereg(fs, e->u.info);
	}
}

static void checkstack(FuncState* fs, int n) {
	if (fs->freereg + n > fs->p->maxstacksize) {
		fs->p->maxstacksize = fs->freereg + n;
	}
}

/* fs->freereg += n */
static void reserve_reg(FuncState* fs, expdesc* e, int n) {
	checkstack(fs, n);
	fs->freereg += n;
}

/* 填充寄存器 */
static int exp2reg(FuncState* fs, expdesc* e, int reg) {
	switch (e->k) {
	case VNIL: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_nilK(fs));
	} break;
	case VFLT: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_floatK(fs, e->u.r));
	} break;
	case VINT: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_intK(fs, e->u.i));
	} break;
	case VTRUE: case VFALSE: {
		luaK_codeABx(fs, OP_LOADK, reg, luaK_boolK(fs, e->k == VTRUE));
	} break;
	case VK: {
		luaK_codeABx(fs, OP_LOADK, reg, e->u.info);
	} break;
	case VRELOCATE: { /* 填充A */
		SET_ARG_A(fs->p->code[e->u.info], reg);
	} break;
	default:break;
	}

	e->k = VNONRELOC;
	e->u.info = reg;

	return e->u.info;
}

int luaK_exp2nextreg(FuncState* fs, expdesc* e) {
	luaK_dischargevars(fs, e);
	freeexp(fs, e);
	reserve_reg(fs, e, 1);
	return exp2reg(fs, e, fs->freereg - 1);
}

int luaK_exp2anyreg(FuncState* fs, expdesc * e) {
	luaK_dischargevars(fs, e);
	if (e->k == VNONRELOC) {
		return e->u.info;
	}
	return luaK_exp2nextreg(fs, e);
}

void luaK_exp2anyregup(FuncState* fs, expdesc* e) {
	if (e->k != VUPVAL) {
		luaK_exp2anyreg(fs, e);
	}
}