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

#ifndef __GTE_H__
#define __GTE_H__

#include "psxcommon.h"
#include "r3000a.h"

#define VX(n) (n < 3 ? psxRegs.CP2D.p[n << 1].sw.l : psxRegs.CP2D.p[9].sw.l)
#define VY(n) (n < 3 ? psxRegs.CP2D.p[n << 1].sw.h : psxRegs.CP2D.p[10].sw.l)
#define VZ(n) (n < 3 ? psxRegs.CP2D.p[(n << 1) + 1].sw.l : psxRegs.CP2D.p[11].sw.l)
#define MX11(n) (n < 3 ? psxRegs.CP2C.p[(n << 3)].sw.l : 0)
#define MX12(n) (n < 3 ? psxRegs.CP2C.p[(n << 3)].sw.h : 0)
#define MX13(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 1].sw.l : 0)
#define MX21(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 1].sw.h : 0)
#define MX22(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 2].sw.l : 0)
#define MX23(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 2].sw.h : 0)
#define MX31(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 3].sw.l : 0)
#define MX32(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 3].sw.h : 0)
#define MX33(n) (n < 3 ? psxRegs.CP2C.p[(n << 3) + 4].sw.l : 0)
#define CV1(n) (n < 3 ? (s32)psxRegs.CP2C.r[(n << 3) + 5] : 0)
#define CV2(n) (n < 3 ? (s32)psxRegs.CP2C.r[(n << 3) + 6] : 0)
#define CV3(n) (n < 3 ? (s32)psxRegs.CP2C.r[(n << 3) + 7] : 0)

#define fSX(n) ((psxRegs.CP2D.p)[((n) + 12)].sw.l)
#define fSY(n) ((psxRegs.CP2D.p)[((n) + 12)].sw.h)
#define fSZ(n) ((psxRegs.CP2D.p)[((n) + 17)].w.l) /* (n == 0) => SZ1; */

#define gteVXY0 (psxRegs.CP2D.r[0])
#define gteVX0  (psxRegs.CP2D.p[0].sw.l)
#define gteVY0  (psxRegs.CP2D.p[0].sw.h)
#define gteVZ0  (psxRegs.CP2D.p[1].sw.l)
#define gteVXY1 (psxRegs.CP2D.r[2])
#define gteVX1  (psxRegs.CP2D.p[2].sw.l)
#define gteVY1  (psxRegs.CP2D.p[2].sw.h)
#define gteVZ1  (psxRegs.CP2D.p[3].sw.l)
#define gteVXY2 (psxRegs.CP2D.r[4])
#define gteVX2  (psxRegs.CP2D.p[4].sw.l)
#define gteVY2  (psxRegs.CP2D.p[4].sw.h)
#define gteVZ2  (psxRegs.CP2D.p[5].sw.l)
#define gteRGB  (psxRegs.CP2D.r[6])
#define gteR    (psxRegs.CP2D.p[6].b.l)
#define gteG    (psxRegs.CP2D.p[6].b.h)
#define gteB    (psxRegs.CP2D.p[6].b.h2)
#define gteCODE (psxRegs.CP2D.p[6].b.h3)
#define gteOTZ  (psxRegs.CP2D.p[7].w.l)
#define gteIR0  (psxRegs.CP2D.p[8].sw.l)
#define gteIR1  (psxRegs.CP2D.p[9].sw.l)
#define gteIR2  (psxRegs.CP2D.p[10].sw.l)
#define gteIR3  (psxRegs.CP2D.p[11].sw.l)
#define gteSXY0 (psxRegs.CP2D.r[12])
#define gteSX0  (psxRegs.CP2D.p[12].sw.l)
#define gteSY0  (psxRegs.CP2D.p[12].sw.h)
#define gteSXY1 (psxRegs.CP2D.r[13])
#define gteSX1  (psxRegs.CP2D.p[13].sw.l)
#define gteSY1  (psxRegs.CP2D.p[13].sw.h)
#define gteSXY2 (psxRegs.CP2D.r[14])
#define gteSX2  (psxRegs.CP2D.p[14].sw.l)
#define gteSY2  (psxRegs.CP2D.p[14].sw.h)
#define gteSXYP (psxRegs.CP2D.r[15])
#define gteSXP  (psxRegs.CP2D.p[15].sw.l)
#define gteSYP  (psxRegs.CP2D.p[15].sw.h)
#define gteSZ0  (psxRegs.CP2D.p[16].w.l)
#define gteSZ1  (psxRegs.CP2D.p[17].w.l)
#define gteSZ2  (psxRegs.CP2D.p[18].w.l)
#define gteSZ3  (psxRegs.CP2D.p[19].w.l)
#define gteRGB0  (psxRegs.CP2D.r[20])
#define gteR0    (psxRegs.CP2D.p[20].b.l)
#define gteG0    (psxRegs.CP2D.p[20].b.h)
#define gteB0    (psxRegs.CP2D.p[20].b.h2)
#define gteCODE0 (psxRegs.CP2D.p[20].b.h3)
#define gteRGB1  (psxRegs.CP2D.r[21])
#define gteR1    (psxRegs.CP2D.p[21].b.l)
#define gteG1    (psxRegs.CP2D.p[21].b.h)
#define gteB1    (psxRegs.CP2D.p[21].b.h2)
#define gteCODE1 (psxRegs.CP2D.p[21].b.h3)
#define gteRGB2  (psxRegs.CP2D.r[22])
#define gteR2    (psxRegs.CP2D.p[22].b.l)
#define gteG2    (psxRegs.CP2D.p[22].b.h)
#define gteB2    (psxRegs.CP2D.p[22].b.h2)
#define gteCODE2 (psxRegs.CP2D.p[22].b.h3)
#define gteRES1  (psxRegs.CP2D.r[23])
#define gteMAC0  (((s32 *)psxRegs.CP2D.r)[24])
#define gteMAC1  (((s32 *)psxRegs.CP2D.r)[25])
#define gteMAC2  (((s32 *)psxRegs.CP2D.r)[26])
#define gteMAC3  (((s32 *)psxRegs.CP2D.r)[27])
#define gteIRGB  (psxRegs.CP2D.r[28])
#define gteORGB  (psxRegs.CP2D.r[29])
#define gteLZCS  (psxRegs.CP2D.r[30])
#define gteLZCR  (psxRegs.CP2D.r[31])

