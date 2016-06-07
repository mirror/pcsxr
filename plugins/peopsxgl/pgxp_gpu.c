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
	unsigned int	flags;
} PGXP_vertex;

unsigned int PGXP_tDebug = 0;
/////////////////////////////////
//// Blade_Arma's Vertex Cache (CatBlade?)
/////////////////////////////////
const unsigned int mode_init = 0;
const unsigned int mode_write = 1;
const unsigned int mode_read = 2;
const unsigned int mode_fail = 3;

PGXP_vertex vertexCache[0x800 * 2][0x800 * 2];

unsigned int baseID = 0;
unsigned int lastID = 0;
unsigned int cacheMode = 0;

unsigned int IsSessionID(unsigned int vertID)
{
	// No wrapping
	if (lastID >= baseID)
		return (vertID >= baseID);

	// If vertID is >= baseID it is pre-wrap and in session
	if (vertID >= baseID)
		return 1;

	// vertID is < baseID, If it is <= lastID it is post-wrap and in session
	if (vertID <= lastID)
		return 1;

	return 0;
}

void CALLBACK GPUpgxpCacheVertex(short sx, short sy, const unsigned char* _pVertex)
{
	const PGXP_vertex*	pNewVertex = (const PGXP_vertex*)_pVertex;
	PGXP_vertex*		pOldVertex = NULL;

	if (!pNewVertex)
	{
		cacheMode = mode_fail;
		return;
	}

	//if (bGteAccuracy)
	{
		if (cacheMode != mode_write)
		{
			// Initialise cache on first use
			if (cacheMode == mode_init)
				memset(vertexCache, 0x00, sizeof(vertexCache));

			// First vertex of write session (frame?)
			cacheMode = mode_write;
			baseID = pNewVertex->count;
		}

		lastID = pNewVertex->count;

		if (sx >= -0x800 && sx <= 0x7ff &&
			sy >= -0x800 && sy <= 0x7ff)
		{
			pOldVertex = &vertexCache[sy + 0x800][sx + 0x800];

			// To avoid ambiguity there can only be one valid entry per-session
			if (IsSessionID(pOldVertex->count) && (pOldVertex->value == pNewVertex->value))
			{
				// check to ensure this isn't identical
				if ((fabsf(pOldVertex->x - pNewVertex->x) > 0.1f) ||
					(fabsf(pOldVertex->y - pNewVertex->y) > 0.1f) ||
					(fabsf(pOldVertex->z - pNewVertex->z) > 0.1f))
				{			
					pOldVertex->valid = 5;
					return;
				}
			}

			// Write vertex into cache
			*pOldVertex = *pNewVertex;
		}
	}
}

PGXP_vertex* PGXP_GetCachedVertex(short sx, short sy)
{
	//if (bGteAccuracy)
	{
		if (cacheMode != mode_read)
		{
			if (cacheMode == mode_fail)
				return NULL;

			// Initialise cache on first use
			if (cacheMode == mode_init)
				memset(vertexCache, 0x00, sizeof(vertexCache));

			// First vertex of read session (frame?)
			cacheMode = mode_read;
		}

		if (sx >= -0x800 && sx <= 0x7ff &&
			sy >= -0x800 && sy <= 0x7ff)
		{
			// Return pointer to cache entry
			return &vertexCache[sy + 0x800][sx + 0x800];
		}
	}

	return NULL;
}


/////////////////////////////////
//// PGXP Implementation
/////////////////////////////////

const unsigned int primStrideTable[] = { 1, 2, 1, 2, 2, 3, 2, 3, 0 };
const unsigned int primCountTable[] = { 3, 3, 4, 4, 3, 3, 4, 4, 0 };

PGXP_vertex*	PGXP_Mem = NULL;	// pointer to parallel memory
unsigned int	currentAddr = 0;	// address of current DMA

unsigned int	numVertices = 0;	// iCB: Used for glVertex3fv fix
unsigned int	vertexIdx = 0;

