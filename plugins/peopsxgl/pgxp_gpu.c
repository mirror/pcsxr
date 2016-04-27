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
	unsigned int	value;
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

void PGXP_SetMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
	GLfloat m[16];
	for (unsigned int i = 0; i < 16; ++i)
		m[i] = 0.f;

	//if ((right-left) != 0)
	//{
	//	m[0] = 2 / (right - left);
	//	m[12] = -((right + left) / (right - left));
	//}
	//if ((top-bottom) != 0)
	//{
	//	m[5] = 2 / (top - bottom);
	//	m[13] = -((top + bottom) / (top - bottom));
	//}
	//m[10] = -2 / (zFar - zNear);
	//m[14] = -((zFar + zNear) / (zFar - zNear));
	//m[15] = 1;

	if ((right-left) != 0)
	{
		m[0] = 2 / (right - left);
		m[8] = -((right + left) / (right - left));
	}
	if ((top-bottom) != 0)
	{
		m[5] = 2 / (top - bottom);
		m[9] = -((top + bottom) / (top - bottom));
	}
	m[10] = -2 / (zFar - zNear);
	m[14] = -((zFar + zNear) / (zFar - zNear));
	m[11] = 1;

	glLoadMatrixf(m);
	//glOrtho(left, right, bottom, top, zNear, zFar);
}

// Get parallel vertex values
int PGXP_GetVertices(unsigned int* addr, void* pOutput, int xOffs, int yOffs)
{
	unsigned int	primCmd		= ((*addr >> 24) & 0xff);			// primitive command
	unsigned int	primIdx		= min((primCmd - 0x20) >> 2, 8);	// index to primitive lookup
	OGLVertex*		pVertex		= (OGLVertex*)pOutput;				// pointer to output vertices
	unsigned int	stride		= primStrideTable[primIdx];			// stride between vertices
	unsigned int	count		= primCountTable[primIdx];			// number of vertices
	PGXP_vertex*	primStart	= NULL;								// pointer to first vertex
	char			invalidVert	= 0;								// Number of vertices without valid PGXP values
	float			w = 0;											// W coordinate of transformed vertex

	if (PGXP_Mem == NULL)
		return 0;

	// Offset to start of primitive
	primStart = &PGXP_Mem[currentAddr + 1];

	// Find any invalid vertices
	for (unsigned i = 0; i < count; ++i)
	{
		if(!primStart[stride * i].valid)
			invalidVert++;
	}

	for (unsigned i = 0; i < count; ++i)
	{
		w = primStart[stride * i].z;
		// If there are any invalid vertices set all w values to 1
		// iCB: Could use plane equation to find w for single invalid vertex in a quad
		if (invalidVert > 0)
			w = 1;

		if (primStart[stride * i].valid)
		{
			// Premultiply x,y coorindates by w because they've already been divided by w
			pVertex[i].x = (primStart[stride * i].x + xOffs) * w;
			pVertex[i].y = (primStart[stride * i].y + yOffs) * w;
			pVertex[i].z = w;
		}
	}

	return 1;
}

