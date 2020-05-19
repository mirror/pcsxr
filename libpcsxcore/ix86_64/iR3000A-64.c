/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

/*
* i386 assembly functions for R3000A core.
*/

#ifdef __x86_64__

#include "ix86-64.h"
#include "../r3000a.h"
#include "../psxhle.h"

#include "../pgxp_cpu.h"
#include "../pgxp_gte.h"
#include "../pgxp_debug.h"

#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

uptr* psxRecLUT;

#define PTRMULT (sizeof(uptr) / sizeof(u32))

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	(psxRecLUT[(x) >> 16] + PTRMULT * ((x) & 0xffff))
#define PC_RECP(x) (*(uptr *)PC_REC(x))

#define RECMEM_SIZE		(PTRMULT * 8 * 1024 * 1024)

static char *recMem;	/* the recompiled blocks will be here */
static char *recRAM;	/* and the ptr to the blocks here */
static char *recROM;	/* and here */

static u32 pc;			/* recompiler pc */
static u32 pcold;		/* recompiler oldpc */
static int count;		/* recompiler intruction count */
static int branch;		/* set for branch */
static u32 target;		/* branch target */
static u32 resp;

typedef struct {
	int state;
	u32 k;
	int reg;
} iRegisters;

static iRegisters iRegs[32];
static iRegisters iRegsS[32];
static psxRegisters psxRegsS;

#define ST_UNK    0
#define ST_CONST  1
#define ST_MAPPED 2

#define IsConst(reg)  (iRegs[reg].state == ST_CONST)
#define IsMapped(reg) (iRegs[reg].state == ST_MAPPED)

static void (*recBSC[64])();
static void (*recSPC[64])();
static void (*recREG[32])();
static void (*recCP0[32])();
static void (*recCP2[64])();
static void (*recCP2BSC[32])();


/// PGXP function tables
static void(*pgxpRecBSC[64])();
static void(*pgxpRecSPC[64])();
static void(*pgxpRecCP0[32])();
static void(*pgxpRecCP2BSC[32])();

static void(*pgxpRecBSCMem[64])();
///

static void(**pRecBSC)() = recBSC;
static void(**pRecSPC)() = recSPC;
static void(**pRecREG)() = recREG;
static void(**pRecCP0)() = recCP0;
static void(**pRecCP2)() = recCP2;
static void(**pRecCP2BSC)() = recCP2BSC;

static void recReset();
static void recSetPGXPMode(u32 pgxpMode)
{
	switch (pgxpMode)
	{
	case 0: //PGXP_MODE_DISABLED:
		pRecBSC = recBSC;
		pRecSPC = recSPC;
		pRecREG = recREG;
		pRecCP0 = recCP0;
		pRecCP2 = recCP2;
		pRecCP2BSC = recCP2BSC;
		break;
	case 1: //PGXP_MODE_MEM:
		pRecBSC = pgxpRecBSCMem;
		pRecSPC = recSPC;
		pRecREG = recREG;
		pRecCP0 = pgxpRecCP0;
		pRecCP2 = recCP2;
		pRecCP2BSC = pgxpRecCP2BSC;
		break;
	case 2: //PGXP_MODE_FULL:
		pRecBSC = pgxpRecBSC;
		pRecSPC = pgxpRecSPC;
		pRecREG = recREG;
		pRecCP0 = pgxpRecCP0;
		pRecCP2 = recCP2;
		pRecCP2BSC = pgxpRecCP2BSC;
		break;
	}

	// set interpreter mode too
	psxInt.SetPGXPMode(pgxpMode);
	// reset to ensure new func tables are used
	recReset();
}

#define STACKSIZE		0x18
static void StackRes()
{
#ifdef __x86_64__
	ADD64ItoR(RSP, STACKSIZE);
#else
	if (resp) ADD32ItoR(ESP, resp);
#endif
}

int dump = FALSE;

static void MapConst(int reg, u32 _const) {
	iRegs[reg].k = _const;
	iRegs[reg].state = ST_CONST;
}

static void iFlushReg(int reg) {
	if (IsConst(reg)) {
		MOV32ItoM((uptr)&psxRegs.GPR.r[reg], iRegs[reg].k);
	}
	iRegs[reg].state = ST_UNK;
}


static inline void iDump(const iRegisters* iRegsParam) {
#ifdef PSXCPU_LOG
	int i;

	for (i=0; i<32; i++) {
		psxRegsS.GPR.r[i] = psxRegs.GPR.r[i];
		if (iRegsParam[i].state != ST_UNK) psxRegs.GPR.r[i] = iRegsParam[i].k;
	}

	PSXCPU_LOG("%s\n", disR3000AF(PSXMu32(pc), pc));
//	PSXCPU_LOG("%s (%x %x)\n", disR3000AF(PSXMu32(pc), pc), iRegsParam[_Rt_].k, psxRegs.GPR.r[_Rt_]);


	for (i=0; i<32; i++) {
		psxRegs.GPR.r[i] = psxRegsS.GPR.r[i];
	}
#endif
}

static void iFlushRegs() {
	int i;

	for (i=1; i<32; i++) {
		iFlushReg(i);
	}
}

static void iStoreCycle() {
	count = ((pc - pcold) / 4) * BIAS;
	ADD32ItoM((uptr)&psxRegs.cycle, count);
}

static void iRet() {
	iStoreCycle();
	StackRes();
	RET();
}

static int iLoadTest() {
	u32 tmp;

	// check for load delay
	tmp = psxRegs.code >> 26;
	switch (tmp) {
		case 0x10: // COP0
			switch (_Rs_) {
				case 0x00: // MFC0
				case 0x02: // CFC0
					return 1;
			}
			break;
		case 0x12: // COP2
			switch (_Funct_) {
				case 0x00:
					switch (_Rs_) {
						case 0x00: // MFC2
						case 0x02: // CFC2
							return 1;
					}
					break;
			}
			break;
		case 0x32: // LWC2
			return 1;
		default:
			if (tmp >= 0x20 && tmp <= 0x26) { // LB/LH/LWL/LW/LBU/LHU/LWR
				return 1;
			}
			break;
	}
	return 0;
}

/* set a pending branch */
static void SetBranch() {
	branch = 1;
	psxRegs.code = PSXMu32(pc);
#ifdef PSXCPU_LOG
	if (dump) iDump(iRegsS);
#endif
	pc+=4;

	if (iLoadTest() == 1) {
		iFlushRegs();
		MOV32ItoM((uptr)&psxRegs.code, psxRegs.code);
		/* store cycle */
		count = ((pc - pcold) / 4) * BIAS;
		ADD32ItoM((uptr)&psxRegs.cycle, count);

		//PUSH64M((uptr)&target);
		MOV32MtoR(X86ARG2, (uptr)&target);
		//PUSHI(_Rt_);
		MOV64ItoR(X86ARG1, _Rt_);
		CALLFunc((uptr)psxDelayTest);
		StackRes();
		RET();
		return;
	}
	switch( psxRegs.code >> 26 ) {
		// Lode Runner (jr - beq)

		// bltz - bgez - bltzal - bgezal / beq - bne - blez - bgtz
		case 0x01:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			break;

		default:
			pRecBSC[psxRegs.code>>26]();
			break;
	}

	iFlushRegs();
	iStoreCycle();
	MOV32MtoR(EAX, (uptr)&target);
	MOV32RtoM((uptr)&psxRegs.pc, EAX);
	CALLFunc((uptr)psxBranchTest);
	CALLFunc((uptr)psxJumpTest);

	StackRes();
	RET();
}

static void iJump(u32 branchPC) {
	branch = 1;
	psxRegs.code = PSXMu32(pc);
#ifdef PSXCPU_LOG
	if (dump) iDump(iRegsS);
#endif
	pc+=4;

	if (iLoadTest() == 1) {
		iFlushRegs();
		MOV32ItoM((uptr)&psxRegs.code, psxRegs.code);
		/* store cycle */
		count = ((pc - pcold) / 4) * BIAS;
		ADD32ItoM((uptr)&psxRegs.cycle, count);

		//PUSHI(branchPC);
		MOV64ItoR(X86ARG2, branchPC);
		//PUSHI(_Rt_);
		MOV64ItoR(X86ARG1, _Rt_);
		CALLFunc((uptr)psxDelayTest);
		//ADD32ItoR(ESP, 2*8);
		StackRes();
		RET();
		return;
	}

	pRecBSC[psxRegs.code>>26]();

	iFlushRegs();
	iStoreCycle();
	MOV32ItoM((uptr)&psxRegs.pc, branchPC);
	CALLFunc((uptr)psxBranchTest);
	CALLFunc((uptr)psxJumpTest);

	StackRes();

		RET();
	//* XXX?
	// maybe just happened an interruption, check so
	CMP32ItoM((uptr)&psxRegs.pc, branchPC);
	j8Ptr[0] = JE8(0);

	RET();

	x86SetJ8(j8Ptr[0]);
	MOV64MtoR(RAX, PC_REC(branchPC));
	TEST64RtoR(RAX,RAX);
	j8Ptr[1] = JNE8(0);

	RET();

	x86SetJ8(j8Ptr[1]);

	RET();
	//JMP32R(EAX);
	JMPR(EAX);
	//*/
}

