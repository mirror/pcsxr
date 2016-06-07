#include "pgxp_debug.h"
#include "pgxp_cpu.h"
#include "pgxp_value.h"

unsigned int pgxp_debug = 0;

// Instruction register decoding
#define op(_instr)		(_instr >> 26)			// The op part of the instruction register 
#define func(_instr)	((_instr) & 0x3F)		// The funct part of the instruction register 
#define sa(_instr)		((_instr >>  6) & 0x1F) // The sa part of the instruction register
#define rd(_instr)		((_instr >> 11) & 0x1F)	// The rd part of the instruction register 
#define rt(_instr)		((_instr >> 16) & 0x1F)	// The rt part of the instruction register 
#define rs(_instr)		((_instr >> 21) & 0x1F)	// The rs part of the instruction register 
#define imm(_instr)		(_instr & 0xFFFF)		// The immediate part of the instruction register

// Operand ID flags
typedef enum
{
	fOp_Hi = 1 << 0,
	fOp_Lo = 1 << 1,
	fOp_Rd = 1 << 2,
	fOp_Rs = 1 << 3,
	fOp_Rt = 1 << 4,
	fOp_Sa = 1 << 5,
	fOp_Im = 1 << 6,
	fOp_Ad = 1 << 7
} PGXP_CPU_OperandIDs;

typedef struct
{
	unsigned char OutputFlags;
	unsigned char InputFlags;
	unsigned char numRegisters;
	unsigned char numArgs;
	const char* szOpString;
	const char* szOpName;
	void(*funcPtr)();
} PGXP_CPU_OpData;

#define PGXP_Data_NULL		{ 0, 0, 0, 0, NULL }
#define PGXP_Data_SPECIAL	{ 0, 0, 0, 0, NULL }
#define PGXP_Data_COP0		{ 0, 0, 0, 0, NULL }
#define PGXP_Data_COP2		{ 0, 0, 0, 0, NULL }
#define PGXP_Data_LWC2		{ 0, 0, 0, 0, NULL }
#define PGXP_Data_SWC2		{ 0, 0, 0, 0, NULL }
#define PGXP_Data_HLE		{ 0, 0, 0, 0, NULL }

// Arithmetic with immediate value
#define PGXP_Data_ADDI	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "+", "ADDI",	(void(*)())PGXP_CPU_ADDI }
#define PGXP_Data_ADDIU	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "+", "ADDIU",	(void(*)())PGXP_CPU_ADDIU }
#define PGXP_Data_ANDI	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "&", "ANDI",	(void(*)())PGXP_CPU_ANDI }
#define PGXP_Data_ORI	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "|", "ORI",	(void(*)())PGXP_CPU_ORI }
#define PGXP_Data_XORI	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "^", "XORI",	(void(*)())PGXP_CPU_XORI }
#define PGXP_Data_SLTI	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "<", "SLTI",	(void(*)())PGXP_CPU_SLTI }
#define PGXP_Data_SLTIU	{ fOp_Rt, fOp_Rs | fOp_Im, 2, 2, "<", "SLTIU",	(void(*)())PGXP_CPU_SLTIU }
// Load Upper
#define PGXP_Data_LUI	{ fOp_Rt, fOp_Im, 1, 1, "<<", "LUI", (void(*)())PGXP_CPU_LUI }

// Load/Store
#define PGXP_Data_LWL	{ fOp_Rt, fOp_Ad, 1, 2, "", "LWL",	(void(*)())PGXP_CPU_LWL }	// 32-bit Loads
#define PGXP_Data_LW	{ fOp_Rt, fOp_Ad, 1, 2, "", "LW",	(void(*)())PGXP_CPU_LW	}
#define PGXP_Data_LWR	{ fOp_Rt, fOp_Ad, 1, 2, "", "LWR",	(void(*)())PGXP_CPU_LWR }
#define PGXP_Data_LH	{ fOp_Rt, fOp_Ad, 1, 2, "", "LH",	(void(*)())PGXP_CPU_LH }	// 16-bit Loads
#define PGXP_Data_LHU	{ fOp_Rt, fOp_Ad, 1, 2, "", "LHU",	(void(*)())PGXP_CPU_LHU }
#define PGXP_Data_LB	{ fOp_Rt, fOp_Ad, 1, 2, "", "LB",	(void(*)())PGXP_CPU_LB }	// 8-bit Loads
#define PGXP_Data_LBU	{ fOp_Rt, fOp_Ad, 1, 2, "", "LBU",	(void(*)())PGXP_CPU_LBU }
#define PGXP_Data_SWL	{ fOp_Ad, fOp_Rt, 1, 2, "", "SWL",	(void(*)())PGXP_CPU_SWL }	// 32-bit Store
#define PGXP_Data_SW	{ fOp_Ad, fOp_Rt, 1, 2, "", "SW",	(void(*)())PGXP_CPU_SW }
#define PGXP_Data_SWR	{ fOp_Ad, fOp_Rt, 1, 2, "", "SWR",	(void(*)())PGXP_CPU_SWR }
#define PGXP_Data_SH	{ fOp_Ad, fOp_Rt, 1, 2, "", "SH",	(void(*)())PGXP_CPU_SH }	// 16-bit Store
#define PGXP_Data_SB	{ fOp_Ad, fOp_Rt, 1, 2, "", "SU",	(void(*)())PGXP_CPU_SB }	// 8-bit Store

