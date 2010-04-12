/*  PCSX-Revolution - PS Emulator for Nintendo Wii
 *  Copyright (C) 2009-2010  PCSX-Revolution Dev Team
 *
 *  PCSX-Revolution is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  PCSX-Revolution is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with PCSX-Revolution. If not, see <http://www.gnu.org/licenses/>.
 */

/*
* GTE functions.
*/

#include "gte.h"
#include "psxmem.h"

#ifdef _MSC_VER_
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)
#endif

static inline s64 BOUNDS(s64 n_value, s64 n_max, int n_maxflag, s64 n_min, int n_minflag) {
	if (n_value > n_max) {
		gteFLAG |= n_maxflag;
	} else if (n_value < n_min) {
		gteFLAG |= n_minflag;
	}
	return n_value;
}

static inline s32 LIM(s32 value, s32 max, s32 min, u32 flag) {
	s32 ret = value;
	if (value > max) {
		gteFLAG |= flag;
		ret = max;
	} else if (value < min) {
		gteFLAG |= flag;
		ret = min;
	}
	return ret;
}

#define A1(a) BOUNDS((a), 0x7fffffff, (1 << 30), -(s64)0x80000000, (1 << 31) | (1 << 27))
#define A2(a) BOUNDS((a), 0x7fffffff, (1 << 29), -(s64)0x80000000, (1 << 31) | (1 << 26))
#define A3(a) BOUNDS((a), 0x7fffffff, (1 << 28), -(s64)0x80000000, (1 << 31) | (1 << 25))
#define Lm_B1(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1 << 31) | (1 << 24))
#define Lm_B2(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1 << 31) | (1 << 23))
#define Lm_B3(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1 << 22) )
#define Lm_C1(a) LIM((a), 0x00ff, 0x0000, (1 << 21) )
#define Lm_C2(a) LIM((a), 0x00ff, 0x0000, (1 << 20) )
#define Lm_C3(a) LIM((a), 0x00ff, 0x0000, (1 << 19) )
#define Lm_D(a) LIM((a), 0xffff, 0x0000, (1 << 31) | (1 << 18))

static inline u32 Lm_E(u32 result) {
	if (result > 0x1ffff) {
		gteFLAG |= (1 << 31) | (1 << 17);
		return 0x1ffff;
	}

	return result;
}

#define F(a) BOUNDS((a), 0x7fffffff, (1 << 31) | (1 << 16), -(s64)0x80000000, (1 << 31) | (1 << 15))
#define Lm_G1(a) LIM((a), 0x3ff, -0x400, (1 << 31) | (1 << 14))
#define Lm_G2(a) LIM((a), 0x3ff, -0x400, (1 << 31) | (1 << 13))
#define Lm_H(a) LIM((a), 0xfff, 0x000, (1 << 12))

__inline u32 MFC2(int reg) {
	switch(reg) {
		case 1: case 3: case 5:
		case 8: case 9: case 10:
		case 11:
			psxRegs.CP2D.r[reg] = (s32)psxRegs.CP2D.p[reg].sw.l;
			break;

		case 7: case 16: case 17:
		case 18: case 19:
			psxRegs.CP2D.r[reg] = (u32)psxRegs.CP2D.p[reg].w.l;
			break;

		case 15:
			psxRegs.CP2D.r[reg] = gteSXY2;
			break;

		case 28: case 30:
//			SysPrintf("MFC2: psxRegs.CP2D.r[%d] cannot be read\n");
			return 0;

		case 29:
			psxRegs.CP2D.r[reg] = LIM(gteIR1 >> 7, 0x1f, 0, 0) |
									(LIM(gteIR2 >> 7, 0x1f, 0, 0) << 5) |
									(LIM(gteIR3 >> 7, 0x1f, 0, 0) << 10);
			break;
	}
	return psxRegs.CP2D.r[reg];
}

