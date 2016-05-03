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
#include "psxmem.h"
#include "r3000a.h"

typedef struct 
{
	float			x;
	float			y;
	float			z;
	unsigned int	valid;
	unsigned int	count;
	unsigned int	value;
} precise_value;

typedef union
{
	struct
	{
		s16 x;
		s16 y;
	};
	u32 word;
} low_value;

#define tolerance 1.0f

precise_value GTE_reg[32];
precise_value CPU_reg[32];

precise_value Mem[3 * 2048 * 1024 / 4];		// mirror 2MB in 32-bit words * 3
const u32 UserMemOffset		= 0;
const u32 ScratchOffset		= 2048 * 1024 / 4;
const u32 RegisterOffset	= 2 * 2048 * 1024 / 4;
const u32 InvalidAddress	= 3 * 2048 * 1024 / 4;

//precise_value Scratch[2048 * 1024 / 4]; // mirror 2MB in 32-bit words
//precise_value Registers[2048 * 1024 / 4]; // mirror 2MB in 32-bit words

void PGXP_Init()
{
	memset(Mem, 0, sizeof(Mem));
}

char* PGXP_GetMem()
{
	return (char*)(Mem);
}

/*  Playstation Memory Map (from Playstation doc by Joshua Walker)
0x0000_0000-0x0000_ffff		Kernel (64K)
0x0001_0000-0x001f_ffff		User Memory (1.9 Meg)

0x1f00_0000-0x1f00_ffff		Parallel Port (64K)

0x1f80_0000-0x1f80_03ff		Scratch Pad (1024 bytes)

0x1f80_1000-0x1f80_2fff		Hardware Registers (8K)

0x1fc0_0000-0x1fc7_ffff		BIOS (512K)

0x8000_0000-0x801f_ffff		Kernel and User Memory Mirror (2 Meg) Cached
0x9fc0_0000-0x9fc7_ffff		BIOS Mirror (512K) Cached

0xa000_0000-0xa01f_ffff		Kernel and User Memory Mirror (2 Meg) Uncached
0xbfc0_0000-0xbfc7_ffff		BIOS Mirror (512K) Uncached
*/
void ValidateAddress(u32 addr)
{
	int* pi = NULL;

	if		((addr >= 0x00000000) && (addr <= 0x007fffff)) {}	// Kernel + User Memory x 8
	else if ((addr >= 0x1f000000) && (addr <= 0x1f00ffff)) {}	// Parallel Port
	else if ((addr >= 0x1f800000) && (addr <= 0x1f8003ff)) {}	// Scratch Pad
	else if ((addr >= 0x1f801000) && (addr <= 0x1f802fff)) {}	// Hardware Registers
	else if ((addr >= 0x1fc00000) && (addr <= 0x1fc7ffff)) {}	// Bios
	else if ((addr >= 0x80000000) && (addr <= 0x807fffff)) {}	// Kernel + User Memory x 8 Cached mirror
	else if ((addr >= 0x9fc00000) && (addr <= 0x9fc7ffff)) {}	// Bios Cached Mirror
	else if ((addr >= 0xa0000000) && (addr <= 0xa07fffff)) {}	// Kernel + User Memory x 8 Uncached mirror
	else if ((addr >= 0xbfc00000) && (addr <= 0xbfc7ffff)) {}	// Bios Uncached Mirror
	else if (addr == 0xfffe0130) {}								// Used for cache flushing
	else
	{
	//	*pi = 5;
	}

}

u32 PGXP_ConvertAddress(u32 addr)
{
	u32 memOffs = 0;
	u32 paddr = addr;

	ValidateAddress(addr);

	switch (paddr >> 24)
	{
	case 0x80:
	case 0xa0:
	case 0x00:
		// RAM further mirrored over 8MB
		paddr = ((paddr & 0x7FFFFF) % 0x200000) >> 2;
		paddr = UserMemOffset + paddr;
		break;
	default:
		if ((paddr >> 20) == 0x1f8)
		{
			if (paddr >= 0x1f801000)
			{
				//	paddr = ((paddr & 0xFFFF) - 0x1000);
				//	paddr = (paddr % 0x2000) >> 2;
				paddr = ((paddr & 0xFFFF) - 0x1000) >> 2;
				paddr = RegisterOffset + paddr;
				break;
			}
			else
			{
				//paddr = ((paddr & 0xFFF) % 0x400) >> 2;
				paddr = (paddr & 0x3FF) >> 2;
				paddr = ScratchOffset + paddr;
				break;
			}
		}

		paddr = InvalidAddress;
		break;
	}

#ifdef GTE_LOG
	//GTE_LOG("PGXP_Read %x [%x] |", addr, paddr);
#endif

	return paddr;
}

