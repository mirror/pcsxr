
#include "pgxp_cpu.h"
#include "pgxp_value.h"
#include "pgxp_mem.h"

// CPU registers
PGXP_value CPU_reg_mem[34];
//PGXP_value CPU_Hi, CPU_Lo;

PGXP_value* CPU_reg = CPU_reg_mem;

// Instruction register decoding
#define op(_instr)		(_instr >> 26)			// The op part of the instruction register 
#define func(_instr)	((_instr) & 0x3F)		// The funct part of the instruction register 
#define sa(_instr)		((_instr >>  6) & 0x1F) // The sa part of the instruction register
#define rd(_instr)		((_instr >> 11) & 0x1F)	// The rd part of the instruction register 
#define rt(_instr)		((_instr >> 16) & 0x1F)	// The rt part of the instruction register 
#define rs(_instr)		((_instr >> 21) & 0x1F)	// The rs part of the instruction register 
#define imm(_instr)		(_instr & 0xFFFF)		// The immediate part of the instruction register

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

	p.valid = 0;

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

	p.valid = 0;
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

	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	tempImm.w.h = imm(instr);
	CPU_reg[rt(instr)].x += tempImm.sw.h;
	// handle x overflow in to y?

	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_ADDIU(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs + Imm (signed) (unsafe?)
	psx_value tempImm;

	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	tempImm.w.h = imm(instr);
	CPU_reg[rt(instr)].x += tempImm.sw.h;
	// handle x overflow in to y?

	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_ANDI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs & Imm
	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	CPU_reg[rt(instr)].y = 0.f;	// remove upper 16-bits

	switch (imm(instr))
	{
	case 0:
		// if 0 then x == 0
		CPU_reg[rt(instr)].x = 0.f;
		break;
	case 0xFFFF:
		// if saturated then x = x
		break;
	default:
		// x is undefined, invalidate value
		CPU_reg[rt(instr)].valid = 0;
	}

	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_ORI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs | Imm
	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	// Invalidate on non-zero values for now
	if (imm(instr) != 0)
		CPU_reg[rt(instr)].valid = 0;

	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_XORI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs ^ Imm
	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	// Invalidate on non-zero values for now
	if (imm(instr) != 0)
		CPU_reg[rt(instr)].valid = 0;

	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_SLTI(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs < Imm (signed)
	psx_value tempImm;

	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	tempImm.w.h = imm(instr);
	CPU_reg[rt(instr)].y = 0.f;
	CPU_reg[rt(instr)].x = (CPU_reg[rs(instr)].x < tempImm.sw.h) ? 1.f : 0.f;

	CPU_reg[rt(instr)].value = rtVal;
}

void PGXP_CPU_SLTIU(u32 instr, u32 rtVal, u32 rsVal)
{
	// Rt = Rs < Imm (signed)
	Validate(&CPU_reg[rs(instr)], rsVal);
	CPU_reg[rt(instr)] = CPU_reg[rs(instr)];

	CPU_reg[rt(instr)].y = 0.f;
	CPU_reg[rt(instr)].x = (fabs(CPU_reg[rs(instr)].x) < (u32)imm(instr)) ? 1.f : 0.f;

	CPU_reg[rt(instr)].value = rtVal;
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
}

////////////////////////////////////
// Register Arithmetic
////////////////////////////////////
void PGXP_CPU_ADD(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs + Rt (signed)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	CPU_reg[rd(instr)] = CPU_reg[rs(instr)];

	CPU_reg[rd(instr)].x += CPU_reg[rt(instr)].x;
	CPU_reg[rd(instr)].y += CPU_reg[rt(instr)].y;

	CPU_reg[rd(instr)].valid &= CPU_reg[rt(instr)].valid;
	CPU_reg[rd(instr)].gFlags |= CPU_reg[rt(instr)].gFlags;
	CPU_reg[rd(instr)].lFlags |= CPU_reg[rt(instr)].lFlags;
	CPU_reg[rd(instr)].hFlags |= CPU_reg[rt(instr)].hFlags;

	CPU_reg[rd(instr)].value = rdVal;
}

void PGXP_CPU_ADDU(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs + Rt (signed) (unsafe?)
	PGXP_CPU_ADD(instr, rdVal, rsVal, rtVal);
}

void PGXP_CPU_SUB(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs - Rt (signed)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	//if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	//{
	//	MakeValid(&CPU_reg[rs(instr)], rsVal);
	//	MakeValid(&CPU_reg[rt(instr)], rtVal);
	//}

	CPU_reg[rd(instr)] = CPU_reg[rs(instr)];

	CPU_reg[rd(instr)].x -= CPU_reg[rt(instr)].x;
	CPU_reg[rd(instr)].y -= CPU_reg[rt(instr)].y;

	CPU_reg[rd(instr)].valid &= CPU_reg[rt(instr)].valid;
	CPU_reg[rd(instr)].gFlags |= CPU_reg[rt(instr)].gFlags;
	CPU_reg[rd(instr)].lFlags |= CPU_reg[rt(instr)].lFlags;
	CPU_reg[rd(instr)].hFlags |= CPU_reg[rt(instr)].hFlags;

	CPU_reg[rd(instr)].value = rdVal;
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

	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	//if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	//{
	//	MakeValid(&CPU_reg[rs(instr)], rsVal);
	//	MakeValid(&CPU_reg[rt(instr)], rtVal);
	//}

	vald.d = rdVal;
	vals.d = rsVal;
	valt.d = rtVal;

	//	CPU_reg[rd(instr)].valid = CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid;
	CPU_reg[rd(instr)].valid = 1;

	if (vald.w.l == 0)
	{
		CPU_reg[rd(instr)].x = 0.f;
		CPU_reg[rd(instr)].lFlags = VALID_HALF;
	}
	else if (vald.w.l == vals.w.l)
	{
		CPU_reg[rd(instr)].x = CPU_reg[rs(instr)].x;
		CPU_reg[rd(instr)].lFlags = CPU_reg[rs(instr)].lFlags;
		CPU_reg[rd(instr)].valid &= CPU_reg[rs(instr)].valid;
	}
	else if (vald.w.l == valt.w.l)
	{
		CPU_reg[rd(instr)].x = CPU_reg[rt(instr)].x;
		CPU_reg[rd(instr)].lFlags = CPU_reg[rt(instr)].lFlags;
		CPU_reg[rd(instr)].valid &= CPU_reg[rt(instr)].valid;
	}
	else
	{
		CPU_reg[rd(instr)].valid = 0;
		CPU_reg[rd(instr)].lFlags = 0;
	}

	if (vald.w.h == 0)
	{
		CPU_reg[rd(instr)].y = 0.f;
		CPU_reg[rd(instr)].hFlags = VALID_HALF;
	}
	else if (vald.w.h == vals.w.h)
	{
		CPU_reg[rd(instr)].y = CPU_reg[rs(instr)].y;
		CPU_reg[rd(instr)].hFlags = CPU_reg[rs(instr)].hFlags;
		CPU_reg[rd(instr)].valid &= CPU_reg[rs(instr)].valid;
	}
	else if (vald.w.h == valt.w.h)
	{
		CPU_reg[rd(instr)].y = CPU_reg[rt(instr)].y;
		CPU_reg[rd(instr)].hFlags = CPU_reg[rt(instr)].hFlags;
		CPU_reg[rd(instr)].valid &= CPU_reg[rt(instr)].valid;
	}
	else
	{
		CPU_reg[rd(instr)].valid = 0;
		CPU_reg[rd(instr)].hFlags = 0;
	}

	// iCB Hack: Force validity if even one half is valid
	if ((CPU_reg[rd(instr)].hFlags & VALID_HALF) || (CPU_reg[rd(instr)].lFlags & VALID_HALF))
		CPU_reg[rd(instr)].valid = 1;
	// /iCB Hack

	CPU_reg[rd(instr)].value = rdVal;
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
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	//if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	//{
	//	MakeValid(&CPU_reg[rs(instr)], rsVal);
	//	MakeValid(&CPU_reg[rt(instr)], rtVal);
	//}

	CPU_reg[rd(instr)] = CPU_reg[rs(instr)];

	// TODO: fix for single or double values?
	CPU_reg[rd(instr)].y = 0.f;
	CPU_reg[rd(instr)].x = (CPU_reg[rs(instr)].x < CPU_reg[rt(instr)].x) ? 1.f : 0.f;

	CPU_reg[rd(instr)].value = rdVal;
}

void PGXP_CPU_SLTU(u32 instr, u32 rdVal, u32 rsVal, u32 rtVal)
{
	// Rd = Rs < Rt (unsigned)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	//if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	//{
	//	MakeValid(&CPU_reg[rs(instr)], rsVal);
	//	MakeValid(&CPU_reg[rt(instr)], rtVal);
	//}

	CPU_reg[rd(instr)] = CPU_reg[rs(instr)];

	CPU_reg[rd(instr)].y = 0.f;
	CPU_reg[rd(instr)].x = (fabs(CPU_reg[rs(instr)].x) < fabs(CPU_reg[rt(instr)].x)) ? 1.f : 0.f;

	CPU_reg[rd(instr)].value = rdVal;
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
	if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	float vs = CPU_reg[rs(instr)].y + (CPU_reg[rs(instr)].x / (float)(1 << 16));
	float vt = CPU_reg[rt(instr)].y + (CPU_reg[rt(instr)].x / (float)(1 << 16));

	CPU_Lo.x = 0;// CPU_reg[rs(instr)].x * CPU_reg[rt(instr)].x;
	CPU_Hi.x = vs * vt;// CPU_reg[rs(instr)].y * CPU_reg[rt(instr)].y;

	CPU_Lo.valid = CPU_Hi.valid = CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid;

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

void PGXP_CPU_MULTU(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Hi/Lo = Rs * Rt (unsigned)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	// iCB: Only require one valid input
	if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	{
		MakeValid(&CPU_reg[rs(instr)], rsVal);
		MakeValid(&CPU_reg[rt(instr)], rtVal);
	}

	float vs = fabs(CPU_reg[rs(instr)].y) + (fabs(CPU_reg[rs(instr)].x) / (float)(1 << 16));
	float vt = fabs(CPU_reg[rt(instr)].y) + (fabs(CPU_reg[rt(instr)].x) / (float)(1 << 16));

	CPU_Lo.x = 0;// fabs(CPU_reg[rs(instr)].x) * fabs(CPU_reg[rt(instr)].x);
	CPU_Hi.x = vs * vt;// fabs(CPU_reg[rs(instr)].y) * fabs(CPU_reg[rt(instr)].y);

	CPU_Lo.valid = CPU_Hi.valid = CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid;

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

void PGXP_CPU_DIV(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Hi = Rs / Rt (signed)
	// Lo = Rs % Rt (signed)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	//// iCB: Only require one valid input
	//if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	//{
	//	MakeValid(&CPU_reg[rs(instr)], rsVal);
	//	MakeValid(&CPU_reg[rt(instr)], rtVal);
	//}

	float vs = CPU_reg[rs(instr)].y + (CPU_reg[rs(instr)].x / (float)(1 << 16));
	float vt = CPU_reg[rt(instr)].y + (CPU_reg[rt(instr)].x / (float)(1 << 16));

	CPU_Lo.x = vs / vt;
	CPU_Hi.x = fmod(vs, vt);
	CPU_Lo.x -= CPU_Hi.x;

	CPU_Lo.valid = CPU_Hi.valid = CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid;

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

void PGXP_CPU_DIVU(u32 instr, u32 hiVal, u32 loVal, u32 rsVal, u32 rtVal)
{
	// Hi = Rs / Rt (unsigned)
	// Lo = Rs % Rt (unsigned)
	Validate(&CPU_reg[rs(instr)], rsVal);
	Validate(&CPU_reg[rt(instr)], rtVal);

	//// iCB: Only require one valid input
	//if (!(CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid) && (CPU_reg[rs(instr)].valid || CPU_reg[rt(instr)].valid))
	//{
	//	MakeValid(&CPU_reg[rs(instr)], rsVal);
	//	MakeValid(&CPU_reg[rt(instr)], rtVal);
	//}

	float vs = CPU_reg[rs(instr)].y + (CPU_reg[rs(instr)].x / (float)(1 << 16));
	float vt = CPU_reg[rt(instr)].y + (CPU_reg[rt(instr)].x / (float)(1 << 16));

	CPU_Lo.x = fabs(vs) / fabs(vt);
	CPU_Hi.x = fmod(fabs(vs), fabs(vt));
	CPU_Lo.x -= CPU_Hi.x;

	CPU_Lo.valid = CPU_Hi.valid = CPU_reg[rs(instr)].valid && CPU_reg[rt(instr)].valid;

	CPU_Lo.value = loVal;
	CPU_Hi.value = hiVal;
}

////////////////////////////////////
// Shift operations (sa)
////////////////////////////////////
void PGXP_CPU_SLL(u32 instr, u32 rdVal, u32 rtVal)
{
	// Rd = Rt << Sa
	u32 sh = sa(instr);
	Validate(&CPU_reg[rt(instr)], rtVal);
	CPU_reg[rd(instr)] = CPU_reg[rt(instr)];

	// Shift y into x?
	if (sh >= 16)
	{
		CPU_reg[rd(instr)].y = CPU_reg[rd(instr)].x;
		CPU_reg[rd(instr)].x = 0;
		CPU_reg[rd(instr)].hFlags = CPU_reg[rd(instr)].lFlags;
		CPU_reg[rd(instr)].lFlags = 0;
		sh -= 16;
	}
	
	// assume multiply with no overflow
	CPU_reg[rd(instr)].x *= (float)(1 << sh);
	CPU_reg[rd(instr)].y *= (float)(1 << sh);

	CPU_reg[rd(instr)].value = rdVal;
}

void PGXP_CPU_SRL(u32 instr, u32 rdVal, u32 rtVal)
{
	// Rd = Rt >> Sa
	u32 sh = sa(instr);
	Validate(&CPU_reg[rt(instr)], rtVal);
	CPU_reg[rd(instr)] = CPU_reg[rt(instr)];

	// Shift x into y?
	if (sh >= 16)
	{
		CPU_reg[rd(instr)].x = CPU_reg[rd(instr)].y;
		CPU_reg[rd(instr)].y = 0;
		CPU_reg[rd(instr)].lFlags = CPU_reg[rd(instr)].hFlags;
		CPU_reg[rd(instr)].hFlags = 0;
		sh -= 16;
	}
	
	// assume divide with no overflow
	CPU_reg[rd(instr)].x /= (float)(1 << sh);
	CPU_reg[rd(instr)].y /= (float)(1 << sh);

	CPU_reg[rd(instr)].value = rdVal;
}

void PGXP_CPU_SRA(u32 instr, u32 rdVal, u32 rtVal)
{
	// Rd = Rt >> Sa
	PGXP_CPU_SRL(instr, rdVal, rtVal);
}

////////////////////////////////////
// Shift operations variable
////////////////////////////////////
void PGXP_CPU_SLLV(u32 instr, u32 rdVal, u32 rtVal, u32 rsVal)
{
	// Rd = Rt << Rs
	u32 sh = rsVal & 0x1F;
	Validate(&CPU_reg[rt(instr)], rtVal);
	Validate(&CPU_reg[rs(instr)], rsVal);

	CPU_reg[rd(instr)] = CPU_reg[rt(instr)];

	// Shift y into x?
	if (sh >= 16)
	{
		CPU_reg[rd(instr)].y = CPU_reg[rd(instr)].x;
		CPU_reg[rd(instr)].x = 0;
		CPU_reg[rd(instr)].hFlags = CPU_reg[rd(instr)].lFlags;
		CPU_reg[rd(instr)].lFlags = 0;
		sh -= 16;
	}

	// assume multiply with no overflow
	CPU_reg[rd(instr)].x *= (float)(1 << sh);
	CPU_reg[rd(instr)].y *= (float)(1 << sh);

	CPU_reg[rd(instr)].value = rdVal;
}

void PGXP_CPU_SRLV(u32 instr, u32 rdVal, u32 rtVal, u32 rsVal)
{
	// Rd = Rt >> Sa
	u32 sh = rsVal & 0x1F;
	Validate(&CPU_reg[rt(instr)], rtVal);
	Validate(&CPU_reg[rs(instr)], rsVal);

	CPU_reg[rd(instr)] = CPU_reg[rt(instr)];

	// Shift x into y?
	if (sh >= 16)
	{
		CPU_reg[rd(instr)].x = CPU_reg[rd(instr)].y;
		CPU_reg[rd(instr)].y = 0;
		CPU_reg[rd(instr)].lFlags = CPU_reg[rd(instr)].hFlags;
		CPU_reg[rd(instr)].hFlags = 0;
		sh -= 16;
	}

	// assume divide with no overflow
	CPU_reg[rd(instr)].x /= (float)(1 << sh);
	CPU_reg[rd(instr)].y /= (float)(1 << sh);

	CPU_reg[rd(instr)].value = rdVal;
}

void PGXP_CPU_SRAV(u32 instr, u32 rdVal, u32 rtVal, u32 rsVal)
{
	PGXP_CPU_SRLV(instr, rdVal, rtVal, rsVal);
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
	//CPU_reg[rt(instr)] = PGXP_validateXY(ReadMem(addr), rtVal);
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
	ValidateAndCopyMem16(&CPU_reg[rt(instr)], addr, val.d);
}

void PGXP_CPU_LHU(u32 instr, u16 rtVal, u32 addr)
{
	// Rt = Mem[Rs + Im] (zero extended)
	psx_value val;
	val.d = rtVal;
	val.w.h = 0;
	ValidateAndCopyMem16(&CPU_reg[rt(instr)], addr, val.d);
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
	//WriteMemOld(PGXP_validateXY(&CPU_reg[rt(instr)], rtVal), addr);
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
	MaskValidate(&CPU_reg[rt(instr)], rtVal, 0xFFFF);
	WriteMem16(&CPU_reg[rt(instr)], addr);
}

// Store 8-bit
void PGXP_CPU_SB(u32 instr, u8 rtVal, u32 addr)
{
	InvalidStore(addr, instr, 208);
}