__inline void MTC2(unsigned long value, int reg) {
	switch (reg) {
		case 15:
			gteSXY0 = gteSXY1;
			gteSXY1 = gteSXY2;
			gteSXY2 = value;
			gteSXYP = value;
			break;

//		case 23: SysPrintf("RES1\n"); break;

		case 28:
			gteIRGB = value;
			gteIR1 = (value & 0x1f) << 7;
			gteIR2 = (value & 0x3e0) << 2;
			gteIR3 = (value & 0x7c00) >> 3;
			break;

		case 30:
			{
				gteLZCS = value;

				int a = gteLZCS;
				if (a > 0) {
					int i;
					for (i = 31; (a & (1 << i)) == 0 && i >= 0; i--);
					gteLZCR = 31 - i;
				} else if (a < 0) {
					int i;
					a ^= 0xffffffff;
					for (i=31; (a & (1 << i)) == 0 && i >= 0; i--);
					gteLZCR = 31 - i;
				} else {
					gteLZCR = 32;
				}
			}
			break;

		case 7: case 29: case 31:
//			SysPrintf("MTC2: psxRegs.CP2D.r[%d] cannot be write\n", reg);
			return;

		default:
			psxRegs.CP2D.r[reg] = value;
	}
}

static void setcp2cr(int reg, u32 value) {
	switch (reg) {
		case 4:
		case 12:
		case 20:
		case 26:
		case 27:
		case 29:
		case 30:
			value = (s32)(s16) value;
			break;

		case 31:
//			SysPrintf("setcp2cr: reg=%d\n", reg);
			value = value & 0x7ffff000;
			if ((value & 0x7f87e000) != 0) {
				value |= 0x80000000;
			}
			break;
	}

	psxRegs.CP2C.r[reg] = value;
}

void gteMFC2() {
	if (!_Rt_) return;
	psxRegs.GPR.r[_Rt_] = MFC2(_Rd_);
}

void gteCFC2() {
	if (!_Rt_) return;
	psxRegs.GPR.r[_Rt_] = psxRegs.CP2C.r[_Rd_];
}

void gteMTC2() {
	MTC2(psxRegs.GPR.r[_Rt_], _Rd_);
}

void gteCTC2() {
	setcp2cr(_Rd_, psxRegs.GPR.r[_Rt_]);
}

#define _oB_ (psxRegs.GPR.r[_Rs_] + _Imm_)

void gteLWC2() {
	MTC2(psxMemRead32(_oB_), _Rt_);
}

void gteSWC2() {
	psxMemWrite32(_oB_, MFC2(_Rt_));
}

void gteRTPS() {
	int h_over_sz3;

#ifdef GTE_LOG
	GTE_LOG("RTPS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteTRX << 12) + (gteR11 * gteVX0) + (gteR12 * gteVY0) + (gteR13 * gteVZ0)) >> 12);
	gteMAC2 = A2((((s64)gteTRY << 12) + (gteR21 * gteVX0) + (gteR22 * gteVY0) + (gteR23 * gteVZ0)) >> 12);
	gteMAC3 = A3((((s64)gteTRZ << 12) + (gteR31 * gteVX0) + (gteR32 * gteVY0) + (gteR33 * gteVZ0)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 0);
	gteIR2 = Lm_B2(gteMAC2, 0);
	gteIR3 = Lm_B3(gteMAC3, 0);
	gteSZ0 = gteSZ1;
	gteSZ1 = gteSZ2;
	gteSZ2 = gteSZ3;
	gteSZ3 = Lm_D(gteMAC3);
	h_over_sz3 = Lm_E((gteH * 65536) / (gteSZ3 + 0.5));
	gteSXY0 = gteSXY1;
	gteSXY1 = gteSXY2;
	gteSX2 = Lm_G1(F((s64)gteOFX + ((s64)gteIR1 * h_over_sz3)) >> 16);
	gteSY2 = Lm_G2(F((s64)gteOFY + ((s64)gteIR2 * h_over_sz3)) >> 16);

	gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * h_over_sz3)) >> 12);
	gteIR0 = Lm_H(gteMAC0);
}