// Set current DMA address and pointer to parallel memory
void CALLBACK GPUpgxpMemory(unsigned int addr, unsigned char* pVRAM)
{
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

// Wrap glVertex3fv/glVertex4fv
void PGXP_glVertexfv(GLfloat* pV)
{
	// If there are PGXP vertices expected
	if (1)//(vertexIdx < numVertices)
	{
	float temp[4];
	memcpy(temp, pV, sizeof(float) * 4);

		//pre-multiply each element by w (to negate perspective divide)
		for (unsigned int i = 0; i < 3; i++)
			temp[i] *= temp[3];

		//pass complete vertex to OpenGL
		glVertex4fv(temp);
		vertexIdx++;

		//pV[3] = 1.f;
	}
	else
	{
		glVertex3fv(pV);
	}
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

	short*			pPrimData	= ((short*)addr) + 2;				// primitive data for cache lookups
	PGXP_vertex*	pCacheVert	= NULL;

	// Reset vertex count
	numVertices = count;
	vertexIdx = 0;

	// if PGXP is enabled
	if (PGXP_Mem != NULL)
	{
		// Offset to start of primitive
		primStart = &PGXP_Mem[currentAddr + 1];

		// Find any invalid vertices
		for (unsigned i = 0; i < count; ++i)
		{
			if (!primStart[stride * i].valid)
				invalidVert++;
		}
	}
	else
		invalidVert = count;

	for (unsigned i = 0; i < count; ++i)
	{
		if (primStart && primStart[stride * i].valid)
		{
			pVertex[i].x = (primStart[stride * i].x + xOffs);
			pVertex[i].y = (primStart[stride * i].y + yOffs);
			pVertex[i].z = 0.95f;
			pVertex[i].w = primStart[stride * i].z;
			pVertex[i].PGXP_flag = 1;
		}
		else
		{
			// Default to low precision vertex data
			pVertex[i].PGXP_flag = 2;

			// Look in cache for valid vertex
			pCacheVert = PGXP_GetCachedVertex(pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1]);
			if (pCacheVert)
			{
				if (IsSessionID(pCacheVert->count))
				{
					if (pCacheVert->valid == 1)
					{
						pVertex[i].x = (pCacheVert->x + xOffs);
						pVertex[i].y = (pCacheVert->y + yOffs);
						pVertex[i].z = 0.95f;
						pVertex[i].w = pCacheVert->z;
						pVertex[i].PGXP_flag = 3;
						// reduce number of invalid vertices
						invalidVert--;
					}
					else if(pCacheVert->valid > 1)
						pVertex[i].PGXP_flag = 4;
				}
			}

		//	if(PGXP_tDebug)
		//		__Log("GPPV: v:%x (%d, %d)|\n", (currentAddr + 1 + (i * stride))*4, pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1]);
		}
	}

	// If there are any invalid vertices set all w values to 1
	// iCB: Could use plane equation to find w for single invalid vertex in a quad
	if (invalidVert > 0)
		for (unsigned i = 0; i < count; ++i)
			pVertex[i].w = 1;

	return 1;
}

/////////////////////////////////
//// Visual Debugging Functions
/////////////////////////////////
unsigned int		PGXP_vDebug = 0;
const unsigned int	PGXP_maxDebug = 3;

const char red[4]	= { 255, 0, 0, 255 };
const char blue[4]	= { 0, 0, 255, 255 };
const char green[4]	= { 0, 255, 0, 255 };

const char yellow[4] = { 255, 255, 0, 255 };
const char magenta[4] = { 255, 0, 255, 255 };
const char black[4] = { 0, 0, 0, 255 };


//void CALLBACK GPUtoggleDebug(void)
//{
//	PGXP_tDebug = (PGXP_tDebug) ? 0 : 1;
//}

void CALLBACK GPUtoggleDebug(void)
{
	PGXP_vDebug++;

	if (PGXP_vDebug == PGXP_maxDebug)
		PGXP_vDebug = 0;
}

void PGXP_colour(OGLVertex* vertex)
{
	const char* pColour;
	float fDepth;

	switch (PGXP_vDebug)
	{
	case 1:
		// Vertex origin mode
		switch (vertex->PGXP_flag)
		{
		case 0:
			pColour = yellow;
			break;
		case 1:
			pColour = blue;
			break;
		case 2:
			pColour = red;
			break;
		case 3:
			pColour = green;
			break;
		case 4:
			pColour = magenta;
			break;
		default:
			pColour = black;
			break;
		}
		glColor4ubv(pColour);
		break;
	case 2:
		// W component visualisation
		fDepth = vertex->w / (float)(0xFFFF);
		glColor4f(fDepth, fDepth, fDepth, 1.f);
		break;
	}
}

