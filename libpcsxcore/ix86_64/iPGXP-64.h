#ifndef _I_PGXP_H_
#define _I_PGXP_H_

// Microsoft Windows uses a different x86_64 calling convention than everyone
// else. I have not yet bothered implementing it here because;
//
// 1. Nobody cares about a Windows 64-bit build of PCSXR since that would mean
//    dropping popular closed source 32-bit plugins like Pete's OpenGL2
// 2. The Windows convention is annoying (only 4 register params, caller must
//    reserve stack space for register spilling) and would require more
//    extensive code changes (e.g. the PGXP_DBG_OP_E() macro would have to
//    handle cases where the arg needs to go on the stack instead of in a
//    register, and cleanup afterwards).
//
// See https://msdn.microsoft.com/en-us/library/ms235286.aspx
// and https://en.wikipedia.org/wiki/X86_calling_conventions#x86-64_calling_conventions
//
// MrLavender
#ifdef _MSC_VER
#error PGXP dynarec support is not implemented for Windows 64-bit
#endif

/////////////////////////////////////////////
// PGXP wrapper functions
/////////////////////////////////////////////

void pgxpRecNULL() {}

// Debug wrappers for x86_64 (because eOp will be last)
#ifdef PGXP_CPU_DEBUG
static void PGXP64_psxTraceOp(u32 code, u32 eOp) {
	PGXP_psxTraceOp(eOp, code);
}
static void PGXP64_psxTraceOp1(u32 code, u32 op1, u32 eOp) {
	PGXP_psxTraceOp1(eOp, code, op1);
}
static void PGXP64_psxTraceOp2(u32 code, u32 op1, u32 op2, u32 eOp) {
	PGXP_psxTraceOp2(eOp, code, op1, op2);
}
static void PGXP64_psxTraceOp3(u32 code, u32 op1, u32 op2, u32 op3, u32 eOp) {
	PGXP_psxTraceOp3(eOp, code, op1, op2, op3);
}
static void PGXP64_psxTraceOp4(u32 code, u32 op1, u32 op2, u32 op3, u32 op4, u32 eOp) {
	PGXP_psxTraceOp4(eOp, code, op1, op2, op3, op4);
}
#endif

// Choose between debug and direct function
#ifdef PGXP_CPU_DEBUG
#define PGXP_REC_FUNC_OP(pu, op, nReg) PGXP64_psxTraceOp##nReg
#define PGXP_DBG_OP_E(op, arg) MOV32ItoR(arg, DBG_E_##op);
#else
#define PGXP_REC_FUNC_OP(pu, op, nReg) PGXP_##pu##_##op
#define PGXP_DBG_OP_E(op, arg)
#endif

#define PGXP_REC_FUNC_PASS(pu, op) \
static void pgxpRec##op() { \
	rec##op();\
}

#define PGXP_REC_FUNC(pu, op) \
static void pgxpRec##op() { \
	MOV32ItoR(X86ARG1, psxRegs.code); \
	PGXP_DBG_OP_E(op, X86ARG2) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, )); \
	rec##op();\
}

#define PGXP_REC_FUNC_1(pu, op, reg1) \
static void pgxpRec##op() { \
	reg1;\
	MOV32ItoR(X86ARG1, psxRegs.code); \
	POP64R(X86ARG2); \
	PGXP_DBG_OP_E(op, X86ARG3) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 1)); \
	rec##op();\
}

//#define PGXP_REC_FUNC_2_2(pu, op, test, nReg, reg1, reg2, reg3, reg4) \
//static void pgxpRec##op() { \
//	if(test) { rec##op(); return; }\
//	reg1;\
//	reg2;\
//	rec##op();\
//	reg3;\
//	reg4;\
//	PUSH32I(psxRegs.code); \
//	PGXP_DBG_OP_E(op) \
//	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, nReg)); \
//	resp += (4 * nReg) + 4; \
//}

#define PGXP_REC_FUNC_2(pu, op, reg1, reg2) \
static void pgxpRec##op() { \
	reg1;\
	reg2;\
	MOV32ItoR(X86ARG1, psxRegs.code); \
	POP64R(X86ARG2); \
	POP64R(X86ARG3); \
	PGXP_DBG_OP_E(op, X86ARG4) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 2)); \
	rec##op();\
}

static u32 gTempAddr = 0;
#define PGXP_REC_FUNC_ADDR_1(pu, op, reg1) \
static void pgxpRec##op()	\
{	\
	if (IsConst(_Rs_))	\
	{	\
		MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);	\
	}	\
	else\
	{\
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);\
		if (_Imm_)\
		{\
			ADD32ItoR(EAX, _Imm_);\
		}\
	}\
	MOV32RtoM((uptr)&gTempAddr, EAX);\
	rec##op();\
	reg1;\
	MOV32ItoR(X86ARG1, psxRegs.code); \
	POP64R(X86ARG2); \
	MOV32MtoR(X86ARG3, (uptr)&gTempAddr); \
	PGXP_DBG_OP_E(op, X86ARG4) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 2)); \
}