void gteRTPT() {
	int h_over_sz3;
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("RTPT\n");
#endif
	gteFLAG = 0;

	gteSZ0 = gteSZ3;
	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
		gteMAC2 = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
		gteMAC3 = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
		gteIR1 = Lm_B1(gteMAC1, 0);
		gteIR2 = Lm_B2(gteMAC2, 0);
		gteIR3 = Lm_B3(gteMAC3, 0);
		fSZ(v) = Lm_D(gteMAC3);
		h_over_sz3 = Lm_E((gteH * 65536) / (fSZ(v) + 0.5));
		fSX(v) = Lm_G1(F((s64)gteOFX + ((s64)gteIR1 * h_over_sz3)) >> 16);
		fSY(v) = Lm_G2(F((s64)gteOFY + ((s64)gteIR2 * h_over_sz3)) >> 16);
	}
	gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * h_over_sz3)) >> 12);
	gteIR0 = Lm_H(gteMAC0);
}

void gteMVMVA() {
	int shift = 12 * GTE_SF(gteop);
	int mx = GTE_MX(gteop);
	int v = GTE_V(gteop);
	int cv = GTE_CV(gteop);
	int lm = GTE_LM(gteop);
	s32 vx = VX(v);
	s32 vy = VY(v);
	s32 vz = VZ(v);

#ifdef GTE_LOG
	GTE_LOG("MVMVA\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)CV1(cv) << 12) + (MX11(mx) * vx) + (MX12(mx) * vy) + (MX13(mx) * vz)) >> shift);
	gteMAC2 = A2((((s64)CV2(cv) << 12) + (MX21(mx) * vx) + (MX22(mx) * vy) + (MX23(mx) * vz)) >> shift);
	gteMAC3 = A3((((s64)CV3(cv) << 12) + (MX31(mx) * vx) + (MX32(mx) * vy) + (MX33(mx) * vz)) >> shift);

	gteIR1 = Lm_B1(gteMAC1, lm);
	gteIR2 = Lm_B2(gteMAC2, lm);
	gteIR3 = Lm_B3(gteMAC3, lm);
}

void gteNCLIP() {
#ifdef GTE_LOG
	GTE_LOG("NCLIP\n");
#endif
	gteFLAG = 0;

	gteMAC0 = F((s64)gteSX0 * (gteSY1 - gteSY2) +
				gteSX1 * (gteSY2 - gteSY0) +
				gteSX2 * (gteSY0 - gteSY1));
}

void gteAVSZ3() {
#ifdef GTE_LOG
	GTE_LOG("AVSZ3\n");
#endif
	gteFLAG = 0;

	gteMAC0 = F((s64)(gteZSF3 * gteSZ1) + (gteZSF3 * gteSZ2) + (gteZSF3 * gteSZ3));
	gteOTZ = Lm_D(gteMAC0 >> 12);
}

void gteAVSZ4() {
#ifdef GTE_LOG
	GTE_LOG("AVSZ4\n");
#endif
	gteFLAG = 0;

	gteMAC0 = F((s64)(gteZSF4 * (gteSZ0 + gteSZ1 + gteSZ2 + gteSZ3)));
	gteOTZ = Lm_D(gteMAC0 >> 12);
}

void gteSQR() {
	int shift = 12 * GTE_SF(gteop);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("SQR\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((gteIR1 * gteIR1) >> shift);
	gteMAC2 = A2((gteIR2 * gteIR2) >> shift);
	gteMAC3 = A3((gteIR3 * gteIR3) >> shift);
	gteIR1 = Lm_B1(gteMAC1 >> shift, lm);
	gteIR2 = Lm_B2(gteMAC2 >> shift, lm);
	gteIR3 = Lm_B3(gteMAC3 >> shift, lm);
}

void gteNCCS() {
#ifdef GTE_LOG
	GTE_LOG("NCCS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
	gteMAC2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
	gteMAC3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1(((s64)gteR * gteIR1) >> 8);
	gteMAC2 = A2(((s64)gteG * gteIR2) >> 8);
	gteMAC3 = A3(((s64)gteB * gteIR3) >> 8);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}

void gteNCCT() {
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("NCCT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		gteMAC2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		gteMAC3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		gteIR1 = Lm_B1(gteMAC1, 1);
		gteIR2 = Lm_B2(gteMAC2, 1);
		gteIR3 = Lm_B3(gteMAC3, 1);
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
		gteIR1 = Lm_B1(gteMAC1, 1);
		gteIR2 = Lm_B2(gteMAC2, 1);
		gteIR3 = Lm_B3(gteMAC3, 1);
		gteMAC1 = A1(((s64)gteR * gteIR1) >> 8);
		gteMAC2 = A2(((s64)gteG * gteIR2) >> 8);
		gteMAC3 = A3(((s64)gteB * gteIR3) >> 8);

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = Lm_C1(gteMAC1 >> 4);
		gteG2 = Lm_C2(gteMAC2 >> 4);
		gteB2 = Lm_C3(gteMAC3 >> 4);
	}
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
}

void gteNCDS() {
#ifdef GTE_LOG
	GTE_LOG("NCDS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
	gteMAC2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
	gteMAC3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1(((((s64)gteR << 4) * gteIR1) + (gteIR0 * Lm_B1(gteRFC - ((gteR * gteIR1) >> 8), 0))) >> 12);
	gteMAC2 = A2(((((s64)gteG << 4) * gteIR2) + (gteIR0 * Lm_B2(gteGFC - ((gteG * gteIR2) >> 8), 0))) >> 12);
	gteMAC3 = A3(((((s64)gteB << 4) * gteIR3) + (gteIR0 * Lm_B3(gteBFC - ((gteB * gteIR3) >> 8), 0))) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}

void gteNCDT() {
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("NCDT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		vx = VX( v );
		vy = VY( v );
		vz = VZ( v );
		gteMAC1 = A1( ( ( (s64) gteL11 * vx ) + ( gteL12 * vy ) + ( gteL13 * vz ) ) >> 12 );
		gteMAC2 = A2( ( ( (s64) gteL21 * vx ) + ( gteL22 * vy ) + ( gteL23 * vz ) ) >> 12 );
		gteMAC3 = A3( ( ( (s64) gteL31 * vx ) + ( gteL32 * vy ) + ( gteL33 * vz ) ) >> 12 );
		gteIR1 = Lm_B1( gteMAC1, 1 );
		gteIR2 = Lm_B2( gteMAC2, 1 );
		gteIR3 = Lm_B3( gteMAC3, 1 );
		gteMAC1 = A1( ( ( (s64) gteRBK << 12 ) + ( gteLR1 * gteIR1 ) + ( gteLR2 * gteIR2 ) + ( gteLR3 * gteIR3 ) ) >> 12 );
		gteMAC2 = A2( ( ( (s64) gteGBK << 12 ) + ( gteLG1 * gteIR1 ) + ( gteLG2 * gteIR2 ) + ( gteLG3 * gteIR3 ) ) >> 12 );
		gteMAC3 = A3( ( ( (s64) gteBBK << 12 ) + ( gteLB1 * gteIR1 ) + ( gteLB2 * gteIR2 ) + ( gteLB3 * gteIR3 ) ) >> 12 );
		gteIR1 = Lm_B1( gteMAC1, 1 );
		gteIR2 = Lm_B2( gteMAC2, 1 );
		gteIR3 = Lm_B3( gteMAC3, 1 );
		gteMAC1 = A1( ( ( ( (s64) gteR << 4 ) * gteIR1 ) + ( gteIR0 * Lm_B1( gteRFC - ( ( gteR * gteIR1 ) >> 8 ), 0 ) ) ) >> 12 );
		gteMAC2 = A2( ( ( ( (s64) gteG << 4 ) * gteIR2 ) + ( gteIR0 * Lm_B2( gteGFC - ( ( gteG * gteIR2 ) >> 8 ), 0 ) ) ) >> 12 );
		gteMAC3 = A3( ( ( ( (s64) gteB << 4 ) * gteIR3 ) + ( gteIR0 * Lm_B3( gteBFC - ( ( gteB * gteIR3 ) >> 8 ), 0 ) ) ) >> 12 );

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = Lm_C1( gteMAC1 >> 4 );
		gteG2 = Lm_C2( gteMAC2 >> 4 );
		gteB2 = Lm_C3( gteMAC3 >> 4 );
	}
	gteIR1 = Lm_B1( gteMAC1, 1 );
	gteIR2 = Lm_B2( gteMAC2, 1 );
	gteIR3 = Lm_B3( gteMAC3, 1 );
}

void gteOP() {
	int shift = 12 * GTE_SF( gteop );
	int lm = GTE_LM( gteop );

#ifdef GTE_LOG
	GTE_LOG("OP\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1( ( (s64) ( gteR22 * gteIR3 ) - ( gteR33 * gteIR2 ) ) >> shift );
	gteMAC2 = A2( ( (s64) ( gteR33 * gteIR1 ) - ( gteR11 * gteIR3 ) ) >> shift );
	gteMAC3 = A3( ( (s64) ( gteR11 * gteIR2 ) - ( gteR22 * gteIR1 ) ) >> shift );
	gteIR1 = Lm_B1( gteMAC1, lm );
	gteIR2 = Lm_B2( gteMAC2, lm );
	gteIR3 = Lm_B3( gteMAC3, lm );
}

void gteDCPL() {
	int shift = 12 * GTE_SF( gteop );
	int lm = GTE_LM( gteop );

#ifdef GTE_LOG
	GTE_LOG("DCPL\n");
#endif
	gteFLAG = 0;

	s64 iR = (gteR << 4) * gteIR1;
	s64 iG = (gteG << 4) * gteIR2;
	s64 iB = (gteB << 4) * gteIR3;

	gteMAC1 = A1( iR + gteIR0 * Lm_B1( gteRFC - iR, 0 ) ) >> shift;
	gteMAC2 = A2( iG + gteIR0 * Lm_B1( gteGFC - iG, 0 ) ) >> shift;
	gteMAC3 = A3( iB + gteIR0 * Lm_B1( gteBFC - iB, 0 ) ) >> shift;

	gteIR1 = Lm_B1( gteMAC1 >> 4, lm );
	gteIR2 = Lm_B2( gteMAC2 >> 4, lm );
	gteIR3 = Lm_B3( gteMAC3 >> 4, lm );
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1( gteMAC1 >> 4 );
	gteG2 = Lm_C2( gteMAC2 >> 4 );
	gteB2 = Lm_C3( gteMAC3 >> 4 );
}

void gteGPF() {
	int shift = 12 * GTE_SF( gteop );

#ifdef GTE_LOG
	GTE_LOG("GPF\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1( ( (s64) gteIR0 * gteIR1 ) >> shift );
	gteMAC2 = A2( ( (s64) gteIR0 * gteIR2 ) >> shift );
	gteMAC3 = A3( ( (s64) gteIR0 * gteIR3 ) >> shift );
	gteIR1 = Lm_B1( gteMAC1, 0 );
	gteIR2 = Lm_B2( gteMAC2, 0 );
	gteIR3 = Lm_B3( gteMAC3, 0 );

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1( gteMAC1 >> 4 );
	gteG2 = Lm_C2( gteMAC2 >> 4 );
	gteB2 = Lm_C3( gteMAC3 >> 4 );
}

void gteGPL() {
	int shift = 12 * GTE_SF( gteop );

#ifdef GTE_LOG
	GTE_LOG("GPL\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1( ( ( (s64) gteMAC1 << shift ) + ( gteIR0 * gteIR1 ) ) >> shift );
	gteMAC2 = A2( ( ( (s64) gteMAC2 << shift ) + ( gteIR0 * gteIR2 ) ) >> shift );
	gteMAC3 = A3( ( ( (s64) gteMAC3 << shift ) + ( gteIR0 * gteIR3 ) ) >> shift );
	gteIR1 = Lm_B1( gteMAC1, 0 );
	gteIR2 = Lm_B2( gteMAC2, 0 );
	gteIR3 = Lm_B3( gteMAC3, 0 );

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1( gteMAC1 >> 4 );
	gteG2 = Lm_C2( gteMAC2 >> 4 );
	gteB2 = Lm_C3( gteMAC3 >> 4 );
}

void gteDPCS() {
	int shift = 12 * GTE_SF( gteop );

#ifdef GTE_LOG
	GTE_LOG("DPCS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1(((gteR << 16) + (gteIR0 * Lm_B1(A1((s64)gteRFC - (gteR << 4)) << (12 - shift), 0))) >> 12);
	gteMAC2 = A2(((gteG << 16) + (gteIR0 * Lm_B2(A2((s64)gteGFC - (gteG << 4)) << (12 - shift), 0))) >> 12);
	gteMAC3 = A3(((gteB << 16) + (gteIR0 * Lm_B3(A3((s64)gteBFC - (gteB << 4)) << (12 - shift), 0))) >> 12);

	gteIR1 = Lm_B1(gteMAC1, 0);
	gteIR2 = Lm_B2(gteMAC2, 0);
	gteIR3 = Lm_B3(gteMAC3, 0);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}

void gteDPCT() {
	int v;

#ifdef GTE_LOG
	GTE_LOG("DPCT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		gteMAC1 = A1((((s64)gteR0 << 16) + ((s64)gteIR0 * (Lm_B1(gteRFC - (gteR0 << 4), 0)))) >> 12);
		gteMAC2 = A2((((s64)gteG0 << 16) + ((s64)gteIR0 * (Lm_B1(gteGFC - (gteG0 << 4), 0)))) >> 12);
		gteMAC3 = A3((((s64)gteB0 << 16) + ((s64)gteIR0 * (Lm_B1(gteBFC - (gteB0 << 4), 0)))) >> 12);

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = Lm_C1(gteMAC1 >> 4);
		gteG2 = Lm_C2(gteMAC2 >> 4);
		gteB2 = Lm_C3(gteMAC3 >> 4);
	}
	gteIR1 = Lm_B1(gteMAC1, 0);
	gteIR2 = Lm_B2(gteMAC2, 0);
	gteIR3 = Lm_B3(gteMAC3, 0);
}

void gteNCS() {
#ifdef GTE_LOG
	GTE_LOG("NCS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
	gteMAC2 = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
	gteMAC3 = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}

void gteNCT() {
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("NCT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
		gteMAC2 = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
		gteMAC3 = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
		gteIR1 = Lm_B1(gteMAC1, 1);
		gteIR2 = Lm_B2(gteMAC2, 1);
		gteIR3 = Lm_B3(gteMAC3, 1);
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = Lm_C1(gteMAC1 >> 4);
		gteG2 = Lm_C2(gteMAC2 >> 4);
		gteB2 = Lm_C3(gteMAC3 >> 4);
	}
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
}

void gteCC() {
#ifdef GTE_LOG
	GTE_LOG("CC\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1(((s64)gteR * gteIR1) >> 8);
	gteMAC2 = A2(((s64)gteG * gteIR2) >> 8);
	gteMAC3 = A3(((s64)gteB * gteIR3) >> 8);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}

void gteINTPL() {
	int shift = 12 * GTE_SF(gteop);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("INTPL\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1(((gteIR1 << 12) + (gteIR0 * Lm_B1(((s64)gteRFC - gteIR1), 0))) >> shift);
	gteMAC2 = A2(((gteIR2 << 12) + (gteIR0 * Lm_B2(((s64)gteGFC - gteIR2), 0))) >> shift);
	gteMAC3 = A3(((gteIR3 << 12) + (gteIR0 * Lm_B3(((s64)gteBFC - gteIR3), 0))) >> shift);
	gteIR1 = Lm_B1(gteMAC1, lm);
	gteIR2 = Lm_B2(gteMAC2, lm);
	gteIR3 = Lm_B3(gteMAC3, lm);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}

void gteCDP() {
#ifdef GTE_LOG
	GTE_LOG("CDP\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);
	gteMAC1 = A1(((((s64)gteR << 4) * gteIR1) + (gteIR0 * Lm_B1(gteRFC - ((gteR * gteIR1) >> 8), 0))) >> 12);
	gteMAC2 = A2(((((s64)gteG << 4) * gteIR2) + (gteIR0 * Lm_B2(gteGFC - ((gteG * gteIR2) >> 8), 0))) >> 12);
	gteMAC3 = A3(((((s64)gteB << 4) * gteIR3) + (gteIR0 * Lm_B3(gteBFC - ((gteB * gteIR3) >> 8), 0))) >> 12);
	gteIR1 = Lm_B1(gteMAC1, 1);
	gteIR2 = Lm_B2(gteMAC2, 1);
	gteIR3 = Lm_B3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = Lm_C1(gteMAC1 >> 4);
	gteG2 = Lm_C2(gteMAC2 >> 4);
	gteB2 = Lm_C3(gteMAC3 >> 4);
}
