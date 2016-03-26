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
*	pgxp_gpu.c
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 25 Mar 2016
*      Author: iCatButler
***************************************************************************/

#include "pgxp_gpu.h"
#include "stdafx.h"
#include "externals.h"

#include <math.h>

typedef struct
{
	float	x;
	float	y;
	float	z;
	unsigned int	valid;
	unsigned int	count;
} PGXP_vertex;

const unsigned int primStrideTable[] = { 1, 2, 1, 2, 2, 3, 2, 3, 0 };
const unsigned int primCountTable[] = { 3, 3, 4, 4, 3, 3, 4, 4, 0 };

PGXP_vertex*	PGXP_Mem = NULL;	// pointer to parallel memory
unsigned int	currentAddr = 0;	// address of current DMA

// Set current DMA address and pointer to parallel memory
void CALLBACK GPUpgxpMemory(unsigned int addr, unsigned char* pVRAM)
{
	if (pVRAM)
		PGXP_Mem = (PGXP_vertex*)(pVRAM);
	currentAddr = addr;
}

// Set current DMA address
void PGXP_SetAddress(unsigned int addr)
{
	currentAddr = addr;
}

// Get parallel vertex values
int PGXP_GetVertices(unsigned int* addr, void* pOutput)
{
	unsigned int	primCmd		= ((*addr >> 24) & 0xff);		// primitive command
	unsigned int	primIdx		= (primCmd - 0x20) >> 2;		// index to primitive lookup
	OGLVertex*		pVertex		= (OGLVertex*)pOutput;			// pointer to output vertices
	unsigned int	stride		= primStrideTable[primIdx];		// stride between vertices
	unsigned int	count		= primCountTable[primIdx];		// number of vertices
	PGXP_vertex*	primStart	= NULL;							// pointer to first vertex

	if (PGXP_Mem == NULL)
		return 0;

	// Offset to start of primitive
	primStart = &PGXP_Mem[currentAddr + 1];

	for (unsigned i = 0; i < count; ++i)
	{
		if (primStart[stride * i].valid)
		{
			pVertex[i].x = primStart[stride * i].x;
			pVertex[i].y = primStart[stride * i].y;
		}
	}

	return 1;
}

