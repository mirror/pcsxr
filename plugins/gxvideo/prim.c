/***************************************************************************
 prim.c  -  description
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
#include <stdio.h>

#include "globals.h"
#include "swap.h"

#define INFO_TW        0
#define INFO_DRAWSTART 1
#define INFO_DRAWEND   2
#define INFO_DRAWOFF   3

////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Some ASM color convertion by LEWPY
////////////////////////////////////////////////////////////////////////

#ifdef USE_NASM

#define BGR24to16 i386_BGR24to16
__inline unsigned short BGR24to16 (uint32_t BGR);

#else

static inline unsigned short BGR24to16(uint32_t BGR) {
	return (unsigned short) (((BGR >> 3) & 0x1f) | ((BGR & 0xf80000) >> 9)
			| ((BGR & 0xf800) >> 6));
}

#endif

////////////////////////////////////////////////////////////////////////
// Update global TP infos
////////////////////////////////////////////////////////////////////////

static inline void UpdateGlobalTP(unsigned short gdata) {
	g_soft.GlobalTextAddrX = (gdata << 6) & 0x3c0; // texture addr

	if (512 == 1024) {
		if (1 == 2) {
			g_soft.GlobalTextAddrY = ((gdata & 0x60) << 3);
			//g_zn.GlobalTextIL = (gdata & 0x2000) >> 13;
			g_soft.GlobalTextABR = (unsigned short) ((gdata >> 7) & 0x3);
			g_soft.GlobalTextTP = (gdata >> 9) & 0x3;
			if (g_soft.GlobalTextTP == 3)
				g_soft.GlobalTextTP = 2;
			g_prim.usMirror = 0;
			g_gpu.status_reg = (g_gpu.status_reg & 0xffffe000) | (gdata
					& 0x1fff);

			// tekken dithering? right now only if dithering is forced by user
			if (g_cfg.Dithering == 2)
				g_prim.iDither = 2;
			else
				g_prim.iDither = 0;

			return;
		} else {
			g_soft.GlobalTextAddrY = (unsigned short) (((gdata << 4) & 0x100)
					| ((gdata >> 2) & 0x200));
		}
	} else
		g_soft.GlobalTextAddrY = (gdata << 4) & 0x100;

	g_soft.GlobalTextTP = (gdata >> 7) & 0x3; // tex mode (4,8,15)

	if (g_soft.GlobalTextTP == 3)
		g_soft.GlobalTextTP = 2; // seen in Wild9 :(

	g_soft.GlobalTextABR = (gdata >> 5) & 0x3; // blend mode

	g_gpu.status_reg &= ~0x000001ff; // Clear the necessary bits
	g_gpu.status_reg |= (gdata & 0x01ff); // set the necessary bits

	switch (g_cfg.Dithering) {
	case 0:
		g_prim.iDither = 0;
		break;
	case 1:
		if (g_gpu.status_reg & 0x0200)
			g_prim.iDither = 2;
		else
			g_prim.iDither = 0;
		break;
	case 2:
		g_prim.iDither = 2;
		break;
	}
}

////////////////////////////////////////////////////////////////////////

static inline void SetRenderMode(uint32_t DrawAttributes) {
	g_soft.DrawSemiTrans = (SEMITRANSBIT(DrawAttributes)) ? 1 : 0;

	if (SHADETEXBIT(DrawAttributes)) {
		g_soft.g_m1 = g_soft.g_m2 = g_soft.g_m3 = 128;
	} else {
		if ((g_prim.dwActFixes & 4) && ((DrawAttributes & 0x00ffffff) == 0))
			DrawAttributes |= 0x007f7f7f;

		g_soft.g_m1 = (short) (DrawAttributes & 0xff);
		g_soft.g_m2 = (short) ((DrawAttributes >> 8) & 0xff);
		g_soft.g_m3 = (short) ((DrawAttributes >> 16) & 0xff);
	}
}

////////////////////////////////////////////////////////////////////////

// oki, here are the psx gpu coord rules: poly coords are
// 11 bit signed values (-1024...1023). If the x or y distance 
// exceeds 1024, the polygon will not be drawn. 
// Since quads are treated as two triangles by the real gpu,
// this 'discard rule' applies to each of the quad's triangle 
// (so one triangle can be drawn, the other one discarded). 
// Also, y drawing is wrapped at 512 one time,
// then it will get negative (and therefore not drawn). The
// 'CheckCoord' funcs are a simple (not comlete!) approach to
// do things right, I will add a better detection soon... the 
// current approach will be easier to do in hw/accel plugins, imho

// 11 bit signed
#define SIGNSHIFT 21
#define CHKMAX_X 1024
#define CHKMAX_Y 512

static void AdjustCoord4() {
	g_soft.lx0 = (short) (((int) g_soft.lx0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.lx1 = (short) (((int) g_soft.lx1 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.lx2 = (short) (((int) g_soft.lx2 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.lx3 = (short) (((int) g_soft.lx3 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly0 = (short) (((int) g_soft.ly0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly1 = (short) (((int) g_soft.ly1 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly2 = (short) (((int) g_soft.ly2 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly3 = (short) (((int) g_soft.ly3 << SIGNSHIFT) >> SIGNSHIFT);
}

static void AdjustCoord3() {
	g_soft.lx0 = (short) (((int) g_soft.lx0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.lx1 = (short) (((int) g_soft.lx1 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.lx2 = (short) (((int) g_soft.lx2 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly0 = (short) (((int) g_soft.ly0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly1 = (short) (((int) g_soft.ly1 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly2 = (short) (((int) g_soft.ly2 << SIGNSHIFT) >> SIGNSHIFT);
}

static void AdjustCoord2() {
	g_soft.lx0 = (short) (((int) g_soft.lx0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.lx1 = (short) (((int) g_soft.lx1 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly0 = (short) (((int) g_soft.ly0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly1 = (short) (((int) g_soft.ly1 << SIGNSHIFT) >> SIGNSHIFT);
}

static void AdjustCoord1() {
	g_soft.lx0 = (short) (((int) g_soft.lx0 << SIGNSHIFT) >> SIGNSHIFT);
	g_soft.ly0 = (short) (((int) g_soft.ly0 << SIGNSHIFT) >> SIGNSHIFT);

	if (g_soft.lx0 < -512 && g_gpu.dsp.DrawOffset.x <= -512)
		g_soft.lx0 += 2048;

	if (g_soft.ly0 < -512 && g_gpu.dsp.DrawOffset.y <= -512)
		g_soft.ly0 += 2048;
}

/*
 ////////////////////////////////////////////////////////////////////////
 // special checks... nascar, syphon filter 2, mgs
 ////////////////////////////////////////////////////////////////////////

 // xenogears FT4: not removed correctly right now... the tri 0,1,2
 // should get removed, the tri 1,2,3 should stay... pfff

 // x -466 1023 180 1023
 // y   20 -228 222 -100

 // 0 __1
 //  \ / \
 //   2___3
 */