#define CPU_REG_NC(idx) MOV32MtoR(EAX,(uptr)&psxRegs.GPR.r[idx])

#define CPU_REG(idx) \
	if (IsConst(idx))	\
		MOV32ItoR(EAX, iRegs[idx].k);	\
	else\
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[idx]);

#define CP0_REG(idx) MOV32MtoR(EAX,(uptr)&psxRegs.CP0.r[idx])
#define GTE_DATA_REG(idx) MOV32MtoR(EAX,(uptr)&psxRegs.CP2D.r[idx])
#define GTE_CTRL_REG(idx) MOV32MtoR(EAX,(uptr)&psxRegs.CP2C.r[idx])

static u32 gTempInstr = 0;
static u32 gTempReg1 = 0;
static u32 gTempReg2 = 0;
#define PGXP_REC_FUNC_R1_1(pu, op, test, reg1, reg2) \
static void pgxpRec##op()	\
{	\
	if(test) { rec##op(); return; }\
	reg1;\
	MOV32RtoM((uptr)&gTempReg1, EAX);\
	rec##op();\
	reg2;\
	MOV32ItoR(X86ARG1, psxRegs.code); \
	POP64R(X86ARG2); \
	MOV32MtoR(X86ARG3, (uptr)&gTempReg1); \
	PGXP_DBG_OP_E(op, X86ARG4) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 2)); \
}

#define PGXP_REC_FUNC_R2_1(pu, op, test, reg1, reg2, reg3) \
static void pgxpRec##op()	\
{	\
	if(test) { rec##op(); return; }\
	reg1;\
	MOV32RtoM((uptr)&gTempReg1, EAX);\
	reg2;\
	MOV32RtoM((uptr)&gTempReg2, EAX);\
	rec##op();\
	reg3;\
	MOV32ItoR(X86ARG1, psxRegs.code); \
	POP64R(X86ARG2); \
	MOV32MtoR(X86ARG3, (uptr)&gTempReg2); \
	MOV32MtoR(X86ARG4, (uptr)&gTempReg1); \
	PGXP_DBG_OP_E(op, X86ARG5) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 3)); \
}

#define PGXP_REC_FUNC_R2_2(pu, op, test, reg1, reg2, reg3, reg4) \
static void pgxpRec##op()	\
{	\
	if(test) { rec##op(); return; }\
	reg1;\
	MOV32RtoM((uptr)&gTempReg1, EAX);\
	reg2;\
	MOV32RtoM((uptr)&gTempReg2, EAX);\
	rec##op();\
	reg3;\
	reg4;\
	MOV32ItoR(X86ARG1, psxRegs.code); \
	POP64R(X86ARG2); \
	POP64R(X86ARG3); \
	MOV32MtoR(X86ARG4, (uptr)&gTempReg2); \
	MOV32MtoR(X86ARG5, (uptr)&gTempReg1); \
	PGXP_DBG_OP_E(op, X86ARG6) \
	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 4)); \
}

//#define PGXP_REC_FUNC_R1i_1(pu, op, test, reg1, reg2) \
//static void pgxpRec##op()	\
//{	\
//	if(test) { rec##op(); return; }\
//	if (IsConst(reg1))	\
//		MOV32ItoR(EAX, iRegs[reg1].k);	\
//	else\
//		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[reg1]);\
//	MOV32RtoM((uptr)&gTempReg, EAX);\
//	rec##op();\
//	PUSH64M((uptr)&gTempReg);\
//	reg2;\
//	PUSH32I(psxRegs.code);	\
//	CALLFunc((uptr)PGXP_REC_FUNC_OP(pu, op, 2)); \
//	resp += 12; \
//}

// Push m32
#define PUSH32M(from) MOV32MtoR(EAX, from); PUSH64R(RAX);

static void iPushReg(int reg)
{
	if (IsConst(reg)) {
		PUSH32I(iRegs[reg].k);
	}
	else {
		PUSH32M((uptr)&psxRegs.GPR.r[reg]);
	}
}

// Rt = Rs op imm 
PGXP_REC_FUNC_R1_1(CPU, ADDI,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CPU, ADDIU,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CPU, ANDI,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CPU, ORI,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CPU, XORI,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CPU, SLTI,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CPU, SLTIU,	!_Rt_, CPU_REG(_Rs_), iPushReg(_Rt_))

// Rt = imm
//PGXP_REC_FUNC_2_2(CPU, LUI, !_Rt_, 1, , , iPushReg(_Rt_), )
//This macro is harder to implement for x86_64, and only used once, so... :) MrL
static void pgxpRecLUI()
{
	if (!_Rt_) { recLUI(); return; }
	recLUI();
	iPushReg(_Rt_);
	MOV32ItoR(X86ARG1, psxRegs.code);
	POP64R(X86ARG2);
	PGXP_DBG_OP_E(LUI, X86ARG3)
	CALLFunc((uptr)PGXP_REC_FUNC_OP(CPU, LUI, 1));
}

