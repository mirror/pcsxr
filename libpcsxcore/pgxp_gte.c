/***************************************************************************
*   Copyright (C) 2016 by iCatButler                                      *
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

/**************************************************************************
*	pgxp_gte.c
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 12 Mar 2016
*      Author: iCatButler
***************************************************************************/

#include "pgxp_gte.h"
#include "pgxp_value.h"
#include "pgxp_mem.h"
#include "pgxp_debug.h"
#include "pgxp_cpu.h"

#include "psxcommon.h"
#include "psxmem.h"
#include "r3000a.h"

PGXP_value GTE_reg[32];

#define SX0 (GTE_reg[ 12 ].x)
#define SY0 (GTE_reg[ 12 ].y)
#define SX1 (GTE_reg[ 13 ].x)
#define SY1 (GTE_reg[ 13 ].y)
#define SX2 (GTE_reg[ 14 ].x)
#define SY2 (GTE_reg[ 14 ].y)

#define SXY0 (GTE_reg[ 12 ])
#define SXY1 (GTE_reg[ 13 ])
#define SXY2 (GTE_reg[ 14 ])
#define SXYP (GTE_reg[ 15 ])

void PGXP_pushSXYZ2f(float _x, float _y, float _z, unsigned int _v)
{
	static unsigned int uCount = 0;
	low_value temp;
	// push values down FIFO
	SXY0 = SXY1;
	SXY1 = SXY2;
	
	SXY2.x		= _x;
	SXY2.y		= _y;
	SXY2.z		= Config.PGXP_Texture ? _z : 1.f;
	SXY2.value	= _v;
	SXY2.valid	= 1;
	SXY2.count	= uCount++;

	// cache value in GPU plugin
	temp.word = _v;
	if(Config.PGXP_Cache)
		GPU_pgxpCacheVertex(temp.x, temp.y, &SXY2);
	else
		GPU_pgxpCacheVertex(0, 0, NULL);

#ifdef GTE_LOG
	GTE_LOG("PGXP_PUSH (%f, %f) %u %u|", SXY2.x, SXY2.y, SXY2.valid, SXY2.count);
#endif
}

void PGXP_pushSXYZ2s(s64 _x, s64 _y, s64 _z, u32 v)
{
	float fx = (float)(_x) / (float)(1 << 16);
	float fy = (float)(_y) / (float)(1 << 16);
	float fz = (float)(_z);

	PGXP_pushSXYZ2f(fx, fy, fz, v);
}

#define VX(n) (psxRegs.CP2D.p[ n << 1 ].sw.l)
#define VY(n) (psxRegs.CP2D.p[ n << 1 ].sw.h)
#define VZ(n) (psxRegs.CP2D.p[ (n << 1) + 1 ].sw.l)

void PGXP_RTPS(u32 _n, u32 _v)
{
	// Transform
	float TRX = (s64)psxRegs.CP2C.p[5].sd;
	float TRY = (s64)psxRegs.CP2C.p[6].sd;
	float TRZ = (s64)psxRegs.CP2C.p[7].sd;

	// Rotation with 12-bit shift
	float R11 = (float)psxRegs.CP2C.p[ 0 ].sw.l / (float)(1 << 12);
	float R12 = (float)psxRegs.CP2C.p[ 0 ].sw.h / (float)(1 << 12);
	float R13 = (float)psxRegs.CP2C.p[ 1 ].sw.l / (float)(1 << 12);
	float R21 = (float)psxRegs.CP2C.p[ 1 ].sw.h / (float)(1 << 12);
	float R22 = (float)psxRegs.CP2C.p[ 2 ].sw.l / (float)(1 << 12);
	float R23 = (float)psxRegs.CP2C.p[ 2 ].sw.h / (float)(1 << 12);
	float R31 = (float)psxRegs.CP2C.p[ 3 ].sw.l / (float)(1 << 12);
	float R32 = (float)psxRegs.CP2C.p[ 3 ].sw.h / (float)(1 << 12);
	float R33 = (float)psxRegs.CP2C.p[ 4 ].sw.l / (float)(1 << 12);

	// Bring vertex into view space
	float MAC1 = TRX + (R11 * VX(_n)) + (R12 * VY(_n)) + (R13 * VZ(_n));
	float MAC2 = TRY + (R21 * VX(_n)) + (R22 * VY(_n)) + (R23 * VZ(_n));
	float MAC3 = TRZ + (R31 * VX(_n)) + (R32 * VY(_n)) + (R33 * VZ(_n));

	float IR1 = max(min(MAC1, 0x7fff), -0x8000);
	float IR2 = max(min(MAC2, 0x7fff), -0x8000);
	float IR3 = max(min(MAC3, 0x7fff), -0x8000);

	float H = psxRegs.CP2C.p[26].sw.l;	// Near plane
	float F = 0xFFFF;					// Far plane?
	float SZ3 = max(min(MAC3, 0xffff), 0x0000);			// Clamp SZ3 to near plane because we have no clipping (no proper Z)
	//	float h_over_sz3 = H / SZ3;

	// Offsets with 16-bit shift
	float OFX = (float)psxRegs.CP2C.p[24].sd / (float)(1 << 16);
	float OFY = (float)psxRegs.CP2C.p[25].sd / (float)(1 << 16);

	float h_over_w = min(H / SZ3, (float)0x1ffff / (float)0xffff);
	h_over_w = (SZ3 == 0) ? ((float)0x1ffff / (float)0xffff) : h_over_w;

	// PSX Screen space X,Y,W components
	float sx = OFX + (IR1 * h_over_w) * (Config.Widescreen ? 0.75 : 1);
	float sy = OFY + (IR2 * h_over_w);
	float sw = SZ3;// max(SZ3, 0.1);

	sx = max(min(sx, 1024.f), -1024.f);
	sy = max(min(sy, 1024.f), -1024.f);

	//float sx2 = SX2;
	//float sy2 = SY2;
	//float sz2 = SXY2.z;

	//float ftolerance = 5.f;

	//if ((fabs(sx - sx2) > ftolerance) ||
	//	(fabs(sy - sy2) > ftolerance) ||
	//	(fabs(sw - sz2) > ftolerance))
	//{
	//	float r = 5;
	//}

	PGXP_pushSXYZ2f(sx , sy , sw, _v);

	return;
}

