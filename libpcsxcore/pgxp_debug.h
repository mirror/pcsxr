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
*	pgxp_debug.h
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 07 Jun 2016
*      Author: iCatButler
***************************************************************************/

#ifndef _PGXP_DEBUG_H_
#define _PGXP_DEBUG_H_

#include "psxcommon.h"

// Debug wrappers
void	PGXP_psxTrace(u32 code);
void	PGXP_psxTraceOp1(u32 code, u32 op1);
void	PGXP_psxTraceOp2(u32 code, u32 op1, u32 op2);
void	PGXP_psxTraceOp3(u32 code, u32 op1, u32 op2, u32 op3);
void	PGXP_psxTraceOp4(u32 code, u32 op1, u32 op2, u32 op3, u32 op4);

extern unsigned int pgxp_debug;

#endif//_PGXP_DEBUG_H_