static void iBranch(u32 branchPC, int savectx) {
	u32 respold=0;

	if (savectx) {
		respold = resp;
		memcpy(iRegsS, iRegs, sizeof(iRegs));
	}

	branch = 1;
	psxRegs.code = PSXMu32(pc);
#ifdef PSXCPU_LOG
	if (dump && savectx) iDump(iRegsS);
#endif
	// the delay test is only made when the branch is taken
	// savectx == 0 will mean that :)
	if (savectx == 0 && iLoadTest() == 1) {
		iFlushRegs();
		MOV32ItoM((uptr)&psxRegs.code, psxRegs.code);
		/* store cycle */
		count = (((pc+4) - pcold) / 4) * BIAS;
		ADD32ItoM((uptr)&psxRegs.cycle, count);
		//if (resp) ADD32ItoR(ESP, resp);

		//PUSHI(branchPC);
		MOV64ItoR(X86ARG2, branchPC);
		//PUSHI(_Rt_);
		MOV64ItoR(X86ARG1,_Rt_);
		CALLFunc((uptr)psxDelayTest);
		StackRes();
		RET();
		return;
	}

	pc+= 4;
	pRecBSC[psxRegs.code>>26]();

	iFlushRegs();
	iStoreCycle();
	MOV32ItoM((uptr)&psxRegs.pc, branchPC);
	CALLFunc((uptr)psxBranchTest);
	CALLFunc((uptr)psxJumpTest);

	StackRes();

	// maybe just happened an interruption, check so
	CMP32ItoM((uptr)&psxRegs.pc, branchPC);
	j8Ptr[1] = JE8(0);

	RET();

	x86SetJ8(j8Ptr[1]);
	MOV64MtoR(RAX, PC_REC(branchPC));
	TEST64RtoR(RAX, RAX);
	j8Ptr[2] = JNE8(0);

	RET();

	x86SetJ8(j8Ptr[2]);
	//JMP32R(EAX);
	JMPR(EAX);

	pc-= 4;
	if (savectx) {
		resp = respold;
		memcpy(iRegs, iRegsS, sizeof(iRegs));
	}
}


char *txt0 = "EAX = %x : ECX = %x : EDX = %x\n";
char *txt1 = "EAX = %x\n";
char *txt2 = "M32 = %x\n";

/*
void iLogX86() {
	PUSHA32();

	PUSH32R  (EDX);
	PUSH32R  (ECX);
	PUSH32R  (EAX);
	PUSH32M  ((uptr)&txt0);
	CALLFunc((uptr)SysPrintf);
	ADD32ItoR(ESP, 4*4);

	POPA32();
}
*/

void iLogEAX() {
	PUSH64R  (EAX);
	PUSH64M  ((uptr)&txt1);
	CALLFunc((uptr)SysPrintf);
	ADD32ItoR(ESP, 8*2);
}

void iLogM32(u32 mem) {
	PUSH64M  (mem);
	PUSH64M  ((uptr)&txt2);
	CALLFunc((uptr)SysPrintf);
	ADD32ItoR(ESP, 8*2);
}

#if 0
static void iDumpRegs() {
	int i, j;

	printf("%x %x\n", psxRegs.pc, psxRegs.cycle);
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 8; j++)
			printf("%x ", psxRegs.GPR.r[j*i]);
		printf("\n");
	}
}
#endif

void iDumpBlock(char *ptr) {
#ifdef PSXCPU_LOG
	u32 i;
	for (i = psxRegs.pc; i < pc; i+=4)
		PSXCPU_LOG("%s\n", disR3000AF(PSXMu32(i), i));
#endif

#ifdef PSXCPUDUMP_LOG
	FILE *f;
	f = fopen("dump1", "w");
	fwrite(ptr, 1, (uptr)x86Ptr - (uptr)ptr, f);
	fclose(f);
	//system("ndisasm -b64 dump1");
#endif
}

#define REC_FUNC(f) \
void psx##f(); \
static void rec##f() { \
	iFlushRegs(); \
	MOV32ItoM((uptr)&psxRegs.code, (u32)psxRegs.code); \
	MOV32ItoM((uptr)&psxRegs.pc, (u32)pc); \
	CALLFunc((uptr)psx##f); \
/*	branch = 2; */\
}

#define REC_SYS(f) \
void psx##f(); \
static void rec##f() { \
	iFlushRegs(); \
	MOV32ItoM((uptr)&psxRegs.code, (u32)psxRegs.code); \
	MOV32ItoM((uptr)&psxRegs.pc, (u32)pc); \
	CALLFunc((uptr)psx##f); \
	branch = 2; \
	iRet(); \
}

#define REC_BRANCH(f) \
void psx##f(); \
static void rec##f() { \
	iFlushRegs(); \
	MOV32ItoM((uptr)&psxRegs.code, (u32)psxRegs.code); \
	MOV32ItoM((uptr)&psxRegs.pc, (u32)pc); \
	CALLFunc((uptr)psx##f); \
	branch = 2; \
	iRet(); \
}

static void recRecompile();