void PGXP_DrawDebugTriQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
	GLboolean	bTexture = glIsEnabled(GL_TEXTURE_2D);
	GLfloat		fColour[4];
	GLint		iShadeModel;

	// Quit if PGXP_flag == ignore
	if ((vertex1->PGXP_flag == 5) ||
		(vertex2->PGXP_flag == 5) ||
		(vertex3->PGXP_flag == 5) ||
		(vertex4->PGXP_flag == 5))
		return;

	glGetIntegerv(GL_SHADE_MODEL, &iShadeModel);
	glGetFloatv(GL_CURRENT_COLOR, fColour);

	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glBegin(GL_TRIANGLE_STRIP);

	PGXP_colour(vertex1);
	PGXP_glVertexfv(&vertex1->x);

	PGXP_colour(vertex2);
	PGXP_glVertexfv(&vertex2->x);

	PGXP_colour(vertex3);
	PGXP_glVertexfv(&vertex3->x);

	PGXP_colour(vertex4);
	PGXP_glVertexfv(&vertex4->x);

	glEnd();

	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);

	glBegin(GL_TRIANGLE_STRIP);

	glColor4ubv(black);
	PGXP_glVertexfv(&vertex1->x);
	PGXP_glVertexfv(&vertex2->x);
	PGXP_glVertexfv(&vertex3->x);
	PGXP_glVertexfv(&vertex4->x);

	glColor4fv(fColour);

	glEnd();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	if(bTexture == GL_TRUE)
		glEnable(GL_TEXTURE_2D);

	glShadeModel(iShadeModel);
}

void PGXP_DrawDebugTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3)
{
	GLboolean	bTexture = glIsEnabled(GL_TEXTURE_2D);
	GLfloat		fColour[4];
	GLint		iShadeModel;

	// Quit if PGXP_flag == ignore
	if ((vertex1->PGXP_flag == 5) ||
		(vertex2->PGXP_flag == 5) ||
		(vertex3->PGXP_flag == 5))
		return;

	glGetIntegerv(GL_SHADE_MODEL, &iShadeModel);
	glGetFloatv(GL_CURRENT_COLOR, fColour);

	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glBegin(GL_TRIANGLES);

	PGXP_colour(vertex1);
	PGXP_glVertexfv(&vertex1->x);

	PGXP_colour(vertex2);
	PGXP_glVertexfv(&vertex2->x);

	PGXP_colour(vertex3);
	PGXP_glVertexfv(&vertex3->x);

	glEnd();

	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);

	glBegin(GL_TRIANGLE_STRIP);

	glColor4ubv(black);
	PGXP_glVertexfv(&vertex1->x);
	PGXP_glVertexfv(&vertex2->x);
	PGXP_glVertexfv(&vertex3->x);

	glColor4fv(fColour);

	glEnd();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	if (bTexture == GL_TRUE)
		glEnable(GL_TEXTURE_2D);

	glShadeModel(iShadeModel);
}

void PGXP_DrawDebugQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4)
{
	GLboolean	bTexture = glIsEnabled(GL_TEXTURE_2D);
	GLfloat		fColour[4];
	GLint		iShadeModel;

	// Quit if PGXP_flag == ignore
	if ((vertex1->PGXP_flag == 5) ||
		(vertex2->PGXP_flag == 5) ||
		(vertex3->PGXP_flag == 5) ||
		(vertex4->PGXP_flag == 5))
		return;

	glGetIntegerv(GL_SHADE_MODEL, &iShadeModel);
	glGetFloatv(GL_CURRENT_COLOR, fColour);

	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glBegin(GL_QUADS);

	PGXP_colour(vertex1);
	PGXP_glVertexfv(&vertex1->x);

	PGXP_colour(vertex2);
	PGXP_glVertexfv(&vertex2->x);

	PGXP_colour(vertex3);
	PGXP_glVertexfv(&vertex3->x);

	PGXP_colour(vertex4);
	PGXP_glVertexfv(&vertex4->x);

	glEnd();

	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);

	glBegin(GL_TRIANGLE_STRIP);

	glColor4ubv(black);
	PGXP_glVertexfv(&vertex1->x);
	PGXP_glVertexfv(&vertex2->x);
	PGXP_glVertexfv(&vertex3->x);
	PGXP_glVertexfv(&vertex4->x);

	glColor4fv(fColour);

	glEnd();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	if (bTexture == GL_TRUE)
		glEnable(GL_TEXTURE_2D);

	glShadeModel(iShadeModel);
}