precise_value* GetPtr(u32 addr)
{
	addr = PGXP_ConvertAddress(addr);

	if (addr != InvalidAddress)
		return &Mem[addr];
	return NULL;
}

precise_value* ReadMem(u32 addr)
{
	return GetPtr(addr);
}

void WriteMem(precise_value value, u32 addr)
{
	precise_value* pMem = GetPtr(addr);

	if (pMem)
		*pMem = value;
}

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

unsigned int PGXP_validate(float high, s16 low)
{
	if (fabs(high - (float)(low)) < tolerance)
		return 1;
	return 0;
}

// Check that value is still within tolerance of low precision value and invalidate if not
precise_value PGXP_validateXY(precise_value *high, u32 low)
{
	low_value temp;
	precise_value ret;

	ret.valid = 0;
	temp.word = low;

	if (!high)
		return ret;

	high->valid = (high->valid) && (high->value == low);//(high->valid && PGXP_validate(high->x, temp.x) && PGXP_validate(high->y, temp.y));

	// Cheat
	//if (!high->valid)
	//{
	//	high->x = temp.x;
	//	high->y = temp.y;
	//	high->valid = 1;
	//}

	return *high;
}

u32 PGXP_compareXY(precise_value high, u32 low)
{
	low_value temp;
	temp.word = low;

	if (PGXP_validate(high.x, temp.x) && PGXP_validate(high.y, temp.y))
		return 1;
	return 0;
}

precise_value PGXP_copyXY(u32 low)
{
	low_value temp;
	precise_value ret;

	ret.valid = 0;
	temp.word = low;

	ret.x = temp.x;
	ret.y = temp.y;
	ret.count = 0;
	ret.valid = 1;

	return ret;
}

void PGXP_pushSXYZ2f(float _x, float _y, float _z, unsigned int _v)
{
	static unsigned int uCount = 0;
	// push values down FIFO
	SXY0 = SXY1;
	SXY1 = SXY2;
	
	SXY2.x		= _x;
	SXY2.y		= _y;
	SXY2.z		= _z;
	SXY2.value	= _v;
	SXY2.valid	= 1;
	SXY2.count	= uCount++;

#ifdef GTE_LOG
	GTE_LOG("PGPR_PUSH (%f, %f) %u %u|", SXY2.x, SXY2.y, SXY2.valid, SXY2.count);
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

	float H = psxRegs.CP2C.p[26].sw.l;	// Near plane
	float F = 0xFFFF;					// Far plane?
	float SZ3 = max(MAC3, H);			// Clamp SZ3 to near plane because we have no clipping (no proper Z)
	//	float h_over_sz3 = H / SZ3;

	// Offsets with 16-bit shift
	float OFX = (float)psxRegs.CP2C.p[24].sd / (float)(1 << 16);
	float OFY = (float)psxRegs.CP2C.p[25].sd / (float)(1 << 16);

	// PSX Screen space X,Y,W components
	float sx = OFX + (MAC1 * (H / SZ3)) * (Config.Widescreen ? 0.75 : 1);
	float sy = OFY + (MAC2 * (H / SZ3));
	float sw = SZ3;

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

static precise_value PGXP_MFC2_int(u32 reg)
{
	switch (reg) 
	{
	case 15:
		GTE_reg[reg] = SXYP = SXY2;
		break;
	}

	return GTE_reg[reg];
}


static void PGXP_MTC2_int(precise_value value, u32 reg)
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

	CPU_reg[gpr] = PGXP_validateXY(&GTE_reg[gtr], value);
}

// copy GPR reg to GTE data reg (MTC2)
void PGXP_MTC2(u32 gpr, u32 gtr, u32 value)
{
#ifdef GTE_LOG
	GTE_LOG("PGXP_MTC2 [%x]->[%x] %x (%u %u)|", gpr, gtr, value, CPU_reg[gpr].valid, CPU_reg[gpr].count);
#endif
	PGXP_MTC2_int(PGXP_validateXY(&CPU_reg[gpr], value), gtr);
}

// copy memory to GTE reg
void PGXP_LWC2(u32 addr, u32 gtr, u32 value)
{
#ifdef GTE_LOG
	precise_value* pp = ReadMem(addr);
	precise_value p;
	low_value temp;
	temp.word = value;

	p.x = p.y = p.valid = 0;

	if (pp)
		p = *pp;

	GTE_LOG("PGXP_LWC2 %x [%x] %x (%d, %d) (%f, %f) %u %u|", addr, gtr, value, temp.x, temp.y, p.x, p.y, p.valid, p.count);
#endif
	PGXP_MTC2_int(PGXP_validateXY(ReadMem(addr), value), gtr);
}

