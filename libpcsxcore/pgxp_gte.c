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

typedef struct 
{
	float			x;
	float			y;
	float			z;
	unsigned int	valid;
	unsigned int	count;
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

precise_value Mem[2048 * 1024 / 4];		// mirror 2MB in 32-bit words
precise_value Scratch[2048 * 1024 / 4]; // mirror 2MB in 32-bit words

void PGXP_Init()
{
	memset(Mem, 0, sizeof(Mem));
}

char* PGXP_GetMem()
{
	return (char*)(Mem);
}

#define VRAM 0
#define SCRATCH 1

precise_value* ReadMem(u32 addr)
{
	u32 memType;
	uint32_t paddr = addr;
	int* ip = NULL;

	switch (paddr >> 24)
	{
	case 0x80:
	case 0xa0:
	case 0x00:
		memType = VRAM;
		break;
	default:
		if ((paddr >> 20) == 0x1f8)
		{
			memType = SCRATCH;
			break;
		}
		//		if (value.valid)	//FAILED @ 0x807FFEE8
		//			*ip = 5;
		return NULL;
	}
	
#ifdef GTE_LOG
	//GTE_LOG("PGXP_Read %x [%x] |", addr, paddr);
#endif
	if (memType == VRAM)
	{
		// RAM furher mirrored over 8MB
		//paddr = (paddr & 0x1FFFFF) >> 2;
		paddr = ((paddr & 0x7FFFFF) % 0x200000) >> 2;
		return &Mem[paddr];
	}
	else if (memType == SCRATCH)
	{
		paddr = (paddr & 0x1FFFFF) >> 2;// (paddr & 0x3FFF) >> 2;
		return &Scratch[paddr];
	}

	return NULL;
}

void WriteMem(precise_value value, u32 addr)
{
	u32 memType;
	uint32_t paddr = addr;
	int* ip = NULL;

	switch (paddr >> 24)
	{
	case 0x80:
	case 0xa0:
	case 0x00:
		memType = VRAM;
		break;
	default:
		if ((paddr >> 20) == 0x1f8)
		{
			memType = SCRATCH;
			break;
		}
		else
			if (value.valid)	//FAILED @ 0x807FFEE8
				*ip = 5;
		return;
	}

#ifdef GTE_LOG
	GTE_LOG("PGXP_Write %x [%x] |", addr, paddr);
#endif

	// Store to RAM
	if (memType == VRAM)
	{
		// RAM furher mirrored over 8MB
		//paddr = (paddr & 0x1FFFFF) >> 2;
		paddr = ((paddr & 0x7FFFFF) % 0x200000) >> 2;// (paddr & 0x3FFF) >> 2;
		Mem[paddr] = value;
	}
	else if (memType == SCRATCH)
	{
		paddr = (paddr & 0x1FFFFF) >> 2;// (paddr & 0x3FFF) >> 2;
		Scratch[paddr] = value;
	}
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

	high->valid = (high->valid && PGXP_validate(high->x, temp.x) && PGXP_validate(high->y, temp.y));

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

void PGXP_pushSXYZ2f(float _x, float _y, float _z)
{
	static unsigned int uCount = 0;
	// push values down FIFO
	SXY0 = SXY1;
	SXY1 = SXY2;
	
	SXY2.x		= _x;
	SXY2.y		= _y;
	SXY2.z		= _z;
	SXY2.valid	= 1;
	SXY2.count	= uCount++;

#ifdef GTE_LOG
	GTE_LOG("PGPR_PUSH (%f, %f) %u %u|", SXY2.x, SXY2.y, SXY2.valid, SXY2.count);
#endif
}

void PGXP_pushSXYZ2s(s64 _x, s64 _y, s64 _z)
{
	float fx = (float)(_x) / (float)(1 << 16);
	float fy = (float)(_y) / (float)(1 << 16);
	float fz = (float)(_z);

	PGXP_pushSXYZ2f(fx, fy, fz);
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
	if (fabs(nclip) < 1.0f)
		nclip += (nclip < 0.f ? -1 : 1);

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
	GTE_LOG("PGXP_MFC2 [%x] [%x] %x (%u %u)|", gpr, gtr, value, GTE_reg[gtr].valid, GTE_reg[gtr].count);
#endif

	CPU_reg[gpr] = PGXP_validateXY(&GTE_reg[gtr], value);
}

// copy GPR reg to GTE data reg (MTC2)
void PGXP_MTC2(u32 gpr, u32 gtr, u32 value)
{
#ifdef GTE_LOG
	GTE_LOG("PGXP_MTC2 [%x] [%x] %x (%u %u)|", gpr, gtr, value, CPU_reg[gtr].valid, CPU_reg[gtr].count);
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

// store 16bit word
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
void PGPR_InvalidLoad(u32 addr, u32 code)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	precise_value p;

	p.x = p.y = p.valid = p.count = 0;

	// invalidate register
	CPU_reg[reg] = p;
}

// invalidate memory address (invalid 8/16 bit write)
void PGPR_InvalidStore(u32 addr, u32 code)
{
	u32 reg = ((code >> 16) & 0x1F); // The rt part of the instruction register 
	precise_value p;

	p.x = p.y = p.valid = p.count = 0;

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
	PGPR_InvalidLoad(mem, code);
	return value;
}

void PGXP_psxMemWrite16Trace(u32 mem, u16 value, u32 code)
{
	PGPR_InvalidStore(mem, code);
	psxMemWrite16(mem, value);
}

u8 PGXP_psxMemRead8Trace(u32 mem, u32 code)
{
	u8 value = psxMemRead8(mem);
	PGPR_InvalidLoad(mem, code);
	return value;
}

void PGXP_psxMemWrite8Trace(u32 mem, u8 value, u32 code)
{
	PGPR_InvalidStore(mem, code);
	psxMemWrite8(mem, value);
}