static inline char CheckCoord4() {
	if (g_soft.lx0 < 0) {
		if (((g_soft.lx1 - g_soft.lx0) > CHKMAX_X)
				|| ((g_soft.lx2 - g_soft.lx0) > CHKMAX_X)) {
			if (g_soft.lx3 < 0) {
				if ((g_soft.lx1 - g_soft.lx3) > CHKMAX_X)
					return 1;
				if ((g_soft.lx2 - g_soft.lx3) > CHKMAX_X)
					return 1;
			}
		}
	}
	if (g_soft.lx1 < 0) {
		if ((g_soft.lx0 - g_soft.lx1) > CHKMAX_X)
			return 1;
		if ((g_soft.lx2 - g_soft.lx1) > CHKMAX_X)
			return 1;
		if ((g_soft.lx3 - g_soft.lx1) > CHKMAX_X)
			return 1;
	}
	if (g_soft.lx2 < 0) {
		if ((g_soft.lx0 - g_soft.lx2) > CHKMAX_X)
			return 1;
		if ((g_soft.lx1 - g_soft.lx2) > CHKMAX_X)
			return 1;
		if ((g_soft.lx3 - g_soft.lx2) > CHKMAX_X)
			return 1;
	}
	if (g_soft.lx3 < 0) {
		if (((g_soft.lx1 - g_soft.lx3) > CHKMAX_X)
				|| ((g_soft.lx2 - g_soft.lx3) > CHKMAX_X)) {
			if (g_soft.lx0 < 0) {
				if ((g_soft.lx1 - g_soft.lx0) > CHKMAX_X)
					return 1;
				if ((g_soft.lx2 - g_soft.lx0) > CHKMAX_X)
					return 1;
			}
		}
	}

	if (g_soft.ly0 < 0) {
		if ((g_soft.ly1 - g_soft.ly0) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly2 - g_soft.ly0) > CHKMAX_Y)
			return 1;
	}
	if (g_soft.ly1 < 0) {
		if ((g_soft.ly0 - g_soft.ly1) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly2 - g_soft.ly1) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly3 - g_soft.ly1) > CHKMAX_Y)
			return 1;
	}
	if (g_soft.ly2 < 0) {
		if ((g_soft.ly0 - g_soft.ly2) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly1 - g_soft.ly2) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly3 - g_soft.ly2) > CHKMAX_Y)
			return 1;
	}
	if (g_soft.ly3 < 0) {
		if ((g_soft.ly1 - g_soft.ly3) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly2 - g_soft.ly3) > CHKMAX_Y)
			return 1;
	}

	return 0;
}

static inline char CheckCoord3() {
	if (g_soft.lx0 < 0) {
		if ((g_soft.lx1 - g_soft.lx0) > CHKMAX_X)
			return 1;
		if ((g_soft.lx2 - g_soft.lx0) > CHKMAX_X)
			return 1;
	}
	if (g_soft.lx1 < 0) {
		if ((g_soft.lx0 - g_soft.lx1) > CHKMAX_X)
			return 1;
		if ((g_soft.lx2 - g_soft.lx1) > CHKMAX_X)
			return 1;
	}
	if (g_soft.lx2 < 0) {
		if ((g_soft.lx0 - g_soft.lx2) > CHKMAX_X)
			return 1;
		if ((g_soft.lx1 - g_soft.lx2) > CHKMAX_X)
			return 1;
	}
	if (g_soft.ly0 < 0) {
		if ((g_soft.ly1 - g_soft.ly0) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly2 - g_soft.ly0) > CHKMAX_Y)
			return 1;
	}
	if (g_soft.ly1 < 0) {
		if ((g_soft.ly0 - g_soft.ly1) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly2 - g_soft.ly1) > CHKMAX_Y)
			return 1;
	}
	if (g_soft.ly2 < 0) {
		if ((g_soft.ly0 - g_soft.ly2) > CHKMAX_Y)
			return 1;
		if ((g_soft.ly1 - g_soft.ly2) > CHKMAX_Y)
			return 1;
	}

	return 0;
}

static inline char CheckCoord2() {
	if (g_soft.lx0 < 0) {
		if ((g_soft.lx1 - g_soft.lx0) > CHKMAX_X)
			return 1;
	}
	if (g_soft.lx1 < 0) {
		if ((g_soft.lx0 - g_soft.lx1) > CHKMAX_X)
			return 1;
	}
	if (g_soft.ly0 < 0) {
		if ((g_soft.ly1 - g_soft.ly0) > CHKMAX_Y)
			return 1;
	}
	if (g_soft.ly1 < 0) {
		if ((g_soft.ly0 - g_soft.ly1) > CHKMAX_Y)
			return 1;
	}

	return 0;
}

