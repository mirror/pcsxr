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
*	pgxp_gpu.h
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 25 Mar 2016
*      Author: iCatButler
***************************************************************************/

#ifndef _PGXP_GPU_H_
#define _PGXP_GPU_H_

#include "stdafx.h"

//struct OGLVertex;

struct OGLVertexTag;
typedef struct OGLVertexTag OGLVertex;

void	PGXP_SetMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
void	PGXP_SetAddress(unsigned int addr, uint32_t *baseAddrL, int size);
void	PGXP_SetDepth(unsigned int addr);
int		PGXP_GetVertices(unsigned int* addr, void* pOutput, int xOffs, int yOffs);
void	PGXP_glVertexfv(GLfloat* pVertex);

#define COLOUR_NONE		0
#define COLOUR_FLAT		1
#define COLOUR_SMOOTH	2

extern unsigned int PGXP_vDebug;
extern unsigned int PGXP_debugFlags[4];
int PGXP_DrawDebugTriQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4, int colourMode, int isTextured);
int PGXP_DrawDebugTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, int colourMode, int isTextured);
int PGXP_DrawDebugQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4, int colourMode, int isTextured);

#endif // _PGXP_GPU_H_
