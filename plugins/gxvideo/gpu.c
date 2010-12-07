/*
 * Copyright (C) 2010 Benoit Gschwind
 * Inspired by original author : Pete Bernert
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "gpu.h"
#include "swap.h"

#define CALLBACK

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

#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) < (b) ? (b) : (a))

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

////////////////////////////////////////////////////////////////////////
// PPDK developer must change libraryName field and can change revision and build
////////////////////////////////////////////////////////////////////////

unsigned char const version = 1; // do not touch - library for PSEmu 1.x
unsigned char const revision = 0;
unsigned char const build = 0; // increase that with each version

static char const * libraryName = N_("GXVideo Driver");
static char const * libraryInfo = N_("GXvideo Driver v1.00");
//static char const * PluginAuthor =
//		N_("gschwind (rewrite from Pete Bernert and the P.E.Op.S. team)");

////////////////////////////////////////////////////////////////////////
// some misc external display funcs
////////////////////////////////////////////////////////////////////////

#include <time.h>

//void CALLBACK GPUdisplayText(char * pText) {
//
//}

////////////////////////////////////////////////////////////////////////

//void CALLBACK GPUdisplayFlags(unsigned long dwFlags) // some info func
//{
//dwCoreFlags = dwFlags;
//BuildDispMenu(0);
//}

////////////////////////////////////////////////////////////////////////
// stuff to make this a true PDK module
////////////////////////////////////////////////////////////////////////

char * CALLBACK PSEgetLibName(void) {
	return _(libraryName);
}

unsigned long CALLBACK PSEgetLibType(void) {
	//return PSE_LT_GPU;
	return 2; /* GPU plugin */
}

unsigned long CALLBACK PSEgetLibVersion(void) {
	return version << 16 | revision << 8 | build;
}

char * GPUgetLibInfos(void) {
	return _(libraryInfo);
}

////////////////////////////////////////////////////////////////////////
// Snapshot func
////////////////////////////////////////////////////////////////////////

//char * pGetConfigInfos(int iCfg) {
//	char szO[2][4] = { "off", "on " };
//	char szTxt[256];
//	char * pB = (char *) malloc(32767);
//
//	if (!pB)
//		return NULL;
//	*pB = 0;
//	//----------------------------------------------------//
//	sprintf(szTxt, "Plugin: %s %d.%d.%d\r\n", libraryName, version, revision,
//			build);
//	strcat(pB, szTxt);
//	sprintf(szTxt, "Author: %s\r\n\r\n", PluginAuthor);
//	strcat(pB, szTxt);
//	//----------------------------------------------------//
//
//	sprintf(szTxt, "Resolution/Color:\r\n- %dx%d ", g_cfg.ResX, g_cfg.ResY);
//
//	strcat(pB, szTxt);
//	if (iWindowMode && iCfg)
//		strcpy(szTxt, "Window mode\r\n");
//	else if (iWindowMode)
//		sprintf(szTxt, "Window mode - [%d Bit]\r\n", iDesktopCol);
//	else
//		sprintf(szTxt, "Fullscreen - [%d Bit]\r\n", iColDepth);
//	strcat(pB, szTxt);
//
//	sprintf(szTxt, "Stretch mode: %d\r\n", iUseNoStretchBlt);
//	strcat(pB, szTxt);
//	sprintf(szTxt, "Dither mode: %d\r\n\r\n", iUseDither);
//	strcat(pB, szTxt);
//	//----------------------------------------------------//
//	sprintf(szTxt, "Framerate:\r\n- FPS limit: %s\r\n", szO[UseFrameLimit]);
//	strcat(pB, szTxt);
//	sprintf(szTxt, "- Frame skipping: %s", szO[UseFrameSkip]);
//	strcat(pB, szTxt);
//	if (iFastFwd)
//		strcat(pB, " (fast forward)");
//	strcat(pB, "\r\n");
//	if (iFrameLimit == 2)
//		strcpy(szTxt, "- FPS limit: Auto\r\n\r\n");
//	else
//		sprintf(szTxt, "- FPS limit: %.1f\r\n\r\n", fFrameRate);
//	strcat(pB, szTxt);
//	//----------------------------------------------------//
//#if !defined (_MACGL) && !defined (_WINDOWS)
//	strcpy(szTxt, "Misc:\r\n- MaintainAspect: ");
//	if (iMaintainAspect == 0)
//		strcat(szTxt, "disabled");
//	else if (iMaintainAspect == 1)
//		strcat(szTxt, "enabled");
//	strcat(szTxt, "\r\n");
//	strcat(pB, szTxt);
//#endif
//	sprintf(szTxt, "- Game fixes: %s [%08x]\r\n", szO[iUseFixes], dwCfgFixes);
//	strcat(pB, szTxt);
//	//----------------------------------------------------//
//	return pB;
//	return 0;
//}

void CALLBACK GPUmakeSnapshot(void) {
	//	FILE *bmpfile;
	//	char filename[256];
	//	unsigned char header[0x36];
	//	long size, height;
	//	unsigned char line[1024 * 3];
	//	short i, j;
	//	unsigned char empty[2] = { 0, 0 };
	//	unsigned short color;
	//	unsigned long snapshotnr = 0;
	//	unsigned char *pD;
	//
	//	height = gpu_ctx.PreviousPSXDisplay.DisplayMode.y;
	//
	//	size = height * PreviousPSXDisplay.Range.x1 * 3 + 0x38;
	//
	//	// fill in proper values for BMP
	//
	//	// hardcoded BMP header
	//	memset(header, 0, 0x36);
	//	header[0] = 'B';
	//	header[1] = 'M';
	//	header[2] = size & 0xff;
	//	header[3] = (size >> 8) & 0xff;
	//	header[4] = (size >> 16) & 0xff;
	//	header[5] = (size >> 24) & 0xff;
	//	header[0x0a] = 0x36;
	//	header[0x0e] = 0x28;
	//	header[0x12] = gpu_ctx.PreviousPSXDisplay.Range.x1 % 256;
	//	header[0x13] = gpu_ctx.PreviousPSXDisplay.Range.x1 / 256;
	//	header[0x16] = height % 256;
	//	header[0x17] = height / 256;
	//	header[0x1a] = 0x01;
	//	header[0x1c] = 0x18;
	//	header[0x26] = 0x12;
	//	header[0x27] = 0x0B;
	//	header[0x2A] = 0x12;
	//	header[0x2B] = 0x0B;
	//
	//	// increment snapshot value & try to get filename
	//	do {
	//		snapshotnr++;
	//#ifdef _WINDOWS
	//		sprintf(filename,"snap\\pcsx%04ld.bmp",snapshotnr);
	//#else
	//		sprintf(filename, "%s/pcsx%04ld.bmp", getenv("HOME"), snapshotnr);
	//#endif
	//
	//		bmpfile = fopen(filename, "rb");
	//		if (bmpfile == NULL)
	//			break;
	//
	//		fclose(bmpfile);
	//	} while (TRUE);
	//
	//	// try opening new snapshot file
	//	if ((bmpfile = fopen(filename, "wb")) == NULL)
	//		return;
	//
	//	fwrite(header, 0x36, 1, bmpfile);
	//	for (i = height + PSXDisplay.DisplayPosition.y - 1; i
	//			>= PSXDisplay.DisplayPosition.y; i--) {
	//		pD = (unsigned char *) &psxVuw[i * 1024 + PSXDisplay.DisplayPosition.x];
	//		for (j = 0; j < PreviousPSXDisplay.Range.x1; j++) {
	//			if (PSXDisplay.RGB24) {
	//				uint32_t lu = *(uint32_t *) pD;
	//				line[j * 3 + 2] = (unsigned char) RED(lu);
	//				line[j * 3 + 1] = (unsigned char) GREEN(lu);
	//				line[j * 3 + 0] = (unsigned char) BLUE(lu);
	//				pD += 3;
	//			} else {
	//				color = GETLE16(pD);
	//				line[j * 3 + 2] = (color << 3) & 0xf1;
	//				line[j * 3 + 1] = (color >> 2) & 0xf1;
	//				line[j * 3 + 0] = (color >> 7) & 0xf1;
	//				pD += 2;
	//			}
	//		}
	//		fwrite(line, PreviousPSXDisplay.Range.x1 * 3, 1, bmpfile);
	//	}
	//	fwrite(empty, 0x2, 1, bmpfile);
	//	fclose(bmpfile);
	//
	//	DoTextSnapShot(snapshotnr);
}

