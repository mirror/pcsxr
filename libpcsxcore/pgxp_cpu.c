
#include "pgxp_cpu.h"
#include "pgxp_value.h"
#include "pgxp_mem.h"

#include "pgxp_debug.h"

// CPU registers
PGXP_value CPU_reg_mem[34];
//PGXP_value CPU_Hi, CPU_Lo;
PGXP_value CP0_reg_mem[32];

PGXP_value* CPU_reg = CPU_reg_mem;
PGXP_value* CP0_reg = CP0_reg_mem;

// Instruction register decoding
#define op(_instr)		(_instr >> 26)			// The op part of the instruction register 
#define func(_instr)	((_instr) & 0x3F)		// The funct part of the instruction register 
#define sa(_instr)		((_instr >>  6) & 0x1F) // The sa part of the instruction register
#define rd(_instr)		((_instr >> 11) & 0x1F)	// The rd part of the instruction register 
#define rt(_instr)		((_instr >> 16) & 0x1F)	// The rt part of the instruction register 
#define rs(_instr)		((_instr >> 21) & 0x1F)	// The rs part of the instruction register 
#define imm(_instr)		(_instr & 0xFFFF)		// The immediate part of the instruction register

void PGXP_InitCPU()
{
	memset(CPU_reg_mem, 0, sizeof(CPU_reg_mem));
	memset(CP0_reg_mem, 0, sizeof(CP0_reg_mem));
}

// invalidate register (invalid 8 bit read)
void InvalidLoad(u32 addr, u32 code, u32 value)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	PGXP_value* pD = NULL;
	PGXP_value p;

	p.x = p.y = -1337; // default values

					   //p.valid = 0;
					   //p.count = value;
	pD = ReadMem(addr);

	if (pD)
	{
		p.count = addr;
		p = *pD;
	}
	else
	{
		p.count = value;
	}

	p.flags = 0;

	// invalidate register
	CPU_reg[reg] = p;
}

// invalidate memory address (invalid 8 bit write)
void InvalidStore(u32 addr, u32 code, u32 value)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	PGXP_value* pD = NULL;
	PGXP_value p;

	pD = ReadMem(addr);

	p.x = p.y = -2337;

	if (pD)
		p = *pD;

	p.flags = 0;
	p.count = (reg * 1000) + value;

	// invalidate memory
	WriteMem(&p, addr);
}

////////////////////////////////////
// Arithmetic with immediate value
////////////////////////////////////
void PGXP_CPU_ADDI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs + Imm (signed)
	psx_value tempImm;
	PGXP_value ret;
	
	Validate(&CPU_reg[rs(instr)], rsVal);
	ret = CPU_reg[rs(instr)];
	tempImm.d = imm(instr);
	tempImm.sd = (tempImm.sd << 16) >> 16;	// sign extend

	ret.x = f16Unsign(ret.x);
	ret.x += tempImm.w.l;

	// carry on over/underflow
	float of = (ret.x > USHRT_MAX) ? 1.f : (ret.x < 0) ? -1.f : 0.f;
	ret.x = f16Sign(ret.x);
	//ret.x -= of * (USHRT_MAX + 1);
	ret.y += tempImm.sw.h + of;

	// truncate on overflow/underflow
	ret.y += (ret.y > SHRT_MAX) ? -(USHRT_MAX + 1) : (ret.y < SHRT_MIN) ? USHRT_MAX + 1 : 0.f;

	CPU_reg[rt(instr)] = ret;
	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_ADDIU(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs + Imm (signed) (unsafe?)
	PGXP_CPU_ADDI(instr, rtVal, rsVal);
}

void PGXP_CPU_ANDI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs & Imm
	psx_value vRt;
	PGXP_value ret;

	Validate(&CPU_reg[rs(instr)], rsVal);
	ret = CPU_reg[rs(instr)];

	vRt.d = rtVal;

	ret.y = 0.f;	// remove upper 16-bits

	switch (imm(instr))
	{
	case 0:
		// if 0 then x == 0
		ret.x = 0.f;
		break;
	case 0xFFFF:
		// if saturated then x == x
		break;
	default:
		// otherwise x is low precision value
		ret.x = vRt.sw.l;
		ret.flags |= VALID_0;
	}

	ret.flags |= VALID_1;

	CPU_reg[rt(instr)] = ret;
	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_ORI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs | Imm
	psx_value vRt;
	PGXP_value ret;

	Validate(&CPU_reg[rs(instr)], rsVal);
	ret = CPU_reg[rs(instr)];

	vRt.d = rtVal;

	switch (imm(instr))
	{
	case 0:
		// if 0 then x == x
		break;
	default:
		// otherwise x is low precision value
		ret.x = vRt.sw.l;
		ret.flags |= VALID_0;
	}

	ret.value = rtVal;
	CPU_reg[rt(instr)] = ret;
}

