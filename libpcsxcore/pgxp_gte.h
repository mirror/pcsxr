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
*	pgxp_gte.h
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 12 Mar 2016
*      Author: iCatButler
***************************************************************************/

#ifndef _PGXP_GTE_H_
#define _PGXP_GTE_H_

#include "psxcommon.h"

void	PGXP_Init();	// initialise memory
char*	PGXP_GetMem();	// return pointer to precision memory

// -- GTE functions
// Transforms
void	PGXP_pushSXYZ2f(float _x, float _y, float _z);
void	PGXP_pushSXYZ2s(s64 _x, s64 _y, s64 _z);
int		PGXP_NLCIP_valid();
float	PGXP_NCLIP();

// Data transfer tracking
void	PGXP_MFC2(u32 gpr, u32 gtr, u32 value);		// copy GTE reg to GPR reg (MFC2)
void	PGXP_MTC2(u32 gpr, u32 gtr, u32 value);		// copy GPR reg to GTR reg (MTC2)
void	PGXP_LWC2(u32 addr, u32 gtr, u32 value);	// copy memory to GTE reg
void	PGXP_SWC2(u32 addr, u32 gtr, u32 value);	// copy GTE reg to memory

// -- CPU functions
// Data transfer tracking
void	PGPR_L32(u32 addr, u32 code, u32 value);	// load 32bit word
void	PGPR_S32(u32 addr, u32 code, u32 value);	// store 32bit word

// Memory Read/Write hooks
u32		PGXP_psxMemRead32Trace(u32 mem, u32 code);
void	PGXP_psxMemWrite32Trace(u32 mem, u32 value, u32 code);

u16		PGXP_psxMemRead16Trace(u32 mem, u32 code);
void	PGXP_psxMemWrite16Trace(u32 mem, u16 value, u32 code);

u8		PGXP_psxMemRead8Trace(u32 mem, u32 code);
void	PGXP_psxMemWrite8Trace(u32 mem, u8 value, u32 code);

#endif /* _PGXP_GTE_H_ */