static inline char CheckCoordL(short slx0, short sly0, short slx1, short sly1) {
	if (slx0 < 0) {
		if ((slx1 - slx0) > CHKMAX_X)
			return 1;
	}
	if (slx1 < 0) {
		if ((slx0 - slx1) > CHKMAX_X)
			return 1;
	}
	if (sly0 < 0) {
		if ((sly1 - sly0) > CHKMAX_Y)
			return 1;
	}
	if (sly1 < 0) {
		if ((sly0 - sly1) > CHKMAX_Y)
			return 1;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////
// mask stuff... used in silent hill
////////////////////////////////////////////////////////////////////////

static void cmdSTP(unsigned char * baseAddr) {
	uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

	g_gpu.status_reg &= ~0x1800; // Clear the necessary bits
	g_gpu.status_reg |= ((gdata & 0x03) << 11); // Set the necessary bits

	if (gdata & 1) {
		g_draw.sSetMask = 0x8000;
		g_draw.lSetMask = 0x80008000;
	} else {
		g_draw.sSetMask = 0;
		g_draw.lSetMask = 0;
	}

	if (gdata & 2)
		g_draw.bCheckMask = 1;
	else
		g_draw.bCheckMask = 0;
}

////////////////////////////////////////////////////////////////////////
// cmd: Set texture page infos
////////////////////////////////////////////////////////////////////////

static void cmdTexturePage(unsigned char * baseAddr) {
	uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

	g_gpu.status_reg &= ~0x000007ff;
	g_gpu.status_reg |= (gdata & 0x07ff);

	g_prim.usMirror = (unsigned short) (gdata & 0x3000);

	UpdateGlobalTP((unsigned short) gdata);
	g_soft.GlobalTextREST = (gdata & 0x00ffffff) >> 9;
}

////////////////////////////////////////////////////////////////////////
// cmd: turn on/off texture window
////////////////////////////////////////////////////////////////////////

static void cmdTextureWindow(unsigned char *baseAddr) {
	uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

	uint32_t YAlign, XAlign;

	g_gpu.lGPUInfoVals[INFO_TW] = gdata & 0xFFFFF;

	if (gdata & 0x020)
		g_prim.TWin.Position.y1 = 8; // xxxx1
	else if (gdata & 0x040)
		g_prim.TWin.Position.y1 = 16; // xxx10
	else if (gdata & 0x080)
		g_prim.TWin.Position.y1 = 32; // xx100
	else if (gdata & 0x100)
		g_prim.TWin.Position.y1 = 64; // x1000
	else if (gdata & 0x200)
		g_prim.TWin.Position.y1 = 128; // 10000
	else
		g_prim.TWin.Position.y1 = 256; // 00000

	// Texture window size is determined by the least bit set of the relevant 5 bits

	if (gdata & 0x001)
		g_prim.TWin.Position.x1 = 8; // xxxx1
	else if (gdata & 0x002)
		g_prim.TWin.Position.x1 = 16; // xxx10
	else if (gdata & 0x004)
		g_prim.TWin.Position.x1 = 32; // xx100
	else if (gdata & 0x008)
		g_prim.TWin.Position.x1 = 64; // x1000
	else if (gdata & 0x010)
		g_prim.TWin.Position.x1 = 128; // 10000
	else
		g_prim.TWin.Position.x1 = 256; // 00000

	// Re-calculate the bit field, because we can't trust what is passed in the data


	YAlign = (uint32_t) (32 - (g_prim.TWin.Position.y1 >> 3));
	XAlign = (uint32_t) (32 - (g_prim.TWin.Position.x1 >> 3));

	// Absolute position of the start of the texture window

	g_prim.TWin.Position.y0 = (short) (((gdata >> 15) & YAlign) << 3);
	g_prim.TWin.Position.x0 = (short) (((gdata >> 10) & XAlign) << 3);

	if ((g_prim.TWin.Position.x0 == 0 && // tw turned off
			g_prim.TWin.Position.y0 == 0 && g_prim.TWin.Position.x1 == 0
			&& g_prim.TWin.Position.y1 == 0) || (g_prim.TWin.Position.x1 == 256
			&& g_prim.TWin.Position.y1 == 256)) {
		g_prim.bUsingTWin = 0; // -> just do it
	} else // otherwise
	{
		g_prim.bUsingTWin = 1; // -> tw turned on
	}
}

////////////////////////////////////////////////////////////////////////
// cmd: start of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////


static void cmdDrawAreaStart(unsigned char * baseAddr) {
	uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);
	g_prim.drawX = gdata & 0x3ff; // for soft drawing
	g_gpu.lGPUInfoVals[INFO_DRAWSTART] = gdata & 0xFFFFF;
	g_prim.drawY = (gdata >> 10) & 0x3ff;
	if (g_prim.drawY >= 512)
		g_prim.drawY = 511; // some security
}

////////////////////////////////////////////////////////////////////////
// cmd: end of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////

static void cmdDrawAreaEnd(unsigned char * baseAddr) {
	uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

	g_prim.drawW = gdata & 0x3ff; // for soft drawing

	g_gpu.lGPUInfoVals[INFO_DRAWEND] = gdata & 0xFFFFF;
	g_prim.drawH = (gdata >> 10) & 0x3ff;
	if (g_prim.drawH >= 512)
		g_prim.drawH = 511; // some security
}

////////////////////////////////////////////////////////////////////////
// cmd: draw offset... will be added to prim coords
////////////////////////////////////////////////////////////////////////

static void cmdDrawOffset(unsigned char * baseAddr) {
	uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

	g_gpu.dsp.DrawOffset.x = (short) (gdata & 0x7ff);

	g_gpu.lGPUInfoVals[INFO_DRAWOFF] = gdata & 0x3FFFFF;
	g_gpu.dsp.DrawOffset.y = (short) ((gdata >> 11) & 0x7ff);

	g_gpu.dsp.DrawOffset.y = (short) (((int) g_gpu.dsp.DrawOffset.y << 21)
			>> 21);
	g_gpu.dsp.DrawOffset.x = (short) (((int) g_gpu.dsp.DrawOffset.x << 21)
			>> 21);
}

////////////////////////////////////////////////////////////////////////
// cmd: load image to vram
////////////////////////////////////////////////////////////////////////

static void primLoadImage(unsigned char * baseAddr) {
	unsigned short *sgpuData = ((unsigned short *) baseAddr);

	g_gpu.VRAMWrite.x = GETLEs16(&sgpuData[2]) & 0x3ff;
	g_gpu.VRAMWrite.y = GETLEs16(&sgpuData[3]) & 0x1ff;
	g_gpu.VRAMWrite.Width = GETLEs16(&sgpuData[4]);
	g_gpu.VRAMWrite.Height = GETLEs16(&sgpuData[5]);

	g_gpu.DataWriteMode = DR_VRAMTRANSFER;

	g_gpu.VRAMWrite.ImagePtr = g_gpu.psx_vram.u16 + (g_gpu.VRAMWrite.y << 10)
			+ g_gpu.VRAMWrite.x;
	g_gpu.VRAMWrite.RowsRemaining = g_gpu.VRAMWrite.Width;
	g_gpu.VRAMWrite.ColsRemaining = g_gpu.VRAMWrite.Height;
}

////////////////////////////////////////////////////////////////////////
// cmd: vram -> psx mem
////////////////////////////////////////////////////////////////////////

static void primStoreImage(unsigned char * baseAddr) {
	unsigned short *sgpuData = ((unsigned short *) baseAddr);

	g_gpu.VRAMRead.x = GETLEs16(&sgpuData[2]) & 0x03ff;
	g_gpu.VRAMRead.y = GETLEs16(&sgpuData[3]) & 0x01ff;
	g_gpu.VRAMRead.Width = GETLEs16(&sgpuData[4]);
	g_gpu.VRAMRead.Height = GETLEs16(&sgpuData[5]);

	g_gpu.VRAMRead.ImagePtr = g_gpu.psx_vram.u16 + (g_gpu.VRAMRead.y << 10)
			+ g_gpu.VRAMRead.x;
	g_gpu.VRAMRead.RowsRemaining = g_gpu.VRAMRead.Width;
	g_gpu.VRAMRead.ColsRemaining = g_gpu.VRAMRead.Height;

	g_gpu.DataReadMode = DR_VRAMTRANSFER;

	g_gpu.status_reg |= STATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// cmd: blkfill - NO primitive! Doesn't care about draw areas...
////////////////////////////////////////////////////////////////////////

static void primBlkFill(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	short sX = GETLEs16(&sgpuData[2]);
	short sY = GETLEs16(&sgpuData[3]);
	short sW = GETLEs16(&sgpuData[4]) & 0x3ff;
	short sH = GETLEs16(&sgpuData[5]) & 0x3ff;

	sW = (sW + 15) & ~15;

	// Increase H & W if they are one short of full values, because they never can be full values
	if (sH >= 1023)
		sH = 1024;
	if (sW >= 1023)
		sW = 1024;

	// x and y of end pos
	sW += sX;
	sH += sY;

	FillSoftwareArea(sX, sY, sW, sH, BGR24to16(GETLE32(&gpuData[0])));

}

////////////////////////////////////////////////////////////////////////
// cmd: move image vram -> vram
////////////////////////////////////////////////////////////////////////

static void primMoveImage(unsigned char * baseAddr) {
	short *sgpuData = ((short *) baseAddr);

	short imageY0, imageX0, imageY1, imageX1, imageSX, imageSY, i, j;

	imageX0 = GETLEs16(&sgpuData[2]) & 0x03ff;
	imageY0 = GETLEs16(&sgpuData[3]) & 0x01ff;
	imageX1 = GETLEs16(&sgpuData[4]) & 0x03ff;
	imageY1 = GETLEs16(&sgpuData[5]) & 0x01ff;
	imageSX = GETLEs16(&sgpuData[6]);
	imageSY = GETLEs16(&sgpuData[7]);

	if ((imageX0 == imageX1) && (imageY0 == imageY1))
		return;
	if (imageSX <= 0)
		return;
	if (imageSY <= 0)
		return;

	if ((imageY0 + imageSY) > 512 || (imageX0 + imageSX) > 1024 || (imageY1
			+ imageSY) > 512 || (imageX1 + imageSX) > 1024) {
		int i, j;
		for (j = 0; j < imageSY; j++)
			for (i = 0; i < imageSX; i++)
				g_gpu.psx_vram.u16[(1024 * ((imageY1 + j) & 0x01ff))
						+ ((imageX1 + i) & 0x3ff)] = g_gpu.psx_vram.u16[(1024
						* ((imageY0 + j) & 0x01ff)) + ((imageX0 + i) & 0x3ff)];

		return;
	}

	if (imageSX & 1) // not dword aligned? slower func
	{
		unsigned short *SRCPtr, *DSTPtr;
		unsigned short LineOffset;

		SRCPtr = g_gpu.psx_vram.u16 + (1024 * imageY0) + imageX0;
		DSTPtr = g_gpu.psx_vram.u16 + (1024 * imageY1) + imageX1;

		LineOffset = 1024 - imageSX;

		for (j = 0; j < imageSY; j++) {
			for (i = 0; i < imageSX; i++)
				*DSTPtr++ = *SRCPtr++;
			SRCPtr += LineOffset;
			DSTPtr += LineOffset;
		}
	} else // dword aligned
	{
		uint32_t *SRCPtr, *DSTPtr;
		unsigned short LineOffset;
		int dx = imageSX >> 1;

		SRCPtr = (uint32_t *) (g_gpu.psx_vram.u16 + (1024 * imageY0) + imageX0);
		DSTPtr = (uint32_t *) (g_gpu.psx_vram.u16 + (1024 * imageY1) + imageX1);

		LineOffset = 512 - dx;

		for (j = 0; j < imageSY; j++) {
			for (i = 0; i < dx; i++)
				*DSTPtr++ = *SRCPtr++;
			SRCPtr += LineOffset;
			DSTPtr += LineOffset;
		}
	}

	imageSX += imageX1;
	imageSY += imageY1;

}

////////////////////////////////////////////////////////////////////////
// cmd: draw free-size Tile 
////////////////////////////////////////////////////////////////////////
static void primTileS(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t*) baseAddr);
	short *sgpuData = ((short *) baseAddr);
	short sW = GETLEs16(&sgpuData[4]) & 0x3ff;
	short sH = GETLEs16(&sgpuData[5]) & 0x01ff; // mmm... limit tiles to 0x1ff or height?

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	// x and y of start
	g_soft.ly2 = g_soft.ly3 = g_soft.ly0 + sH + g_gpu.dsp.DrawOffset.y;
	g_soft.ly0 = g_soft.ly1 = g_soft.ly0 + g_gpu.dsp.DrawOffset.y;
	g_soft.lx1 = g_soft.lx2 = g_soft.lx0 + sW + g_gpu.dsp.DrawOffset.x;
	g_soft.lx0 = g_soft.lx3 = g_soft.lx0 + g_gpu.dsp.DrawOffset.x;

	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	if (!(0 && sH == 32 && GETLE32(&gpuData[0]) == 0x60ffffff)) // special cheat for certain ZiNc games
		FillSoftwareAreaTrans(g_soft.lx0, g_soft.ly0, g_soft.lx2, g_soft.ly2,
				BGR24to16(GETLE32(&gpuData[0])));

}