static PGXP_CPU_OpData PGXP_BSC_LUT[64] = {
	PGXP_Data_SPECIAL, PGXP_Data_NULL  , PGXP_Data_NULL, PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_ADDI   , PGXP_Data_ADDIU , PGXP_Data_SLTI, PGXP_Data_SLTIU, PGXP_Data_ANDI, PGXP_Data_ORI , PGXP_Data_XORI, PGXP_Data_LUI ,
	PGXP_Data_COP0   , PGXP_Data_NULL  , PGXP_Data_COP2, PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL   , PGXP_Data_NULL  , PGXP_Data_NULL, PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_LB     , PGXP_Data_LH    , PGXP_Data_LWL , PGXP_Data_LW   , PGXP_Data_LBU , PGXP_Data_LHU , PGXP_Data_LWR , PGXP_Data_NULL,
	PGXP_Data_SB     , PGXP_Data_SH    , PGXP_Data_SWL , PGXP_Data_SW   , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_SWR , PGXP_Data_NULL,
	PGXP_Data_NULL   , PGXP_Data_NULL  , PGXP_Data_LWC2, PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL   , PGXP_Data_NULL  , PGXP_Data_SWC2, PGXP_Data_HLE  , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL
};

// Register Arithmetic
#define PGXP_Data_ADD	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "+", "ADD",	(void(*)())PGXP_CPU_ADD }
#define PGXP_Data_ADDU	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "+", "ADDU",	(void(*)())PGXP_CPU_ADDU }
#define PGXP_Data_SUB	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "+", "SUB",	(void(*)())PGXP_CPU_SUB }
#define PGXP_Data_SUBU	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "+", "SUBU",	(void(*)())PGXP_CPU_SUBU }
#define PGXP_Data_AND	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "&", "AND",	(void(*)())PGXP_CPU_AND }
#define PGXP_Data_OR	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "|", "OR",		(void(*)())PGXP_CPU_OR }
#define PGXP_Data_XOR	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "^", "XOR",	(void(*)())PGXP_CPU_XOR }
#define PGXP_Data_NOR	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "^", "NOR",	(void(*)())PGXP_CPU_NOR }
#define PGXP_Data_SLT	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "<", "SLT",	(void(*)())PGXP_CPU_SLT }
#define PGXP_Data_SLTU	{ fOp_Rd, fOp_Rs | fOp_Rt, 3, 3, "<", "SLTU",	(void(*)())PGXP_CPU_SLTU }

// Register mult/div
#define PGXP_Data_MULT	{ fOp_Hi | fOp_Lo, fOp_Rs | fOp_Rt, 4, 4, "*", "MULT",	(void(*)())PGXP_CPU_MULT }
#define PGXP_Data_MULTU	{ fOp_Hi | fOp_Lo, fOp_Rs | fOp_Rt, 4, 4, "*", "MULTU",	(void(*)())PGXP_CPU_MULTU }
#define PGXP_Data_DIV	{ fOp_Hi | fOp_Lo, fOp_Rs | fOp_Rt, 4, 4, "/", "DIV",	(void(*)())PGXP_CPU_DIV }
#define PGXP_Data_DIVU	{ fOp_Hi | fOp_Lo, fOp_Rs | fOp_Rt, 4, 4, "/", "DIVU",	(void(*)())PGXP_CPU_DIVU }

// Shift operations (sa)
#define PGXP_Data_SLL	{ fOp_Rd, fOp_Rt | fOp_Sa, 2, 2, ">>", "SLL",	(void(*)())PGXP_CPU_SLL }
#define PGXP_Data_SRL	{ fOp_Rd, fOp_Rt | fOp_Sa, 2, 2, "<<", "SRL",	(void(*)())PGXP_CPU_SRL }
#define PGXP_Data_SRA	{ fOp_Rd, fOp_Rt | fOp_Sa, 2, 2, "<<", "SRA",	(void(*)())PGXP_CPU_SRA }

