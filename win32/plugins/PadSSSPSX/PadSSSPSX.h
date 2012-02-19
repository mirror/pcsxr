/*	PADwin
 *	Copyright (C) 2002-2004  PADwin Team
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __PAD_H__
#define __PAD_H__

#include "PadSSSPSXres.h"

typedef signed char	s8;
typedef signed short s16;
typedef signed int s32;
typedef __int64 s64;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned __int64 u64;

typedef struct
{
	u32 key;
	u32 event;
} keyEvent;

typedef struct
{
	u32 keys[2][21];
	u32 dualshock;
	u32 visualvibration[2];
} Config;

#endif