////////////////////////////////////////////////////////////////////////
// cmd: draw 1 dot Tile (point)
////////////////////////////////////////////////////////////////////////

static void primTile1(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t*) baseAddr);
	short *sgpuData = ((short *) baseAddr);
	short sH = 1;
	short sW = 1;

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	// x and y of start
	g_soft.ly2 = g_soft.ly3 = g_soft.ly0 + sH + g_gpu.dsp.DrawOffset.y;
	g_soft.ly0 = g_soft.ly1 = g_soft.ly0 + g_gpu.dsp.DrawOffset.y;
	g_soft.lx1 = g_soft.lx2 = g_soft.lx0 + sW + g_gpu.dsp.DrawOffset.x;
	g_soft.lx0 = g_soft.lx3 = g_soft.lx0 + g_gpu.dsp.DrawOffset.x;

	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	FillSoftwareAreaTrans(g_soft.lx0, g_soft.ly0, g_soft.lx2, g_soft.ly2,
			BGR24to16(GETLE32(&gpuData[0]))); // Takes Start and Offset

}

////////////////////////////////////////////////////////////////////////
// cmd: draw 8 dot Tile (small rect)
////////////////////////////////////////////////////////////////////////

static void primTile8(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t*) baseAddr);
	short *sgpuData = ((short *) baseAddr);
	short sH = 8;
	short sW = 8;

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	// x and y of start
	g_soft.ly2 = g_soft.ly3 = g_soft.ly0 + sH + g_gpu.dsp.DrawOffset.y;
	g_soft.ly0 = g_soft.ly1 = g_soft.ly0 + g_gpu.dsp.DrawOffset.y;
	g_soft.lx1 = g_soft.lx2 = g_soft.lx0 + sW + g_gpu.dsp.DrawOffset.x;
	g_soft.lx0 = g_soft.lx3 = g_soft.lx0 + g_gpu.dsp.DrawOffset.x;

	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	FillSoftwareAreaTrans(g_soft.lx0, g_soft.ly0, g_soft.lx2, g_soft.ly2,
			BGR24to16(GETLE32(&gpuData[0]))); // Takes Start and Offset

}