//copy GTE reg to memory
void PGXP_SWC2(u32 addr, u32 gtr, u32 value)
{
#ifdef GTE_LOG
	low_value temp;
	temp.word = value;

	if (PGXP_compareXY(GTE_reg[gtr], value))
		GTE_LOG("PGPR_SWC2 %x [%x] %x (%d, %d) (%f, %f) %u %u|", addr, gtr, value, temp.x, temp.y, GTE_reg[gtr].x, GTE_reg[gtr].y, GTE_reg[gtr].valid, GTE_reg[gtr].count);
#endif
	WriteMem(PGXP_validateXY(&GTE_reg[gtr], value), addr);
}

// load 32bit word
void PGPR_L32(u32 addr, u32 code, u32 value)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	u32 op = ((code >> 26));
	precise_value p;

	low_value temp;
	temp.word = value;

	p.x = p.y = p.valid = p.count = 0;

	switch (op)
	{
	case 34:	//LWL
		CPU_reg[reg] = PGXP_validateXY(ReadMem(addr), value);
		break;
	case 35:	//LW
		CPU_reg[reg] = PGXP_validateXY(ReadMem(addr), value);
		break;
	case 37:	//LWR
		CPU_reg[reg] = PGXP_validateXY(ReadMem(addr), value);
		break;
	case 50:	//LWC2 (GTE vertex reads)
		GTE_reg[reg] = PGXP_validateXY(ReadMem(addr), value);
		break;
	default:
		// invalidate register
	//	CPU_reg[reg] = p;
		break;
	}

#ifdef GTE_LOG
	GTE_LOG("PGPR_L32 %u: %x %x[%x %x] (%d, %d) (%f, %f) %x %u|", op, addr, value, code, reg, temp.x, temp.y, CPU_reg[reg].x, CPU_reg[reg].y, CPU_reg[reg].valid, CPU_reg[reg].count);
#endif
}

// store 32bit word
void PGPR_S32(u32 addr, u32 code, u32 value)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	u32 op = ((code >> 26));
	precise_value p;

	low_value temp;
	temp.word = value;

	p.x = p.y = p.valid = p.count = 0;

#ifdef GTE_LOG
	GTE_LOG("PGPR_S32 %u: %x %x[%x %x] (%d, %d) (%f, %f) %x %u|", op, addr, value, code, reg, temp.x, temp.y, CPU_reg[reg].x, CPU_reg[reg].y, CPU_reg[reg].valid, CPU_reg[reg].count);
#endif

	switch (op)
	{
	case 42:	//SWL
		WriteMem(PGXP_validateXY(&CPU_reg[reg], value), addr);
		break;
	case 43:	//SW
		WriteMem(PGXP_validateXY(&CPU_reg[reg], value), addr);
		break;
	case 46:	//SWR
		WriteMem(PGXP_validateXY(&CPU_reg[reg], value), addr);
		break;
	case 58:	//SWC2 (GTE vertex writes)
		WriteMem(PGXP_validateXY(&GTE_reg[reg], value), addr);
		break;
	default:
		// invalidate memory
	//	WriteMem(p, addr);
		break;
	}
}

// invalidate register (invalid 8/16 bit read)
void PGPR_InvalidLoad(u32 addr, u32 code, u32 value)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	precise_value* pD = NULL;
	precise_value p;

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

// invalidate memory address (invalid 8/16 bit write)
void PGPR_InvalidStore(u32 addr, u32 code, u32 value)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	precise_value* pD = NULL;
	precise_value p;

	pD = ReadMem(addr);

	p.x = p.y = -2337;

	if (pD)
		p = *pD;

	p.valid = 0;
	p.count = (reg * 1000) + value;

	// invalidate memory
	WriteMem(p, addr);
}

u32 PGXP_psxMemRead32Trace(u32 mem, u32 code)
{
	u32 value = psxMemRead32(mem);
	PGPR_L32(mem, code, value);
	return value;
}

void PGXP_psxMemWrite32Trace(u32 mem, u32 value, u32 code)
{
	PGPR_S32(mem, code, value);
	psxMemWrite32(mem, value);
}

u16 PGXP_psxMemRead16Trace(u32 mem, u32 code)
{
	u16 value = psxMemRead16(mem);
	PGPR_InvalidLoad(mem, code, 116);
	return value;
}

void PGXP_psxMemWrite16Trace(u32 mem, u16 value, u32 code)
{
	PGPR_InvalidStore(mem, code, 216);
	psxMemWrite16(mem, value);
}

u8 PGXP_psxMemRead8Trace(u32 mem, u32 code)
{
	u8 value = psxMemRead8(mem);
	PGPR_InvalidLoad(mem, code, 18);
	return value;
}

void PGXP_psxMemWrite8Trace(u32 mem, u8 value, u32 code)
{
	PGPR_InvalidStore(mem, code, 28);
	psxMemWrite8(mem, value);
}