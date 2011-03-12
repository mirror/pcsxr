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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef __PSXMEMORY_H__
#define __PSXMEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"

#if defined(__BIGENDIAN__)

#define _SWAP16(b) ((((unsigned char *)&(b))[0] & 0xff) | (((unsigned char *)&(b))[1] & 0xff) << 8)
#define _SWAP32(b) ((((unsigned char *)&(b))[0] & 0xff) | ((((unsigned char *)&(b))[1] & 0xff) << 8) | ((((unsigned char *)&(b))[2] & 0xff) << 16) | (((unsigned char *)&(b))[3] << 24))

#define SWAP16(v) ((((v) & 0xff00) >> 8) +(((v) & 0xff) << 8))
#define SWAP32(v) ((((v) & 0xff000000ul) >> 24) + (((v) & 0xff0000ul) >> 8) + (((v) & 0xff00ul)<<8) +(((v) & 0xfful) << 24))
#define SWAPu32(v) SWAP32((u32)(v))
#define SWAPs32(v) SWAP32((s32)(v))

#define SWAPu16(v) SWAP16((u16)(v))
#define SWAPs16(v) SWAP16((s16)(v))

#else

#define SWAP16(b) (b)
#define SWAP32(b) (b)

#define SWAPu16(b) (b)
#define SWAPu32(b) (b)

#endif

extern s8 *psxM;
#define PSXM_SIZE 0x200000
#define psxMs8(mem)		psxM[(mem) & (PSXM_SIZE - 1)]
#define psxMs16(mem)	(SWAP16(*(s16 *)&psxM[(mem) & (PSXM_SIZE - 1)]))
#define psxMs32(mem)	(SWAP32(*(s32 *)&psxM[(mem) & (PSXM_SIZE - 1)]))
#define psxMu8(mem)		(*(u8 *)&psxM[(mem) & (PSXM_SIZE - 1)])
#define psxMu16(mem)	(SWAP16(*(u16 *)&psxM[(mem) & (PSXM_SIZE - 1)]))
#define psxMu32(mem)	(SWAP32(*(u32 *)&psxM[(mem) & (PSXM_SIZE - 1)]))

#define psxMs8ref(mem)	psxM[(mem) & (PSXM_SIZE - 1)]
#define psxMs16ref(mem)	(*(s16 *)&psxM[(mem) & (PSXM_SIZE - 1)])
#define psxMs32ref(mem)	(*(s32 *)&psxM[(mem) & (PSXM_SIZE - 1)])
#define psxMu8ref(mem)	(*(u8 *)&psxM[(mem) & (PSXM_SIZE - 1)])
#define psxMu16ref(mem)	(*(u16 *)&psxM[(mem) & (PSXM_SIZE - 1)])
#define psxMu32ref(mem)	(*(u32 *)&psxM[(mem) & (PSXM_SIZE - 1)])

#define PSXP_OFFSET PSXM_SIZE
#define PSXP_SIZE 0x10000
#define psxP (&psxM[PSXP_OFFSET])
#define psxPs8(mem)	    psxP[(mem) & (PSXP_SIZE - 1)]
#define psxPs16(mem)	(SWAP16(*(s16 *)&psxP[(mem) & (PSXP_SIZE - 1)]))
#define psxPs32(mem)	(SWAP32(*(s32 *)&psxP[(mem) & (PSXP_SIZE - 1)]))
#define psxPu8(mem)		(*(u8 *)&psxP[(mem) & (PSXP_SIZE - 1)])
#define psxPu16(mem)	(SWAP16(*(u16 *)&psxP[(mem) & (PSXP_SIZE - 1)]))
#define psxPu32(mem)	(SWAP32(*(u32 *)&psxP[(mem) & (PSXP_SIZE - 1)]))

#define psxPs8ref(mem)	psxP[(mem) & (PSXP_SIZE - 1)]
#define psxPs16ref(mem)	(*(s16 *)&psxP[(mem) & (PSXP_SIZE - 1)])
#define psxPs32ref(mem)	(*(s32 *)&psxP[(mem) & (PSXP_SIZE - 1)])
#define psxPu8ref(mem)	(*(u8 *)&psxP[(mem) & (PSXP_SIZE - 1)])
#define psxPu16ref(mem)	(*(u16 *)&psxP[(mem) & (PSXP_SIZE - 1)])
#define psxPu32ref(mem)	(*(u32 *)&psxP[(mem) & (PSXP_SIZE - 1)])