////////////////////////////////////////////////////////////////////////
// cmd: draw 16 dot Tile (medium rect)
////////////////////////////////////////////////////////////////////////

static void primTile16(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t*) baseAddr);
	short *sgpuData = ((short *) baseAddr);
	short sH = 16;
	short sW = 16;

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	// x and y of start
	g_soft.ly2 = g_soft.ly3 = g_soft.ly0 + sH + g_gpu.dsp.DrawOffset.y;
	g_soft.ly0 = g_soft.ly1 = g_soft.ly0 + g_gpu.dsp.DrawOffset.y;
	g_soft.lx1 = g_soft.lx2 = g_soft.lx0 + sW + g_gpu.dsp.DrawOffset.x;
	g_soft.lx0 = g_soft.lx3 = g_soft.lx0 + g_gpu.dsp.DrawOffset.x;

	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	FillSoftwareAreaTrans(g_soft.lx0, g_soft.ly0, g_soft.lx2, g_soft.ly2,
			BGR24to16(GETLE32(&gpuData[0]))); // Takes Start and Offset

}

////////////////////////////////////////////////////////////////////////
// cmd: small sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprt8(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	SetRenderMode(GETLE32(&gpuData[0]));

	if (g_prim.bUsingTWin)
		DrawSoftwareSpriteTWin(baseAddr, 8, 8);
	else if (g_prim.usMirror)
		DrawSoftwareSpriteMirror(baseAddr, 8, 8);
	else
		DrawSoftwareSprite(baseAddr, 8, 8, baseAddr[8], baseAddr[9]);

}

////////////////////////////////////////////////////////////////////////
// cmd: medium sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprt16(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	SetRenderMode(GETLE32(&gpuData[0]));

	if (g_prim.bUsingTWin)
		DrawSoftwareSpriteTWin(baseAddr, 16, 16);
	else if (g_prim.usMirror)
		DrawSoftwareSpriteMirror(baseAddr, 16, 16);
	else
		DrawSoftwareSprite(baseAddr, 16, 16, baseAddr[8], baseAddr[9]);

}

////////////////////////////////////////////////////////////////////////
// cmd: free-size sprite (textured rect)
////////////////////////////////////////////////////////////////////////

// func used on texture coord wrap
static void primSprtSRest(unsigned char * baseAddr, unsigned short type) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);
	unsigned short sTypeRest = 0;

	short s;
	short sX = GETLEs16(&sgpuData[2]);
	short sY = GETLEs16(&sgpuData[3]);
	short sW = GETLEs16(&sgpuData[6]) & 0x3ff;
	short sH = GETLEs16(&sgpuData[7]) & 0x1ff;
	short tX = baseAddr[8];
	short tY = baseAddr[9];

	switch (type) {
	case 1:
		s = 256 - baseAddr[8];
		sW -= s;
		sX += s;
		tX = 0;
		break;
	case 2:
		s = 256 - baseAddr[9];
		sH -= s;
		sY += s;
		tY = 0;
		break;
	case 3:
		s = 256 - baseAddr[8];
		sW -= s;
		sX += s;
		tX = 0;
		s = 256 - baseAddr[9];
		sH -= s;
		sY += s;
		tY = 0;
		break;
	case 4:
		s = 512 - baseAddr[8];
		sW -= s;
		sX += s;
		tX = 0;
		break;
	case 5:
		s = 512 - baseAddr[9];
		sH -= s;
		sY += s;
		tY = 0;
		break;
	case 6:
		s = 512 - baseAddr[8];
		sW -= s;
		sX += s;
		tX = 0;
		s = 512 - baseAddr[9];
		sH -= s;
		sY += s;
		tY = 0;
		break;
	}

	SetRenderMode(GETLE32(&gpuData[0]));

	if (tX + sW > 256) {
		sW = 256 - tX;
		sTypeRest += 1;
	}
	if (tY + sH > 256) {
		sH = 256 - tY;
		sTypeRest += 2;
	}

	g_soft.lx0 = sX;
	g_soft.ly0 = sY;

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	DrawSoftwareSprite(baseAddr, sW, sH, tX, tY);

	if (sTypeRest && type < 4) {
		if (sTypeRest & 1 && type == 1)
			primSprtSRest(baseAddr, 4);
		if (sTypeRest & 2 && type == 2)
			primSprtSRest(baseAddr, 5);
		if (sTypeRest == 3 && type == 3)
			primSprtSRest(baseAddr, 6);
	}

}

