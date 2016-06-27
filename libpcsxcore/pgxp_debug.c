#include "pgxp_debug.h"
#include "pgxp_cpu.h"
#include "pgxp_gte.h"
#include "pgxp_mem.h"
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
	fOp_CPU_Hi = 1 << 0,
	fOp_CPU_Lo = 1 << 1,
	fOp_CPU_Rd = 1 << 2,
	fOp_CPU_Rs = 1 << 3,
	fOp_CPU_Rt = 1 << 4,

	fOp_GTE_Dd = 1 << 5,
	fOp_GTE_Dt = 1 << 6,
	fOp_GTE_Cd = 1 << 7,
	fOp_GTE_Ct = 1 << 8,

	fOp_CP0_Dd = 1 << 9,
	fOp_CP0_Cd = 1 << 10,

	fOp_Ad = 1 << 11,
	fOp_Sa = 1 << 12,
	fOp_Im = 1 << 13
} PGXP_CPU_OperandIDs;

typedef struct
{
	unsigned short OutputFlags;
	unsigned short InputFlags;
	unsigned char numRegisters;
	unsigned char numArgs;
	const char* szOpString;
	const char* szOpName;
	void(*funcPtr)();
} PGXP_CPU_OpData;

void PGXP_CPU_EMPTY() {}
void PGXP_CPU_NULL() { int* pi = NULL; *pi = 5; }
void PGXP_CPU_ERROR() { int* pi = NULL; *pi = 5; }

#define PGXP_Data_ERROR		{ 0, 0, 0, 0, "", "ERROR",		(void(*)())PGXP_CPU_ERROR }
#define PGXP_Data_NULL		{ 0, 0, 0, 0, "", "NULL",		(void(*)())PGXP_CPU_NULL }
#define PGXP_Data_SPECIAL	{ 0, 0, 0, 0, "", "SPECIAL",	(void(*)())PGXP_CPU_EMPTY }
#define PGXP_Data_COP0		{ 0, 0, 0, 0, "", "COP0",		(void(*)())PGXP_CPU_EMPTY }
#define PGXP_Data_COP2		{ 0, 0, 0, 0, "", "COP2",		(void(*)())PGXP_CPU_EMPTY }
#define PGXP_Data_HLE		{ 0, 0, 0, 0, "", "HLE",		(void(*)())PGXP_CPU_EMPTY }

// Arithmetic with immediate value
#define PGXP_Data_ADDI	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "+", "ADDI",	(void(*)())PGXP_CPU_ADDI }
#define PGXP_Data_ADDIU	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "+", "ADDIU",	(void(*)())PGXP_CPU_ADDIU }
#define PGXP_Data_ANDI	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "&", "ANDI",	(void(*)())PGXP_CPU_ANDI }
#define PGXP_Data_ORI	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "|", "ORI",	(void(*)())PGXP_CPU_ORI }
#define PGXP_Data_XORI	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "^", "XORI",	(void(*)())PGXP_CPU_XORI }
#define PGXP_Data_SLTI	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "<", "SLTI",	(void(*)())PGXP_CPU_SLTI }
#define PGXP_Data_SLTIU	{ fOp_CPU_Rt, fOp_CPU_Rs | fOp_Im, 2, 2, "<", "SLTIU",	(void(*)())PGXP_CPU_SLTIU }
// Load Upper
#define PGXP_Data_LUI	{ fOp_CPU_Rt, fOp_Im, 1, 1, "<<", "LUI", (void(*)())PGXP_CPU_LUI }