// Rd = Rs op Rt 
PGXP_REC_FUNC_R2_1(CPU, ADD,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, ADDU,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, SUB,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, SUBU,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, AND,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, OR,		!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, XOR,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, NOR,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, SLT,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, SLTU,	!_Rd_, CPU_REG(_Rt_), CPU_REG(_Rs_), iPushReg(_Rd_))

// Hi/Lo = Rs op Rt
PGXP_REC_FUNC_R2_2(CPU, MULT, 0, CPU_REG(_Rt_), CPU_REG(_Rs_), PUSH32M((uptr)&psxRegs.GPR.n.lo), PUSH32M((uptr)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_R2_2(CPU, MULTU, 0, CPU_REG(_Rt_), CPU_REG(_Rs_), PUSH32M((uptr)&psxRegs.GPR.n.lo), PUSH32M((uptr)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_R2_2(CPU, DIV, 0, CPU_REG(_Rt_), CPU_REG(_Rs_), PUSH32M((uptr)&psxRegs.GPR.n.lo), PUSH32M((uptr)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_R2_2(CPU, DIVU, 0, CPU_REG(_Rt_), CPU_REG(_Rs_), PUSH32M((uptr)&psxRegs.GPR.n.lo), PUSH32M((uptr)&psxRegs.GPR.n.hi))

PGXP_REC_FUNC_ADDR_1(CPU, SB, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, SH, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, SW, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, SWL, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, SWR, iPushReg(_Rt_))

PGXP_REC_FUNC_ADDR_1(CPU, LWL, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LW, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LWR, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LH, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LHU, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LB, iPushReg(_Rt_))
PGXP_REC_FUNC_ADDR_1(CPU, LBU, iPushReg(_Rt_))

//Rd = Rt op Sa
PGXP_REC_FUNC_R1_1(CPU, SLL, !_Rd_, CPU_REG(_Rt_), iPushReg(_Rd_))
PGXP_REC_FUNC_R1_1(CPU, SRL, !_Rd_, CPU_REG(_Rt_), iPushReg(_Rd_))
PGXP_REC_FUNC_R1_1(CPU, SRA, !_Rd_, CPU_REG(_Rt_), iPushReg(_Rd_))

// Rd = Rt op Rs
PGXP_REC_FUNC_R2_1(CPU, SLLV, !_Rd_, CPU_REG(_Rs_), CPU_REG(_Rt_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, SRLV, !_Rd_, CPU_REG(_Rs_), CPU_REG(_Rt_), iPushReg(_Rd_))
PGXP_REC_FUNC_R2_1(CPU, SRAV, !_Rd_, CPU_REG(_Rs_), CPU_REG(_Rt_), iPushReg(_Rd_))

PGXP_REC_FUNC_R1_1(CPU, MFHI, !_Rd_,	CPU_REG_NC(33), iPushReg(_Rd_))
PGXP_REC_FUNC_R1_1(CPU, MTHI, 0,		CPU_REG(_Rd_),  PUSH32M((uptr)&psxRegs.GPR.n.hi))
PGXP_REC_FUNC_R1_1(CPU, MFLO, !_Rd_,	CPU_REG_NC(32), iPushReg(_Rd_))
PGXP_REC_FUNC_R1_1(CPU, MTLO, 0,		CPU_REG(_Rd_),  PUSH32M((uptr)&psxRegs.GPR.n.lo))

// COP2 (GTE)
PGXP_REC_FUNC_R1_1(GTE, MFC2, !_Rt_, GTE_DATA_REG(_Rd_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(GTE, CFC2, !_Rt_, GTE_CTRL_REG(_Rd_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(GTE, MTC2, 0, CPU_REG(_Rt_), PUSH32M((uptr)&psxRegs.CP2D.r[_Rd_]))
PGXP_REC_FUNC_R1_1(GTE, CTC2, 0, CPU_REG(_Rt_), PUSH32M((uptr)&psxRegs.CP2C.r[_Rd_]))

PGXP_REC_FUNC_ADDR_1(GTE, LWC2, PUSH32M((uptr)&psxRegs.CP2D.r[_Rt_]))
PGXP_REC_FUNC_ADDR_1(GTE, SWC2, PUSH32M((uptr)&psxRegs.CP2D.r[_Rt_]))

// COP0
PGXP_REC_FUNC_R1_1(CP0, MFC0, !_Rd_, CP0_REG(_Rd_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CP0, CFC0, !_Rd_, CP0_REG(_Rd_), iPushReg(_Rt_))
PGXP_REC_FUNC_R1_1(CP0, MTC0, !_Rt_, CPU_REG(_Rt_), PUSH32M((uptr)&psxRegs.CP0.r[_Rd_]))
PGXP_REC_FUNC_R1_1(CP0, CTC0, !_Rt_, CPU_REG(_Rt_), PUSH32M((uptr)&psxRegs.CP0.r[_Rd_]))
PGXP_REC_FUNC(CP0, RFE)

#endif//_I_PGXP_H_