#define gteR11R12 (((s32 *)psxRegs.CP2C.r)[0])
#define gteR22R23 (((s32 *)psxRegs.CP2C.r)[2])
#define gteR11 (psxRegs.CP2C.p[0].sw.l)
#define gteR12 (psxRegs.CP2C.p[0].sw.h)
#define gteR13 (psxRegs.CP2C.p[1].sw.l)
#define gteR21 (psxRegs.CP2C.p[1].sw.h)
#define gteR22 (psxRegs.CP2C.p[2].sw.l)
#define gteR23 (psxRegs.CP2C.p[2].sw.h)
#define gteR31 (psxRegs.CP2C.p[3].sw.l)
#define gteR32 (psxRegs.CP2C.p[3].sw.h)
#define gteR33 (psxRegs.CP2C.p[4].sw.l)
#define gteTRX (((s32 *)psxRegs.CP2C.r)[5])
#define gteTRY (((s32 *)psxRegs.CP2C.r)[6])
#define gteTRZ (((s32 *)psxRegs.CP2C.r)[7])
#define gteL11 (psxRegs.CP2C.p[8].sw.l)
#define gteL12 (psxRegs.CP2C.p[8].sw.h)
#define gteL13 (psxRegs.CP2C.p[9].sw.l)
#define gteL21 (psxRegs.CP2C.p[9].sw.h)
#define gteL22 (psxRegs.CP2C.p[10].sw.l)
#define gteL23 (psxRegs.CP2C.p[10].sw.h)
#define gteL31 (psxRegs.CP2C.p[11].sw.l)
#define gteL32 (psxRegs.CP2C.p[11].sw.h)
#define gteL33 (psxRegs.CP2C.p[12].sw.l)
#define gteRBK (((s32 *)psxRegs.CP2C.r)[13])
#define gteGBK (((s32 *)psxRegs.CP2C.r)[14])
#define gteBBK (((s32 *)psxRegs.CP2C.r)[15])
#define gteLR1 (psxRegs.CP2C.p[16].sw.l)
#define gteLR2 (psxRegs.CP2C.p[16].sw.h)
#define gteLR3 (psxRegs.CP2C.p[17].sw.l)
#define gteLG1 (psxRegs.CP2C.p[17].sw.h)
#define gteLG2 (psxRegs.CP2C.p[18].sw.l)
#define gteLG3 (psxRegs.CP2C.p[18].sw.h)
#define gteLB1 (psxRegs.CP2C.p[19].sw.l)
#define gteLB2 (psxRegs.CP2C.p[19].sw.h)
#define gteLB3 (psxRegs.CP2C.p[20].sw.l)
#define gteRFC (((s32 *)psxRegs.CP2C.r)[21])
#define gteGFC (((s32 *)psxRegs.CP2C.r)[22])
#define gteBFC (((s32 *)psxRegs.CP2C.r)[23])
#define gteOFX (((s32 *)psxRegs.CP2C.r)[24])
#define gteOFY (((s32 *)psxRegs.CP2C.r)[25])
#define gteH   (psxRegs.CP2C.p[26].sw.l)
#define gteDQA (psxRegs.CP2C.p[27].sw.l)
#define gteDQB (((s32 *)psxRegs.CP2C.r)[28])
#define gteZSF3 (psxRegs.CP2C.p[29].sw.l)
#define gteZSF4 (psxRegs.CP2C.p[30].sw.l)
#define gteFLAG (psxRegs.CP2C.r[31])

#define GTE_OP(op) ((op >> 20) & 31)
#define GTE_SF(op) ((op >> 19) & 1)
#define GTE_MX(op) ((op >> 17) & 3)
#define GTE_V(op) ((op >> 15) & 3)
#define GTE_CV(op) ((op >> 13) & 3)
#define GTE_CD(op) ((op >> 11 ) & 3) /* not used */
#define GTE_LM(op) ((op >> 10 ) & 1)
#define GTE_CT(op) ((op >> 6) & 15) /* not used */
#define GTE_FUNCT(op) (op & 63)
#define INS_COFUN(op) (op & 0x1ffffff)

#define gteop (INS_COFUN(psxRegs.code))

void gteMFC2();
void gteCFC2();
void gteMTC2();
void gteCTC2();
void gteLWC2();
void gteSWC2();

void gteRTPS();
void gteOP();
void gteNCLIP();
void gteDPCS();
void gteINTPL();
void gteMVMVA();
void gteNCDS();
void gteNCDT();
void gteCDP();
void gteNCCS();
void gteCC();
void gteNCS();
void gteNCT();
void gteSQR();
void gteDCPL();
void gteDPCT();
void gteAVSZ3();
void gteAVSZ4();
void gteRTPT();
void gteGPF();
void gteGPL();
void gteNCCT();

#endif