////////////////////////////////////////////////////////////////////////
// INIT, will be called after lib load... well, just do some var init...
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUinit() {
	fprintf(stderr, "%s\n", __FUNCTION__);
	memset(g_gpu.ulStatusControl, 0, 256 * sizeof(uint32_t)); // init save state scontrol field
	g_gpu.psxVSecure.u8 = (uint8_t *) malloc((512 * 2) * 1024 + (1024 * 1024)); // always alloc one extra MB for soft drawing funcs security
	if (!g_gpu.psxVSecure.u8)
		return -1;

	//!!! ATTENTION !!!
	g_gpu.psx_vram.u8 = g_gpu.psxVSecure.u8 + 512 * 1024; // security offset into double sized psx vram!

	g_gpu.psxVuw_eom.u16 = g_gpu.psx_vram.u16 + 1024 * 512; // pre-calc of end of vram

	memset(g_gpu.psxVSecure.s8, 0x00, (512 * 2) * 1024 + (1024 * 1024));
	memset(g_gpu.lGPUInfoVals, 0x00, 16 * sizeof(uint32_t));

	g_gpu.dsp.range.x0 = 0;
	g_gpu.dsp.range.x1 = 0;
	g_gpu.lGPUdataRet = 0x400;

	g_gpu.DataWriteMode = DR_NORMAL;

	// Reset transfer values, to prevent mis-transfer of data
	memset(&g_gpu.VRAMWrite, 0, sizeof(gxv_vram_load_t));
	memset(&g_gpu.VRAMRead, 0, sizeof(gxv_vram_load_t));

	// device initialised already !
	g_gpu.status_reg = 0x14802000;
	return 0;
}

/* Here starts all... */
long GPUopen(unsigned long * disp, char * CapText, char * CfgFile) {
	unsigned long d;

	ReadConfig();

	g_draw.bIsFirstFrame = 1;

	d = init_display(); // setup x

	if (disp)
		*disp = d; // wanna x pointer? ok

	if (d)
		return 0;
	return -1;
}

////////////////////////////////////////////////////////////////////////
// time to leave...
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUclose() {
	fprintf(stderr, "%s\n", __FUNCTION__);
	//ReleaseKeyHandler(); // de-subclass window
	CloseDisplay(); // shutdown direct draw
	return 0;
}

////////////////////////////////////////////////////////////////////////
// I shot the sheriff
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUshutdown() {
	free(g_gpu.psxVSecure.s8);
	return 0;
}

////////////////////////////////////////////////////////////////////////
// roughly emulated screen centering bits... not complete !!!
////////////////////////////////////////////////////////////////////////
//
//void ChangeDispOffsetsX(void) // X CENTER
//{
//	long lx, l;
//
//	if (!g_gpu.dsp.range.x1)
//		return;
//
//	l = g_gpu.prev_dsp.DisplayMode.x;
//
//	l *= (long) g_gpu.dsp.range.x1;
//	l /= 2560;
//	lx = l;
//	l &= 0xfffffff8;
//
//	if (l == g_gpu.prev_dsp.range.y1)
//		return; // abusing range.y1 for
//	g_gpu.prev_dsp.range.y1 = (short) l; // storing last x range and test
//
//	if (lx >= g_gpu.prev_dsp.DisplayMode.x) {
//		g_gpu.prev_dsp.range.x1 = (short) g_gpu.prev_dsp.DisplayMode.x;
//		g_gpu.prev_dsp.range.x0 = 0;
//	} else {
//		g_gpu.prev_dsp.range.x1 = (short) l;
//
//		g_gpu.prev_dsp.range.x0 = (g_gpu.dsp.range.x0 - 500) / 8;
//
//		if (g_gpu.prev_dsp.range.x0 < 0)
//			g_gpu.prev_dsp.range.x0 = 0;
//
//		if ((g_gpu.prev_dsp.range.x0 + lx) > g_gpu.prev_dsp.DisplayMode.x) {
//			g_gpu.prev_dsp.range.x0 = (short) (g_gpu.prev_dsp.DisplayMode.x
//					- lx);
//			g_gpu.prev_dsp.range.x0 += 2; //???
//
//			g_gpu.prev_dsp.range.x1 += (short) (lx - l);
//
//			g_gpu.prev_dsp.range.x1 -= 2; // makes linux stretching easier
//		}
//
//		// some linux alignment security
//		g_gpu.prev_dsp.range.x0 = g_gpu.prev_dsp.range.x0 >> 1;
//		g_gpu.prev_dsp.range.x0 = g_gpu.prev_dsp.range.x0 << 1;
//		g_gpu.prev_dsp.range.x1 = g_gpu.prev_dsp.range.x1 >> 1;
//		g_gpu.prev_dsp.range.x1 = g_gpu.prev_dsp.range.x1 << 1;
//
//		DoClearScreenBuffer();
//	}
//
//	g_prim.bDoVSyncUpdate = 1;
//}

