#ifndef _I_PGXP_H_
#define _I_PGXP_H_


/////////////////////////////////////////////
// PGXP wrapper functions
/////////////////////////////////////////////

pgxpRecNULL() {}

// Choose between debug and direct function
#ifdef PGXP_CPU_DEBUG
#define PGXP_REC_FUNC_OP(pu, op, nReg) PGXP_psxTraceOp##nReg
#else
#define PGXP_REC_FUNC_OP(pu, op, nReg) PGXP_##pu##_##op
#endif

#define PGXP_REC_FUNC_PASS(pu, op) \
static void pgxpRec##op() { \
	rec##op();\
}

#define PGXP_REC_FUNC(pu, op) \
static void pgxpRec##op() { \
	PUSH32I(psxRegs.code); \
	CALLFunc((u32)PGXP_REC_FUNC_OP(pu, op, )); \
	resp += 4; \
	rec##op();\
}

#define PGXP_REC_FUNC_1(pu, op, reg1) \
static void pgxpRec##op() { \
	reg1;\
	PUSH32I(psxRegs.code); \
	CALLFunc((u32)PGXP_REC_FUNC_OP(pu, op, 1)); \
	resp += 8; \
	rec##op();\
}

#define PGXP_REC_FUNC_2_2(pu, op, test, nReg, reg1, reg2, reg3, reg4) \
static void pgxpRec##op() { \
	if(test) return;\
	reg1;\
	reg2;\
	rec##op();\
	reg3;\
	reg4;\
	PUSH32I(psxRegs.code); \
	CALLFunc((u32)PGXP_REC_FUNC_OP(pu, op, nReg)); \
	resp += (4 * nReg) + 4; \
}

#define PGXP_REC_FUNC_2(pu, op, reg1, reg2) \
static void pgxpRec##op() { \
	reg1;\
	reg2;\
	PUSH32I(psxRegs.code); \
	CALLFunc((u32)PGXP_REC_FUNC_OP(pu, op, 2)); \
	resp += 12; \
	rec##op();\
}

#define PGXP_REC_FUNC_ADDR_1(pu, op, reg1) \
static void pgxpRec##op()	\
{	\
	if (IsConst(_Rs_))	\
	{	\
		MOV32ItoR(EBX, iRegs[_Rs_].k + _Imm_);	\
	}	\
	else\
	{\
		MOV32MtoR(EBX, (u32)&psxRegs.GPR.r[_Rs_]);\
		if (_Imm_)\
		{\
			ADD32ItoR(EBX, _Imm_);\
		}\
	}\
	rec##op();\
	PUSH32R(EBX);\
	reg1;\
	PUSH32I(psxRegs.code);	\
	CALLFunc((u32)PGXP_REC_FUNC_OP(pu, op, 2)); \
	resp += 12; \
}

// Rt = Rs op imm 
PGXP_REC_FUNC_2_2(CPU, ADDI, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )
PGXP_REC_FUNC_2_2(CPU, ADDIU, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )
PGXP_REC_FUNC_2_2(CPU, ANDI, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )
PGXP_REC_FUNC_2_2(CPU, ORI, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )
PGXP_REC_FUNC_2_2(CPU, XORI, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )
PGXP_REC_FUNC_2_2(CPU, SLTI, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )
PGXP_REC_FUNC_2_2(CPU, SLTIU, !_Rt_, 2, iPushReg(_Rs_), , iPushReg(_Rt_), )

// Rt = imm
PGXP_REC_FUNC_2_2(CPU, LUI, !_Rt_, 1, , , iPushReg(_Rt_), )

// Rd = Rs op Rt 
PGXP_REC_FUNC_2_2(CPU, ADD, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, ADDU, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SUB, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SUBU, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, AND, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, OR, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, XOR, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, NOR, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SLT, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SLTU, !_Rd_, 3, iPushReg(_Rt_), iPushReg(_Rs_), iPushReg(_Rd_), )