// Shift operations variable
#define PGXP_Data_SLLV	{ fOp_Rd, fOp_Rt | fOp_Rs, 3, 3, ">>", "SLLV",	(void(*)())PGXP_CPU_SLLV }
#define PGXP_Data_SRLV	{ fOp_Rd, fOp_Rt | fOp_Rs, 3, 3, "<<", "SRLV",	(void(*)())PGXP_CPU_SRLV }
#define PGXP_Data_SRAV	{ fOp_Rd, fOp_Rt | fOp_Rs, 3, 3, "<<", "SRAV",	(void(*)())PGXP_CPU_SRAV }

// Move registers
#define PGXP_Data_MFHI	{ fOp_Rd, fOp_Hi, 2, 2, "<-", "MFHI",	(void(*)())PGXP_CPU_MFHI }
#define PGXP_Data_MTHI	{ fOp_Hi, fOp_Rd, 2, 2, "<-", "MTHI",	(void(*)())PGXP_CPU_MTHI }
#define PGXP_Data_MFLO	{ fOp_Rd, fOp_Lo, 2, 2, "<-", "MFLO",	(void(*)())PGXP_CPU_MFLO }
#define PGXP_Data_MTLO	{ fOp_Lo, fOp_Rd, 2, 2, "<-", "MFHI",	(void(*)())PGXP_CPU_MTLO }

static PGXP_CPU_OpData PGXP_SPC_LUT[64] = {
	PGXP_Data_SLL , PGXP_Data_NULL, PGXP_Data_SRL , PGXP_Data_SRA , PGXP_Data_SLLV   , PGXP_Data_NULL , PGXP_Data_SRLV, PGXP_Data_SRAV,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL	 , PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_MFHI, PGXP_Data_MTHI, PGXP_Data_MFLO, PGXP_Data_MTLO, PGXP_Data_NULL   , PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_MULT, PGXP_Data_MULTU, PGXP_Data_DIV, PGXP_Data_DIVU, PGXP_Data_NULL   , PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_ADD , PGXP_Data_ADDU, PGXP_Data_SUB , PGXP_Data_SUBU, PGXP_Data_AND    , PGXP_Data_OR   , PGXP_Data_XOR , PGXP_Data_NOR ,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_SLT , PGXP_Data_SLTU, PGXP_Data_NULL   , PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL   , PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL   , PGXP_Data_NULL , PGXP_Data_NULL, PGXP_Data_NULL
};

PGXP_CPU_OpData GetOpData(u32 instr)
{
	PGXP_CPU_OpData pOpData = PGXP_Data_NULL;
	switch (op(instr))
	{
	case 0:
		pOpData = PGXP_SPC_LUT[func(instr)];
		break;
	case 1:
		//pOpData = PGXP_BCOND_LUT[rt(instr)];
		break;
	case 16:
		//pOpData = PGXP_COP0_LUT[rs(instr)];
		break;
	case 18:
		//if (func(instr) == 1)
		//	pOpData = PGXP_CO2BSC_LUT[rs(instr)];
		//else
		//	pOpData = PGXP_COP2_LUT[func(instr)];
		break;
	default:
		pOpData = PGXP_BSC_LUT[op(instr)];
		break;
	}

	return pOpData;
}