int PGXP_NLCIP_valid()
{
	if (SXY0.valid && SXY1.valid && SXY2.valid)
		return 1;
	return 0;
}

float PGXP_NCLIP()
{
	float nclip = ((SX0 * SY1) + (SX1 * SY2) + (SX2 * SY0) - (SX0 * SY2) - (SX1 * SY0) - (SX2 * SY1));

	// ensure fractional values are not incorrectly rounded to 0
	if ((fabs(nclip) < 1.0f) && (nclip != 0))
		nclip += (nclip < 0.f ? -1 : 1);

	//float AX = SX1 - SX0;
	//float AY = SY1 - SY0;

	//float BX = SX2 - SX0;
	//float BY = SY2 - SY0;

	//// normalise A and B
	//float mA = sqrt((AX*AX) + (AY*AY));
	//float mB = sqrt((BX*BX) + (BY*BY));

	//// calculate AxB to get Z component of C
	//float CZ = ((AX * BY) - (AY * BX)) * (1 << 12);

	return nclip;
}

static PGXP_value PGXP_MFC2_int(u32 reg)
{
	switch (reg) 
	{
	case 15:
		GTE_reg[reg] = SXYP = SXY2;
		break;
	}

	return GTE_reg[reg];
}


static void PGXP_MTC2_int(PGXP_value value, u32 reg)
{
	switch(reg)
	{
		case 15:
			// push FIFO
			SXY0 = SXY1;
			SXY1 = SXY2;
			SXY2 = value;
			SXYP = SXY2;
			break;

		case 31:
			return;
	}

	GTE_reg[reg] = value;
}

// copy GTE data reg to GPR reg (MFC2)
void PGXP_MFC2(u32 gpr, u32 gtr, u32 value)
{
	if (!gpr) return;
#ifdef GTE_LOG
	GTE_LOG("PGXP_MFC2 [%x]<-[%x] %x (%u %u)|", gpr, gtr, value, GTE_reg[gtr].valid, GTE_reg[gtr].count);
#endif

	Validate(&GTE_reg[gtr], value);
	CPU_reg[gpr] = GTE_reg[gtr];
}

// copy GPR reg to GTE data reg (MTC2)
void PGXP_MTC2(u32 gpr, u32 gtr, u32 value)
{
#ifdef GTE_LOG
	GTE_LOG("PGXP_MTC2 [%x]->[%x] %x (%u %u)|", gpr, gtr, value, CPU_reg[gpr].valid, CPU_reg[gpr].count);
#endif
	Validate(&CPU_reg[gpr], value);
	PGXP_MTC2_int(CPU_reg[gpr], gtr);
}

// copy memory to GTE reg
void PGXP_LWC2(u32 addr, u32 gtr, u32 value)
{
	PGXP_value val;
	ValidateAndCopyMem(&val, addr, value);
#ifdef GTE_LOG
	PGXP_value* pp = ReadMem(addr);
	PGXP_value p;
	low_value temp;
	temp.word = value;

	p.x = p.y = p.valid = 0;

	if (pp)
		p = *pp;

	GTE_LOG("PGXP_LWC2 %x [%x] %x (%d, %d) (%f, %f) %u %u|", addr, gtr, value, temp.x, temp.y, p.x, p.y, p.valid, p.count);
#endif
	PGXP_MTC2_int(val, gtr);
}

//copy GTE reg to memory
void PGXP_SWC2(u32 addr, u32 gtr, u32 value)
{
#ifdef GTE_LOG
	low_value temp;
	temp.word = value;

//	if (PGXP_compareXY(GTE_reg[gtr], value))
		GTE_LOG("PGXP_SWC2 %x [%x] %x (%d, %d) (%f, %f) %u %u|", addr, gtr, value, temp.x, temp.y, GTE_reg[gtr].x, GTE_reg[gtr].y, GTE_reg[gtr].valid, GTE_reg[gtr].count);
#endif
	Validate(&GTE_reg[gtr], value);
	WriteMem(&GTE_reg[gtr], addr);
}

