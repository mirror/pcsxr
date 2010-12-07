/***************************************************************************
 gpu.h  -  description
 -------------------
 begin                : Sun Oct 28 2001
 copyright            : (C) 2001 by Pete Bernert
 email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef _GPU_INTERNALS_H
#define _GPU_INTERNALS_H

#include <gpu_utils.h>

#define OPAQUEON   10
#define OPAQUEOFF  11

#define KEY_RESETTEXSTORE 1
#define KEY_SHOWFPS       2
#define KEY_RESETOPAQUE   4
#define KEY_RESETDITHER   8
#define KEY_RESETFILTER   16
#define KEY_RESETADVBLEND 32
//#define KEY_BLACKWHITE    64
#define KEY_BADTEXTURES   128
#define KEY_CHECKTHISOUT  256

#if !defined(__BIG_ENDIAN__) || defined(__x86_64__) || defined(__i386__)
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#endif

#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)
#define COLOR(x) (x & 0xffffff)


#define INFO_TW        0
#define INFO_DRAWSTART 1
#define INFO_DRAWEND   2
#define INFO_DRAWOFF   3

#define SHADETEXBIT(x) ((x>>24) & 0x1)
#define SEMITRANSBIT(x) ((x>>25) & 0x1)
#define PSXRGB(r,g,b) ((g<<10)|(b<<5)|r)

#define DATAREGISTERMODES unsigned short

#define DR_NORMAL        0
#define DR_VRAMTRANSFER  1

#define STATUS_ODDLINES            0x80000000
#define STATUS_DMABITS             0x60000000 // Two bits
#define STATUS_READYFORCOMMANDS    0x10000000
#define STATUS_READYFORVRAM        0x08000000
#define STATUS_IDLE                0x04000000
#define STATUS_DISPLAYDISABLED     0x00800000
#define STATUS_INTERLACED          0x00400000
#define STATUS_RGB24               0x00200000
#define STATUS_PAL                 0x00100000
#define STATUS_DOUBLEHEIGHT        0x00080000
#define STATUS_WIDTHBITS           0x00070000 // Three bits
#define STATUS_MASKENABLED         0x00001000
#define STATUS_MASKDRAWN           0x00000800
#define STATUS_DRAWINGALLOWED      0x00000400
#define STATUS_DITHER              0x00000200

typedef struct {
	short x;
	short y;
	short Width;
	short Height;
	short RowsRemaining;
	short ColsRemaining;
	unsigned short *ImagePtr;
} gxv_vram_load_t;

typedef struct {
	gxv_vram_load_t VRAMWrite;
	gxv_vram_load_t VRAMRead;
	uint16_t DataWriteMode;
	uint16_t DataReadMode;
	int iColDepth;
	int iWindowMode;
	int bDebugText;

	gxv_display_t dsp;
	uint32_t status_reg;
	long lGPUdataRet;

	gxv_pointer_t psxVSecure;
	gxv_pointer_t psx_vram;
	gxv_pointer_t psxVuw_eom;
	uint32_t lGPUInfoVals[16];
	uint32_t ulStatusControl[256];

	uint32_t gpuDataM[256];
	unsigned char gpuCommand;
	long gpuDataC;
	long gpuDataP;

	int fps;

} gxv_gpu_t;

#endif // _GPU_INTERNALS_H
