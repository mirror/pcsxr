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

#ifndef _GPU_DRAW_H_
#define _GPU_DRAW_H_

#include <time.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

#include "gpu_utils.h"

typedef struct {
	int iResX;
	int iResY;
	long lLowerpart;
	int bIsFirstFrame;
	int bCheckMask;
	uint16_t sSetMask;
	uint32_t lSetMask;
	int iDesktopCol;
	int iShowFPS;
	int iWinSize;
	int iMaintainAspect;
	int iUseNoStretchBlt;
	int iFastFwd;
	int iDebugMode;
	int iFVDisplay;
	gxv_point_t ptCursorPoint[8];
	unsigned short usCursorActive;

	int xv_port;
	int xv_id;
	int xv_vsync;
	int xv;

	XShmSegmentInfo shminfo;
	int finalw, finalh;

	/* X Stuff */
	Cursor cursor;
	XVisualInfo vi;
	XVisualInfo * myvisual;
	Display * display;
	Colormap colormap;
	Window window;
	GC hGC;
	XImage * Ximage;
	XvImage * XCimage;
	XImage * XFimage;
	XImage * XPimage;
	char * Xpixels;
	char * pCaptionText;

	int fx;

	int depth;
	int root_window_id;

	time_t tStart;

	double fps;
	char msg[512];

} gxv_draw_t;

typedef struct {
	void (*blit_24_bits)(int8_t * buff, int32_t x, int32_t y, int32_t w, int32_t h);
	void (*blit_16_bits)(int8_t * buff, int32_t x, int32_t y, int32_t w, int32_t h);
} gxv_blit_t;

void do_buffer_swap(void);
void do_clear_screen_buffer(void);
void do_clear_front_buffer(void);
unsigned long init_display(void);
void CloseDisplay(void);
void CreatePic(unsigned char * pMem);
void DestroyPic(void);
void DisplayPic(void);
void ShowGpuPic(void);
void ShowTextGpuPic(void);

typedef struct {
#define MWM_HINTS_DECORATIONS   2
	long flags;
	long functions;
	long decorations;
	long input_mode;
} MotifWmHints;

#endif // _GPU_DRAW_H_