void PGXP_CPU_XORI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs ^ Imm
	psx_value vRt;
	PGXP_value ret;

	Validate(&CPU_reg[rs(instr)], rsVal);
	ret = CPU_reg[rs(instr)];

	vRt.d = rtVal;

	switch (imm(instr))
	{
	case 0:
		// if 0 then x == x
		break;
	default:
		// otherwise x is low precision value
		ret.x = vRt.sw.l;
		ret.flags |= VALID_0;
	}

	ret.value = rtVal;
	CPU_reg[rt(instr)] = ret;
}

void PGXP_CPU_SLTI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs < Imm (signed)
	psx_value tempImm;
	PGXP_value ret;

	Validate(&CPU_reg[rs(instr)], rsVal);
	ret = CPU_reg[rs(instr)];

	tempImm.w.h = imm(instr);
	ret.y		= 0.f;
	ret.x		= (CPU_reg[rs(instr)].x < tempImm.sw.h) ? 1.f : 0.f;
	ret.flags	|= VALID_1;
	ret.value	= rtVal;

	CPU_reg[rt(instr)] = ret;
}

void PGXP_CPU_SLTIU(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs < Imm (Unsigned)
	psx_value tempImm;
	PGXP_value ret;

	Validate(&CPU_reg[rs(instr)], rsVal);
	ret = CPU_reg[rs(instr)];

	tempImm.w.h	= imm(instr);
	ret.y		= 0.f;
	ret.x		= (f16Unsign(CPU_reg[rs(instr)].x) < tempImm.w.h) ? 1.f : 0.f;
	ret.flags	|= VALID_1;
	ret.value	= rtVal;

	CPU_reg[rt(instr)] = ret;
}

////////////////////////////////////
// Load Upper
////////////////////////////////////
void PGXP_CPU_LUI(u32 instr, u32 rtVal)
{
	//Rt = Imm << 16
	CPU_reg[rt(instr)] = PGXP_value_zero;
	CPU_reg[rt(instr)].y = (float)(s16)imm(instr);
	CPU_reg[rt(instr)].hFlags = VALID_HALF;
	CPU_reg[rt(instr)].value = rtVal;
	CPU_reg[rt(instr)].flags = VALID_01;
}

////////////////////////////////////
// Register Arithmetic
////////////////////////////////////

