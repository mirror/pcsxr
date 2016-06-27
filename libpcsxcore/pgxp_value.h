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
*	pgxp_value.h
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 07 Jun 2016
*      Author: iCatButler
***************************************************************************/

#ifndef _PGXP_VALUE_H_
#define _PGXP_VALUE_H_

#include "psxcommon.h"

typedef union {
#if defined(__BIGENDIAN__)
	struct { u8 h3, h2, h, l; } b;
	struct { s8 h3, h2, h, l; } sb;
	struct { u16 h, l; } w;
	struct { s16 h, l; } sw;
#else
	struct { u8 l, h, h2, h3; } b;
	struct { u16 l, h; } w;
	struct { s8 l, h, h2, h3; } sb;
	struct { s16 l, h; } sw;
#endif
	u32 d;
	s32 sd;
} psx_value;

typedef struct PGXP_value_Tag
{
	float			x;
	float			y;
	float			z;
	unsigned int	valid;
	unsigned int	count;
	unsigned int	value;

	unsigned short	gFlags;
	unsigned char	lFlags;
	unsigned char	hFlags;
} PGXP_value;

typedef enum
{
	UNINITIALISED		= 0,
	INVALID_PSX_VALUE	= 1,
	INVALID_ADDRESS		= 2,
	INVALID_BITWISE_OP	= 3,
	DIVIDE_BY_ZERO		= 4,
	INVALID_8BIT_LOAD	= 5,
	INVALID_8BIT_STORE	= 6
} PGXP_error_states;

typedef enum
{
	VALID_HALF = (1 << 0)
} PGXP_half_flags;

static const PGXP_value PGXP_value_invalid_address = { 0.f, 0.f, 0.f, 0, 0, 0, INVALID_ADDRESS, 0, 0 };
static const PGXP_value PGXP_value_zero = { 0.f, 0.f, 0.f, 0, 0, 1, 0, 0, 0 };

void MakeValid(PGXP_value *pV, u32 psxV);
void Validate(PGXP_value *pV, u32 psxV);
void MaskValidate(PGXP_value *pV, u32 psxV, u32 mask);


typedef union
{
	struct
	{
		s16 x;
		s16 y;
	};
	struct
	{
		u16 ux;
		u16 uy;
	};
	u32 word;
} low_value;

#endif//_PGX_VALUE_H_