// Hi/Lo = Rs op Rt
PGXP_REC_FUNC_2_2(CPU, MULT, 0, 4, iPushReg(_Rt_), iPushReg(_Rs_), PUSH32M((u32)&psxRegs.GPR.n.lo), PUSH32M((u32)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_2_2(CPU, MULTU, 0, 4, iPushReg(_Rt_), iPushReg(_Rs_), PUSH32M((u32)&psxRegs.GPR.n.lo), PUSH32M((u32)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_2_2(CPU, DIV, 0, 4, iPushReg(_Rt_), iPushReg(_Rs_), PUSH32M((u32)&psxRegs.GPR.n.lo), PUSH32M((u32)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_2_2(CPU, DIVU, 0, 4, iPushReg(_Rt_), iPushReg(_Rs_), PUSH32M((u32)&psxRegs.GPR.n.lo), PUSH32M((u32)&psxRegs.GPR.n.hi))

PGXP_REC_FUNC_2(CPU, SB, iPushOfB(), iPushReg(_Rt_))
PGXP_REC_FUNC_2(CPU, SH, iPushOfB(), iPushReg(_Rt_))
PGXP_REC_FUNC_2(CPU, SW, iPushOfB(), iPushReg(_Rt_))
PGXP_REC_FUNC_2(CPU, SWL, iPushOfB(), iPushReg(_Rt_))
PGXP_REC_FUNC_2(CPU, SWR, iPushOfB(), iPushReg(_Rt_))

PGXP_REC_FUNC_ADDR_1(CPU, LWL, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LW, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LWR, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LH, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LHU, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LB, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LBU, iPushReg(_Rt_))

//Rd = Rt op Sa
PGXP_REC_FUNC_2_2(CPU, SLL, !_Rd_, 2, iPushReg(_Rt_), , iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SRL, !_Rd_, 2, iPushReg(_Rt_), , iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SRA, !_Rd_, 2, iPushReg(_Rt_), , iPushReg(_Rd_), )

// Rd = Rt op Rs
PGXP_REC_FUNC_2_2(CPU, SLLV, !_Rd_, 3, iPushReg(_Rs_), iPushReg(_Rt_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SRLV, !_Rd_, 3, iPushReg(_Rs_), iPushReg(_Rt_), iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, SRAV, !_Rd_, 3, iPushReg(_Rs_), iPushReg(_Rt_), iPushReg(_Rd_), )

PGXP_REC_FUNC_2_2(CPU, MFHI, !_Rd_, 2, PUSH32M((u32)&psxRegs.GPR.n.hi), , iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, MTHI, 0, 2, iPushReg(_Rd_), , PUSH32M((u32)&psxRegs.GPR.n.hi), )
PGXP_REC_FUNC_2_2(CPU, MFLO, !_Rd_, 2, PUSH32M((u32)&psxRegs.GPR.n.lo), , iPushReg(_Rd_), )
PGXP_REC_FUNC_2_2(CPU, MTLO, 0, 2, iPushReg(_Rd_), , PUSH32M((u32)&psxRegs.GPR.n.lo), )

// COP2 (GTE)
PGXP_REC_FUNC_2_2(GTE, MFC2, !_Rt_, 1, , , , PUSH32M((u32)&psxRegs.CP2D.r[_Rd_]))
PGXP_REC_FUNC_2_2(GTE, CFC2, !_Rt_, 1, , , , PUSH32M((u32)&psxRegs.CP2C.r[_Rd_]))
PGXP_REC_FUNC_2_2(GTE, MTC2, 0, 1, , , , iPushReg(_Rt_))
PGXP_REC_FUNC_2_2(GTE, CTC2, 0, 1, , , , iPushReg(_Rt_))

PGXP_REC_FUNC_ADDR_1(GTE, LWC2, PUSH32M((u32)&psxRegs.CP2D.r[_Rt_]))
PGXP_REC_FUNC_ADDR_1(GTE, SWC2, PUSH32M((u32)&psxRegs.CP2D.r[_Rt_]))

// COP0
PGXP_REC_FUNC_2_2(CP0, MFC0, !_Rd_, 1, , , , PUSH32M((u32)&psxRegs.CP0.r[_Rd_]))
PGXP_REC_FUNC_2_2(CP0, CFC0, !_Rd_, 1, , , , PUSH32M((u32)&psxRegs.CP0.r[_Rd_]))
PGXP_REC_FUNC_2_2(CP0, MTC0, !_Rt_, 1, , , , iPushReg(_Rt_))
PGXP_REC_FUNC_2_2(CP0, CTC0, !_Rt_, 1, , , , iPushReg(_Rt_))
PGXP_REC_FUNC(CP0, RFE)

#endif//_I_PGXP_H_