void PGXP_CPU_ADD(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs + Rt (signed)
	PGXP_value ret;
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	ret = CPU_reg[rs(instr)];

	ret.x = f16Unsign(ret.x);
	ret.x += f16Unsign(CPU_reg[rt(instr)].x);

	// carry on over/underflow
	float of = (ret.x > USHRT_MAX) ? 1.f : (ret.x < 0) ? -1.f : 0.f;
	ret.x = f16Sign(ret.x);
	//ret.x -= of * (USHRT_MAX + 1);
	ret.y += CPU_reg[rt(instr)].y + of;

	// truncate on overflow/underflow
	ret.y += (ret.y > SHRT_MAX) ? -(USHRT_MAX + 1) : (ret.y < SHRT_MIN) ? USHRT_MAX + 1 : 0.f;

	// TODO: decide which "z/w" component to use

	ret.halfFlags[0] &= CPU_reg[rt(instr)].halfFlags[0];
	ret.gFlags |= CPU_reg[rt(instr)].gFlags;
	ret.lFlags |= CPU_reg[rt(instr)].lFlags;
	ret.hFlags |= CPU_reg[rt(instr)].hFlags;

	ret.value = rdVal;

	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_ADDU(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs + Rt (signed) (unsafe?)
	PGXP_CPU_ADD(instr, rdVal, rsVal, rtVal);
}

void PGXP_CPU_SUB(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs - Rt (signed)
	PGXP_value ret;
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	ret = CPU_reg[rs(instr)];

	ret.x = f16Unsign(ret.x);
	ret.x -= f16Unsign(CPU_reg[rt(instr)].x);

	// carry on over/underflow
	float of = (ret.x > USHRT_MAX) ? 1.f : (ret.x < 0) ? -1.f : 0.f;
	ret.x = f16Sign(ret.x);
	//ret.x -= of * (USHRT_MAX + 1);
	ret.y -= CPU_reg[rt(instr)].y - of;

	// truncate on overflow/underflow
	ret.y += (ret.y > SHRT_MAX) ? -(USHRT_MAX + 1) : (ret.y < SHRT_MIN) ? USHRT_MAX + 1 : 0.f;

	ret.halfFlags[0] &= CPU_reg[rt(instr)].halfFlags[0];
	ret.gFlags |= CPU_reg[rt(instr)].gFlags;
	ret.lFlags |= CPU_reg[rt(instr)].lFlags;
	ret.hFlags |= CPU_reg[rt(instr)].hFlags;

	ret.value = rdVal;

	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_SUBU(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs - Rt (signed) (unsafe?)
	PGXP_CPU_SUB(instr, rdVal, rsVal, rtVal);
}

void PGXP_CPU_AND(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs & Rt
	psx_value vald, vals, valt;
	PGXP_value ret;

	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	vald.d = rdVal;
	vals.d = rsVal;
	valt.d = rtVal;

	//	CPU_reg[rd(instr)].valid = CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid;
	ret.flags = VALID_01;

	if (vald.w.l == 0)
	{
		ret.x = 0.f;
		ret.lFlags = VALID_HALF;
	}
	else if (vald.w.l == vals.w.l)
	{
		ret.x = CPU_reg[rs(instr)].x;
		ret.lFlags = CPU_reg[rs(instr)].lFlags;
		ret.compFlags[0] = CPU_reg[rs(instr)].compFlags[0];
	}
	else if (vald.w.l == valt.w.l)
	{
		ret.x = CPU_reg[rt(instr)].x;
		ret.lFlags = CPU_reg[rt(instr)].lFlags;
		ret.compFlags[0] = CPU_reg[rt(instr)].compFlags[0];
	}
	else
	{
		ret.x = (float)vald.sw.l;
		ret.compFlags[0] = VALID;
		ret.lFlags = 0;
	}

	if (vald.w.h == 0)
	{
		ret.y = 0.f;
		ret.hFlags = VALID_HALF;
	}
	else if (vald.w.h == vals.w.h)
	{
		ret.y = CPU_reg[rs(instr)].y;
		ret.hFlags = CPU_reg[rs(instr)].hFlags;
		ret.compFlags[1] &= CPU_reg[rs(instr)].compFlags[1];
	}
	else if (vald.w.h == valt.w.h)
	{
		ret.y = CPU_reg[rt(instr)].y;
		ret.hFlags = CPU_reg[rt(instr)].hFlags;
		ret.compFlags[1] &= CPU_reg[rt(instr)].compFlags[1];
	}
	else
	{
		ret.y = (float)vald.sw.h;
		ret.compFlags[1] = VALID;
		ret.hFlags = 0;
	}

	// iCB Hack: Force validity if even one half is valid
	//if ((ret.hFlags & VALID_HALF) || (ret.lFlags & VALID_HALF))
	//	ret.valid = 1;
	// /iCB Hack

	// Get a valid W
	if ((CPU_reg[rs(instr)].flags & VALID_2) == VALID_2)
	{
		ret.z = CPU_reg[rs(instr)].z;
		ret.compFlags[2] = CPU_reg[rs(instr)].compFlags[2];
	}
	else if((CPU_reg[rt(instr)].flags & VALID_2) == VALID_2)
	{
		ret.z = CPU_reg[rt(instr)].z;
		ret.compFlags[2] = CPU_reg[rt(instr)].compFlags[2];
	}

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_OR(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs | Rt
	PGXP_CPU_AND(instr, rdVal, rsVal, rtVal);
}

void PGXP_CPU_XOR(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs ^ Rt
	PGXP_CPU_AND(instr, rdVal, rsVal, rtVal);
}

void PGXP_CPU_NOR(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs NOR Rt
	PGXP_CPU_AND(instr, rdVal, rsVal, rtVal);
}

void PGXP_CPU_SLT(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs < Rt (signed)
	PGXP_value ret;
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	ret = CPU_reg[rs(instr)];
	ret.y = 0.f;
	ret.compFlags[1] = VALID;

	ret.x = (CPU_reg[rs(instr)].y < CPU_reg[rt(instr)].y) ? 1.f : (f16Unsign(CPU_reg[rs(instr)].x) < f16Unsign(CPU_reg[rt(instr)].x)) ? 1.f : 0.f;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_SLTU(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs < Rt (unsigned)
	PGXP_value ret;
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	ret = CPU_reg[rs(instr)];
	ret.y = 0.f;
	ret.compFlags[1] = VALID;

	ret.x = (f16Unsign(CPU_reg[rs(instr)].y) < f16Unsign(CPU_reg[rt(instr)].y)) ? 1.f : (f16Unsign(CPU_reg[rs(instr)].x) < f16Unsign(CPU_reg[rt(instr)].x)) ? 1.f : 0.f;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

////////////////////////////////////
// Register mult/div
////////////////////////////////////

void PGXP_CPU_MULT(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Hi/Lo = Rs * Rt (signed)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	CPU_Lo = CPU_Hi = CPU_reg[rs(instr)];

	CPU_Lo.halfFlags[0] = CPU_Hi.halfFlags[0] = (CPU_reg[rs(instr)].halfFlags[0] & CPU_reg[rt(instr)].halfFlags[0]);

	double xx, xy, yx, yy;
	double lx = 0, ly = 0, hx = 0, hy = 0;
	s64 of = 0;

	// Multiply out components
	xx = f16Unsign(CPU_reg[rs(instr)].x) * f16Unsign(CPU_reg[rt(instr)].x);
	xy = f16Unsign(CPU_reg[rs(instr)].x) * (CPU_reg[rt(instr)].y);
	yx = (CPU_reg[rs(instr)].y) * f16Unsign(CPU_reg[rt(instr)].x);
	yy = (CPU_reg[rs(instr)].y) * (CPU_reg[rt(instr)].y);

	// Split values into outputs
	lx = xx;

	ly = f16Overflow(xx);
	ly += xy + yx;

	hx = f16Overflow(ly);
	hx += yy;

	hy = f16Overflow(hx);

	CPU_Lo.x = f16Sign(lx);
	CPU_Lo.y = f16Sign(ly);
	CPU_Hi.x = f16Sign(hx);
	CPU_Hi.y = f16Sign(hy);

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

void PGXP_CPU_MULTU(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Hi/Lo = Rs * Rt (unsigned)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	CPU_Lo = CPU_Hi = CPU_reg[rs(instr)];

	CPU_Lo.halfFlags[0] = CPU_Hi.halfFlags[0] = (CPU_reg[rs(instr)].halfFlags[0] & CPU_reg[rt(instr)].halfFlags[0]);

	double xx, xy, yx, yy;
	double lx = 0, ly = 0, hx = 0, hy = 0;
	s64 of = 0;

	// Multiply out components
	xx = f16Unsign(CPU_reg[rs(instr)].x) * f16Unsign(CPU_reg[rt(instr)].x);
	xy = f16Unsign(CPU_reg[rs(instr)].x) * f16Unsign(CPU_reg[rt(instr)].y);
	yx = f16Unsign(CPU_reg[rs(instr)].y) * f16Unsign(CPU_reg[rt(instr)].x);
	yy = f16Unsign(CPU_reg[rs(instr)].y) * f16Unsign(CPU_reg[rt(instr)].y);

	// Split values into outputs
	lx = xx;

	ly = f16Overflow(xx);
	ly += xy + yx;

	hx = f16Overflow(ly);
	hx += yy;

	hy = f16Overflow(hx);

	CPU_Lo.x = f16Sign(lx);
	CPU_Lo.y = f16Sign(ly);
	CPU_Hi.x = f16Sign(hx);
	CPU_Hi.y = f16Sign(hy);

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

void PGXP_CPU_DIV(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Lo = Rs / Rt (signed)
	// Hi = Rs % Rt (signed)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	//// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	CPU_Lo = CPU_Hi = CPU_reg[rs(instr)];

	CPU_Lo.halfFlags[0] = CPU_Hi.halfFlags[0] = (CPU_reg[rs(instr)].halfFlags[0] & CPU_reg[rt(instr)].halfFlags[0]);

	double vs = f16Unsign(CPU_reg[rs(instr)].x) + (CPU_reg[rs(instr)].y) * (double)(1 << 16);
	double vt = f16Unsign(CPU_reg[rt(instr)].x) + (CPU_reg[rt(instr)].y) * (double)(1 << 16);

	double lo = vs / vt;
	CPU_Lo.y = f16Sign(f16Overflow(lo));
	CPU_Lo.x = f16Sign(lo);

	double hi = fmod(vs, vt);
	CPU_Hi.y = f16Sign(f16Overflow(hi));
	CPU_Hi.x = f16Sign(hi);

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

void PGXP_CPU_DIVU(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Lo = Rs / Rt (unsigned)
	// Hi = Rs % Rt (unsigned)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	//// iCB: Only require one valid input
	if (((CPU_reg[rt(instr)].flags & VALID_01) != VALID_01) != ((CPU_reg[rs(instr)].flags & VALID_01) != VALID_01))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	CPU_Lo = CPU_Hi = CPU_reg[rs(instr)];

	CPU_Lo.halfFlags[0] = CPU_Hi.halfFlags[0] = (CPU_reg[rs(instr)].halfFlags[0] & CPU_reg[rt(instr)].halfFlags[0]);

	double vs = f16Unsign(CPU_reg[rs(instr)].x) + f16Unsign(CPU_reg[rs(instr)].y) * (double)(1 << 16);
	double vt = f16Unsign(CPU_reg[rt(instr)].x) + f16Unsign(CPU_reg[rt(instr)].y) * (double)(1 << 16);

	double lo = vs / vt;
	CPU_Lo.y = f16Sign(f16Overflow(lo));
	CPU_Lo.x = f16Sign(lo);

	double hi = fmod(vs, vt);
	CPU_Hi.y = f16Sign(f16Overflow(hi));
	CPU_Hi.x = f16Sign(hi);

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

////////////////////////////////////
// Shift operations (sa)
////////////////////////////////////
void PGXP_CPU_SLL(u32 instr, u32 rdVal, u32 rtVal)
{
	// Rd = Rt << Sa
	PGXP_value ret;
	u32 sh = sa(instr);
	Validate(&CPU_reg[rt(instr)], rtVal);
	
	ret = CPU_reg[rt(instr)];

	// TODO: Shift flags
#if 1 
	double x = f16Unsign(CPU_reg[rt(instr)].x);
	double y = f16Unsign(CPU_reg[rt(instr)].y);
	if (sh >= 32)
	{
		x = 0.f;
		y = 0.f;
	}
	else if (sh == 16)
	{
		y = f16Sign(x);
		x = 0.f;
	}
	else if (sh >= 16)
	{
		y = x * (1 << (sh - 16));
		y = f16Sign(y);
		x = 0.f;
	}
	else
	{
		x = x * (1 << sh);
		y = y * (1 << sh);
		y += f16Overflow(x);
		x = f16Sign(x);
		y = f16Sign(y);
	}
#else
	double x = CPU_reg[rt(instr)].x, y = f16Unsign(CPU_reg[rt(instr)].y);

	psx_value iX; iX.d = rtVal;
	psx_value iY; iY.d = rtVal;

	iX.w.h = 0;		// remove Y
	iY.w.l = 0;		// remove X

					// Shift test values
	psx_value dX;
	dX.d = iX.d << sh;
	psx_value dY;
	dY.d = iY.d << sh;


	if ((dY.sw.h == 0) || (dY.sw.h == -1))
		y = dY.sw.h;
	else
		y = y * (1 << sh);

	if (dX.sw.h != 0.f)
	{
		if (sh == 16)
		{
			y = x;
		}
		else if (sh < 16)
		{
			y += f16Unsign(x) / (1 << (16 - sh));
			//if (in.x < 0)
			//	y += 1 << (16 - sh);
		}
		else
		{
			y += x * (1 << (sh - 16));
		}
	}

	// if there's anything left of X write it in
	if (dX.w.l != 0.f)
		x = x * (1 << sh);
	else
		x = 0;

	x = f16Sign(x);
	y = f16Sign(y);

#endif

	ret.x = x;
	ret.y = y;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_SRL(u32 instr, u32 rdVal, u32 rtVal)
{
	// Rd = Rt >> Sa
	PGXP_value ret;
	u32 sh = sa(instr);
	Validate(&CPU_reg[rt(instr)], rtVal);

	ret = CPU_reg[rt(instr)];

#if 0
	double x = f16Unsign(CPU_reg[rt(instr)].x);
	double y = f16Unsign(CPU_reg[rt(instr)].y);
	if (sh >= 32)
	{
		x = y = 0.f;
	}
	else if (sh >= 16)
	{
		x = y / (1 << (sh - 16));
		x = f16Sign(x);
		y = (y < 0) ? -1.f : 0.f;	// sign extend
	}
	else
	{
		x = x / (1 << sh);

		// check for potential sign extension in overflow
		psx_value valt;
		valt.d = rtVal;
		u16 mask = 0xFFFF >> (16 - sh);
		if ((valt.w.h & mask) == mask)
			x += mask << (16 - sh);
		else if ((valt.w.h & mask) == 0)
			x = x;
		else
			x += y * (1 << (16 - sh));//f16Overflow(y);	

		y = y / (1 << sh);
		x = f16Sign(x);
		y = f16Sign(y);
	}
#else
	double x = CPU_reg[rt(instr)].x, y = f16Unsign(CPU_reg[rt(instr)].y);

	psx_value iX; iX.d = rtVal;
	psx_value iY; iY.d = rtVal;

	iX.sd = (iX.sd << 16) >> 16;	// remove Y
	iY.sw.l = iX.sw.h;				// overwrite x with sign(x)

									// Shift test values
	psx_value dX;
	dX.sd = iX.sd >> sh;
	psx_value dY;
	dY.d = iY.d >> sh;

	if (dX.sw.l != iX.sw.h)
		x = x / (1 << sh);
	else
		x = dX.sw.l;	// only sign bits left

	if (dY.sw.l != iX.sw.h)
	{
		if (sh == 16)
		{
			x = y;
		}
		else if (sh < 16)
		{
			x += y * (1 << (16 - sh));
			if (CPU_reg[rt(instr)].x < 0)
				x += 1 << (16 - sh);
		}
		else
		{
			x += y / (1 << (sh - 16));
		}
	}

	if ((dY.sw.h == 0) || (dY.sw.h == -1))
		y = dY.sw.h;
	else
		y = y / (1 << sh);

	x = f16Sign(x);
	y = f16Sign(y);

#endif
	ret.x = x;
	ret.y = y;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_SRA(u32 instr, u32 rdVal, u32 rtVal)
{
	// Rd = Rt >> Sa
	PGXP_value ret;
	u32 sh = sa(instr);
	Validate(&CPU_reg[rt(instr)], rtVal);
	ret = CPU_reg[rt(instr)];

#if 0
	double x = f16Unsign(CPU_reg[rt(instr)].x);
	double y = (CPU_reg[rt(instr)].y);
	if (sh >= 32)
	{
		// sign extend
		x = y = (y < 0) ? -1.f : 0.f;
	}
	else if (sh >= 16)
	{
		x = y / (1 << (sh - 16));
		x = f16Sign(x);
		y = (y < 0) ? -1.f : 0.f;	// sign extend
	}
	else
	{
		x = x / (1 << sh);
		
		// check for potential sign extension in overflow
		psx_value valt;
		valt.d = rtVal;
		u16 mask = 0xFFFF >> (16 - sh);
		if ((valt.w.h & mask) == mask)
			x += mask << (16 - sh);
		else if ((valt.w.h & mask) == 0)
			x = x;
		else
			x += y * (1 << (16 - sh));//f16Overflow(y);	

		y = y / (1 << sh);
		x = f16Sign(x);
		y = f16Sign(y);
	}

#else
	double x = CPU_reg[rt(instr)].x, y = CPU_reg[rt(instr)].y;

	psx_value iX; iX.d = rtVal;
	psx_value iY; iY.d = rtVal;

	iX.sd = (iX.sd << 16) >> 16;	// remove Y
	iY.sw.l = iX.sw.h;				// overwrite x with sign(x)

									// Shift test values
	psx_value dX;
	dX.sd = iX.sd >> sh;
	psx_value dY;
	dY.sd = iY.sd >> sh;

	if (dX.sw.l != iX.sw.h)
		x = x / (1 << sh);
	else
		x = dX.sw.l;	// only sign bits left

	if (dY.sw.l != iX.sw.h)
	{
		if (sh == 16)
		{
			x = y;
		}
		else if (sh < 16)
		{
			x += y * (1 << (16 - sh));
			if (CPU_reg[rt(instr)].x < 0)
				x += 1 << (16 - sh);
		}
		else
		{
			x += y / (1 << (sh - 16));
		}
	}

	if ((dY.sw.h == 0) || (dY.sw.h == -1))
		y = dY.sw.h;
	else
		y = y / (1 << sh);

	x = f16Sign(x);
	y = f16Sign(y);

#endif

	ret.x = x;
	ret.y = y;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

////////////////////////////////////
// Shift operations variable
////////////////////////////////////
void PGXP_CPU_SLLV(u32 instr, u32 rdVal, u32 rtVal, u32 rsVal)
{
	// Rd = Rt << Rs
	PGXP_value ret;
	u32 sh = rsVal & 0x1F;
	Validate(&CPU_reg[rt(instr)], rtVal);
	Validate(&CPU_reg[rs(instr)], rsVal);

	ret = CPU_reg[rt(instr)];

#if 1
	double x = f16Unsign(CPU_reg[rt(instr)].x);
	double y = f16Unsign(CPU_reg[rt(instr)].y);
	if (sh >= 32)
	{
		x = 0.f;
		y = 0.f;
	}
	else if (sh == 16)
	{
		y = f16Sign(x);
		x = 0.f;
	}
	else if (sh >= 16)
	{
		y = x * (1 << (sh - 16));
		y = f16Sign(y);
		x = 0.f;
	}
	else
	{
		x = x * (1 << sh);
		y = y * (1 << sh);
		y += f16Overflow(x);
		x = f16Sign(x);
		y = f16Sign(y);
	}
#else
	double x = CPU_reg[rt(instr)].x, y = f16Unsign(CPU_reg[rt(instr)].y);

	psx_value iX; iX.d = rtVal;
	psx_value iY; iY.d = rtVal;

	iX.w.h = 0;		// remove Y
	iY.w.l = 0;		// remove X

					// Shift test values
	psx_value dX;
	dX.d = iX.d << sh;
	psx_value dY;
	dY.d = iY.d << sh;


	if ((dY.sw.h == 0) || (dY.sw.h == -1))
		y = dY.sw.h;
	else
		y = y * (1 << sh);

	if (dX.sw.h != 0.f)
	{
		if (sh == 16)
		{
			y = x;
		}
		else if (sh < 16)
		{
			y += f16Unsign(x) / (1 << (16 - sh));
			//if (in.x < 0)
			//	y += 1 << (16 - sh);
		}
		else
		{
			y += x * (1 << (sh - 16));
		}
	}

	// if there's anything left of X write it in
	if (dX.w.l != 0.f)
		x = x * (1 << sh);
	else
		x = 0;

	x = f16Sign(x);
	y = f16Sign(y);

#endif
	ret.x = x;
	ret.y = y;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_SRLV(u32 instr, u32 rdVal, u32 rtVal, u32 rsVal)
{
	// Rd = Rt >> Sa
	PGXP_value ret;
	u32 sh = rsVal & 0x1F;
	Validate(&CPU_reg[rt(instr)], rtVal);
	Validate(&CPU_reg[rs(instr)], rsVal);

	ret = CPU_reg[rt(instr)];

#if 0
	double x = f16Unsign(CPU_reg[rt(instr)].x);
	double y = f16Unsign(CPU_reg[rt(instr)].y);
	if (sh >= 32)
	{
		x = y = 0.f;
	}
	else if (sh >= 16)
	{
		x = y / (1 << (sh - 16));
		x = f16Sign(x);
		y = (y < 0) ? -1.f : 0.f;	// sign extend
	}
	else
	{
		x = x / (1 << sh);
		
		// check for potential sign extension in overflow
		psx_value valt;
		valt.d = rtVal;
		u16 mask = 0xFFFF >> (16 - sh);
		if ((valt.w.h & mask) == mask)
			x += mask << (16 - sh);
		else if ((valt.w.h & mask) == 0)
			x = x;
		else
			x += y * (1 << (16 - sh));//f16Overflow(y);	

		y = y / (1 << sh);
		x = f16Sign(x);
		y = f16Sign(y);
	}

#else
	double x = CPU_reg[rt(instr)].x, y = f16Unsign(CPU_reg[rt(instr)].y);

	psx_value iX; iX.d = rtVal;
	psx_value iY; iY.d = rtVal;

	iX.sd = (iX.sd << 16) >> 16;	// remove Y
	iY.sw.l = iX.sw.h;				// overwrite x with sign(x)

									// Shift test values
	psx_value dX;
	dX.sd = iX.sd >> sh;
	psx_value dY;
	dY.d = iY.d >> sh;

	if (dX.sw.l != iX.sw.h)
		x = x / (1 << sh);
	else
		x = dX.sw.l;	// only sign bits left

	if (dY.sw.l != iX.sw.h)
	{
		if (sh == 16)
		{
			x = y;
		}
		else if (sh < 16)
		{
			x += y * (1 << (16 - sh));
			if (CPU_reg[rt(instr)].x < 0)
				x += 1 << (16 - sh);
		}
		else
		{
			x += y / (1 << (sh - 16));
		}
	}

	if ((dY.sw.h == 0) || (dY.sw.h == -1))
		y = dY.sw.h;
	else
		y = y / (1 << sh);

	x = f16Sign(x);
	y = f16Sign(y);

#endif

	ret.x = x;
	ret.y = y;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

void PGXP_CPU_SRAV(u32 instr, u32 rdVal, u32 rtVal, u32 rsVal)
{
	// Rd = Rt >> Sa
	PGXP_value ret;
	u32 sh = rsVal & 0x1F;
	Validate(&CPU_reg[rt(instr)], rtVal);
	Validate(&CPU_reg[rs(instr)], rsVal);

	ret = CPU_reg[rt(instr)];
#if 0
	double x = f16Unsign(CPU_reg[rt(instr)].x);
	double y = f16Unsign(CPU_reg[rt(instr)].y);
	if (sh >= 32)
	{
		x = y = 0.f;
	}
	else if (sh >= 16)
	{
		x = y / (1 << (sh - 16));
		x = f16Sign(x);
		y = (y < 0) ? -1.f : 0.f;	// sign extend
	}
	else
	{
		x = x / (1 << sh);
		
		// check for potential sign extension in overflow
		psx_value valt;
		valt.d = rtVal;
		u16 mask = 0xFFFF >> (16 - sh);
		if ((valt.w.h & mask) == mask)
			x += mask << (16 - sh);
		else if ((valt.w.h & mask) == 0)
			x = x;
		else
			x += y * (1 << (16 - sh));//f16Overflow(y);	

		y = y / (1 << sh);
		x = f16Sign(x);
		y = f16Sign(y);
	}

#else
	double x = CPU_reg[rt(instr)].x, y = CPU_reg[rt(instr)].y;

	psx_value iX; iX.d = rtVal;
	psx_value iY; iY.d = rtVal;

	iX.sd = (iX.sd << 16) >> 16;	// remove Y
	iY.sw.l = iX.sw.h;				// overwrite x with sign(x)

									// Shift test values
	psx_value dX;
	dX.sd = iX.sd >> sh;
	psx_value dY;
	dY.sd = iY.sd >> sh;

	if (dX.sw.l != iX.sw.h)
		x = x / (1 << sh);
	else
		x = dX.sw.l;	// only sign bits left

	if (dY.sw.l != iX.sw.h)
	{
		if (sh == 16)
		{
			x = y;
		}
		else if (sh < 16)
		{
			x += y * (1 << (16 - sh));
			if (CPU_reg[rt(instr)].x < 0)
				x += 1 << (16 - sh);
		}
		else
		{
			x += y / (1 << (sh - 16));
		}
	}

	if ((dY.sw.h == 0) || (dY.sw.h == -1))
		y = dY.sw.h;
	else
		y = y / (1 << sh);

	x = f16Sign(x);
	y = f16Sign(y);

#endif

	ret.x = x;
	ret.y = y;

	ret.value = rdVal;
	CPU_reg[rd(instr)] = ret;
}

////////////////////////////////////
// Move registers
////////////////////////////////////
void PGXP_CPU_MFHI(u32 instr, u32 rdVal, u32 hiVal)
{
	// Rd = Hi
	Validate(&CPU_Hi, hiVal);

	CPU_reg[rd(instr)] = CPU_Hi;
}

void PGXP_CPU_MTHI(u32 instr, u32 hiVal, u32 rdVal)
{
	// Hi = Rd
	Validate(&CPU_reg[rd(instr)], rdVal);

	CPU_Hi = CPU_reg[rd(instr)];
}

void PGXP_CPU_MFLO(u32 instr, u32 rdVal, u32 loVal)
{
	// Rd = Lo
	Validate(&CPU_Lo, loVal);

	CPU_reg[rd(instr)] = CPU_Lo;
}

void PGXP_CPU_MTLO(u32 instr, u32 loVal, u32 rdVal)
{
	// Lo = Rd
	Validate(&CPU_reg[rd(instr)], rdVal);

	CPU_Lo = CPU_reg[rd(instr)];
}

////////////////////////////////////
// Memory Access
////////////////////////////////////

// Load 32-bit word
void PGXP_CPU_LWL(u32 instr, u32 rtVal, u32 addr)
{
	// Rt = Mem[Rs + Im]
	PGXP_CPU_LW(instr, rtVal, addr);
}

void PGXP_CPU_LW(u32 instr, u32 rtVal, u32 addr)
{
	// Rt = Mem[Rs + Im]
	ValidateAndCopyMem(&CPU_reg[rt(instr)], addr, rtVal);
}

void PGXP_CPU_LWR(u32 instr, u32 rtVal, u32 addr)
{
	// Rt = Mem[Rs + Im]
	PGXP_CPU_LW(instr, rtVal, addr);
}

// Load 16-bit
void PGXP_CPU_LH(u32 instr, u16 rtVal, u32 addr)
{
	// Rt = Mem[Rs + Im] (sign extended)
	psx_value val;
	val.sd = (s32)(s16)rtVal;
	ValidateAndCopyMem16(&CPU_reg[rt(instr)], addr, val.d, 1);
}

void PGXP_CPU_LHU(u32 instr, u16 rtVal, u32 addr)
{
	// Rt = Mem[Rs + Im] (zero extended)
	psx_value val;
	val.d = rtVal;
	val.w.h = 0;
	ValidateAndCopyMem16(&CPU_reg[rt(instr)], addr, val.d, 0);
}

// Load 8-bit
void PGXP_CPU_LB(u32 instr, u8 rtVal, u32 addr)
{
	InvalidLoad(addr, instr, 116);
}

void PGXP_CPU_LBU(u32 instr, u8 rtVal, u32 addr)
{
	InvalidLoad(addr, instr, 116);
}

// Store 32-bit word
void PGXP_CPU_SWL(u32 instr, u32 rtVal, u32 addr)
{
	// Mem[Rs + Im] = Rt
	PGXP_CPU_SW(instr, rtVal, addr);
}

void PGXP_CPU_SW(u32 instr, u32 rtVal, u32 addr)
{
	// Mem[Rs + Im] = Rt
	Validate(&CPU_reg[rt(instr)], rtVal);
	WriteMem(&CPU_reg[rt(instr)], addr);
}

void PGXP_CPU_SWR(u32 instr, u32 rtVal, u32 addr)
{
	// Mem[Rs + Im] = Rt
	PGXP_CPU_SW(instr, rtVal, addr);
}

// Store 16-bit
void PGXP_CPU_SH(u32 instr, u16 rtVal, u32 addr)
{
	// validate and copy half value
	MaskValidate(&CPU_reg[rt(instr)], rtVal, 0xFFFF, VALID_0);
	WriteMem16(&CPU_reg[rt(instr)], addr);
}

// Store 8-bit
void PGXP_CPU_SB(u32 instr, u8 rtVal, u32 addr)
{
	InvalidStore(addr, instr, 208);
}

////////////////////////////////////
// Data transfer tracking
////////////////////////////////////
void PGXP_CP0_MFC0(u32 instr, u32 rtVal, u32 rdVal)
{
	// CPU[Rt] = CP0[Rd]
	Validate(&CP0_reg[rd(instr)], rdVal);
	CPU_reg[rt(instr)] = CP0_reg[rd(instr)];
	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CP0_MTC0(u32 instr, u32 rdVal, u32 rtVal)
{
	// CP0[Rd] = CPU[Rt]
	Validate(&CPU_reg[rt(instr)], rtVal);
	CP0_reg[rd(instr)] = CPU_reg[rt(instr)];
	CP0_reg[rd(instr)].value = rdVal;
}

void PGXP_CP0_CFC0(u32 instr, u32 rtVal, u32 rdVal)
{
	// CPU[Rt] = CP0[Rd]
	Validate(&CP0_reg[rd(instr)], rdVal);
	CPU_reg[rt(instr)] = CP0_reg[rd(instr)];
	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CP0_CTC0(u32 instr, u32 rdVal, u32 rtVal)
{
	// CP0[Rd] = CPU[Rt]
	Validate(&CPU_reg[rt(instr)], rtVal);
	CP0_reg[rd(instr)] = CPU_reg[rt(instr)];
	CP0_reg[rd(instr)].value = rdVal;
}

void PGXP_CP0_RFE(u32 instr)
{}