// Load/Store
#define PGXP_Data_LWL	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LWL",	(void(*)())PGXP_CPU_LWL }	// 32-bit Loads
#define PGXP_Data_LW	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LW",	(void(*)())PGXP_CPU_LW	}
#define PGXP_Data_LWR	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LWR",	(void(*)())PGXP_CPU_LWR }
#define PGXP_Data_LH	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LH",	(void(*)())PGXP_CPU_LH }	// 16-bit Loads
#define PGXP_Data_LHU	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LHU",	(void(*)())PGXP_CPU_LHU }
#define PGXP_Data_LB	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LB",	(void(*)())PGXP_CPU_LB }	// 8-bit Loads
#define PGXP_Data_LBU	{ fOp_CPU_Rt, fOp_Ad, 1, 2, "", "LBU",	(void(*)())PGXP_CPU_LBU }
#define PGXP_Data_SWL	{ fOp_Ad, fOp_CPU_Rt, 1, 2, "", "SWL",	(void(*)())PGXP_CPU_SWL }	// 32-bit Store
#define PGXP_Data_SW	{ fOp_Ad, fOp_CPU_Rt, 1, 2, "", "SW",	(void(*)())PGXP_CPU_SW }
#define PGXP_Data_SWR	{ fOp_Ad, fOp_CPU_Rt, 1, 2, "", "SWR",	(void(*)())PGXP_CPU_SWR }
#define PGXP_Data_SH	{ fOp_Ad, fOp_CPU_Rt, 1, 2, "", "SH",	(void(*)())PGXP_CPU_SH }	// 16-bit Store
#define PGXP_Data_SB	{ fOp_Ad, fOp_CPU_Rt, 1, 2, "", "SB",	(void(*)())PGXP_CPU_SB }	// 8-bit Store

// Load/Store GTE
#define PGXP_Data_LWC2	{ fOp_GTE_Dt, fOp_Ad, 1, 2, "", "LWC2",	(void(*)())PGXP_GTE_LWC2 }	// 32-bit Loads
#define PGXP_Data_SWC2	{ fOp_Ad, fOp_GTE_Dt, 1, 2, "", "SWC2",	(void(*)())PGXP_GTE_SWC2 }	// 32-bit Store

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
#define PGXP_Data_ADD	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "+", "ADD",	(void(*)())PGXP_CPU_ADD }
#define PGXP_Data_ADDU	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "+", "ADDU",	(void(*)())PGXP_CPU_ADDU }
#define PGXP_Data_SUB	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "-", "SUB",	(void(*)())PGXP_CPU_SUB }
#define PGXP_Data_SUBU	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "-", "SUBU",	(void(*)())PGXP_CPU_SUBU }
#define PGXP_Data_AND	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "&", "AND",	(void(*)())PGXP_CPU_AND }
#define PGXP_Data_OR	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "|", "OR",		(void(*)())PGXP_CPU_OR }
#define PGXP_Data_XOR	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "^", "XOR",	(void(*)())PGXP_CPU_XOR }
#define PGXP_Data_NOR	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "^", "NOR",	(void(*)())PGXP_CPU_NOR }
#define PGXP_Data_SLT	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "<", "SLT",	(void(*)())PGXP_CPU_SLT }
#define PGXP_Data_SLTU	{ fOp_CPU_Rd, fOp_CPU_Rs | fOp_CPU_Rt, 3, 3, "<", "SLTU",	(void(*)())PGXP_CPU_SLTU }

// Register mult/div
#define PGXP_Data_MULT	{ fOp_CPU_Hi | fOp_CPU_Lo, fOp_CPU_Rs | fOp_CPU_Rt, 4, 4, "*", "MULT",	(void(*)())PGXP_CPU_MULT }
#define PGXP_Data_MULTU	{ fOp_CPU_Hi | fOp_CPU_Lo, fOp_CPU_Rs | fOp_CPU_Rt, 4, 4, "*", "MULTU",	(void(*)())PGXP_CPU_MULTU }
#define PGXP_Data_DIV	{ fOp_CPU_Hi | fOp_CPU_Lo, fOp_CPU_Rs | fOp_CPU_Rt, 4, 4, "/", "DIV",	(void(*)())PGXP_CPU_DIV }
#define PGXP_Data_DIVU	{ fOp_CPU_Hi | fOp_CPU_Lo, fOp_CPU_Rs | fOp_CPU_Rt, 4, 4, "/", "DIVU",	(void(*)())PGXP_CPU_DIVU }

// Shift operations (sa)
#define PGXP_Data_SLL	{ fOp_CPU_Rd, fOp_CPU_Rt | fOp_Sa, 2, 2, ">>", "SLL",	(void(*)())PGXP_CPU_SLL }
#define PGXP_Data_SRL	{ fOp_CPU_Rd, fOp_CPU_Rt | fOp_Sa, 2, 2, "<<", "SRL",	(void(*)())PGXP_CPU_SRL }
#define PGXP_Data_SRA	{ fOp_CPU_Rd, fOp_CPU_Rt | fOp_Sa, 2, 2, "<<", "SRA",	(void(*)())PGXP_CPU_SRA }