#define PSXR_SIZE 0x80000
extern s8 *psxR;
#define psxRs8(mem)		psxR[(mem) & (PSXR_SIZE - 1)]
#define psxRs16(mem)	(SWAP16(*(s16 *)&psxR[(mem) & (PSXR_SIZE - 1)]))
#define psxRs32(mem)	(SWAP32(*(s32 *)&psxR[(mem) & (PSXR_SIZE - 1)]))
#define psxRu8(mem)		(*(u8* )&psxR[(mem) & (PSXR_SIZE - 1)])
#define psxRu16(mem)	(SWAP16(*(u16 *)&psxR[(mem) & (PSXR_SIZE - 1)]))
#define psxRu32(mem)	(SWAP32(*(u32 *)&psxR[(mem) & (PSXR_SIZE - 1)]))

#define psxRs8ref(mem)	psxR[(mem) & (PSXR_SIZE - 1)]
#define psxRs16ref(mem)	(*(s16*)&psxR[(mem) & (PSXR_SIZE - 1)])
#define psxRs32ref(mem)	(*(s32*)&psxR[(mem) & (PSXR_SIZE - 1)])
#define psxRu8ref(mem)	(*(u8 *)&psxR[(mem) & (PSXR_SIZE - 1)])
#define psxRu16ref(mem)	(*(u16*)&psxR[(mem) & (PSXR_SIZE - 1)])
#define psxRu32ref(mem)	(*(u32*)&psxR[(mem) & (PSXR_SIZE - 1)])

#define PSXH_SIZE 0x10000
#define PSXH_OFFSET (PSXP_OFFSET + PSXP_SIZE)
#define psxH (&psxM[PSXH_OFFSET])
#define psxHs8(mem)		psxH[(mem) & (PSXH_SIZE - 1)]
#define psxHs16(mem)	(SWAP16(*(s16 *)&psxH[(mem) & (PSXH_SIZE - 1)]))
#define psxHs32(mem)	(SWAP32(*(s32 *)&psxH[(mem) & (PSXH_SIZE - 1)]))
#define psxHu8(mem)		(*(u8 *)&psxH[(mem) & (PSXH_SIZE - 1)])
#define psxHu16(mem)	(SWAP16(*(u16 *)&psxH[(mem) & (PSXH_SIZE - 1)]))
#define psxHu32(mem)	(SWAP32(*(u32 *)&psxH[(mem) & (PSXH_SIZE - 1)]))

#define psxHs8ref(mem)	psxH[(mem) & (PSXH_SIZE - 1)]
#define psxHs16ref(mem)	(*(s16 *)&psxH[(mem) & (PSXH_SIZE - 1)])
#define psxHs32ref(mem)	(*(s32 *)&psxH[(mem) & (PSXH_SIZE - 1)])
#define psxHu8ref(mem)	(*(u8 *)&psxH[(mem) & (PSXH_SIZE - 1)])
#define psxHu16ref(mem)	(*(u16 *)&psxH[(mem) & (PSXH_SIZE - 1)])
#define psxHu32ref(mem)	(*(u32 *)&psxH[(mem) & (PSXH_SIZE - 1)])

extern u8 **psxMemWLUT;
extern u8 **psxMemRLUT;

#define PSXM(mem)		(psxMemRLUT[(mem) >> 16] == 0 ? NULL : (u8*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)))
/* prevent crashes */
#define PSXMt(t, mem)		(psxMemRLUT[(mem) >> 16] == 0 ? (t)0 : *(t*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)))
#define PSXMs8(mem)		PSXMt(s8, mem)
#define PSXMs16(mem)	(SWAP16(PSXMt(s16, mem)))
#define PSXMs32(mem)	(SWAP32(PSXMt(s32, mem)))
#define PSXMu8(mem)		PSXMt(u8, mem)
#define PSXMu16(mem)	(SWAP16(PSXMt(u16, mem)))
#define PSXMu32(mem)	(SWAP32(PSXMt(u32, mem)))

#define PSXMu32ref(mem)	(*(u32 *)PSXM(mem))

#if !defined(PSXREC) && (defined(__x86_64__) || defined(__i386__) || defined(__ppc__)) && !defined(NOPSXREC)
#define PSXREC
#endif

int psxMemInit();
void psxMemReset();
void psxMemShutdown();

u8 psxMemRead8 (u32 mem);
u16 psxMemRead16(u32 mem);
u32 psxMemRead32(u32 mem);
void psxMemWrite8 (u32 mem, u8 value);
void psxMemWrite16(u32 mem, u16 value);
void psxMemWrite32(u32 mem, u32 value);
void *psxMemPointer(u32 mem);

#ifdef __cplusplus
}
#endif
#endif