static int recInit() {
	int i;

	psxRecLUT = (uptr*) malloc(0x010000 * sizeof(uptr));

	recMem = mmap(0,
		RECMEM_SIZE + PTRMULT*0x1000,
		PROT_EXEC | PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	recRAM = mmap(0,
		0x280000*PTRMULT,
		PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	recROM = &recRAM[0x200000*PTRMULT];

	if (recRAM == NULL || recROM == NULL || recMem == NULL || psxRecLUT == NULL) {
		SysMessage("Error allocating memory"); return -1;
	}
	memset(recMem, 0, RECMEM_SIZE);
	memset(recRAM, 0, 0x200000 * PTRMULT);
	memset(recROM, 0, 0x080000 * PTRMULT);

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (uptr)&recRAM[PTRMULT*((i & 0x1f) << 16)];
	memcpy(psxRecLUT + 0x8000, psxRecLUT, 0x80 * sizeof(uptr));
	memcpy(psxRecLUT + 0xa000, psxRecLUT, 0x80 * sizeof(uptr));

	for (i=0; i<0x08; i++) psxRecLUT[i + 0xbfc0] = (uptr)&recROM[PTRMULT*(i << 16)];

    //x86Init();
    cpudetectInit();

	return 0;
}

static void recReset() {
	memset(recRAM, 0, 0x200000 * PTRMULT);
	memset(recROM, 0, 0x080000 * PTRMULT);

	x86SetPtr(recMem);

	branch = 0;
	memset(iRegs, 0, sizeof(iRegs));
	iRegs[0].state = ST_CONST;
	iRegs[0].k     = 0;
}

static void recShutdown() {
	if (recMem == NULL) return;
	free(psxRecLUT);
	munmap(recMem, RECMEM_SIZE + PTRMULT*0x1000);
	munmap(recRAM, 0x280000*PTRMULT);
	x86Shutdown();
}

static void recError() {
	SysReset();
	ClosePlugins();
	SysMessage("Unrecoverable error while running recompiler\n");
	SysRunGui();
}

/*__inline*/ static void execute() {
	void (*recFunc)();
	uptr *p;

	p = (uptr *)PC_REC(psxRegs.pc);
	// if (!p) { recError(); return; }

	if (*p == 0) {
		recRecompile();
	}
	else if (psxRegs.ICache_valid == FALSE) { // Xenogears: fixes memory card access with original BIOS (0a_44_FlushCache issue)
		//psxCpu->Clear(0x0, 0x20000);
		memset(recRAM, 0, 0x10000 * PTRMULT);
		recRecompile();
		p = (uptr *)PC_REC(psxRegs.pc);
		psxRegs.ICache_valid = TRUE;
	}

	if (*p < (uptr)recMem || *p >= (uptr)recMem + RECMEM_SIZE)
	{
		recError();
		return;
	}
	recFunc = (void (*)())*p;
	(*recFunc)();
}

static void recExecute() {
	for (;;) execute();
}

static void recExecuteBlock() {
	execute();
}

static void recClear(u32 Addr, u32 Size) {
	memset((void *)PC_REC(Addr), 0, Size * sizeof(uptr));
}

static void recNULL() {
//	SysMessage("recUNK: %8.8x\n", psxRegs.code);
}

/*********************************************************
* goes to opcodes tables...                              *
* Format:  table[something....]                          *
*********************************************************/

//REC_SYS(SPECIAL);
#if 1
static void recSPECIAL() {
	pRecSPC[_Funct_]();
}
#endif

static void recREGIMM() {
	pRecREG[_Rt_]();
}

static void recCOP0() {
	pRecCP0[_Rs_]();
}

//REC_SYS(COP2);
#if 1
static void recCOP2() {
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.n.Status);
	AND32ItoR(EAX, 0x40000000);
	j32Ptr[31] = JZ32(0);

	pRecCP2[_Funct_]();

	x86SetJ32(j32Ptr[31]);
}
#endif

static void recBASIC() {
	pRecCP2BSC[_Rs_]();
}

//end of Tables opcodes...

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/

#if 0
REC_FUNC(ADDI);
REC_FUNC(ADDIU);
REC_FUNC(ANDI);
REC_FUNC(ORI);
REC_FUNC(XORI);
REC_FUNC(SLTI);
REC_FUNC(SLTIU);
#endif

#if 1
static void recADDIU()  {
// Rt = Rs + Im
	if (!_Rt_) return;

//	iFlushRegs();

	if (_Rs_ == _Rt_) {
		if (IsConst(_Rt_)) {
			iRegs[_Rt_].k+= _Imm_;
		} else {
			if (_Imm_ == 1) {
				INC32M((uptr)&psxRegs.GPR.r[_Rt_]);
			} else if (_Imm_ == -1) {
				DEC32M((uptr)&psxRegs.GPR.r[_Rt_]);
			} else if (_Imm_) {
				ADD32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _Imm_);
			}
		}
	} else {
		if (IsConst(_Rs_)) {
			MapConst(_Rt_, iRegs[_Rs_].k + _Imm_);
		} else {
			iRegs[_Rt_].state = ST_UNK;

			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			if (_Imm_ == 1) {
				INC32R(EAX);
			} else if (_Imm_ == -1) {
				DEC32R(EAX);
			} else if (_Imm_) {
				ADD32ItoR(EAX, _Imm_);
			}
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	}
}

static void recADDI()  {
// Rt = Rs + Im
	recADDIU();
}

static void recSLTI() {
// Rt = Rs < Im (signed)
	if (!_Rt_) return;

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		MapConst(_Rt_, (s32)iRegs[_Rs_].k < _Imm_);
	} else {
		iRegs[_Rt_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	    CMP32ItoR(EAX, _Imm_);
	    SETL8R   (EAX);
	    AND32ItoR(EAX, 0xff);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
}

static void recSLTIU() {
// Rt = Rs < Im (unsigned)
	if (!_Rt_) return;

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		MapConst(_Rt_, iRegs[_Rs_].k < _ImmU_);
	} else {
		iRegs[_Rt_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	    CMP32ItoR(EAX, _Imm_);
	    SETB8R   (EAX);
	    AND32ItoR(EAX, 0xff);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
}

static void recANDI() {
// Rt = Rs And Im
	if (!_Rt_) return;

//	iFlushRegs();

	if (_Rs_ == _Rt_) {
		if (IsConst(_Rt_)) {
			iRegs[_Rt_].k&= _ImmU_;
		} else {
			AND32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
		}
	} else {
		if (IsConst(_Rs_)) {
			MapConst(_Rt_, iRegs[_Rs_].k & _ImmU_);
		} else {
			iRegs[_Rt_].state = ST_UNK;

			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			AND32ItoR(EAX, _ImmU_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	}
}

static void recORI() {
// Rt = Rs Or Im
	if (!_Rt_) return;

//	iFlushRegs();

	if (_Rs_ == _Rt_) {
		if (IsConst(_Rt_)) {
			iRegs[_Rt_].k|= _ImmU_;
		} else {
			OR32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
		}
	} else {
		if (IsConst(_Rs_)) {
			MapConst(_Rt_, iRegs[_Rs_].k | _ImmU_);
		} else {
			iRegs[_Rt_].state = ST_UNK;

			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			if (_ImmU_) OR32ItoR (EAX, _ImmU_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	}
}

static void recXORI() {
// Rt = Rs Xor Im
	if (!_Rt_) return;

//	iFlushRegs();

	if (_Rs_ == _Rt_) {
		if (IsConst(_Rt_)) {
			iRegs[_Rt_].k^= _ImmU_;
		} else {
			XOR32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
		}
	} else {
		if (IsConst(_Rs_)) {
			MapConst(_Rt_, iRegs[_Rs_].k ^ _ImmU_);
		} else {
			iRegs[_Rt_].state = ST_UNK;

			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			XOR32ItoR(EAX, _ImmU_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	}
}
#endif
//end of * Arithmetic with immediate operand

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
//REC_FUNC(LUI);
#if 1
static void recLUI()  {
// Rt = Imm << 16
	if (!_Rt_) return;
	//iFlushReg(_Rt_);
	MapConst(_Rt_, _ImmLU_);
}
#endif
//End of Load Higher .....


/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/


#if 0
REC_FUNC(ADD);
REC_FUNC(ADDU);
REC_FUNC(SUB);
REC_FUNC(SUBU);
REC_FUNC(AND);
REC_FUNC(OR);
REC_FUNC(XOR);
REC_FUNC(NOR);
REC_FUNC(SLT);
REC_FUNC(SLTU);
#endif

#if 1
static void recADDU() {
// Rd = Rs + Rt
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k + iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		if (_Rt_ == _Rd_) {
			if (iRegs[_Rs_].k == 1) {
				INC32M((uptr)&psxRegs.GPR.r[_Rd_]);
			} else if (iRegs[_Rs_].k == -1) {
				DEC32M((uptr)&psxRegs.GPR.r[_Rd_]);
			} else if (iRegs[_Rs_].k) {
				ADD32ItoM((uptr)&psxRegs.GPR.r[_Rd_], iRegs[_Rs_].k);
			}
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
			if (iRegs[_Rs_].k == 1) {
				INC32R(EAX);
			} else if (iRegs[_Rs_].k == 0xffffffff) {
				DEC32R(EAX);
			} else if (iRegs[_Rs_].k) {
				ADD32ItoR(EAX, iRegs[_Rs_].k);
			}
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		}
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		if (_Rs_ == _Rd_) {
			if (iRegs[_Rt_].k == 1) {
				INC32M((uptr)&psxRegs.GPR.r[_Rd_]);
			} else if (iRegs[_Rt_].k == -1) {
				DEC32M((uptr)&psxRegs.GPR.r[_Rd_]);
			} else if (iRegs[_Rt_].k) {
				ADD32ItoM((uptr)&psxRegs.GPR.r[_Rd_], iRegs[_Rt_].k);
			}
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			if (iRegs[_Rt_].k == 1) {
				INC32R(EAX);
			} else if (iRegs[_Rt_].k == 0xffffffff) {
				DEC32R(EAX);
			} else if (iRegs[_Rt_].k) {
				ADD32ItoR(EAX, iRegs[_Rt_].k);
			}
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		}
	} else {
		iRegs[_Rd_].state = ST_UNK;

		if (_Rs_ == _Rd_) { // Rd+= Rt
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
			ADD32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		} else if (_Rt_ == _Rd_) { // Rd+= Rs
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			ADD32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		} else { // Rd = Rs + Rt
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			ADD32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		}
	}
}

static void recADD() {
// Rd = Rs + Rt
	recADDU();
}

static void recSUBU() {
// Rd = Rs - Rt
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k - iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rs_].k);
		SUB32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SUB32ItoR(EAX, iRegs[_Rt_].k);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SUB32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSUB() {
// Rd = Rs - Rt
	recSUBU();
}

static void recAND() {
// Rd = Rs And Rt
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k & iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		if (_Rd_ == _Rt_) { // Rd&= Rs
			AND32ItoM((uptr)&psxRegs.GPR.r[_Rd_], iRegs[_Rs_].k);
		} else {
			MOV32ItoR(EAX, iRegs[_Rs_].k);
			AND32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		}
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		if (_Rd_ == _Rs_) { // Rd&= kRt
			AND32ItoM((uptr)&psxRegs.GPR.r[_Rd_], iRegs[_Rt_].k);
		} else { // Rd = Rs & kRt
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			AND32ItoR(EAX, iRegs[_Rt_].k);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		}
	} else {
		iRegs[_Rd_].state = ST_UNK;

		if (_Rs_ == _Rd_) { // Rd&= Rt
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
			AND32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		} else if (_Rt_ == _Rd_) { // Rd&= Rs
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			AND32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		} else { // Rd = Rs & Rt
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			AND32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
		}
	}
}

static void recOR() {
// Rd = Rs Or Rt
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k | iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rs_].k);
		OR32MtoR (EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		OR32ItoR (EAX, iRegs[_Rt_].k);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		OR32MtoR (EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recXOR() {
// Rd = Rs Xor Rt
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k ^ iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rs_].k);
		XOR32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		XOR32ItoR(EAX, iRegs[_Rt_].k);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		XOR32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recNOR() {
// Rd = Rs Nor Rt
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, ~(iRegs[_Rs_].k | iRegs[_Rt_].k));
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rs_].k);
		OR32MtoR (EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		NOT32R   (EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		OR32ItoR (EAX, iRegs[_Rt_].k);
		NOT32R   (EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		OR32MtoR (EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		NOT32R   (EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSLT() {
// Rd = Rs < Rt (signed)
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, (s32)iRegs[_Rs_].k < (s32)iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rs_].k);
		CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		SETL8R   (EAX);
		AND32ItoR(EAX, 0xff);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		CMP32ItoR(EAX, iRegs[_Rt_].k);
		SETL8R   (EAX);
		AND32ItoR(EAX, 0xff);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		SETL8R   (EAX);
		AND32ItoR(EAX, 0xff);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSLTU() {
// Rd = Rs < Rt (unsigned)
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rs_].k < iRegs[_Rt_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rs_].k);
	    CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		SBB32RtoR(EAX, EAX);
		NEG32R   (EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		CMP32ItoR(EAX, iRegs[_Rt_].k);
		SBB32RtoR(EAX, EAX);
		NEG32R   (EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	    CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		SBB32RtoR(EAX, EAX);
		NEG32R   (EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}
#endif
//End of * Register arithmetic

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/

#if 0
REC_FUNC(MULT);
REC_FUNC(MULTU);
REC_FUNC(DIV);
REC_FUNC(DIVU);
#endif

#if 1
static void recMULT() {
// Lo/Hi = Rs * Rt (signed)

//	iFlushRegs();

	if ((IsConst(_Rs_) && iRegs[_Rs_].k == 0) ||
		(IsConst(_Rt_) && iRegs[_Rt_].k == 0)) {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
		return;
	}

	if (IsConst(_Rs_)) {
		MOV32ItoR(EAX, iRegs[_Rs_].k);// printf("multrsk %x\n", iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	}
	if (IsConst(_Rt_)) {
		MOV32ItoR(EDX, iRegs[_Rt_].k);// printf("multrtk %x\n", iRegs[_Rt_].k);
		IMUL32R  (EDX);
	} else {
		IMUL32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
}

static void recMULTU() {
// Lo/Hi = Rs * Rt (unsigned)

//	iFlushRegs();

	if ((IsConst(_Rs_) && iRegs[_Rs_].k == 0) ||
		(IsConst(_Rt_) && iRegs[_Rt_].k == 0)) {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
		return;
	}

	if (IsConst(_Rs_)) {
		MOV32ItoR(EAX, iRegs[_Rs_].k);// printf("multursk %x\n", iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	}
	if (IsConst(_Rt_)) {
		MOV32ItoR(EDX, iRegs[_Rt_].k);// printf("multurtk %x\n", iRegs[_Rt_].k);
		MUL32R   (EDX);
	} else {
		MUL32M   ((uptr)&psxRegs.GPR.r[_Rt_]);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
}

static void recDIV() {
// Lo/Hi = Rs / Rt (signed)

//	iFlushRegs();

	if (IsConst(_Rt_)) {
		if (iRegs[_Rt_].k == 0) {
			MOV32ItoM((uptr)&psxRegs.GPR.n.lo, 0xffffffff);
			if (IsConst(_Rs_)) {
				MOV32ItoM((uptr)&psxRegs.GPR.n.hi, iRegs[_Rs_].k);
			} else {
				MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
				MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
			}
			return;
		}
		MOV32ItoR(ECX, iRegs[_Rt_].k);// printf("divrtk %x\n", iRegs[_Rt_].k);
	} else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
		CMP32ItoR(ECX, 0);
		j8Ptr[0] = JE8(0);
	}
	if (IsConst(_Rs_)) {
		MOV32ItoR(EAX, iRegs[_Rs_].k);// printf("divrsk %x\n", iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	}
	CDQ();
	IDIV32R  (ECX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	if (!IsConst(_Rt_)) {
		j8Ptr[1] = JMP8(1);

		x86SetJ8(j8Ptr[0]);

		MOV32ItoM((uptr)&psxRegs.GPR.n.lo, 0xffffffff);
		if (IsConst(_Rs_)) {
			MOV32ItoM((uptr)&psxRegs.GPR.n.hi, iRegs[_Rs_].k);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
		}

		x86SetJ8(j8Ptr[1]);
	}
}

static void recDIVU() {
// Lo/Hi = Rs / Rt (unsigned)

//	iFlushRegs();

	if (IsConst(_Rt_)) {
		if (iRegs[_Rt_].k == 0) {
			MOV32ItoM((uptr)&psxRegs.GPR.n.lo, 0xffffffff);
			if (IsConst(_Rs_)) {
				MOV32ItoM((uptr)&psxRegs.GPR.n.hi, iRegs[_Rs_].k);
			} else {
				MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
				MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
			}
			return;
		}
		MOV32ItoR(ECX, iRegs[_Rt_].k);// printf("divurtk %x\n", iRegs[_Rt_].k);
	} else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
		CMP32ItoR(ECX, 0);
		j8Ptr[0] = JE8(0);
	}
	if (IsConst(_Rs_)) {
		MOV32ItoR(EAX, iRegs[_Rs_].k);// printf("divursk %x\n", iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	}
	XOR32RtoR(EDX, EDX);
	DIV32R   (ECX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	if (!IsConst(_Rt_)) {
		j8Ptr[1] = JMP8(1);

		x86SetJ8(j8Ptr[0]);

		MOV32ItoM((uptr)&psxRegs.GPR.n.lo, 0xffffffff);
		if (IsConst(_Rs_)) {
			MOV32ItoM((uptr)&psxRegs.GPR.n.hi, iRegs[_Rs_].k);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
		}

		x86SetJ8(j8Ptr[1]);
	}
}
#endif
//End of * Register mult/div & Register trap logic

#if 0
REC_FUNC(LB);
REC_FUNC(LBU);
REC_FUNC(LH);
REC_FUNC(LHU);
REC_FUNC(LW);

REC_FUNC(SB);
REC_FUNC(SH);
REC_FUNC(SW);

REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);
#endif


static void SetArg_OfB(x86IntRegType arg) {
	if (IsConst(_Rs_))
#ifdef __x86_64__
		MOV64ItoR(arg, iRegs[_Rs_].k + _Imm_);
#else
		PUSH32I  (iRegs[_Rs_].k + _Imm_);
#endif
	else {
#ifdef __x86_64__
		MOV32MtoR(arg, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_)
			ADD32ItoR(arg, _Imm_);
#else
		if (_Imm_) {
			MOV32MtoR(EAX, (u32)&psxRegs.GPR.r[_Rs_]);
			ADD32ItoR(EAX, _Imm_);
			PUSH32R  (EAX);
		} else {
			PUSH32M  ((u32)&psxRegs.GPR.r[_Rs_]);
		}
#endif
	}
#ifndef __x86_64__
	resp += 4;
#endif
}

#if 1

static void recLB() {
// Rt = mem[Rs + Im] (signed)

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0)  == 0xbfc0) {
			if (!_Rt_) return;
			// since bios is readonly it won't change
			MapConst(_Rt_, psxRs8(addr));
			return;
		}
		if ((t & 0x1fe0) == 0) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVSX32M8toR(EAX, (uptr)&psxM[addr & 0x1fffff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVSX32M8toR(EAX, (uptr)&psxH[addr & 0xfff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
//		SysPrintf("unhandled r8 %x\n", addr);
	}

	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemRead8);
	if (_Rt_) {
		iRegs[_Rt_].state = ST_UNK;
		MOVSX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
//	ADD32ItoR(ESP, 4);
}

static void recLBU() {
// Rt = mem[Rs + Im] (unsigned)

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0)  == 0xbfc0) {
			if (!_Rt_) return;
			// since bios is readonly it won't change
			MapConst(_Rt_, psxRu8(addr));
			return;
		}
		if ((t & 0x1fe0) == 0) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVZX32M8toR(EAX, (uptr)&psxM[addr & 0x1fffff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVZX32M8toR(EAX, (uptr)&psxH[addr & 0xfff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
//		SysPrintf("unhandled r8u %x\n", addr);
	}

	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemRead8);
	if (_Rt_) {
		iRegs[_Rt_].state = ST_UNK;
		MOVZX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
//	ADD32ItoR(ESP, 4);
}

static void recLH() {
// Rt = mem[Rs + Im] (signed)

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0)  == 0xbfc0) {
			if (!_Rt_) return;
			// since bios is readonly it won't change
			MapConst(_Rt_, psxRs16(addr));
			return;
		}
		if ((t & 0x1fe0) == 0) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVSX32M16toR(EAX, (uptr)&psxM[addr & 0x1fffff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVSX32M16toR(EAX, (uptr)&psxH[addr & 0xfff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
//		SysPrintf("unhandled r16 %x\n", addr);
	}

	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemRead16);
	if (_Rt_) {
		iRegs[_Rt_].state = ST_UNK;
		MOVSX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
//	ADD32ItoR(ESP, 4);
}

static void recLHU() {
// Rt = mem[Rs + Im] (unsigned)

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (!_Rt_) return;
			// since bios is readonly it won't change
			MapConst(_Rt_, psxRu16(addr));
			return;
		}
		if ((t & 0x1fe0) == 0) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVZX32M16toR(EAX, (uptr)&psxM[addr & 0x1fffff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOVZX32M16toR(EAX, (uptr)&psxH[addr & 0xfff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80) {
			if (addr >= 0x1f801c00 && addr < 0x1f801e00) {
				if (!_Rt_) return;
				iRegs[_Rt_].state = ST_UNK;

				//PUSHI  (addr);
				MOV64ItoR(X86ARG1, addr);
				//CALLFunc  ((uptr)SPU_readRegister);
				MOV64ItoR(RAX, (uptr)SPU_readRegister);
				CALL64R(RAX);
				MOVZX32R16toR(EAX, EAX);
				MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
#ifndef __WIN32__
				resp+= 4;
#endif
				return;
			}
			switch (addr) {
				case 0x1f801100: case 0x1f801110: case 0x1f801120:
					if (!_Rt_) return;
					iRegs[_Rt_].state = ST_UNK;

					//PUSHI((addr >> 4) & 0x3);
					MOV64ItoR(X86ARG1, (addr >> 4) & 0x3);
					CALLFunc((uptr)psxRcntRcount);
					MOVZX32R16toR(EAX, EAX);
					MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
					resp+= 4;
					return;

				case 0x1f801104: case 0x1f801114: case 0x1f801124:
					if (!_Rt_) return;
					iRegs[_Rt_].state = ST_UNK;

					MOV64ItoR(X86ARG1, (addr >> 4) & 0x3);
                    CALLFunc((uptr)psxRcntRmode);
                    MOVZX32R16toR(EAX, EAX);
					MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
                    resp+= 4;
					return;

				case 0x1f801108: case 0x1f801118: case 0x1f801128:
					if (!_Rt_) return;
					iRegs[_Rt_].state = ST_UNK;

					MOV64ItoR(X86ARG1, (addr >> 4) & 0x3);
                    CALLFunc((uptr)psxRcntRtarget);
                    MOVZX32R16toR(EAX, EAX);
					MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
                    resp+= 4;
					return;
			}
		}
//		SysPrintf("unhandled r16u %x\n", addr);
	}

	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemRead16);
	if (_Rt_) {
		iRegs[_Rt_].state = ST_UNK;
		MOVZX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
//	ADD32ItoR(ESP, 4);
}


static void recLW() {
// Rt = mem[Rs + Im] (unsigned)

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (!_Rt_) return;
			// since bios is readonly it won't change
			MapConst(_Rt_, psxRu32(addr));
			return;
		}
		if ((t & 0x1fe0) == 0) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOV32MtoR(EAX, (uptr)&psxM[addr & 0x1fffff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (!_Rt_) return;
			iRegs[_Rt_].state = ST_UNK;

			MOV32MtoR(EAX, (uptr)&psxH[addr & 0xfff]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084: case 0x1f801088:
				case 0x1f801090: case 0x1f801094: case 0x1f801098:
				case 0x1f8010a0: case 0x1f8010a4: case 0x1f8010a8:
				case 0x1f8010b0: case 0x1f8010b4: case 0x1f8010b8:
				case 0x1f8010c0: case 0x1f8010c4: case 0x1f8010c8:
				case 0x1f8010d0: case 0x1f8010d4: case 0x1f8010d8:
				case 0x1f8010e0: case 0x1f8010e4: case 0x1f8010e8:
				case 0x1f801070: case 0x1f801074:
				case 0x1f8010f0: case 0x1f8010f4:
					if (!_Rt_) return;
					iRegs[_Rt_].state = ST_UNK;
					MOV32MtoR(EAX, (uptr)&psxH[addr & 0xffff]);
					MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
					
					return;

				case 0x1f801810:
					if (!_Rt_) return;
					iRegs[_Rt_].state = ST_UNK;

					CALLFunc((uptr)GPU_readData);
					MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
					return;

				case 0x1f801814:
					if (!_Rt_) return;
					iRegs[_Rt_].state = ST_UNK;

					CALLFunc((uptr)GPU_readStatus);
					MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
					return;
			}
		}
//		SysPrintf("unhandled r32 %x\n", addr);
	}

	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemRead32);
	if (_Rt_) {
		iRegs[_Rt_].state = ST_UNK;
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
//	ADD32ItoR(ESP, 4);

	// Delay is memread delay + 1 cycle.
	// Seems to work nicely with games that require cycle accuracy like CART World Series.
	// TODO: this could be needed with other L ops such as LB.
	if (Config.HackFix)INC32M((uptr)&psxRegs.cycle);
}

extern u32 LWL_MASK[4];
extern u32 LWL_SHIFT[4];

void iLWLk(u32 shift) {
	if (IsConst(_Rt_)) {
		MOV32ItoR(ECX, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	AND32ItoR(ECX, LWL_MASK[shift]);
	SHL32ItoR(EAX, LWL_SHIFT[shift]);
	OR32RtoR (EAX, ECX);
}

void recLWL() {
// Rt = Rt Merge mem[Rs + Im]

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0) {
			MOV32MtoR(EAX, (uptr)&psxM[addr & 0x1ffffc]);
			iLWLk(addr & 3);

			iRegs[_Rt_].state = ST_UNK;
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(EAX, (uptr)&psxH[addr & 0xffc]);
			iLWLk(addr & 3);

			iRegs[_Rt_].state = ST_UNK;
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
	}

	if (IsConst(_Rs_)) MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_) ADD32ItoR(EAX, _Imm_);
	}
	//PUSH64R  (EAX);
	AND32ItoR(EAX, ~3);
	//PUSH64R  (EAX);
	MOV32RtoR(X86ARG1, EAX);
	CALLFunc((uptr)psxMemRead32);

	if (_Rt_) {
		//ADD32ItoR(ESP, 4);
		//POP64R   (EDX);
		if (IsConst(_Rs_)) MOV32ItoR(EDX, iRegs[_Rs_].k + _Imm_);
		else {
			MOV32MtoR(EDX, (uptr)&psxRegs.GPR.r[_Rs_]);
			if (_Imm_) ADD32ItoR(EDX, _Imm_);
		}

		AND32ItoR(EDX, 0x3); // shift = addr & 3;

		MOV64ItoR(ECX, (uptr)LWL_SHIFT);
		MOV32RmStoR(ECX, ECX, EDX, 2);
		SHL32CLtoR(EAX); // mem(EAX) << LWL_SHIFT[shift]

		MOV64ItoR(ECX, (uptr)LWL_MASK);
		MOV32RmStoR(ECX, ECX, EDX, 2);
		if (IsConst(_Rt_)) {
			MOV32ItoR(EDX, iRegs[_Rt_].k);
		} else {
			MOV32MtoR(EDX, (uptr)&psxRegs.GPR.r[_Rt_]);
		}
		AND32RtoR(EDX, ECX); // _rRt_ & LWL_MASK[shift]

		OR32RtoR(EAX, EDX);

		iRegs[_Rt_].state = ST_UNK;
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	//} else {
		//ADD64ItoR(RSP, 8);
		//resp+= 8;
	}
}

/*
static void recLWBlock(int count) {
	u32 *code = PSXM(pc);
	int i, respsave;
// Rt = mem[Rs + Im] (unsigned)

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			// since bios is readonly it won't change
			for (i=0; i<count; i++, code++, addr+=4) {
				if (_fRt_(*code)) {
					MapConst(_fRt_(*code), psxRu32(addr));
				}
			}
			return;
		}
		if ((t & 0x1fe0) == 0) {
			for (i=0; i<count; i++, code++, addr+=4) {
				if (!_fRt_(*code)) return;
				iRegs[_fRt_(*code)].state = ST_UNK;

				MOV32MtoR(EAX, (uptr)&psxM[addr & 0x1fffff]);
				MOV32RtoM((uptr)&psxRegs.GPR.r[_fRt_(*code)], EAX);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			for (i=0; i<count; i++, code++, addr+=4) {
				if (!_fRt_(*code)) return;
				iRegs[_fRt_(*code)].state = ST_UNK;

				MOV32MtoR(EAX, (uptr)&psxH[addr & 0xfff]);
				MOV32RtoM((uptr)&psxRegs.GPR.r[_fRt_(*code)], EAX);
			}
			return;
		}
	}

	SysPrintf("recLWBlock %d: %d\n", count, IsConst(_Rs_));
	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemPointer);
//	ADD32ItoR(ESP, 4);

	respsave = resp; resp = 0;
	TEST64RtoR(RAX,RAX);
	j32Ptr[4] = JZ32(0);
	XOR32RtoR(ECX, ECX);
	for (i=0; i<count; i++, code++) {
		if (_fRt_(*code)) {
			iRegs[_fRt_(*code)].state = ST_UNK;

			MOV64RmStoR(EDX, EAX, ECX, 2);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_fRt_(*code)], EDX);
		}
		if (i != (count-1)) INC32R(ECX);
	}
	j32Ptr[5] = JMP32(0);
	x86SetJ32(j32Ptr[4]);
	for (i=0, code = PSXM(pc); i<count; i++, code++) {
		psxRegs.code = *code;
		recLW();
	}
#ifndef __x86_64__
	ADD32ItoR(ESP, resp);
#endif
	x86SetJ32(j32Ptr[5]);
	resp = respsave;
}
*/

extern u32 LWR_MASK[4];
extern u32 LWR_SHIFT[4];

void iLWRk(u32 shift) {
	if (IsConst(_Rt_)) {
		MOV32ItoR(ECX, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	AND32ItoR(ECX, LWR_MASK[shift]);
	SHR32ItoR(EAX, LWR_SHIFT[shift]);
	OR32RtoR (EAX, ECX);
}

void recLWR() {
// Rt = Rt Merge mem[Rs + Im]

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0) {
			MOV32MtoR(EAX, (uptr)&psxM[addr & 0x1ffffc]);
			iLWRk(addr & 3);

			iRegs[_Rt_].state = ST_UNK;
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(EAX, (uptr)&psxH[addr & 0xffc]);
			iLWRk(addr & 3);

			iRegs[_Rt_].state = ST_UNK;
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
			return;
		}
	}

	if (IsConst(_Rs_)) MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_) ADD32ItoR(EAX, _Imm_);
	}
	PUSHR(EAX);
	AND32ItoR(EAX, ~3);
	MOV32RtoR(X86ARG1, EAX);
	CALLFunc((uptr)psxMemRead32);

	POPR   (EDX);
	if (_Rt_) {
		AND32ItoR(EDX, 0x3); // shift = addr & 3;

		MOV64ItoR(ECX, (uptr)LWR_SHIFT);
		MOV32RmStoR(ECX, ECX, EDX, 2);
		SHR32CLtoR(EAX); // mem(EAX) >> LWR_SHIFT[shift]

		MOV64ItoR(ECX, (uptr)LWR_MASK);
		MOV32RmStoR(ECX, ECX, EDX, 2);

		if (IsConst(_Rt_)) {
			MOV32ItoR(EDX, iRegs[_Rt_].k);
		} else {
			MOV32MtoR(EDX, (uptr)&psxRegs.GPR.r[_Rt_]);
		}
		AND32RtoR(EDX, ECX); // _rRt_ & LWR_MASK[shift]

		OR32RtoR(EAX, EDX);

		iRegs[_Rt_].state = ST_UNK;
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	//} else {
		//resp+= 8;
	}
}

static void recSB() {
// mem[Rs + Im] = Rt

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (IsConst(_Rt_)) {
				MOV8ItoM((uptr)&psxM[addr & 0x1fffff], (u8)iRegs[_Rt_].k);
			} else {
				MOV8MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
				MOV8RtoM((uptr)&psxM[addr & 0x1fffff], EAX);
			}
			MOV32ItoR(X86ARG2, 1);
			MOV32ItoR(X86ARG1, addr & ~3);
			CALLFunc((uptr)&recClear);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (IsConst(_Rt_)) {
				MOV8ItoM((uptr)&psxH[addr & 0xfff], (u8)iRegs[_Rt_].k);
			} else {
				MOV8MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
				MOV8RtoM((uptr)&psxH[addr & 0xfff], EAX);
			}
			return;
		}
//		SysPrintf("unhandled w8 %x\n", addr);
	}

	if (IsConst(_Rt_)) {
		MOV64ItoR(X86ARG2, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(X86ARG2, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemWrite8);

//	ADD32ItoR(ESP, 8);
}

static void recSH() {
// mem[Rs + Im] = Rt

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (IsConst(_Rt_)) {
				MOV16ItoM((uptr)&psxM[addr & 0x1fffff], (u16)iRegs[_Rt_].k);
			} else {
				MOV16MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
				MOV16RtoM((uptr)&psxM[addr & 0x1fffff], EAX);
			}
			MOV32ItoR(X86ARG2, 1);
			MOV32ItoR(X86ARG1, addr & ~3);
			CALLFunc((uptr)&recClear);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (IsConst(_Rt_)) {
				MOV16ItoM((uptr)&psxH[addr & 0xfff], (u16)iRegs[_Rt_].k);
			} else {
				MOV16MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
				MOV16RtoM((uptr)&psxH[addr & 0xfff], EAX);
			}
			return;
		}
		if (t == 0x1f80) {
			if (addr >= 0x1f801c00 && addr < 0x1f801e00) {
				if (IsConst(_Rt_)) {
					MOV64ItoR(X86ARG2, iRegs[_Rt_].k);
				} else {
					MOV32MtoR(X86ARG2, (uptr)&psxRegs.GPR.r[_Rt_]);
				}
				MOV64ItoR(X86ARG1, addr);
				CALLFunc  ((uptr)SPU_writeRegister);
#ifndef __WIN32__
				//resp+= 8;
#endif
				return;
			}
		}
//		SysPrintf("unhandled w16 %x\n", addr);
	}

	if (IsConst(_Rt_)) {
		MOV64ItoR(X86ARG2, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(X86ARG2, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemWrite16);
//	ADD32ItoR(ESP, 8);
}

static void recSW() {
// mem[Rs + Im] = Rt

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (IsConst(_Rt_)) {
				MOV32ItoM((uptr)&psxM[addr & 0x1fffff], iRegs[_Rt_].k);
			} else {
				MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
				MOV32RtoM((uptr)&psxM[addr & 0x1fffff], EAX);
			}
			MOV32ItoR(X86ARG2, 1);
			MOV32ItoR(X86ARG1, addr);
			CALLFunc((uptr)&recClear);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (IsConst(_Rt_)) {
				MOV32ItoM((uptr)&psxH[addr & 0xfff], iRegs[_Rt_].k);
			} else {
				MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
				MOV32RtoM((uptr)&psxH[addr & 0xfff], EAX);
			}
			return;
		}
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084:
				case 0x1f801090: case 0x1f801094:
				case 0x1f8010a0: case 0x1f8010a4:
				case 0x1f8010b0: case 0x1f8010b4:
				case 0x1f8010c0: case 0x1f8010c4:
				case 0x1f8010d0: case 0x1f8010d4:
				case 0x1f8010e0: case 0x1f8010e4:
				case 0x1f801074:
				case 0x1f8010f0:
					if (IsConst(_Rt_)) {
						MOV32ItoM((uptr)&psxH[addr & 0xffff], iRegs[_Rt_].k);
					} else {
						MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
						MOV32RtoM((uptr)&psxH[addr & 0xffff], EAX);
					}
					return;

				case 0x1f801810:
					if (IsConst(_Rt_)) {
						MOV64ItoR(X86ARG1, iRegs[_Rt_].k);
					} else {
						MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rt_]);
					}
					CALLFunc((uptr)GPU_writeData);
#ifndef __WIN32__
					//resp+= 4;
#endif
					return;

				case 0x1f801814:
					if (IsConst(_Rt_)) {
						MOV64ItoR(X86ARG1, iRegs[_Rt_].k);
					} else {
						MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rt_]);
					}
					CALLFunc((uptr)GPU_writeStatus);
#ifndef __WIN32__
					//resp+= 4;
#endif
			}
		}
//		SysPrintf("unhandled w32 %x\n", addr);
	}

	if (IsConst(_Rt_)) {
		MOV64ItoR(X86ARG2, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(X86ARG2, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemWrite32);
//	ADD32ItoR(ESP, 8);
	//resp+= 8;
}

/*
static void recSWBlock(int count) {
	u32 *code;
	int i, respsave;
// mem[Rs + Im] = Rt

//	iFlushRegs();

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;
		code = PSXM(pc);

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			for (i=0; i<count; i++, code++, addr+=4) {
				if (IsConst(_fRt_(*code))) {
					MOV32ItoM((uptr)&psxM[addr & 0x1fffff], iRegs[_fRt_(*code)].k);
				} else {
					MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_fRt_(*code)]);
					MOV32RtoM((uptr)&psxM[addr & 0x1fffff], EAX);
				}
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			for (i=0; i<count; i++, code++, addr+=4) {
				if (!_fRt_(*code)) return;
				iRegs[_fRt_(*code)].state = ST_UNK;

				MOV32MtoR(EAX, (uptr)&psxH[addr & 0xfff]);
				MOV32RtoM((uptr)&psxRegs.GPR.r[_fRt_(*code)], EAX);
			}
			return;
		}
	}

	SysPrintf("recSWBlock %d: %d\n", count, IsConst(_Rs_));
	SetArg_OfB(X86ARG1);
	CALLFunc((uptr)psxMemPointer);
//	ADD32ItoR(ESP, 4);
	//resp+= 4;

	respsave = resp; resp = 0;
	TEST64RtoR(RAX,RAX);
	j32Ptr[4] = JZ32(0);
	XOR32RtoR(ECX, ECX);
	for (i=0, code = PSXM(pc); i<count; i++, code++) {
		if (IsConst(_fRt_(*code))) {
			MOV32ItoR(EDX, iRegs[_fRt_(*code)].k);
		} else {
			MOV32MtoR(EDX, (uptr)&psxRegs.GPR.r[_fRt_(*code)]);
		}
		MOV32RtoRmS(EAX, ECX, 2, EDX);
		if (i != (count-1)) INC32R(ECX);
	}
	j32Ptr[5] = JMP32(0);
	x86SetJ32(j32Ptr[4]);
	for (i=0, code = PSXM(pc); i<count; i++, code++) {
		psxRegs.code = *code;
		recSW();
	}
	//ADD32ItoR(ESP, resp);
	x86SetJ32(j32Ptr[5]);
	resp = respsave;
}
*/

extern u32 SWL_MASK[4];
extern u32 SWL_SHIFT[4];

void iSWLk(u32 shift) {
	if (IsConst(_Rt_)) {
		MOV32ItoR(ECX, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SHR32ItoR(ECX, SWL_SHIFT[shift]);
	AND32ItoR(EAX, SWL_MASK[shift]);
	OR32RtoR (EAX, ECX);
}

void recSWL() {
// mem[Rs + Im] = Rt Merge mem[Rs + Im]

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32MtoR(EAX, (uptr)&psxM[addr & 0x1ffffc]);
			iSWLk(addr & 3);
			MOV32RtoM((uptr)&psxM[addr & 0x1ffffc], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(EAX, (uptr)&psxH[addr & 0xffc]);
			iSWLk(addr & 3);
			MOV32RtoM((uptr)&psxH[addr & 0xffc], EAX);
			return;
		}
	}

	if (IsConst(_Rs_)) MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_) ADD32ItoR(EAX, _Imm_);
	}
	PUSHR  (EAX);
	AND32ItoR(EAX, ~3);
	MOV32RtoR(X86ARG1, EAX);

	CALLFunc((uptr)psxMemRead32);

	POPR   (EDX);
	AND32ItoR(EDX, 0x3); // shift = addr & 3;

	MOV64ItoR(ECX, (uptr)SWL_MASK);
	MOV32RmStoR(ECX, ECX, EDX, 2);
	AND32RtoR(EAX, ECX); // mem & SWL_MASK[shift]

	MOV64ItoR(ECX, (uptr)SWL_SHIFT);
	MOV32RmStoR(ECX, ECX, EDX, 2);
	if (IsConst(_Rt_)) {
		MOV32ItoR(EDX, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(EDX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SHR32CLtoR(EDX); // _rRt_ >> SWL_SHIFT[shift]

	OR32RtoR (EAX, EDX);
	MOV32RtoR(X86ARG2, EAX);

	if (IsConst(_Rs_)) MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_) ADD32ItoR(EAX, _Imm_);
	}
	AND32ItoR(EAX, ~3);
	MOV32RtoR(X86ARG1, EAX);

	CALLFunc((uptr)psxMemWrite32);
//	ADD32ItoR(ESP, 8);
	//resp+= 8;
}

extern u32 SWR_MASK[4];
extern u32 SWR_SHIFT[4];

void iSWRk(u32 shift) {
	if (IsConst(_Rt_)) {
		MOV32ItoR(ECX, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SHL32ItoR(ECX, SWR_SHIFT[shift]);
	AND32ItoR(EAX, SWR_MASK[shift]);
	OR32RtoR (EAX, ECX);
}

void recSWR() {
// mem[Rs + Im] = Rt Merge mem[Rs + Im]

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32MtoR(EAX, (uptr)&psxM[addr & 0x1ffffc]);
			iSWRk(addr & 3);
			MOV32RtoM((uptr)&psxM[addr & 0x1ffffc], EAX);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(EAX, (uptr)&psxH[addr & 0xffc]);
			iSWRk(addr & 3);
			MOV32RtoM((uptr)&psxH[addr & 0xffc], EAX);
			return;
		}
	}

	if (IsConst(_Rs_)) MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_) ADD32ItoR(EAX, _Imm_);
	}
	PUSHR  (EAX);

	AND32ItoR(EAX, ~3);
	MOV32RtoR(X86ARG1, EAX);

	CALLFunc((uptr)psxMemRead32);

	POPR   (EDX);
	AND32ItoR(EDX, 0x3); // shift = addr & 3;

	MOV64ItoR(ECX, (uptr)SWR_MASK);
	MOV32RmStoR(ECX, ECX, EDX, 2);
	AND32RtoR(EAX, ECX); // mem & SWR_MASK[shift]

	MOV64ItoR(ECX, (uptr)SWR_SHIFT);
	MOV32RmStoR(ECX, ECX, EDX, 2);
	if (IsConst(_Rt_)) {
		MOV32ItoR(EDX, iRegs[_Rt_].k);
	} else {
		MOV32MtoR(EDX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	SHL32CLtoR(EDX); // _rRt_ << SWR_SHIFT[shift]

	OR32RtoR (EAX, EDX);
	MOV32RtoR(X86ARG2, EAX);

	if (IsConst(_Rs_)) MOV32ItoR(EAX, iRegs[_Rs_].k + _Imm_);
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		if (_Imm_) ADD32ItoR(EAX, _Imm_);
	}
	AND32ItoR(EAX, ~3);
	MOV32RtoR(X86ARG1, EAX);
	CALLFunc((uptr)psxMemWrite32);
//	ADD32ItoR(ESP, 8);
	//resp+= 8;
}

#endif

#if 0
REC_FUNC(SLL);
REC_FUNC(SRL);
REC_FUNC(SRA);
#endif
#if 1
static void recSLL() {
// Rd = Rt << Sa
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rt_].k << _Sa_);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Sa_) SHL32ItoR(EAX, _Sa_);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSRL() {
// Rd = Rt >> Sa
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rt_)) {
		MapConst(_Rd_, iRegs[_Rt_].k >> _Sa_);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Sa_) SHR32ItoR(EAX, _Sa_);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSRA() {
// Rd = Rt >> Sa
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rt_)) {
		MapConst(_Rd_, (s32)iRegs[_Rt_].k >> _Sa_);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Sa_) SAR32ItoR(EAX, _Sa_);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}
#endif

#if 0
REC_FUNC(SLLV);
REC_FUNC(SRLV);
REC_FUNC(SRAV);
#endif

#if 1
static void recSLLV() {
// Rd = Rt << Rs
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(_Rd_, iRegs[_Rt_].k << iRegs[_Rs_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32ItoR(ECX, iRegs[_Rs_].k);
		SHL32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rt_].k);
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SHL32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SHL32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSRLV() {
// Rd = Rt >> Rs
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(_Rd_, iRegs[_Rt_].k >> iRegs[_Rs_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32ItoR(ECX, iRegs[_Rs_].k);
		SHR32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rt_].k);
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SHR32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SHR32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSRAV() {
// Rd = Rt >> Rs
	if (!_Rd_) return;

//	iFlushRegs();

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		MapConst(_Rd_, (s32)iRegs[_Rt_].k >> iRegs[_Rs_].k);
	} else if (IsConst(_Rs_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32ItoR(ECX, iRegs[_Rs_].k);
		SAR32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else if (IsConst(_Rt_)) {
		iRegs[_Rd_].state = ST_UNK;

		MOV32ItoR(EAX, iRegs[_Rt_].k);
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SAR32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		iRegs[_Rd_].state = ST_UNK;

		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SAR32CLtoR(EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}
#endif

#if 0
REC_SYS(SYSCALL);
REC_SYS(BREAK);
#endif

#if 1
static void recSYSCALL() {
//	dump=1;
	iFlushRegs();

	MOV32ItoR(EAX, pc - 4);
	MOV32RtoM((uptr)&psxRegs.pc, EAX);
	MOV64ItoR(X86ARG2, branch == 1 ? 1 : 0);
	MOV64ItoR(X86ARG1, 0x20);
	CALLFunc((uptr)psxException);
	//ADD32ItoR(ESP, 8);

	branch = 2;
	iRet();
}

static void recBREAK() {
}
#endif

#if 0
REC_FUNC(MFHI);
REC_FUNC(MTHI);
REC_FUNC(MFLO);
REC_FUNC(MTLO);
#endif
#if 1
static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;

	iRegs[_Rd_].state = ST_UNK;
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.n.hi);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recMTHI() {
// Hi = Rs

	if (IsConst(_Rs_)) {
		MOV32ItoM((uptr)&psxRegs.GPR.n.hi, iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
	}
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;

	iRegs[_Rd_].state = ST_UNK;
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.n.lo);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recMTLO() {
// Lo = Rs

	if (IsConst(_Rs_)) {
		MOV32ItoM((uptr)&psxRegs.GPR.n.lo, iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	}
}
#endif

#if 0
REC_BRANCH(J);
REC_BRANCH(JR);
REC_BRANCH(JAL);
REC_BRANCH(JALR);
REC_BRANCH(BLTZ);
REC_BRANCH(BGTZ);
REC_BRANCH(BLTZAL);
REC_BRANCH(BGEZAL);
REC_BRANCH(BNE);
REC_BRANCH(BEQ);
REC_BRANCH(BLEZ);
REC_BRANCH(BGEZ);
#endif
#if 1
static void recBLTZ() {
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[4] = JL32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc+=4;
}

static void recBGTZ() {
// Branch if Rs > 0
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k > 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[4] = JG32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc+=4;
}

static void recBLTZAL() {
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
			MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc + 4);
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[4] = JL32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc + 4);
	iBranch(bpc, 0);
	pc+=4;
}

static void recBGEZAL() {
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc + 4);
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[4] = JGE32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc + 4);
	iBranch(bpc, 0);
	pc+=4;
}

static void recJ() {
// j target

	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJAL() {
// jal target

	MapConst(31, pc + 4);

	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJR() {
// jr Rs

	if (IsConst(_Rs_)) {
		MOV32ItoM((uptr)&target, iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32RtoM((uptr)&target, EAX);
	}

	SetBranch();
}

static void recJALR() {
// jalr Rs

	if (IsConst(_Rs_)) {
		MOV32ItoM((uptr)&target, iRegs[_Rs_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32RtoM((uptr)&target, EAX);
	}

	if (_Rd_) {
		MapConst(_Rd_, pc + 4);
	}

	SetBranch();
}

static void recBEQ() {
// Branch if Rs == Rt
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (_Rs_ == _Rt_) {
		iJump(bpc);
	} else {
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			if (iRegs[_Rs_].k == iRegs[_Rt_].k) {
				iJump(bpc); return;
			} else {
				iJump(pc+4); return;
			}
		} else if (IsConst(_Rs_)) {
			CMP32ItoM((uptr)&psxRegs.GPR.r[_Rt_], iRegs[_Rs_].k);
		} else if (IsConst(_Rt_)) {
			CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], iRegs[_Rt_].k);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		}

		j32Ptr[4] = JE32(0);

		iBranch(pc+4, 1);

		x86SetJ32(j32Ptr[4]);

		iBranch(bpc, 0);
		pc+=4;
	}
}

static void recBNE() {
// Branch if Rs != Rt
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		if (iRegs[_Rs_].k != iRegs[_Rt_].k) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	} else if (IsConst(_Rs_)) {
		CMP32ItoM((uptr)&psxRegs.GPR.r[_Rt_], iRegs[_Rs_].k);
	} else if (IsConst(_Rt_)) {
		CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], iRegs[_Rt_].k);
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	}
	j32Ptr[4] = JNE32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc+=4;
}

static void recBLEZ() {
// Branch if Rs <= 0
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k <= 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[4] = JLE32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc+=4;
}

static void recBGEZ() {
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;

//	iFlushRegs();
	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[4] = JGE32(0);

	iBranch(pc+4, 1);

	x86SetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc+=4;
}
#endif

#if 0
REC_FUNC(MFC0);
REC_SYS(MTC0);
REC_FUNC(CFC0);
REC_SYS(CTC0);
REC_FUNC(RFE);
#endif
//REC_SYS(MTC0);
#if 1
static void recMFC0() {
// Rt = Cop0->Rd
	if (!_Rt_) return;

	iRegs[_Rt_].state = ST_UNK;
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.r[_Rd_]);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

static void recCFC0() {
// Rt = Cop0->Rd

	recMFC0();
}

//*
void psxMTC0();
static void recMTC0() {
// Cop0->Rd = Rt

	if (IsConst(_Rt_)) {
		switch (_Rd_) {
			case 12:
				MOV32ItoM((uptr)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k);
				break;
			case 13:
				MOV32ItoM((uptr)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k & ~(0xfc00));
				break;
			default:
				MOV32ItoM((uptr)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k);
				break;
		}
	} else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		switch (_Rd_) {
			case 13:
				AND32ItoR(EAX, ~(0xfc00));
				break;
		}
		MOV32RtoM((uptr)&psxRegs.CP0.r[_Rd_], EAX);
	}

	if (_Rd_ == 12 || _Rd_ == 13) {
		iFlushRegs();
		MOV32ItoM((uptr)&psxRegs.pc, (u32)pc);
		CALLFunc((uptr)psxTestSWInts);
		if (branch == 0) {
			branch = 2;
			iRet();
		}
	}
}//*/

static void recCTC0() {
// Cop0->Rd = Rt

	recMTC0();
}

static void recRFE() {
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.n.Status);
	MOV32RtoR(ECX, EAX);
	AND32ItoR(EAX, 0xfffffff0);
	AND32ItoR(ECX, 0x3c);
	SHR32ItoR(ECX, 2);
	OR32RtoR (EAX, ECX);
	MOV32RtoM((uptr)&psxRegs.CP0.n.Status, EAX);

	iFlushRegs();
	MOV32ItoM((uptr)&psxRegs.pc, (u32)pc);
	CALLFunc((uptr)psxTestSWInts);
	if (branch == 0) {
		branch = 2;
		iRet();
	}
}
#endif

#include "iGte.h"

//

static void recHLE() {
	iFlushRegs();

	CALLFunc((uptr)psxHLEt[psxRegs.code & 0xffff]);
	branch = 2;
	iRet();
}

//

#include "iPGXP-64.h"

static void (*recBSC[64])() = {
	recSPECIAL, recREGIMM, recJ   , recJAL  , recBEQ , recBNE , recBLEZ, recBGTZ,
	recADDI   , recADDIU , recSLTI, recSLTIU, recANDI, recORI , recXORI, recLUI ,
	recCOP0   , recNULL  , recCOP2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	recLB     , recLH    , recLWL , recLW   , recLBU , recLHU , recLWR , recNULL,
	recSB     , recSH    , recSWL , recSW   , recNULL, recNULL, recSWR , recNULL,
	recNULL   , recNULL  , recLWC2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recSWC2, recHLE  , recNULL, recNULL, recNULL, recNULL
};

static void (*recSPC[64])() = {
	recSLL , recNULL, recSRL , recSRA , recSLLV   , recNULL , recSRLV, recSRAV,
	recJR  , recJALR, recNULL, recNULL, recSYSCALL, recBREAK, recNULL, recNULL,
	recMFHI, recMTHI, recMFLO, recMTLO, recNULL   , recNULL , recNULL, recNULL,
	recMULT, recMULTU, recDIV, recDIVU, recNULL   , recNULL , recNULL, recNULL,
	recADD , recADDU, recSUB , recSUBU, recAND    , recOR   , recXOR , recNOR ,
	recNULL, recNULL, recSLT , recSLTU, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL
};

static void (*recREG[32])() = {
	recBLTZ  , recBGEZ  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recBLTZAL, recBGEZAL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void (*recCP0[32])() = {
	recMFC0, recNULL, recCFC0, recNULL, recMTC0, recNULL, recCTC0, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recRFE , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void (*recCP2[64])() = {
	recBASIC, recRTPS , recNULL , recNULL, recNULL, recNULL , recNCLIP, recNULL, // 00
	recNULL , recNULL , recNULL , recNULL, recOP  , recNULL , recNULL , recNULL, // 08
	recDPCS , recINTPL, recMVMVA, recNCDS, recCDP , recNULL , recNCDT , recNULL, // 10
	recNULL , recNULL , recNULL , recNCCS, recCC  , recNULL , recNCS  , recNULL, // 18
	recNCT  , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 20
	recSQR  , recDCPL , recDPCT , recNULL, recNULL, recAVSZ3, recAVSZ4, recNULL, // 28
	recRTPT , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 30
	recNULL , recNULL , recNULL , recNULL, recNULL, recGPF  , recGPL  , recNCCT  // 38
};

static void (*recCP2BSC[32])() = {
	recMFC2, recNULL, recCFC2, recNULL, recMTC2, recNULL, recCTC2, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

// Trace all functions using PGXP
static void(*pgxpRecBSC[64])() = {
	recSPECIAL, recREGIMM, recJ   , recJAL  , recBEQ , recBNE , recBLEZ, recBGTZ,
	pgxpRecADDI   , pgxpRecADDIU , pgxpRecSLTI, pgxpRecSLTIU, pgxpRecANDI, pgxpRecORI , pgxpRecXORI, pgxpRecLUI ,
	recCOP0   , recNULL  , recCOP2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	pgxpRecLB     , pgxpRecLH    , pgxpRecLWL , pgxpRecLW   , pgxpRecLBU , pgxpRecLHU , pgxpRecLWR , pgxpRecNULL,
	pgxpRecSB     , pgxpRecSH    , pgxpRecSWL , pgxpRecSW   , pgxpRecNULL, pgxpRecNULL, pgxpRecSWR , pgxpRecNULL,
	recNULL   , recNULL  , pgxpRecLWC2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , pgxpRecSWC2, recHLE  , recNULL, recNULL, recNULL, recNULL
};

static void(*pgxpRecSPC[64])() = {
	pgxpRecSLL , pgxpRecNULL, pgxpRecSRL , pgxpRecSRA , pgxpRecSLLV   , pgxpRecNULL , pgxpRecSRLV, pgxpRecSRAV,
	recJR  , recJALR, recNULL, recNULL, recSYSCALL, recBREAK, recNULL, recNULL,
	pgxpRecMFHI, pgxpRecMTHI, pgxpRecMFLO, pgxpRecMTLO, pgxpRecNULL   , pgxpRecNULL , pgxpRecNULL, pgxpRecNULL,
	pgxpRecMULT, pgxpRecMULTU, pgxpRecDIV, pgxpRecDIVU, pgxpRecNULL   , pgxpRecNULL , pgxpRecNULL, pgxpRecNULL,
	pgxpRecADD , pgxpRecADDU, pgxpRecSUB , pgxpRecSUBU, pgxpRecAND    , pgxpRecOR   , pgxpRecXOR , pgxpRecNOR ,
	pgxpRecNULL, pgxpRecNULL, pgxpRecSLT , pgxpRecSLTU, pgxpRecNULL   , pgxpRecNULL , pgxpRecNULL, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL   , pgxpRecNULL , pgxpRecNULL, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL   , pgxpRecNULL , pgxpRecNULL, pgxpRecNULL
};

static void(*pgxpRecCP0[32])() = {
	pgxpRecMFC0, pgxpRecNULL, pgxpRecCFC0, pgxpRecNULL, pgxpRecMTC0, pgxpRecNULL, pgxpRecCTC0, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL,
	pgxpRecRFE , pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL
};

static void(*pgxpRecCP2BSC[32])() = {
	pgxpRecMFC2, pgxpRecNULL, pgxpRecCFC2, pgxpRecNULL, pgxpRecMTC2, pgxpRecNULL, pgxpRecCTC2, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL,
	pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL, pgxpRecNULL
};

// Trace memory functions only
static void(*pgxpRecBSCMem[64])() = {
	recSPECIAL, recREGIMM, recJ   , recJAL  , recBEQ , recBNE , recBLEZ, recBGTZ,
	recADDI   , recADDIU , recSLTI, recSLTIU, recANDI, recORI , recXORI, recLUI ,
	recCOP0   , recNULL  , recCOP2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	pgxpRecLB     , pgxpRecLH    , pgxpRecLWL , pgxpRecLW   , pgxpRecLBU , pgxpRecLHU , pgxpRecLWR , pgxpRecNULL,
	pgxpRecSB     , pgxpRecSH    , pgxpRecSWL , pgxpRecSW   , pgxpRecNULL, pgxpRecNULL, pgxpRecSWR , pgxpRecNULL,
	recNULL   , recNULL  , pgxpRecLWC2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , pgxpRecSWC2, recHLE  , recNULL, recNULL, recNULL, recNULL
};

static void recRecompile() {
	char *p;
	char *ptr;
	resp = 0;

	/* if x86Ptr reached the mem limit reset whole mem */
	if (((uptr)x86Ptr - (uptr)recMem) >= (RECMEM_SIZE - PTRMULT*0x10000))
		recReset();

	x86Align(32);
	ptr = x86Ptr;

	PC_RECP(psxRegs.pc) = (uptr)x86Ptr;
	pc = psxRegs.pc;
	pcold = pc;

	//Make some stack space for function arguments spill (x86-64 calling conventions)
	// 0x38 = 7 args, should be plenty...
	SUB64ItoR(RSP, STACKSIZE);

	for (count=0; count<500;) {
		p = (char *)PSXM(pc);
		if (p == NULL) recError();
		psxRegs.code = *(u32 *)p;
/*
		if ((psxRegs.code >> 26) == 0x23) { // LW
			int i;
			u32 code;

			for (i=1;; i++) {
				p = (char *)PSXM(pc+i*4);
				if (p == NULL) recError();
				code = *(u32 *)p;

				if ((code >> 26) != 0x23 ||
					_fRs_(code)  != _Rs_ ||
					_fImm_(code) != (_Imm_+i*4))
					break;
			}
			if (i > 1) {
				recLWBlock(i);
				pc = pc + i*4; continue;
			}
		}

		if ((psxRegs.code >> 26) == 0x2b) { // SW
			int i;
			u32 code;

			for (i=1;; i++) {
				p = (char *)PSXM(pc+i*4);
				if (p == NULL) recError();
				code = *(u32 *)p;

				if ((code >> 26) != 0x2b ||
					_fRs_(code)  != _Rs_ ||
					_fImm_(code) != (_Imm_+i*4))
					break;
			}
			if (i > 1) {
				recSWBlock(i);
				pc = pc + i*4; continue;
			}
		}*/
#ifdef PSXCPU_LOG
		if (Config.PsxOut) { dump = TRUE; }
		iDump(iRegs);
#endif
		pc+=4; count++;
		pRecBSC[psxRegs.code>>26]();
#ifdef PSXCPU_LOG
		dump = FALSE;
#endif
		if (branch) {
			branch = 0;
#ifdef PSXCPU_LOG
			if (dump) iDumpBlock(ptr);
#endif
			return;
		}
	}

	iFlushRegs();

	MOV32ItoM((uptr)&psxRegs.pc, pc);
	iRet();
}


R3000Acpu psxRec = {
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown,
	recSetPGXPMode
};
#endif