////////////////////////////////////////////////////////////////////////

//void ChangeDispOffsetsY(void) // Y CENTER
//{
//	int iT, iO = g_gpu.prev_dsp.range.y0;
//	int iOldYOffset = g_gpu.prev_dsp.DisplayModeNew.y;
//
//	// new
//
//	if ((g_gpu.prev_dsp.DisplayModeNew.x + g_gpu.dsp.DisplayModeNew.y) > 512) {
//		int dy1 = 512 - g_gpu.prev_dsp.DisplayModeNew.x;
//		int dy2 =
//				(g_gpu.prev_dsp.DisplayModeNew.x + g_gpu.dsp.DisplayModeNew.y)
//						- 512;
//
//		if (dy1 >= dy2) {
//			g_gpu.prev_dsp.DisplayModeNew.y = -dy2;
//		} else {
//			g_gpu.dsp.position.y = 0;
//			g_gpu.prev_dsp.DisplayModeNew.y = -dy1;
//		}
//	} else
//		g_gpu.prev_dsp.DisplayModeNew.y = 0;
//
//	// eon
//
//	if (g_gpu.prev_dsp.DisplayModeNew.y != iOldYOffset) // if old offset!=new offset: recalc height
//	{
//		g_gpu.dsp.Height = g_gpu.dsp.range.y1 - g_gpu.dsp.range.y0
//				+ g_gpu.prev_dsp.DisplayModeNew.y;
//		g_gpu.dsp.DisplayModeNew.y = g_gpu.dsp.Height * g_gpu.dsp.Double;
//	}
//
//	//
//
//	if (g_gpu.dsp.PAL)
//		iT = 48;
//	else
//		iT = 28;
//
//	if (g_gpu.dsp.range.y0 >= iT) {
//		g_gpu.prev_dsp.range.y0 = (short) ((g_gpu.dsp.range.y0 - iT - 4)
//				* g_gpu.dsp.Double);
//		if (g_gpu.prev_dsp.range.y0 < 0)
//			g_gpu.prev_dsp.range.y0 = 0;
//		g_gpu.dsp.DisplayModeNew.y += g_gpu.prev_dsp.range.y0;
//	} else
//		g_gpu.prev_dsp.range.y0 = 0;
//
//	if (iO != g_gpu.prev_dsp.range.y0) {
//		DoClearScreenBuffer();
//	}
//}

////////////////////////////////////////////////////////////////////////
// check if update needed
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////

// TOGGLE FULLSCREEN - WINDOW DISABLED
//void ChangeWindowMode(void) {
//	extern Display *display;
//	extern Window window;
//	extern int root_window_id;
//	Screen *screen;
//	XSizeHints hints;
//	MotifWmHints mwmhints;
//	Atom mwmatom;
//
//	screen = DefaultScreenOfDisplay(display);
//	iWindowMode = !iWindowMode;
//
//	if (!iWindowMode) // fullscreen
//	{
//		mwmhints.flags = MWM_HINTS_DECORATIONS;
//		mwmhints.functions = 0;
//		mwmhints.decorations = 0;
//		mwmhints.input_mode = 0;
//		mwmatom = XInternAtom(display, "_MOTIF_WM_HINTS", 0);
//		XChangeProperty(display, window, mwmatom, mwmatom, 32, PropModeReplace,
//				(unsigned char *) &mwmhints, 5);
//
//		XResizeWindow(display, window, screen->width, screen->height);
//
//		hints.min_width = hints.max_width = hints.base_width = screen->width;
//		hints.min_height = hints.max_height = hints.base_height
//				= screen->height;
//
//		XSetWMNormalHints(display, window, &hints);
//
//		{
//			XEvent xev;
//
//			memset(&xev, 0, sizeof(xev));
//			xev.xclient.type = ClientMessage;
//			xev.xclient.serial = 0;
//			xev.xclient.send_event = 1;
//			xev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", 0);
//			xev.xclient.window = window;
//			xev.xclient.format = 32;
//			xev.xclient.data.l[0] = 1;
//			xev.xclient.data.l[1] = XInternAtom(display,
//					"_NET_WM_STATE_FULLSCREEN", 0);
//			xev.xclient.data.l[2] = 0;
//			xev.xclient.data.l[3] = 0;
//			xev.xclient.data.l[4] = 0;
//
//			XSendEvent(display, root_window_id, 0, SubstructureRedirectMask
//					| SubstructureNotifyMask, &xev);
//		}
//	} else {
//		{
//			XEvent xev;
//
//			memset(&xev, 0, sizeof(xev));
//			xev.xclient.type = ClientMessage;
//			xev.xclient.serial = 0;
//			xev.xclient.send_event = 1;
//			xev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", 0);
//			xev.xclient.window = window;
//			xev.xclient.format = 32;
//			xev.xclient.data.l[0] = 0;
//			xev.xclient.data.l[1] = XInternAtom(display,
//					"_NET_WM_STATE_FULLSCREEN", 0);
//			xev.xclient.data.l[2] = 0;
//			xev.xclient.data.l[3] = 0;
//			xev.xclient.data.l[4] = 0;
//
//			XSendEvent(display, root_window_id, 0, SubstructureRedirectMask
//					| SubstructureNotifyMask, &xev);
//		}
//
//		mwmhints.flags = MWM_HINTS_DECORATIONS;
//		mwmhints.functions = 0;
//		mwmhints.decorations = 1;
//		mwmhints.input_mode = 0;
//		mwmatom = XInternAtom(display, "_MOTIF_WM_HINTS", 0);
//
//		//This shouldn't work on 64 bit longs, but it does...in fact, it breaks when I change all the mwmhints to int.
//		//I don't pretend to understand it.
//		XChangeProperty(display, window, mwmatom, mwmatom, 32, PropModeReplace,
//				(unsigned char *) &mwmhints, 5);
//
//		hints.flags = USPosition | USSize;
//		hints.base_width = iResX;
//		hints.base_height = iResY;
//		XSetWMNormalHints(display, window, &hints);
//
//		XResizeWindow(display, window, iResX, iResY);
//	}
//
//	DoClearScreenBuffer();
//
//	bChangeWinMode = 0;
//	bDoVSyncUpdate = 1;
//}

////////////////////////////////////////////////////////////////////////
// gun cursor func: player=0-7, x=0-511, y=0-255
////////////////////////////////////////////////////////////////////////