////////////////////////////////////////////////////////////////////////

static void primSprtS(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);
	short sW, sH;

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);

	if (!(g_prim.dwActFixes & 8))
		AdjustCoord1();

	sW = GETLEs16(&sgpuData[6]) & 0x3ff;
	sH = GETLEs16(&sgpuData[7]) & 0x1ff;

	SetRenderMode(GETLE32(&gpuData[0]));

	if (g_prim.bUsingTWin)
		DrawSoftwareSpriteTWin(baseAddr, sW, sH);
	else if (g_prim.usMirror)
		DrawSoftwareSpriteMirror(baseAddr, sW, sH);
	else {
		unsigned short sTypeRest = 0;
		short tX = baseAddr[8];
		short tY = baseAddr[9];

		if (tX + sW > 256) {
			sW = 256 - tX;
			sTypeRest += 1;
		}
		if (tY + sH > 256) {
			sH = 256 - tY;
			sTypeRest += 2;
		}

		DrawSoftwareSprite(baseAddr, sW, sH, tX, tY);

		if (sTypeRest) {
			if (sTypeRest & 1)
				primSprtSRest(baseAddr, 1);
			if (sTypeRest & 2)
				primSprtSRest(baseAddr, 2);
			if (sTypeRest == 3)
				primSprtSRest(baseAddr, 3);
		}

	}

}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Poly4
////////////////////////////////////////////////////////////////////////

static void primPolyF4(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[4]);
	g_soft.ly1 = GETLEs16(&sgpuData[5]);
	g_soft.lx2 = GETLEs16(&sgpuData[6]);
	g_soft.ly2 = GETLEs16(&sgpuData[7]);
	g_soft.lx3 = GETLEs16(&sgpuData[8]);
	g_soft.ly3 = GETLEs16(&sgpuData[9]);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord4();
		if (CheckCoord4())
			return;
	}

	offsetPSX4();
	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	drawPoly4F(GETLE32(&gpuData[0]));

}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly4
////////////////////////////////////////////////////////////////////////

static void primPolyG4(unsigned char * baseAddr) {
	uint32_t *gpuData = (uint32_t *) baseAddr;
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[6]);
	g_soft.ly1 = GETLEs16(&sgpuData[7]);
	g_soft.lx2 = GETLEs16(&sgpuData[10]);
	g_soft.ly2 = GETLEs16(&sgpuData[11]);
	g_soft.lx3 = GETLEs16(&sgpuData[14]);
	g_soft.ly3 = GETLEs16(&sgpuData[15]);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord4();
		if (CheckCoord4())
			return;
	}

	offsetPSX4();
	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	drawPoly4G(GETLE32(&gpuData[0]), GETLE32(&gpuData[2]),
			GETLE32(&gpuData[4]), GETLE32(&gpuData[6]));

}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture3
////////////////////////////////////////////////////////////////////////

static void primPolyFT3(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[6]);
	g_soft.ly1 = GETLEs16(&sgpuData[7]);
	g_soft.lx2 = GETLEs16(&sgpuData[10]);
	g_soft.ly2 = GETLEs16(&sgpuData[11]);

	g_draw.lLowerpart = GETLE32(&gpuData[4]) >> 16;
	UpdateGlobalTP((unsigned short) g_draw.lLowerpart);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord3();
		if (CheckCoord3())
			return;
	}

	offsetPSX3();
	SetRenderMode(GETLE32(&gpuData[0]));

	drawPoly3FT(baseAddr);

}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture4
////////////////////////////////////////////////////////////////////////

static void primPolyFT4(unsigned char * baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[6]);
	g_soft.ly1 = GETLEs16(&sgpuData[7]);
	g_soft.lx2 = GETLEs16(&sgpuData[10]);
	g_soft.ly2 = GETLEs16(&sgpuData[11]);
	g_soft.lx3 = GETLEs16(&sgpuData[14]);
	g_soft.ly3 = GETLEs16(&sgpuData[15]);

	g_draw.lLowerpart = GETLE32(&gpuData[4]) >> 16;
	UpdateGlobalTP((unsigned short) g_draw.lLowerpart);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord4();
		if (CheckCoord4())
			return;
	}

	offsetPSX4();

	SetRenderMode(GETLE32(&gpuData[0]));

	drawPoly4FT(baseAddr);

}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture3
////////////////////////////////////////////////////////////////////////

static void primPolyGT3(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[8]);
	g_soft.ly1 = GETLEs16(&sgpuData[9]);
	g_soft.lx2 = GETLEs16(&sgpuData[14]);
	g_soft.ly2 = GETLEs16(&sgpuData[15]);

	g_draw.lLowerpart = GETLE32(&gpuData[5]) >> 16;
	UpdateGlobalTP((unsigned short) g_draw.lLowerpart);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord3();
		if (CheckCoord3())
			return;
	}

	offsetPSX3();
	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	if (SHADETEXBIT(GETLE32(&gpuData[0]))) {
		gpuData[0] = (gpuData[0] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
		gpuData[3] = (gpuData[3] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
		gpuData[6] = (gpuData[6] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
	}

	drawPoly3GT(baseAddr);

}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

static void primPolyG3(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[6]);
	g_soft.ly1 = GETLEs16(&sgpuData[7]);
	g_soft.lx2 = GETLEs16(&sgpuData[10]);
	g_soft.ly2 = GETLEs16(&sgpuData[11]);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord3();
		if (CheckCoord3())
			return;
	}

	offsetPSX3();
	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	drawPoly3G(GETLE32(&gpuData[0]), GETLE32(&gpuData[2]), GETLE32(&gpuData[4]));

}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture4
////////////////////////////////////////////////////////////////////////