void PrintOperands(char* szBuffer, u32 instr, u32 flags, const char* szDelim, psx_value* psx_regs, u32* regIdx)
{
	char		szTempBuffer[256];
	PGXP_value* pReg = NULL;
	psx_value	psx_reg;
	char		szOpdName[8];
	const char*	szPre = "";

	memset(szTempBuffer, 0, sizeof(szTempBuffer));
	for (u32 opdIdx = 0; opdIdx < 8; opdIdx++)
	{
		u32 flag = 1 << opdIdx;

		// iCB Hack: reorder Rs and Rt for SLLV SRLV and SRAV
		if ((op(instr) < 8) && (op(instr) > 3))
			flag = (flag == fOp_Rs) ? fOp_Rt : ((flag == fOp_Rt) ? fOp_Rs : flag);
		// /iCB Hack

		if (flags & flag)
		{
			switch (flag)
			{
			case fOp_Hi:
				pReg = &CPU_Hi;
				sprintf(szOpdName, "Hi");
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_Lo:
				pReg = &CPU_Lo;
				sprintf(szOpdName, "Lo");
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_Rd:
				pReg = &CPU_reg[rd(instr)];
				sprintf(szOpdName, "Rd[%d]", rd(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_Rs:
				pReg = &CPU_reg[rs(instr)];
				sprintf(szOpdName, "Rs[%d]", rs(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_Rt:
				pReg = &CPU_reg[rt(instr)];
				sprintf(szOpdName, "Rt[%d]", rt(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_Sa:
				pReg = NULL;
				sprintf(szOpdName, "Sa");
				psx_reg.d = sa(instr);
				break;
			case fOp_Im:
				pReg = NULL;
				sprintf(szOpdName, "Imm");
				psx_reg.d = imm(instr);
				break;
			case fOp_Ad:
				pReg = NULL;
				sprintf(szOpdName, "Addr");
				psx_reg = psx_regs[(*regIdx)++];
				break;
			}

			if (pReg)
			{
				sprintf(szTempBuffer, "%s %s [%x(%d, %d) %x(%.2f, %.2f, %.2f)%x : %x] ", szPre, szOpdName,
					psx_reg.d, psx_reg.sw.l, psx_reg.sw.h,
					pReg->value, pReg->x, pReg->y, pReg->z, pReg->count, pReg->valid);
				strcat(szBuffer, szTempBuffer);
			}
			else
			{
				sprintf(szTempBuffer, "%s %s [%x(%d, %d)] ", szPre, szOpdName,
					psx_reg.d, psx_reg.sw.l, psx_reg.sw.h);
				strcat(szBuffer, szTempBuffer);
			}

			szPre = szDelim;
		}
	}
}

void PGXP_CPU_DebugOutput(u32 instr, u32 numOps, u32 op1, u32 op2, u32 op3, u32 op4)
{
	char szBuffer[512];
	PGXP_CPU_OpData opData = GetOpData(instr);
	psx_value psx_regs[4];
	u32 regIdx = 0;
	psx_regs[0].d = op1;
	psx_regs[1].d = op2;
	psx_regs[2].d = op3;
	psx_regs[3].d = op4;

	// iCB Hack: Switch operands around for store functions
	if ((op(instr) >= 40) && (op(instr) < 48))
	{
		psx_regs[0].d = op2;
		psx_regs[1].d = op1;
	}
	// /iCB Hack

	if (!pgxp_debug)
		return;

	memset(szBuffer, 0, sizeof(szBuffer));
	// Print operation details
	sprintf(szBuffer, "%s %x %x: ", opData.szOpName, op(instr), func(instr));
	// Print outputs
	PrintOperands(szBuffer, instr, opData.OutputFlags, "/", psx_regs, &regIdx);
	strcat(szBuffer, "= ");
	// Print inputs
	PrintOperands(szBuffer, instr, opData.InputFlags, opData.szOpString, psx_regs, &regIdx);

#ifdef GTE_LOG
	GTE_LOG("PGXP_Trace: %s |", szBuffer);
#endif
}

void PGXP_psxTrace(u32 instr)
{
	PGXP_CPU_OpData opData = GetOpData(instr);
	if (opData.funcPtr && (opData.numArgs == 0))
		((void(*)(u32))opData.funcPtr)(instr);
	PGXP_CPU_DebugOutput(instr, 0, 0, 0, 0, 0);
}

void PGXP_psxTraceOp1(u32 instr, u32 op1)
{
	PGXP_CPU_OpData opData = GetOpData(instr);
	if (opData.funcPtr && (opData.numArgs == 1))
		((void(*)(u32, u32))opData.funcPtr)(instr, op1);
	PGXP_CPU_DebugOutput(instr, 1, op1, 0, 0, 0);
}

void PGXP_psxTraceOp2(u32 instr, u32 op1, u32 op2)
{
	PGXP_CPU_OpData opData = GetOpData(instr);
	if (opData.funcPtr && (opData.numArgs == 2))
		((void(*)(u32, u32, u32))opData.funcPtr)(instr, op1, op2);
	PGXP_CPU_DebugOutput(instr, 2, op1, op2, 0, 0);
}

void PGXP_psxTraceOp3(u32 instr, u32 op1, u32 op2, u32 op3)
{
	PGXP_CPU_OpData opData = GetOpData(instr);
	if (opData.funcPtr && (opData.numArgs == 3))
		((void(*)(u32, u32, u32, u32))opData.funcPtr)(instr, op1, op2, op3);
	PGXP_CPU_DebugOutput(instr, 3, op1, op2, op3, 0);
}

void PGXP_psxTraceOp4(u32 instr, u32 op1, u32 op2, u32 op3, u32 op4)
{
	PGXP_CPU_OpData opData = GetOpData(instr);
	if (opData.funcPtr && (opData.numArgs == 4))
		((void(*)(u32, u32, u32, u32, u32))opData.funcPtr)(instr, op1, op2, op3, op4);
	PGXP_CPU_DebugOutput(instr, 4, op1, op2, op3, op4);
}