/* disabled */
void CALLBACK GPUcursor(int iPlayer, int x, int y) {
	//	if (iPlayer < 0)
	//		return;
	//	if (iPlayer > 7)
	//		return;
	//
	//	usCursorActive |= (1 << iPlayer);
	//
	//	if (x < 0)
	//		x = 0;
	//	if (x > 511)
	//		x = 511;
	//	if (y < 0)
	//		y = 0;
	//	if (y > 255)
	//		y = 255;
	//
	//	ptCursorPoint[iPlayer].x = x;
	//	ptCursorPoint[iPlayer].y = y;
}

////////////////////////////////////////////////////////////////////////
// update lace is called evry VSync
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUupdateLace(void) // VSYNC
{

	g_gpu.status_reg ^= STATUS_ODDLINES;

	if (g_gpu.status_reg & STATUS_DISPLAYDISABLED) {
		do_clear_screen_buffer();
		return;
	}

	/* I do not roughly emulate interlace, I just draw 1/2 frame
	 * to save CPU, and improve render */
	if (!(g_gpu.status_reg & STATUS_INTERLACED) || (g_gpu.status_reg
			& STATUS_ODDLINES))
		do_buffer_swap();

	frame_cap(g_gpu.fps);
}

////////////////////////////////////////////////////////////////////////
// process read request from GPU status register
////////////////////////////////////////////////////////////////////////