static void primPolyGT4(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[8]);
	g_soft.ly1 = GETLEs16(&sgpuData[9]);
	g_soft.lx2 = GETLEs16(&sgpuData[14]);
	g_soft.ly2 = GETLEs16(&sgpuData[15]);
	g_soft.lx3 = GETLEs16(&sgpuData[20]);
	g_soft.ly3 = GETLEs16(&sgpuData[21]);

	g_draw.lLowerpart = GETLE32(&gpuData[5]) >> 16;
	UpdateGlobalTP((unsigned short) g_draw.lLowerpart);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord4();
		if (CheckCoord4())
			return;
	}

	offsetPSX4();
	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	if (SHADETEXBIT(GETLE32(&gpuData[0]))) {
		gpuData[0] = (gpuData[0] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
		gpuData[3] = (gpuData[3] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
		gpuData[6] = (gpuData[6] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
		gpuData[9] = (gpuData[9] & HOST2LE32(0xff000000))
				|HOST2LE32(0x00808080);
	}

	drawPoly4GT(baseAddr);

}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

static void primPolyF3(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[4]);
	g_soft.ly1 = GETLEs16(&sgpuData[5]);
	g_soft.lx2 = GETLEs16(&sgpuData[6]);
	g_soft.ly2 = GETLEs16(&sgpuData[7]);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord3();
		if (CheckCoord3())
			return;
	}

	offsetPSX3();
	SetRenderMode(GETLE32(&gpuData[0]));

	drawPoly3F(GETLE32(&gpuData[0]));

}

////////////////////////////////////////////////////////////////////////
// cmd: skipping shaded polylines
////////////////////////////////////////////////////////////////////////

#if 0
static void primLineGSkip(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	int iMax = 255;
	int i = 2;

	g_soft.ly1 = (short) ((GETLE32(&gpuData[1]) >> 16) & 0xffff);
	g_soft.lx1 = (short) (GETLE32(&gpuData[1]) & 0xffff);

	while (!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i >= 4)) {
		i++;
		g_soft.ly1 = (short) ((GETLE32(&gpuData[i]) >> 16) & 0xffff);
		g_soft.lx1 = (short) (GETLE32(&gpuData[i]) & 0xffff);
		i++;
		if (i > iMax)
			break;
	}
}
#endif

////////////////////////////////////////////////////////////////////////
// cmd: shaded polylines
////////////////////////////////////////////////////////////////////////

static void primLineGEx(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	int iMax = 255;
	uint32_t lc0, lc1;
	short slx0, slx1, sly0, sly1;
	int i = 2;
	char bDraw = 1;

	sly1 = (short) ((GETLE32(&gpuData[1]) >> 16) & 0xffff);
	slx1 = (short) (GETLE32(&gpuData[1]) & 0xffff);

	if (!(g_prim.dwActFixes & 8)) {
		slx1 = (short) (((int) slx1 << SIGNSHIFT) >> SIGNSHIFT);
		sly1 = (short) (((int) sly1 << SIGNSHIFT) >> SIGNSHIFT);
	}

	lc1 = gpuData[0] & 0xffffff;

	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;

	while (!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i >= 4)) {
		sly0 = sly1;
		slx0 = slx1;
		lc0 = lc1;
		lc1 = GETLE32(&gpuData[i]) & 0xffffff;

		i++;

		// no check needed on gshaded polyline positions
		// if((gpuData[i] & 0xF000F000) == 0x50005000) break;

		sly1 = (short) ((GETLE32(&gpuData[i]) >> 16) & 0xffff);
		slx1 = (short) (GETLE32(&gpuData[i]) & 0xffff);

		if (!(g_prim.dwActFixes & 8)) {
			slx1 = (short) (((int) slx1 << SIGNSHIFT) >> SIGNSHIFT);
			sly1 = (short) (((int) sly1 << SIGNSHIFT) >> SIGNSHIFT);
			if (CheckCoordL(slx0, sly0, slx1, sly1))
				bDraw = 0;
			else
				bDraw = 1;
		}

		if ((g_soft.lx0 != g_soft.lx1) || (g_soft.ly0 != g_soft.ly1)) {
			g_soft.ly0 = sly0;
			g_soft.lx0 = slx0;
			g_soft.ly1 = sly1;
			g_soft.lx1 = slx1;

			offsetPSX2();
			if (bDraw)
				DrawSoftwareLineShade(lc0, lc1);
		}
		i++;
		if (i > iMax)
			break;
	}

}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polyline2
////////////////////////////////////////////////////////////////////////

static void primLineG2(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[6]);
	g_soft.ly1 = GETLEs16(&sgpuData[7]);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord2();
		if (CheckCoord2())
			return;
	}

	if ((g_soft.lx0 == g_soft.lx1) && (g_soft.ly0 == g_soft.ly1)) {
		g_soft.lx1++;
		g_soft.ly1++;
	}

	g_soft.DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? 1 : 0;
	offsetPSX2();
	DrawSoftwareLineShade(GETLE32(&gpuData[0]), GETLE32(&gpuData[2]));

}

////////////////////////////////////////////////////////////////////////
// cmd: skipping flat polylines
////////////////////////////////////////////////////////////////////////

#if 0
static void primLineFSkip(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	int i = 2, iMax = 255;

	g_soft.ly1 = (short) ((GETLE32(&gpuData[1]) >> 16) & 0xffff);
	g_soft.lx1 = (short) (GETLE32(&gpuData[1]) & 0xffff);

	while (!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i >= 3)) {
		g_soft.ly1 = (short) ((GETLE32(&gpuData[i]) >> 16) & 0xffff);
		g_soft.lx1 = (short) (GETLE32(&gpuData[i]) & 0xffff);
		i++;
		if (i > iMax)
			break;
	}
}
#endif

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polylines
////////////////////////////////////////////////////////////////////////