// Shift operations variable
#define PGXP_Data_SLLV	{ fOp_CPU_Rd, fOp_CPU_Rt | fOp_CPU_Rs, 3, 3, ">>", "SLLV",	(void(*)())PGXP_CPU_SLLV }
#define PGXP_Data_SRLV	{ fOp_CPU_Rd, fOp_CPU_Rt | fOp_CPU_Rs, 3, 3, "<<", "SRLV",	(void(*)())PGXP_CPU_SRLV }
#define PGXP_Data_SRAV	{ fOp_CPU_Rd, fOp_CPU_Rt | fOp_CPU_Rs, 3, 3, "<<", "SRAV",	(void(*)())PGXP_CPU_SRAV }

// Move registers
#define PGXP_Data_MFHI	{ fOp_CPU_Rd, fOp_CPU_Hi, 2, 2, "<-", "MFHI",	(void(*)())PGXP_CPU_MFHI }
#define PGXP_Data_MTHI	{ fOp_CPU_Hi, fOp_CPU_Rd, 2, 2, "<-", "MTHI",	(void(*)())PGXP_CPU_MTHI }
#define PGXP_Data_MFLO	{ fOp_CPU_Rd, fOp_CPU_Lo, 2, 2, "<-", "MFLO",	(void(*)())PGXP_CPU_MFLO }
#define PGXP_Data_MTLO	{ fOp_CPU_Lo, fOp_CPU_Rd, 2, 2, "<-", "MFHI",	(void(*)())PGXP_CPU_MTLO }

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

// GTE transfer registers
#define PGXP_Data_MFC2	{ fOp_CPU_Rt, fOp_GTE_Dd, 2, 1, "<-", "MFC2",	(void(*)())PGXP_GTE_MFC2 }
#define PGXP_Data_MTC2	{ fOp_GTE_Dd, fOp_CPU_Rt, 2, 1, "<-", "MTC2",	(void(*)())PGXP_GTE_MTC2 }
#define PGXP_Data_CFC2	{ fOp_CPU_Rt, fOp_GTE_Cd, 2, 1, "<-", "CFC2",	(void(*)())PGXP_GTE_CFC2 }
#define PGXP_Data_CTC2	{ fOp_GTE_Cd, fOp_CPU_Rt, 2, 1, "<-", "CTC2",	(void(*)())PGXP_GTE_CTC2 }

static PGXP_CPU_OpData  PGXP_CO2BSC_LUT[32] = {
	PGXP_Data_MFC2, PGXP_Data_NULL, PGXP_Data_CFC2, PGXP_Data_NULL, PGXP_Data_MTC2, PGXP_Data_NULL, PGXP_Data_CTC2, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL
};

// CP0 transfer registers
#define PGXP_Data_MFC0	{ fOp_CPU_Rt, fOp_CP0_Dd, 2, 1, "<-", "MFC0",	(void(*)())PGXP_CP0_MFC0 }
#define PGXP_Data_MTC0	{ fOp_CP0_Dd, fOp_CPU_Rt, 2, 1, "<-", "MTC0",	(void(*)())PGXP_CP0_MTC0 }
#define PGXP_Data_CFC0	{ fOp_CPU_Rt, fOp_CP0_Cd, 2, 1, "<-", "CFC0",	(void(*)())PGXP_CP0_CFC0 }
#define PGXP_Data_CTC0	{ fOp_CP0_Cd, fOp_CPU_Rt, 2, 1, "<-", "CTC0",	(void(*)())PGXP_CP0_CTC0 }
#define PGXP_Data_RFE	{ 0, 0, 0, 0,"", "RFE", PGXP_CPU_EMPTY }

static PGXP_CPU_OpData PGXP_COP0_LUT[32] = {
	PGXP_Data_MFC0, PGXP_Data_NULL, PGXP_Data_CFC0, PGXP_Data_NULL, PGXP_Data_MTC0, PGXP_Data_NULL, PGXP_Data_CTC0, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_RFE , PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL,
	PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL, PGXP_Data_NULL
};