uint32_t CALLBACK GPUreadStatus(void) // READ STATUS
{
	return g_gpu.status_reg;
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU status register
// these are always single packet commands.
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUwriteStatus(uint32_t gdata) {
	uint32_t lCommand = (gdata >> 24) & 0xff;
	g_gpu.ulStatusControl[lCommand] = gdata; // store command for freezing
	switch (lCommand) {
	case 0x00:
		/* reset GPU */
		memset(g_gpu.lGPUInfoVals, 0x00, 16 * sizeof(uint32_t));
		/* STATUS_READYFORCOMMANDS | STATUS_IDLE | STATUS_DISPLAYDISABLED | UNKNOW13 */
		g_gpu.status_reg = 0x14802000;
		g_gpu.DataWriteMode = DR_NORMAL;
		g_gpu.DataReadMode = DR_NORMAL;
		g_prim.drawX = 0;
		g_prim.drawY = 0;
		g_prim.drawW = 0;
		g_prim.drawH = 0;
		g_draw.sSetMask = 0;
		g_draw.lSetMask = 0;
		g_draw.bCheckMask = 0;
		g_prim.usMirror = 0;
		g_soft.GlobalTextAddrX = 0;
		g_soft.GlobalTextAddrY = 0;
		g_soft.GlobalTextTP = 0;
		g_soft.GlobalTextABR = 0;
		g_prim.bUsingTWin = 0;
		return;
	case 0x01:
		/* reset command buffer */
		//fprintf(stderr, "Reset command buffer not implemented\n");
		return;
	case 0x02:
		/* reset IRQ */
		//fprintf(stderr, "Reset IRQ not implemented\n");
		return;
	case 0x03:
		/* Enable or disable the display */
		if (gdata & 0x01)
			g_gpu.status_reg |= STATUS_DISPLAYDISABLED;
		else
			g_gpu.status_reg &= ~STATUS_DISPLAYDISABLED;
		return;
	case 0x04:
		/* Set the transfering mode */
		gdata &= 0x03; // Only want the lower two bits
		g_gpu.DataWriteMode = DR_NORMAL;
		g_gpu.DataReadMode = DR_NORMAL;
		if (gdata == 0x02)
			g_gpu.DataWriteMode = DR_VRAMTRANSFER;
		if (gdata == 0x03)
			g_gpu.DataReadMode = DR_VRAMTRANSFER;
		g_gpu.status_reg &= ~STATUS_DMABITS; // Clear the current settings of the DMA bits
		g_gpu.status_reg |= (gdata << 29); // Set the DMA bits according to the received data
		return;
	case 0x05:
		/* setting display position */
		g_gpu.dsp.position.y = (short) ((gdata >> 10) & 0x1ff);
		g_gpu.dsp.position.x = (short) (gdata & 0x3ff);
		//fprintf(stderr, "Update display position X=%d,Y=%d\n",
		//		g_gpu.dsp.position.x, g_gpu.dsp.position.y);
		return;
	case 0x06:
		/* Set width */
		g_gpu.dsp.range.x0 = (short) (gdata & 0x03ff);
		g_gpu.dsp.range.x1 = (short) ((gdata >> 12) & 0x0fff);
		//fprintf(stderr, "Setrange x0 : %d, x1 : %d\n", g_gpu.dsp.range.x0,
		//		g_gpu.dsp.range.x1);
		return;
	case 0x07:
		/* Set height */
		g_gpu.dsp.range.y0 = (short) (gdata & 0x3ff);
		g_gpu.dsp.range.y1 = (short) ((gdata >> 10) & 0x3ff);
		//fprintf(stderr, "Setrange y0 : %d, y1 : %d\n", g_gpu.dsp.range.y0,
		//		g_gpu.dsp.range.y1);
		return;
	case 0x08:
		/* setting display infos */
		//fprintf(stderr, "command %x\n", gdata&0x000043);
		switch (gdata & 0x000043) {
		case 0x00:
			g_gpu.dsp.mode.x = 10;
			break;
		case 0x01:
			g_gpu.dsp.mode.x = 8;
			break;
		case 0x02:
			g_gpu.dsp.mode.x = 5;
			break;
		case 0x03:
			g_gpu.dsp.mode.x = 4;
			break;
		case 0x40:
			g_gpu.dsp.mode.x = 7;
			break;
		default:
			g_gpu.dsp.mode.x = 0;
		}

		/* clear width status */
		g_gpu.status_reg &= ~STATUS_WIDTHBITS;
		/* update status reg */
		g_gpu.status_reg |= ((gdata & 0x00000003) << 17)
				| ((gdata & 0x00000040) << 10);

		switch (gdata & 0x000004) {
		case 0x00:
			g_gpu.status_reg &= ~STATUS_DOUBLEHEIGHT;
			g_gpu.dsp.mode.y = 1;
			break;
		case 0x04:
			g_gpu.status_reg |= STATUS_DOUBLEHEIGHT;
			g_gpu.dsp.mode.y = 2;
			break;
		}

		switch (gdata & 0x08) {
		case 0x00:
			g_gpu.status_reg &= ~STATUS_PAL;
			g_gpu.fps = 60;
			break;
		case 0x08:
			g_gpu.status_reg |= STATUS_PAL;
			g_gpu.fps = 50;
			break;
		}
		//fprintf(stderr, "video mode %dx%d\n", g_gpu.dsp.mode.x, g_gpu.dsp.mode.y);

		if ((gdata & 0x10)) {
			g_gpu.status_reg |= STATUS_RGB24;
		} else {
			g_gpu.status_reg &= ~STATUS_RGB24;
		}

		if ((gdata & 0x20)) {
			g_gpu.status_reg |= STATUS_INTERLACED;
		} else {
			g_gpu.status_reg &= ~STATUS_INTERLACED;
		}
		return;
	case 0x10:
		/* ask about GPU version and other stuff */
		gdata &= 0xff;
		switch (gdata) {
		case 0x02:
			g_gpu.lGPUdataRet = g_gpu.lGPUInfoVals[INFO_TW]; // tw infos
			return;
		case 0x03:
			g_gpu.lGPUdataRet = g_gpu.lGPUInfoVals[INFO_DRAWSTART]; // draw start
			return;
		case 0x04:
			g_gpu.lGPUdataRet = g_gpu.lGPUInfoVals[INFO_DRAWEND]; // draw end
			return;
		case 0x05:
		case 0x06:
			g_gpu.lGPUdataRet = g_gpu.lGPUInfoVals[INFO_DRAWOFF]; // draw offset
			return;
		case 0x07:
			if (0)
				g_gpu.lGPUdataRet = 0x01;
			else
				g_gpu.lGPUdataRet = 0x02; // gpu type
			return;
		case 0x08:
		case 0x0F: // some bios addr?
			g_gpu.lGPUdataRet = 0xBFC03720;
			return;
		}
		return;
	default:
		fprintf(stderr, "Unknow command %02x\n", lCommand);
	}
}

////////////////////////////////////////////////////////////////////////
// vram read/write helpers, needed by LEWPY's optimized vram read/write :)
////////////////////////////////////////////////////////////////////////

inline void FinishedVRAMWrite(void) {
	// Set register to NORMAL operation
	g_gpu.DataWriteMode = DR_NORMAL;
	// Reset transfer values, to prevent mis-transfer of data
	g_gpu.VRAMWrite.x = 0;
	g_gpu.VRAMWrite.y = 0;
	g_gpu.VRAMWrite.Width = 0;
	g_gpu.VRAMWrite.Height = 0;
	g_gpu.VRAMWrite.ColsRemaining = 0;
	g_gpu.VRAMWrite.RowsRemaining = 0;
}

inline void FinishedVRAMRead(void) {
	// Set register to NORMAL operation
	g_gpu.DataReadMode = DR_NORMAL;
	// Reset transfer values, to prevent mis-transfer of data
	g_gpu.VRAMRead.x = 0;
	g_gpu.VRAMRead.y = 0;
	g_gpu.VRAMRead.Width = 0;
	g_gpu.VRAMRead.Height = 0;
	g_gpu.VRAMRead.ColsRemaining = 0;
	g_gpu.VRAMRead.RowsRemaining = 0;

	// Indicate GPU is no longer ready for VRAM data in the STATUS REGISTER
	g_gpu.status_reg &= ~STATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// core read from vram
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUreadDataMem(uint32_t * pMem, int iSize) {
	int i;

	if (g_gpu.DataReadMode != DR_VRAMTRANSFER)
		return;

	g_gpu.status_reg &= ~STATUS_IDLE;

	// adjust read ptr, if necessary

	while (g_gpu.VRAMRead.ImagePtr >= g_gpu.psxVuw_eom.u16)
		g_gpu.VRAMRead.ImagePtr -= 512 * 1024;
	while (g_gpu.VRAMRead.ImagePtr < g_gpu.psx_vram.u16)
		g_gpu.VRAMRead.ImagePtr += 512 * 1024;

	for (i = 0; i < iSize; i++) {
		// do 2 seperate 16bit reads for compatibility (wrap issues)
		if ((g_gpu.VRAMRead.ColsRemaining > 0) && (g_gpu.VRAMRead.RowsRemaining
				> 0)) {
			// lower 16 bit
			g_gpu.lGPUdataRet = (uint32_t) GETLE16(g_gpu.VRAMRead.ImagePtr);

			g_gpu.VRAMRead.ImagePtr++;
			if (g_gpu.VRAMRead.ImagePtr >= g_gpu.psxVuw_eom.u16)
				g_gpu.VRAMRead.ImagePtr -= 512 * 1024;
			g_gpu.VRAMRead.RowsRemaining--;

			if (g_gpu.VRAMRead.RowsRemaining <= 0) {
				g_gpu.VRAMRead.RowsRemaining = g_gpu.VRAMRead.Width;
				g_gpu.VRAMRead.ColsRemaining--;
				g_gpu.VRAMRead.ImagePtr += 1024 - g_gpu.VRAMRead.Width;
				if (g_gpu.VRAMRead.ImagePtr >= g_gpu.psxVuw_eom.u16)
					g_gpu.VRAMRead.ImagePtr -= 512 * 1024;
			}

			// higher 16 bit (always, even if it's an odd width)
			g_gpu.lGPUdataRet |= (uint32_t) GETLE16(g_gpu.VRAMRead.ImagePtr)
					<< 16;
			PUTLE32(pMem, g_gpu.lGPUdataRet);
			pMem++;

			if (g_gpu.VRAMRead.ColsRemaining <= 0) {
				FinishedVRAMRead();
				goto ENDREAD;
			}

			g_gpu.VRAMRead.ImagePtr++;
			if (g_gpu.VRAMRead.ImagePtr >= g_gpu.psxVuw_eom.u16)
				g_gpu.VRAMRead.ImagePtr -= 512 * 1024;
			g_gpu.VRAMRead.RowsRemaining--;
			if (g_gpu.VRAMRead.RowsRemaining <= 0) {
				g_gpu.VRAMRead.RowsRemaining = g_gpu.VRAMRead.Width;
				g_gpu.VRAMRead.ColsRemaining--;
				g_gpu.VRAMRead.ImagePtr += 1024 - g_gpu.VRAMRead.Width;
				if (g_gpu.VRAMRead.ImagePtr >= g_gpu.psxVuw_eom.u16)
					g_gpu.VRAMRead.ImagePtr -= 512 * 1024;
			}
			if (g_gpu.VRAMRead.ColsRemaining <= 0) {
				FinishedVRAMRead();
				goto ENDREAD;
			}
		} else {
			FinishedVRAMRead();
			goto ENDREAD;
		}
	}

	ENDREAD: g_gpu.status_reg |= STATUS_IDLE;
}

////////////////////////////////////////////////////////////////////////

uint32_t CALLBACK GPUreadData(void) {
	uint32_t l;
	GPUreadDataMem(&l, 1);
	return g_gpu.lGPUdataRet;
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU data register
// extra table entries for fixing polyline troubles
////////////////////////////////////////////////////////////////////////

const unsigned char primTableCX[256] = {
// 00
		0, 0, 3, 0, 0, 0, 0, 0,
		// 08
		0, 0, 0, 0, 0, 0, 0, 0,
		// 10
		0, 0, 0, 0, 0, 0, 0, 0,
		// 18
		0, 0, 0, 0, 0, 0, 0, 0,
		// 20
		4, 4, 4, 4, 7, 7, 7, 7,
		// 28
		5, 5, 5, 5, 9, 9, 9, 9,
		// 30
		6, 6, 6, 6, 9, 9, 9, 9,
		// 38
		8, 8, 8, 8, 12, 12, 12, 12,
		// 40
		3, 3, 3, 3, 0, 0, 0, 0,
		// 48
		//  5,5,5,5,6,6,6,6,    // FLINE
		254, 254, 254, 254, 254, 254, 254, 254,
		// 50
		4, 4, 4, 4, 0, 0, 0, 0,
		// 58
		//  7,7,7,7,9,9,9,9,    // GLINE
		255, 255, 255, 255, 255, 255, 255, 255,
		// 60
		3, 3, 3, 3, 4, 4, 4, 4,
		// 68
		2, 2, 2, 2, 3, 3, 3, 3, // 3=SPRITE1???
		// 70
		2, 2, 2, 2, 3, 3, 3, 3,
		// 78
		2, 2, 2, 2, 3, 3, 3, 3,
		// 80
		4, 0, 0, 0, 0, 0, 0, 0,
		// 88
		0, 0, 0, 0, 0, 0, 0, 0,
		// 90
		0, 0, 0, 0, 0, 0, 0, 0,
		// 98
		0, 0, 0, 0, 0, 0, 0, 0,
		// a0
		3, 0, 0, 0, 0, 0, 0, 0,
		// a8
		0, 0, 0, 0, 0, 0, 0, 0,
		// b0
		0, 0, 0, 0, 0, 0, 0, 0,
		// b8
		0, 0, 0, 0, 0, 0, 0, 0,
		// c0
		3, 0, 0, 0, 0, 0, 0, 0,
		// c8
		0, 0, 0, 0, 0, 0, 0, 0,
		// d0
		0, 0, 0, 0, 0, 0, 0, 0,
		// d8
		0, 0, 0, 0, 0, 0, 0, 0,
		// e0
		0, 1, 1, 1, 1, 1, 1, 0,
		// e8
		0, 0, 0, 0, 0, 0, 0, 0,
		// f0
		0, 0, 0, 0, 0, 0, 0, 0,
		// f8
		0, 0, 0, 0, 0, 0, 0, 0 };

void CALLBACK GPUwriteDataMem(uint32_t * pMem, int iSize) {
	unsigned char command;
	uint32_t gdata = 0;
	int i = 0;

	g_gpu.status_reg &= ~STATUS_IDLE;
	g_gpu.status_reg &= ~STATUS_READYFORCOMMANDS;

	STARTVRAM:

	if (g_gpu.DataWriteMode == DR_VRAMTRANSFER) {
		char bFinished = 0;

		// make sure we are in vram
		while (g_gpu.VRAMWrite.ImagePtr >= g_gpu.psxVuw_eom.u16)
			g_gpu.VRAMWrite.ImagePtr -= 512 * 1024;
		while (g_gpu.VRAMWrite.ImagePtr < g_gpu.psx_vram.u16)
			g_gpu.VRAMWrite.ImagePtr += 512 * 1024;

		// now do the loop
		while (g_gpu.VRAMWrite.ColsRemaining > 0) {
			while (g_gpu.VRAMWrite.RowsRemaining > 0) {
				if (i >= iSize) {
					goto ENDVRAM;
				}
				i++;

				gdata = GETLE32(pMem);
				pMem++;

				PUTLE16(g_gpu.VRAMWrite.ImagePtr, (unsigned short)gdata);
				g_gpu.VRAMWrite.ImagePtr++;
				if (g_gpu.VRAMWrite.ImagePtr >= g_gpu.psxVuw_eom.u16)
					g_gpu.VRAMWrite.ImagePtr -= 512 * 1024;
				g_gpu.VRAMWrite.RowsRemaining--;

				if (g_gpu.VRAMWrite.RowsRemaining <= 0) {
					g_gpu.VRAMWrite.ColsRemaining--;
					if (g_gpu.VRAMWrite.ColsRemaining <= 0) // last pixel is odd width
					{
						gdata
								= (gdata & 0xFFFF)
										| (((uint32_t) GETLE16(g_gpu.VRAMWrite.ImagePtr))
												<< 16);
						FinishedVRAMWrite();
						goto ENDVRAM;
					}
					g_gpu.VRAMWrite.RowsRemaining = g_gpu.VRAMWrite.Width;
					g_gpu.VRAMWrite.ImagePtr += 1024 - g_gpu.VRAMWrite.Width;
				}

				PUTLE16(g_gpu.VRAMWrite.ImagePtr, (unsigned short)(gdata>>16));
				g_gpu.VRAMWrite.ImagePtr++;
				if (g_gpu.VRAMWrite.ImagePtr >= g_gpu.psxVuw_eom.u16)
					g_gpu.VRAMWrite.ImagePtr -= 512 * 1024;
				g_gpu.VRAMWrite.RowsRemaining--;
			}

			g_gpu.VRAMWrite.RowsRemaining = g_gpu.VRAMWrite.Width;
			g_gpu.VRAMWrite.ColsRemaining--;
			g_gpu.VRAMWrite.ImagePtr += 1024 - g_gpu.VRAMWrite.Width;
			bFinished = 1;
		}

		FinishedVRAMWrite();
	}

	ENDVRAM:

	if (g_gpu.DataWriteMode == DR_NORMAL) {
		void (* *primFunc)(unsigned char *);
		primFunc = primTableJ;

		for (; i < iSize;) {
			if (g_gpu.DataWriteMode == DR_VRAMTRANSFER)
				goto STARTVRAM;

			gdata = GETLE32(pMem);
			pMem++;
			i++;

			if (g_gpu.gpuDataC == 0) {
				command = (unsigned char) ((gdata >> 24) & 0xff);

				//if(command>=0xb0 && command<0xc0) auxprintf("b0 %x!!!!!!!!!\n",command);

				if (primTableCX[command]) {
					g_gpu.gpuDataC = primTableCX[command];
					g_gpu.gpuCommand = command;
					PUTLE32(&g_gpu.gpuDataM[0], gdata);
					g_gpu.gpuDataP = 1;
				} else {
					//fprintf(stderr, "unknow command %02x \n", command);
					continue;
				}
			} else {
				PUTLE32(&g_gpu.gpuDataM[g_gpu.gpuDataP], gdata);
				if (g_gpu.gpuDataC > 128) {
					if ((g_gpu.gpuDataC == 254 && g_gpu.gpuDataP >= 3)
							|| (g_gpu.gpuDataC == 255 && g_gpu.gpuDataP >= 4
									&& !(g_gpu.gpuDataP & 1))) {
						if ((g_gpu.gpuDataM[g_gpu.gpuDataP] & 0xF000F000)
								== 0x50005000)
							g_gpu.gpuDataP = g_gpu.gpuDataC - 1;
					}
				}
				g_gpu.gpuDataP++;
			}

			if (g_gpu.gpuDataP == g_gpu.gpuDataC) {
				g_gpu.gpuDataC = g_gpu.gpuDataP = 0;
				primFunc[g_gpu.gpuCommand]((unsigned char *) g_gpu.gpuDataM);
			}
		}
	}

	g_gpu.lGPUdataRet = gdata;

	g_gpu.status_reg |= STATUS_READYFORCOMMANDS;
	g_gpu.status_reg |= STATUS_IDLE;
}

////////////////////////////////////////////////////////////////////////

void CALLBACK GPUwriteData(uint32_t gdata) {
	PUTLE32(&gdata, gdata);
	GPUwriteDataMem(&gdata, 1);
}

////////////////////////////////////////////////////////////////////////
// this functions will be removed soon (or 'soonish')... not really needed, but some emus want them
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUsetMode(unsigned long gdata) {
	// Peops does nothing here...
	// DataWriteMode=(gdata&1)?DR_VRAMTRANSFER:DR_NORMAL;
	// DataReadMode =(gdata&2)?DR_VRAMTRANSFER:DR_NORMAL;
}

long CALLBACK GPUgetMode(void) {
	long iT = 0;

	if (g_gpu.DataWriteMode == DR_VRAMTRANSFER)
		iT |= 0x1;
	if (g_gpu.DataReadMode == DR_VRAMTRANSFER)
		iT |= 0x2;
	return iT;
}

////////////////////////////////////////////////////////////////////////
// call config dlg
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUconfigure(void) {
	SoftDlgProc();
	return 0;
}

////////////////////////////////////////////////////////////////////////
// sets all kind of act fixes
////////////////////////////////////////////////////////////////////////

void SetFixes(void) {

}

////////////////////////////////////////////////////////////////////////
// process gpu commands
////////////////////////////////////////////////////////////////////////

unsigned long lUsedAddr[3];

inline char CheckForEndlessLoop(unsigned long laddr) {
	if (laddr == lUsedAddr[1])
		return 1;
	if (laddr == lUsedAddr[2])
		return 1;

	if (laddr < lUsedAddr[0])
		lUsedAddr[1] = laddr;
	else
		lUsedAddr[2] = laddr;
	lUsedAddr[0] = laddr;
	return 0;
}

long CALLBACK GPUdmaChain(uint32_t * baseAddrL, uint32_t addr) {
	uint32_t dmaMem;
	unsigned char * baseAddrB;
	short count;
	unsigned int DMACommandCounter = 0;

	g_gpu.status_reg &= ~STATUS_IDLE;

	lUsedAddr[0] = lUsedAddr[1] = lUsedAddr[2] = 0xffffff;

	baseAddrB = (unsigned char*) baseAddrL;

	do {
		if (1)
			addr &= 0x1FFFFC;
		if (DMACommandCounter++ > 2000000)
			break;
		if (CheckForEndlessLoop(addr))
			break;

		count = baseAddrB[addr + 3];

		dmaMem = addr + 4;

		if (count > 0)
			GPUwriteDataMem(&baseAddrL[dmaMem >> 2], count);

		addr = GETLE32(&baseAddrL[addr>>2]) & 0xffffff;
	} while (addr != 0xffffff);

	g_gpu.status_reg |= STATUS_IDLE;
	return 0;
}

////////////////////////////////////////////////////////////////////////
// show about dlg
////////////////////////////////////////////////////////////////////////


void CALLBACK GPUabout(void) // ABOUT
{
	AboutDlgProc();
	return;
}

////////////////////////////////////////////////////////////////////////
// We are ever fine ;)
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUtest(void) {
	// if test fails this function should return negative value for error (unable to continue)
	// and positive value for warning (can continue but output might be crappy)
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Freeze
////////////////////////////////////////////////////////////////////////

typedef struct GPUFREEZETAG {
	uint32_t ulFreezeVersion; // should be always 1 for now (set by main emu)
	uint32_t ulStatus; // current gpu status
	uint32_t ulControl[256]; // latest control register values
	unsigned char psxVRam[1024 * 1024 * 2]; // current VRam image (full 2 MB for ZN)
} GPUFreeze_t;

////////////////////////////////////////////////////////////////////////

long CALLBACK GPUfreeze(uint32_t ulGetFreezeData, GPUFreeze_t * pF) {
	//----------------------------------------------------//
	if (ulGetFreezeData == 2) // 2: info, which save slot is selected? (just for display)
	{
		long lSlotNum = *((long *) pF);
		if (lSlotNum < 0)
			return 0;
		if (lSlotNum > 8)
			return 0;
		//g_gpu.lSelectedSlot = lSlotNum + 1;
		//BuildDispMenu(0);
		return 1;
	}
	//----------------------------------------------------//
	if (!pF)
		return 0; // some checks
	if (pF->ulFreezeVersion != 1)
		return 0;

	if (ulGetFreezeData == 1) // 1: get data
	{
		pF->ulStatus = g_gpu.status_reg;
		memcpy(pF->ulControl, g_gpu.ulStatusControl, 256 * sizeof(uint32_t));
		memcpy(pF->psxVRam, g_gpu.psx_vram.u8, 1024 * 512 * 2);

		return 1;
	}

	if (ulGetFreezeData != 0)
		return 0; // 0: set data

	g_gpu.status_reg = pF->ulStatus;
	memcpy(g_gpu.ulStatusControl, pF->ulControl, 256 * sizeof(uint32_t));
	memcpy(g_gpu.psx_vram.u8, pF->psxVRam, 1024 * 512 * 2);

	// RESET TEXTURE STORE HERE, IF YOU USE SOMETHING LIKE THAT

	GPUwriteStatus(g_gpu.ulStatusControl[0]);
	GPUwriteStatus(g_gpu.ulStatusControl[1]);
	GPUwriteStatus(g_gpu.ulStatusControl[2]);
	GPUwriteStatus(g_gpu.ulStatusControl[3]);
	GPUwriteStatus(g_gpu.ulStatusControl[8]); // try to repair things
	GPUwriteStatus(g_gpu.ulStatusControl[6]);
	GPUwriteStatus(g_gpu.ulStatusControl[7]);
	GPUwriteStatus(g_gpu.ulStatusControl[5]);
	GPUwriteStatus(g_gpu.ulStatusControl[4]);

	return 1;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SAVE STATE DISPLAY STUFF
////////////////////////////////////////////////////////////////////////

// font 0-9, 24x20 pixels, 1 byte = 4 dots
// 00 = black
// 01 = white
// 10 = red
// 11 = transparent

unsigned char cFont[10][120] = {
// 0
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80,
				0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 1
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x05, 0x50, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x80, 0x00, 0x05, 0x55, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 2
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40, 0x00,
				0x00, 0x80, 0x00, 0x05, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x15, 0x55, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 3
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x01, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 4
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x54, 0x00,
				0x00, 0x80, 0x00, 0x01, 0x54, 0x00, 0x00, 0x80, 0x00, 0x01,
				0x54, 0x00, 0x00, 0x80, 0x00, 0x05, 0x14, 0x00, 0x00, 0x80,
				0x00, 0x14, 0x14, 0x00, 0x00, 0x80, 0x00, 0x15, 0x55, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x55, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 5
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x15, 0x55, 0x00, 0x00, 0x80, 0x00, 0x14, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x15, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 6
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x01, 0x54, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x00, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x15, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x15, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 7
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x15, 0x55, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x14, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x14, 0x00, 0x00, 0x80, 0x00, 0x00, 0x50, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x50, 0x00, 0x00, 0x80, 0x00, 0x01, 0x40, 0x00,
				0x00, 0x80, 0x00, 0x01, 0x40, 0x00, 0x00, 0x80, 0x00, 0x05,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 8
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x05, 0x54, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
		// 9
		{ 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x05, 0x54, 0x00, 0x00, 0x80, 0x00, 0x14, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x14, 0x05, 0x00, 0x00, 0x80, 0x00, 0x14,
				0x05, 0x00, 0x00, 0x80, 0x00, 0x14, 0x15, 0x00, 0x00, 0x80,
				0x00, 0x05, 0x55, 0x00, 0x00, 0x80, 0x00, 0x00, 0x05, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x14, 0x00, 0x00, 0x80, 0x00, 0x05, 0x50, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa } };

////////////////////////////////////////////////////////////////////////

void PaintPicDot(unsigned char * p, unsigned char c) {

	if (c == 0) {
		*p++ = 0x00;
		*p++ = 0x00;
		*p = 0x00;
		return;
	} // black
	if (c == 1) {
		*p++ = 0xff;
		*p++ = 0xff;
		*p = 0xff;
		return;
	} // white
	if (c == 2) {
		*p++ = 0x00;
		*p++ = 0x00;
		*p = 0xff;
		return;
	} // red
	// transparent
}

////////////////////////////////////////////////////////////////////////
// the main emu allocs 128x96x3 bytes, and passes a ptr
// to it in pMem... the plugin has to fill it with
// 8-8-8 bit BGR screen data (Win 24 bit BMP format 
// without header). 
// Beware: the func can be called at any time,
// so you have to use the frontbuffer to get a fully
// rendered picture
// LINUX version:

//extern char * Xpixels;

void GPUgetScreenPic(unsigned char * pMem) {
	/*
	 unsigned short c;unsigned char * pf;int x,y;

	 float XS=(float)iResX/128;
	 float YS=(float)iResY/96;

	 pf=pMem;
	 memset(pMem, 0, 128*96*3);

	 if(Xpixels)
	 {
	 unsigned char * ps=(unsigned char *)Xpixels;
	 {
	 long lPitch=iResX<<2;
	 uint32_t sx;

	 for(y=0;y<96;y++)
	 {
	 for(x=0;x<128;x++)
	 {
	 sx=*((uint32_t *)((ps)+
	 (((int)((float)y*YS))*lPitch)+
	 ((int)((float)x*XS))*4));
	 *(pf+0)=(sx&0xff);
	 *(pf+1)=(sx&0xff00)>>8;
	 *(pf+2)=(sx&0xff0000)>>16;
	 pf+=3;
	 }
	 }
	 }
	 }


	 /////////////////////////////////////////////////////////////////////
	 // generic number/border painter

	 pf=pMem+(103*3);                                      // offset to number rect

	 for(y=0;y<20;y++)                                     // loop the number rect pixel
	 {
	 for(x=0;x<6;x++)
	 {
	 c=cFont[lSelectedSlot][x+y*6];                    // get 4 char dot infos at once (number depends on selected slot)
	 PaintPicDot(pf,(c&0xc0)>>6);pf+=3;                // paint the dots into the rect
	 PaintPicDot(pf,(c&0x30)>>4);pf+=3;
	 PaintPicDot(pf,(c&0x0c)>>2);pf+=3;
	 PaintPicDot(pf,(c&0x03));   pf+=3;
	 }
	 pf+=104*3;                                          // next rect y line
	 }

	 pf=pMem;                                              // ptr to first pos in 128x96 pic
	 for(x=0;x<128;x++)                                    // loop top/bottom line
	 {
	 *(pf+(95*128*3))=0x00;*pf++=0x00;
	 *(pf+(95*128*3))=0x00;*pf++=0x00;                   // paint it red
	 *(pf+(95*128*3))=0xff;*pf++=0xff;
	 }
	 pf=pMem;                                              // ptr to first pos
	 for(y=0;y<96;y++)                                     // loop left/right line
	 {
	 *(pf+(127*3))=0x00;*pf++=0x00;
	 *(pf+(127*3))=0x00;*pf++=0x00;                      // paint it red
	 *(pf+(127*3))=0xff;*pf++=0xff;
	 pf+=127*3;                                          // offset to next line
	 }
	 */
}

////////////////////////////////////////////////////////////////////////
// func will be called with 128x96x3 BGR data.
// the plugin has to store the data and display
// it in the upper right corner.
// If the func is called with a NULL ptr, you can
// release your picture data and stop displaying
// the screen pic

void CALLBACK GPUshowScreenPic(unsigned char * pMem) {
	DestroyPic(); // destroy old pic data
	if (pMem == 0)
		return; // done
	CreatePic(pMem); // create new pic... don't free pMem or something like that... just read from it
}

void CALLBACK GPUsetfix(uint32_t dwFixBits) {
	g_prim.dwEmuFixes = dwFixBits;
}

void CALLBACK GPUvBlank(int val) {
	//fprintf(stderr, "Vblanc %d\n", val);
	//if(val)
	//	g_gpu.status_reg |= STATUS_ODDLINES;
	//else
	//	g_gpu.status_reg &= ~STATUS_ODDLINES;
}