static void primLineFEx(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	int iMax;
	short slx0, slx1, sly0, sly1;
	int i = 2;
	char bDraw = 1;

	iMax = 255;

	sly1 = (short) ((GETLE32(&gpuData[1]) >> 16) & 0xffff);
	slx1 = (short) (GETLE32(&gpuData[1]) & 0xffff);
	if (!(g_prim.dwActFixes & 8)) {
		slx1 = (short) (((int) slx1 << SIGNSHIFT) >> SIGNSHIFT);
		sly1 = (short) (((int) sly1 << SIGNSHIFT) >> SIGNSHIFT);
	}

	SetRenderMode(GETLE32(&gpuData[0]));

	while (!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i >= 3)) {
		sly0 = sly1;
		slx0 = slx1;
		sly1 = (short) ((GETLE32(&gpuData[i]) >> 16) & 0xffff);
		slx1 = (short) (GETLE32(&gpuData[i]) & 0xffff);
		if (!(g_prim.dwActFixes & 8)) {
			slx1 = (short) (((int) slx1 << SIGNSHIFT) >> SIGNSHIFT);
			sly1 = (short) (((int) sly1 << SIGNSHIFT) >> SIGNSHIFT);

			if (CheckCoordL(slx0, sly0, slx1, sly1))
				bDraw = 0;
			else
				bDraw = 1;
		}

		g_soft.ly0 = sly0;
		g_soft.lx0 = slx0;
		g_soft.ly1 = sly1;
		g_soft.lx1 = slx1;

		offsetPSX2();
		if (bDraw)
			DrawSoftwareLineFlat(GETLE32(&gpuData[0]));

		i++;
		if (i > iMax)
			break;
	}

}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polyline2
////////////////////////////////////////////////////////////////////////

static void primLineF2(unsigned char *baseAddr) {
	uint32_t *gpuData = ((uint32_t *) baseAddr);
	short *sgpuData = ((short *) baseAddr);

	g_soft.lx0 = GETLEs16(&sgpuData[2]);
	g_soft.ly0 = GETLEs16(&sgpuData[3]);
	g_soft.lx1 = GETLEs16(&sgpuData[4]);
	g_soft.ly1 = GETLEs16(&sgpuData[5]);

	if (!(g_prim.dwActFixes & 8)) {
		AdjustCoord2();
		if (CheckCoord2())
			return;
	}

	if ((g_soft.lx0 == g_soft.lx1) && (g_soft.ly0 == g_soft.ly1)) {
		g_soft.lx1++;
		g_soft.ly1++;
	}

	offsetPSX2();
	SetRenderMode(GETLE32(&gpuData[0]));

	DrawSoftwareLineFlat(GETLE32(&gpuData[0]));

}

////////////////////////////////////////////////////////////////////////
// cmd: well, easiest command... not implemented
////////////////////////////////////////////////////////////////////////

static void primNI(unsigned char *bA) {
	fprintf(stderr, "Unknow GPU func \n");
}

////////////////////////////////////////////////////////////////////////
// cmd func ptr table
////////////////////////////////////////////////////////////////////////


void (*primTableJ[256])(unsigned char *) = {
		/* 00 */
		primNI,
		primNI,
		primBlkFill,
		primNI,
		/* 04 */
		primNI,
		primNI,
		primNI,
		primNI,
		/* 08 */
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		// 10
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		// 18
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		primNI,
		// 20
		primPolyF3,
		primPolyF3,
		primPolyF3,
		primPolyF3,
		primPolyFT3,
		primPolyFT3,
		primPolyFT3,
		primPolyFT3,
		// 28
		primPolyF4,
		primPolyF4,
		primPolyF4,
		primPolyF4,
		primPolyFT4,
		primPolyFT4,
		primPolyFT4,
		primPolyFT4,
		// 30
		primPolyG3,
		primPolyG3,
		primPolyG3,
		primPolyG3,
		primPolyGT3,
		primPolyGT3,
		primPolyGT3,
		primPolyGT3,
		// 38
		primPolyG4,
		primPolyG4,
		primPolyG4,
		primPolyG4,
		primPolyGT4,
		primPolyGT4,
		primPolyGT4,
		primPolyGT4,
		// 40
		primLineF2,
		primLineF2,
		primLineF2,
		primLineF2,
		primNI,
		primNI,
		primNI,
		primNI,
		// 48
		primLineFEx,
		primLineFEx,
		primLineFEx,
		primLineFEx,
		primLineFEx,
		primLineFEx,
		primLineFEx,
		primLineFEx,
		// 50
		primLineG2, primLineG2,
		primLineG2,
		primLineG2,
		primNI,
		primNI,
		primNI,
		primNI,
		// 58
		primLineGEx, primLineGEx, primLineGEx,
		primLineGEx,
		primLineGEx,
		primLineGEx,
		primLineGEx,
		primLineGEx,
		// 60
		primTileS, primTileS, primTileS, primTileS,
		primSprtS,
		primSprtS,
		primSprtS,
		primSprtS,
		// 68
		primTile1, primTile1, primTile1, primTile1, primNI,
		primNI,
		primNI,
		primNI,
		// 70
		primTile8, primTile8, primTile8, primTile8, primSprt8, primSprt8,
		primSprt8,
		primSprt8,
		// 78
		primTile16, primTile16, primTile16, primTile16, primSprt16, primSprt16,
		primSprt16, primSprt16,
		// 80
		primMoveImage, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// 88
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// 90
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// 98
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// a0
		primLoadImage, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// a8
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// b0
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// b8
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// c0
		primStoreImage, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// c8
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// d0
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// d8
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// e0
		primNI, cmdTexturePage, cmdTextureWindow, cmdDrawAreaStart,
		cmdDrawAreaEnd, cmdDrawOffset, cmdSTP, primNI,
		// e8
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// f0
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI,
		// f8
		primNI, primNI, primNI, primNI, primNI, primNI, primNI, primNI };