PGXP_CPU_OpData GetOpData(u32 instr)
{
	PGXP_CPU_OpData pOpData = PGXP_Data_ERROR;
	switch (op(instr))
	{
	case 0:
		if (func(instr) < 64)
			pOpData = PGXP_SPC_LUT[func(instr)];
		break;
	case 1:
		//pOpData = PGXP_BCOND_LUT[rt(instr)];
		break;
	case 16:
		if (rs(instr) < 32)
			pOpData = PGXP_COP0_LUT[rs(instr)];
		break;
	case 18:
		if ((func(instr) == 0) && (rs(instr) < 32))
			pOpData = PGXP_CO2BSC_LUT[rs(instr)];
		//else
		//	pOpData = PGXP_COP2_LUT[func(instr)];
		break;
	default:
		if(op(instr) < 64)
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
	char		szOpdName[16];
	const char*	szPre = "";

	memset(szTempBuffer, 0, sizeof(szTempBuffer));
	for (u32 opdIdx = 0; opdIdx < 14; opdIdx++)
	{
		u32 flag = 1 << opdIdx;

		// iCB Hack: reorder Rs and Rt for SLLV SRLV and SRAV
		if ((op(instr) < 8) && (op(instr) > 3))
			flag = (flag == fOp_CPU_Rs) ? fOp_CPU_Rt : ((flag == fOp_CPU_Rt) ? fOp_CPU_Rs : flag);
		// /iCB Hack

		if (flags & flag)
		{
			switch (flag)
			{
			case fOp_CPU_Hi:
				pReg = &CPU_Hi;
				sprintf(szOpdName, "Hi");
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_CPU_Lo:
				pReg = &CPU_Lo;
				sprintf(szOpdName, "Lo");
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_CPU_Rd:
				pReg = &CPU_reg[rd(instr)];
				sprintf(szOpdName, "Rd[%d]", rd(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_CPU_Rs:
				pReg = &CPU_reg[rs(instr)];
				sprintf(szOpdName, "Rs[%d]", rs(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_CPU_Rt:
				pReg = &CPU_reg[rt(instr)];
				sprintf(szOpdName, "Rt[%d]", rt(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_GTE_Dd:
				pReg = &GTE_data_reg[rd(instr)];
				sprintf(szOpdName, "GTE_Dd[%d]", rd(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_GTE_Dt:
				pReg = &GTE_data_reg[rt(instr)];
				sprintf(szOpdName, "GTE_Dt[%d]", rt(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_GTE_Cd:
				pReg = &GTE_ctrl_reg[rd(instr)];
				sprintf(szOpdName, "GTE_Cd[%d]", rd(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_GTE_Ct:
				pReg = &GTE_ctrl_reg[rt(instr)];
				sprintf(szOpdName, "GTE_Ct[%d]", rt(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_CP0_Dd:
				pReg = &CP0_reg[rd(instr)];
				sprintf(szOpdName, "CP0_Dd[%d]", rd(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_CP0_Cd:
				pReg = &CP0_reg[rd(instr)];
				sprintf(szOpdName, "CP0_Cd[%d]", rd(instr));
				psx_reg = psx_regs[(*regIdx)++];
				break;
			case fOp_Ad:
				pReg = NULL;
				sprintf(szOpdName, "Addr");
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
			}

			if (pReg)
			{
				sprintf(szTempBuffer, "%s %s [%x(%d, %d) %x(%.2f, %.2f, %.2f)%x : %x] ", szPre, szOpdName,
					psx_reg.d, psx_reg.sw.l, psx_reg.sw.h,
					pReg->value, pReg->x, pReg->y, pReg->z, pReg->count, pReg->valid);
				strcat(szBuffer, szTempBuffer);
			}
			else if(flag == fOp_Ad)
			{
				sprintf(szTempBuffer, "%s %s [%x(%d, %d) (%x)] ", szPre, szOpdName,
					psx_reg.d, psx_reg.sw.l, psx_reg.sw.h, PGXP_ConvertAddress(psx_reg.d));
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
	char szOutputBuffer[256];
	char szInputBuffer[512];
	PGXP_CPU_OpData opData = GetOpData(instr);
	psx_value psx_regs[4];
	u32 regIdx = 0;
	psx_regs[0].d = op1;
	psx_regs[1].d = op2;
	psx_regs[2].d = op3;
	psx_regs[3].d = op4;

	// iCB Hack: Switch operands around for store functions
	if ((op(instr) >= 40)&& (op(instr) != 50))
	{
		psx_regs[0].d = op2;
		psx_regs[1].d = op1;
	}

	// Hack: duplicate psx register data for GTE register movement funcs
	if ((op(instr) == 18) && (func(instr) == 0))
		psx_regs[1] = psx_regs[0];

	// /iCB Hack

	// reset buffers
	if (pgxp_debug)
	{
		memset(szInputBuffer, 0, sizeof(szInputBuffer));
		memset(szOutputBuffer, 0, sizeof(szOutputBuffer));

		// skip output arguments
		for (u32 opdIdx = 0; opdIdx < 12; opdIdx++)
		{
			if (opData.OutputFlags & (1 << opdIdx))
				regIdx++;
		}

		// Print inputs
		PrintOperands(szInputBuffer, instr, opData.InputFlags, opData.szOpString, psx_regs, &regIdx);
	}

	// Call function
	if (numOps != opData.numArgs)
		PGXP_CPU_ERROR();

	switch (numOps)
	{
	case 0:
		((void(*)(u32))opData.funcPtr)(instr);
		break;
	case 1:
		((void(*)(u32, u32))opData.funcPtr)(instr, op1);
		break;
	case 2:
		((void(*)(u32, u32, u32))opData.funcPtr)(instr, op1, op2);
		break;
	case 3:
		((void(*)(u32, u32, u32, u32))opData.funcPtr)(instr, op1, op2, op3);
		break;
	case 4:
		((void(*)(u32, u32, u32, u32, u32))opData.funcPtr)(instr, op1, op2, op3, op4);
		break;
	}

	// Print operation details
	if (pgxp_debug)
	{
		sprintf(szOutputBuffer, "%s %x %x: ", opData.szOpName, op(instr), func(instr));
		// Print outputs
		regIdx = 0;
		PrintOperands(szOutputBuffer, instr, opData.OutputFlags, "/", psx_regs, &regIdx);
		strcat(szOutputBuffer, "=");

#ifdef GTE_LOG
		GTE_LOG("PGXP_Trace: %s %s|", szOutputBuffer, szInputBuffer);
#endif
	}
}

void PGXP_psxTraceOp(u32 instr)
{
	//PGXP_CPU_OpData opData = GetOpData(instr);
	//if (opData.funcPtr && (opData.numArgs == 0))
	//	((void(*)(u32))opData.funcPtr)(instr);
	PGXP_CPU_DebugOutput(instr, 0, 0, 0, 0, 0);
}

void PGXP_psxTraceOp1(u32 instr, u32 op1)
{
	//PGXP_CPU_OpData opData = GetOpData(instr);
	//if (opData.funcPtr && (opData.numArgs == 1))
	//	((void(*)(u32, u32))opData.funcPtr)(instr, op1);
	PGXP_CPU_DebugOutput(instr, 1, op1, 0, 0, 0);
}

void PGXP_psxTraceOp2(u32 instr, u32 op1, u32 op2)
{
	//PGXP_CPU_OpData opData = GetOpData(instr);
	//if (opData.funcPtr && (opData.numArgs == 2))
	//	((void(*)(u32, u32, u32))opData.funcPtr)(instr, op1, op2);
	PGXP_CPU_DebugOutput(instr, 2, op1, op2, 0, 0);
}

void PGXP_psxTraceOp3(u32 instr, u32 op1, u32 op2, u32 op3)
{
	//PGXP_CPU_OpData opData = GetOpData(instr);
	//if (opData.funcPtr && (opData.numArgs == 3))
	//	((void(*)(u32, u32, u32, u32))opData.funcPtr)(instr, op1, op2, op3);
	PGXP_CPU_DebugOutput(instr, 3, op1, op2, op3, 0);
}

void PGXP_psxTraceOp4(u32 instr, u32 op1, u32 op2, u32 op3, u32 op4)
{
	//PGXP_CPU_OpData opData = GetOpData(instr);
	//if (opData.funcPtr && (opData.numArgs == 4))
	//	((void(*)(u32, u32, u32, u32, u32))opData.funcPtr)(instr, op1, op2, op3, op4);
	PGXP_CPU_DebugOutput(instr, 4, op1, op2, op3, op4);
}