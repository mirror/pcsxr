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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/shm.h>
#include <X11/cursorfont.h>

#include "globals.h"
#include "swap.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

enum {
	GXV_YUV_PORT = 0, GXV_RGB_PORT = 1, GXV_MAX_PORT = 2,
};

gxv_blit_t available_port[GXV_MAX_PORT];

/* Convert RGB to YUV */
inline uint32_t rgb_to_yuv(uint8_t R, uint8_t G, uint8_t B) {
	uint8_t Y, U, V;
	Y = abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13;
	Y = MIN(Y, 235);
	U = abs(R * -1214 + G * -2384 + B * 3598 + 4096 + 1048576) >> 13;
	U = MIN(U, 240);
	V = abs(R * 3598 + G * -3013 + B * -585 + 4096 + 1048576) >> 13;
	V = MIN(V, 240);
	return Y << 24 | V << 16 | Y << 8 | U;
}

inline uint32_t rgb_to_yuv2(uint8_t R0, uint8_t G0, uint8_t B0, uint8_t R1,
		uint8_t G1, uint8_t B1) {
	int32_t Y0, U0, V0;
	int32_t Y1, U1, V1;
	int32_t U, V;
	Y0 = abs(R0 * 2104 + G0 * 4130 + B0 * 802 + 4096 + 131072) >> 13;
	Y0 = MIN(Y0, 235);
	U0 = abs(R0 * -1214 + G0 * -2384 + B0 * 3598 + 4096 + 1048576) >> 13;
	U0 = MIN(U0, 240);
	V0 = abs(R0 * 3598 + G0 * -3013 + B0 * -585 + 4096 + 1048576) >> 13;
	V0 = MIN(V0, 240);
	Y1 = abs(R1 * 2104 + G1 * 4130 + B1 * 802 + 4096 + 131072) >> 13;
	Y1 = MIN(Y1, 235);
	U1 = abs(R1 * -1214 + G1 * -2384 + B1 * 3598 + 4096 + 1048576) >> 13;
	U1 = MIN(U1, 240);
	V1 = abs(R1 * 3598 + G1 * -3013 + B1 * -585 + 4096 + 1048576) >> 13;
	V1 = MIN(V1, 240);
	U = (U0 + U1) / 2;
	V = (V0 + V1) / 2;
	return Y1 << 24 | V << 16 | Y0 << 8 | U;
}

#define colorMask8     0x00FEFEFE
#define lowPixelMask8  0x00010101
#define qcolorMask8    0x00FCFCFC
#define qlowpixelMask8 0x00030303

static Atom xv_intern_atom_if_exists(Display *display, char const * atom_name) {
	XvAttribute * attributes;
	int attrib_count, i;
	Atom xv_atom = None;

	attributes = XvQueryPortAttributes(display, g_draw.xv_port, &attrib_count);
	if (attributes != NULL) {
		for (i = 0; i < attrib_count; ++i) {
			if (strcmp(attributes[i].name, atom_name) == 0) {
				xv_atom = XInternAtom(display, atom_name, False);
				break; // found what we want, break out
			}
		}
		XFree(attributes);
	}

	return xv_atom;
}

// close display

void DestroyDisplay(void) {
	if (g_draw.display) {
		XFreeColormap(g_draw.display, g_draw.colormap);
		if (g_draw.hGC) {
			XFreeGC(g_draw.display, g_draw.hGC);
			g_draw.hGC = 0;
		}
		if (g_draw.Ximage) {
			XDestroyImage(g_draw.Ximage);
			g_draw.Ximage = 0;
		}
		if (g_draw.XCimage) {
			XFree(g_draw.XCimage);
			g_draw.XCimage = 0;
		}
		if (g_draw.XFimage) {
			XDestroyImage(g_draw.XFimage);
			g_draw.XFimage = 0;
		}

		XShmDetach(g_draw.display, &g_draw.shminfo);
		shmdt(g_draw.shminfo.shmaddr);
		shmctl(g_draw.shminfo.shmid, IPC_RMID, NULL);

		Atom atom_vsync = xv_intern_atom_if_exists(g_draw.display,
				"XV_SYNC_TO_VBLANK");
		if (atom_vsync != None) {
			XvSetPortAttribute(g_draw.display, g_draw.xv_port, atom_vsync,
					g_draw.xv_vsync);
		}

		XSync(g_draw.display, False);

		XCloseDisplay(g_draw.display);
	}
}

// Create display

void create_display(void) {
	//fprintf(stderr, "Entering %s\n", __FUNCTION__);
	XSetWindowAttributes winattr;
	int myscreen;
	Screen * screen;
	XEvent event;
	XSizeHints hints;
	XWMHints wm_hints;
	MotifWmHints mwmhints;
	Atom mwmatom;

	Atom delwindow;

	XGCValues gcv;
	int i;

	int ret, j, p;
	int formats;
	unsigned int p_num_adaptors = 0, p_num_ports = 0;

	XvAdaptorInfo *ai;
	XvImageFormatValues *fo;

	// Open display
	g_draw.display = XOpenDisplay(NULL);

	if (!g_draw.display) {
		fprintf(stderr, "Failed to open display!!!\n");
		DestroyDisplay();
		return;
	}

	myscreen = DefaultScreen(g_draw.display);

	// desktop fullscreen switch
	if (g_cfg.FullScreen)
		g_draw.fx = 1;

	screen = DefaultScreenOfDisplay(g_draw.display);

	g_draw.root_window_id = RootWindow(g_draw.display, myscreen);

	//Look for an Xvideo RGB port
	ret = XvQueryAdaptors(g_draw.display, g_draw.root_window_id,
			&p_num_adaptors, &ai);
	if (ret != Success) {
		if (ret == XvBadExtension)
			fprintf(stderr, "XvBadExtension returned at XvQueryExtension.\n");
		else if (ret == XvBadAlloc)
			fprintf(stderr, "XvBadAlloc returned at XvQueryExtension.\n");
		else
			fprintf(stderr, "other error happaned at XvQueryAdaptors.\n");
		exit(EXIT_FAILURE);
	}

	g_draw.depth = DefaultDepth(g_draw.display, myscreen);

	g_draw.xv_id = -1;
	g_draw.xv_port = -1;
	g_draw.xv = -1;

	for (i = 0; i < p_num_adaptors; i++) {
		p_num_ports = ai[i].base_id + ai[i].num_ports;
		for (p = ai[i].base_id; p < p_num_ports; p++) {
			fo = XvListImageFormats(g_draw.display, p, &formats);
			for (j = 0; j < formats; ++j) {

				if (fo[j].type == XvYUV && fo[j].bits_per_pixel == 16
						&& fo[j].format == XvPacked && strncmp("UYVY",
						fo[j].component_order, 4) == 0) {
					g_draw.xv_port = p;
					g_draw.xv_id = fo[j].id;

					g_draw.xv = GXV_YUV_PORT;
					j = formats;
					p = p_num_ports;
					i = p_num_adaptors;
					break;
				}

				if (fo[j].type == XvRGB && fo[j].bits_per_pixel == 32) {
					g_draw.xv_port = p;
					g_draw.xv_id = fo[j].id;
					printf("RGB mode found.  id: %x\n", g_draw.xv_id);
					g_draw.xv = GXV_RGB_PORT;
					//break out of loops
					j = formats;
					p = p_num_ports;
					i = p_num_adaptors;
				}
			}

			if (fo)
				XFree(fo);
		}
	}

	if (p_num_adaptors > 0)
		XvFreeAdaptorInfo(ai);

	if (g_draw.xv < 0) {
		fprintf(stderr, "RGB & YUV not found. Quitting.\n");
		exit(EXIT_FAILURE);
	}

	Atom atom_vsync = xv_intern_atom_if_exists(g_draw.display,
			"XV_SYNC_TO_VBLANK");
	if (atom_vsync != None) {
		XvGetPortAttribute(g_draw.display, g_draw.xv_port, atom_vsync,
				&g_draw.xv_vsync);
		XvSetPortAttribute(g_draw.display, g_draw.xv_port, atom_vsync, 0);
	}

	g_draw.myvisual = 0;

	if (XMatchVisualInfo(g_draw.display, myscreen, g_draw.depth, TrueColor,
			&g_draw.vi))
		g_draw.myvisual = &g_draw.vi;

	if (!g_draw.myvisual) {
		fprintf(stderr, "Failed to obtain visual!\n");
		DestroyDisplay();
		return;
	}

	if (!g_cfg.FullScreen)
		g_draw.cursor = XCreateFontCursor(g_draw.display, XC_trek);
	else {
		g_draw.cursor = None;
	}

	g_draw.colormap = XCreateColormap(g_draw.display, g_draw.root_window_id,
			g_draw.myvisual->visual, AllocNone);

	winattr.background_pixel = BlackPixelOfScreen(screen);
	winattr.border_pixel = WhitePixelOfScreen(screen);
	winattr.bit_gravity = ForgetGravity;
	winattr.win_gravity = NorthWestGravity;
	winattr.backing_store = NotUseful;

	winattr.override_redirect = False;
	winattr.save_under = False;
	winattr.event_mask = 0;
	winattr.do_not_propagate_mask = 0;
	winattr.colormap = g_draw.colormap;
	winattr.cursor = None;

	g_draw.window = XCreateWindow(g_draw.display, g_draw.root_window_id, 0, 0,
			g_cfg.ResX, g_cfg.ResY, 0, g_draw.myvisual->depth, InputOutput,
			g_draw.myvisual->visual, CWBorderPixel | CWBackPixel | CWEventMask
					| CWDontPropagate | CWColormap | CWCursor, &winattr);

	if (!g_draw.window) {
		fprintf(stderr, "Failed in XCreateWindow()!!!\n");
		DestroyDisplay();
		return;
	}

	delwindow = XInternAtom(g_draw.display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(g_draw.display, g_draw.window, &delwindow, 1);

	hints.flags = USPosition | USSize;
	hints.base_width = g_cfg.ResX;
	hints.base_height = g_cfg.ResY;

	wm_hints.input = 1;
	wm_hints.flags = InputHint;

	XSetWMHints(g_draw.display, g_draw.window, &wm_hints);
	XSetWMNormalHints(g_draw.display, g_draw.window, &hints);
	//if (pCaptionText)
	//	XStoreName(draw_ctx.display, draw_ctx.window, pCaptionText);
	//else
	XStoreName(g_draw.display, g_draw.window, "GXVideo GPU");

	XDefineCursor(g_draw.display, g_draw.window, g_draw.cursor);

	// hack to get rid of window title bar
	if (g_draw.fx) {
		mwmhints.flags = MWM_HINTS_DECORATIONS;
		mwmhints.decorations = 0;
		mwmatom = XInternAtom(g_draw.display, "_MOTIF_WM_HINTS", 0);
		XChangeProperty(g_draw.display, g_draw.window, mwmatom, mwmatom, 32,
				PropModeReplace, (unsigned char *) &mwmhints, 4);
	}

	// key stuff
	XSelectInput(g_draw.display, g_draw.window, FocusChangeMask | ExposureMask
			| KeyPressMask | KeyReleaseMask);

	XMapRaised(g_draw.display, g_draw.window);
	XClearWindow(g_draw.display, g_draw.window);
	XWindowEvent(g_draw.display, g_draw.window, ExposureMask, &event);

	if (g_draw.fx) {
		XResizeWindow(g_draw.display, g_draw.window, screen->width,
				screen->height);

		hints.min_width = hints.max_width = hints.base_width = screen->width;
		hints.min_height = hints.max_height = hints.base_height
				= screen->height;

		XSetWMNormalHints(g_draw.display, g_draw.window, &hints);

		// set the window layer for GNOME
		{
			XEvent xev;

			memset(&xev, 0, sizeof(xev));
			xev.xclient.type = ClientMessage;
			xev.xclient.serial = 0;
			xev.xclient.send_event = 1;
			xev.xclient.message_type = XInternAtom(g_draw.display,
					"_NET_WM_STATE", 0);
			xev.xclient.window = g_draw.window;
			xev.xclient.format = 32;
			xev.xclient.data.l[0] = 1;
			xev.xclient.data.l[1] = XInternAtom(g_draw.display,
					"_NET_WM_STATE_FULLSCREEN", 0);
			xev.xclient.data.l[2] = 0;
			xev.xclient.data.l[3] = 0;
			xev.xclient.data.l[4] = 0;

			XSendEvent(g_draw.display, g_draw.root_window_id, 0,
					SubstructureRedirectMask | SubstructureNotifyMask, &xev);
		}
	}

	gcv.graphics_exposures = False;
	gcv.foreground = WhitePixelOfScreen(screen);
	gcv.background = BlackPixelOfScreen(screen);
	g_draw.hGC = XCreateGC(g_draw.display, g_draw.window, GCGraphicsExposures
			|GCForeground | GCBackground, &gcv);
	if (!g_draw.hGC) {
		fprintf(stderr, "No gfx context!!!\n");
		DestroyDisplay();
	}

	g_draw.Xpixels = (char *) malloc(600 * 15 * 4);

	uint32_t color;
	if (g_draw.xv == GXV_YUV_PORT)
		color = rgb_to_yuv(0x00, 0x00, 0xff);
	else
		color = 0x0000ff;

	for (i = 0; i < 600 * 15; ++i)
		((uint32_t *) g_draw.Xpixels)[i] = color;

	g_draw.XFimage = XCreateImage(g_draw.display, g_draw.myvisual->visual,
			g_draw.depth, ZPixmap, 0, (char *) g_draw.Xpixels, 600, 15,
			g_draw.depth > 16 ? 32 : 16, 0);

	/* fix the green back ground in YUV mode */
	if (g_draw.xv == GXV_YUV_PORT)
		color = rgb_to_yuv(0x00, 0x00, 0x00);
	else
		color = 0;

	g_draw.Xpixels = (char *) malloc(8 * 8 * 4);
	for (i = 0; i < 8 * 8; ++i)
		((uint32_t *) g_draw.Xpixels)[i] = color;

	g_draw.XCimage = XvCreateImage(g_draw.display, g_draw.xv_port,
			g_draw.xv_id, (char *) g_draw.Xpixels, 8, 8);

	/*
	 Allocate max that could be needed:
	 Big(est?) PSX res: 640x512
	 32bpp (times 4)
	 2xsai func= 3xwidth,3xheight
	 = approx 11.8mb
	 */
	g_draw.shminfo.shmid
			= shmget(IPC_PRIVATE, 1024 * 512 * 4, IPC_CREAT | 0777);
	g_draw.shminfo.shmaddr = shmat(g_draw.shminfo.shmid, 0, 0);
	g_draw.shminfo.readOnly = 0;

	if (!XShmAttach(g_draw.display, &g_draw.shminfo)) {
		printf("XShmAttach failed !\n");
		exit(-1);
	}

	uint32_t *pShmaddr = (uint32_t *) g_draw.shminfo.shmaddr;
	for (i = 0; i < 1024 * 512; ++i)
		pShmaddr[i] = color;

	//fprintf(stderr, "Return %s\n", __FUNCTION__);
	return;
}

int rgb_check_port(XvImageFormatValues const * fo) {
	if (fo->type == XvRGB && fo->bits_per_pixel == 32) {
		g_draw.xv_id = fo->id;
		return 1;
	}
	return 0;
}

void rgb_blit_16(int8_t * buff, int32_t x, int32_t y, int32_t w, int32_t h) {
	//fprintf(stderr, "BlitToYUV\n");
	int16_t * src_pxl;
	int8_t * dst_pxl;
	int32_t startxy;
	int32_t _x, _y;

	startxy = 0;
	dst_pxl = buff;
	for (_y = y; _y < h; ++_y) {
		src_pxl = &g_gpu.psx_vram.s16[startxy];
		for (_x = x; _x < w; ++_x) {
			dst_pxl[0] = ((*src_pxl) << 3) & 0xf8;
			dst_pxl[1] = ((*src_pxl) >> 2) & 0xf8;
			dst_pxl[2] = ((*src_pxl) >> 7) & 0xf8;
			dst_pxl += 3;
			src_pxl += 1;
		}
		startxy += 2048;
	}
}

void rgb_blit_24(int8_t * buff, int32_t x, int32_t y, int32_t w, int32_t h) {
	//fprintf(stderr, "BlitToYUV\n");
	int8_t * src_pxl;
	int8_t * dst_pxl;
	int32_t startxy;
	int32_t _x, _y;

	dst_pxl = buff;
	startxy = 0;
	for (_y = y; _y < h; ++_y) {
		src_pxl = &g_gpu.psx_vram.s8[startxy];
		for (_x = x; _x < w; ++_x) {
			dst_pxl[0] = src_pxl[0];
			dst_pxl[1] = src_pxl[1];
			dst_pxl[2] = src_pxl[2];
			dst_pxl += 3;
			src_pxl += 3;
		}
		startxy += 2048;
	}
}

int yuyv_check_port(XvImageFormatValues * fo) {
	if (fo->type == XvYUV && fo->bits_per_pixel == 16 && fo->format == XvPacked
			&& strncmp("UYVY", fo->component_order, 4) == 0) {
		g_draw.xv_id = fo->id;
		return 1;
	}
	return 0;
}

void yuyv_blit_24(int8_t * buff, int32_t x, int32_t y, int32_t w, int32_t h) {
	//fprintf(stderr, "yuyv_blit_24 %d %d %d %d\n", x, y, w, h);
	uint8_t * src_pxl;
	int8_t R0, G0, B0;
	int8_t R1, G1, B1;
	uint32_t * dst_pxl;
	int32_t _x, _y;

	/* x and w should be multiple of 2 to be align */
	x = x & 0xfffffffe;
	w = (w + 1) & 0xfffffffe;

	dst_pxl = (uint32_t *) (buff);
	for (_y = y; _y < (y + h); _y += 1) {
		src_pxl = &g_gpu.psx_vram.u8[x * 3 + (_y * 2048)];
		for (_x = 0; _x < (w / 2); ++_x) {
			R0 = src_pxl[0];
			G0 = src_pxl[1];
			B0 = src_pxl[2];
			R1 = src_pxl[3];
			G1 = src_pxl[4];
			B1 = src_pxl[5];
			*dst_pxl++ = rgb_to_yuv2(R0, G0, B0, R1, G1, B1);
			src_pxl += 6;
		}
	}
}

void yuyv_blit_16(int8_t * buff, int32_t x, int32_t y, int32_t w, int32_t h) {
	//fprintf(stderr, "yuyv_blit_16 %d %d %d %d\n", x, y, w, h);
	int8_t R0, G0, B0;
	int8_t R1, G1, B1;
	uint32_t * dst_pxl;
	int32_t _x, _y;
	int32_t s;
	uint16_t * src_pxl;

	/* x must be a multiple of 2 */
	/* x and w must be a multiple of 2 */
	x = x & 0xfffffffe;
	w = (w + 1) & 0xfffffffe;

	dst_pxl = (uint32_t *) (buff);
	for (_y = y; _y < (y + h); _y += 1) {
		src_pxl = &g_gpu.psx_vram.u16[x + (_y * 1024)];
		for (_x = 0; _x < (w / 2); ++_x) {
			s = GETLE16(src_pxl++);
			R0 = (s << 3) & 0xf8;
			G0 = (s >> 2) & 0xf8;
			B0 = (s >> 7) & 0xf8;
			s = GETLE16(src_pxl++);
			R1 = (s << 3) & 0xf8;
			G1 = (s >> 2) & 0xf8;
			B1 = (s >> 7) & 0xf8;
			*dst_pxl++ = rgb_to_yuv2(R0, G0, B0, R1, G1, B1);
		}
	}
}

/**
 * compute the position and the size of output screen
 * The aspect of the psx output mode is preserved.
 * Note: dest dx,dy,dw,dh are both input and output variables
 */
inline void maintain_aspect(uint32_t * dx, uint32_t * dy, uint32_t * dw,
		uint32_t * dh, int32_t w, int32_t h) {

	double ratio_x = ((double) *dw) / ((double) w);
	double ratio_y = ((double) *dh) / ((double) h);

	double ratio;
	if (ratio_x < ratio_y) {
		ratio = ratio_x;
	} else {
		ratio = ratio_y;
	}

	uint32_t tw = (uint32_t) floor(w * ratio);
	uint32_t th = (uint32_t) floor(h * ratio);

	*dx += (uint32_t) floor((*dw - tw) / 2.0);
	*dy += (uint32_t) floor((*dh - th) / 2.0);
	*dw = tw;
	*dh = th;
}

void do_buffer_swap(void) {
	//Screen *screen;
	Window _dw;
	XvImage *xvi;
	unsigned int dstx, dsty;
	unsigned int _d, _w, _h; //don't care about _d

	dstx = 0;
	dsty = 0;

	XSync(g_draw.display, False);

	XGetGeometry(g_draw.display, g_draw.window, &_dw, (int *) &_d, (int *) &_d,
			&_w, &_h, &_d, &_d);

	if (g_cfg.ShowFPS) {
		dsty += 15;
		_h -= 15;
		compute_fps();
		snprintf(g_draw.msg, 511, "FPS : %f", g_draw.fps);
		XDrawImageString(g_draw.display, g_draw.window, g_draw.hGC, 0, 12,
				g_draw.msg, strlen(g_draw.msg));
	}

	uint32_t _x_, _y_, _w_, _h_;

	/* if video mode is valid */
	if (g_gpu.dsp.mode.x) {

		if (g_gpu.status_reg & STATUS_RGB24) {
			_x_ = (g_gpu.dsp.position.x * 2) / 3;
		} else {
			_x_ = g_gpu.dsp.position.x;
		}
		_y_ = g_gpu.dsp.position.y;
		_w_ = (g_gpu.dsp.range.x1 - g_gpu.dsp.range.x0) / g_gpu.dsp.mode.x;
		_h_ = (g_gpu.dsp.range.y1 - g_gpu.dsp.range.y0) * g_gpu.dsp.mode.y;

		if (g_gpu.status_reg & STATUS_RGB24) {
			available_port[g_draw.xv].blit_24_bits(
					(int8_t *) g_draw.shminfo.shmaddr, _x_, _y_, _w_, _h_);
		} else {
			available_port[GXV_YUV_PORT].blit_16_bits(
					(int8_t *) g_draw.shminfo.shmaddr, _x_, _y_, _w_, _h_);
		}

		xvi = XvShmCreateImage(g_draw.display, g_draw.xv_port, g_draw.xv_id, 0,
				_w_, _h_, &g_draw.shminfo);

		xvi->data = g_draw.shminfo.shmaddr;

		//screen = DefaultScreenOfDisplay(g_draw.display);
		//screennum = DefaultScreen(display);

		//	if (g_cfg.FullScreen) {
		//		_w = screen->width;
		//		_h = screen->height;
		//	}

		if (g_cfg.Maintain43)
			maintain_aspect(&dstx, &dsty, &_w, &_h, _w_, (g_gpu.dsp.range.y1
					- g_gpu.dsp.range.y0));

		//	int _i;
		//	int32_t * dst_pxl;
		//	dst_pxl = (int32_t *) &g_draw.shminfo.shmaddr[2048 * _y_];
		//	for (_i = _x_ / 2; _i < (_x_ + _w_) / 2; ++_i)
		//		dst_pxl[_i] = rgb_to_yuv2(255, 0, 0, 255, 0, 0);
		//	dst_pxl = (int32_t *) &g_draw.shminfo.shmaddr[2048 * (_y_ + _h_)];
		//	for (_i = _x_ / 2; _i < (_x_ + _w_) / 2; ++_i)
		//		dst_pxl[_i] = rgb_to_yuv2(255, 0, 0, 255, 0, 0);
		//	dst_pxl = (int32_t *) &g_draw.shminfo.shmaddr[4 * (_x_ / 2)];
		//	for (_i = _y_; _i < (_y_ + _h_); ++_i)
		//		dst_pxl[_i * 512] = rgb_to_yuv2(255, 0, 0, 255, 0, 0);
		//	dst_pxl = (int32_t *) &g_draw.shminfo.shmaddr[4 * ((_x_ + _w_) / 2)];
		//	for (_i = _y_; _i < (_y_ + _h_); ++_i)
		//		dst_pxl[_i * 512] = rgb_to_yuv2(255, 0, 0, 255, 0, 0);

		//fprintf(stderr, "From %d,%d,%d,%d\n", _x_, _y_, _w_, _h_);
		XvShmPutImage(g_draw.display, g_draw.xv_port, g_draw.window,
				g_draw.hGC, xvi, 0, 0, _w_, _h_,
				//0, 0, 1024, 512,
				/* dst */
				dstx, dsty, _w, _h, 1);
		XFree(xvi);
	}
}

void do_clear_screen_buffer(void) // CLEAR DX BUFFER
{
	XClearWindow(g_draw.display, g_draw.window);
}

void Xcleanup() // X CLEANUP
{
	//CloseMenu();
}

/**
 *  Call on GPUopen
 *  Return the current display.
 **/
unsigned long init_display() {

	available_port[GXV_YUV_PORT].blit_24_bits = yuyv_blit_24;
	available_port[GXV_YUV_PORT].blit_16_bits = yuyv_blit_16;

	available_port[GXV_RGB_PORT].blit_24_bits = rgb_blit_24;
	available_port[GXV_RGB_PORT].blit_16_bits = rgb_blit_16;

	create_display(); // x stuff
	g_draw.bIsFirstFrame = 0;
	return (unsigned long) g_draw.display;
}

void CloseDisplay(void) {
	Xcleanup(); // cleanup dx
	DestroyDisplay();
}

void CreatePic(unsigned char * pMem) {
	unsigned char * p = (unsigned char *) malloc(128 * 96 * 4);
	unsigned char * ps;
	//int x, y;

	ps = p;

	//	if (iDesktopCol == 16) {
	//		unsigned short s;
	//		for (y = 0; y < 96; y++) {
	//			for (x = 0; x < 128; x++) {
	//				s = (*(pMem + 0)) >> 3;
	//				s |= ((*(pMem + 1)) & 0xfc) << 3;
	//				s |= ((*(pMem + 2)) & 0xf8) << 8;
	//				pMem += 3;
	//				*((unsigned short *) (ps + y * 256 + x * 2)) = s;
	//			}
	//		}
	//	} else if (iDesktopCol == 15) {
	//		unsigned short s;
	//		for (y = 0; y < 96; y++) {
	//			for (x = 0; x < 128; x++) {
	//				s = (*(pMem + 0)) >> 3;
	//				s |= ((*(pMem + 1)) & 0xfc) << 2;
	//				s |= ((*(pMem + 2)) & 0xf8) << 7;
	//				pMem += 3;
	//				*((unsigned short *) (ps + y * 256 + x * 2)) = s;
	//			}
	//		}
	//	} else if (iDesktopCol == 32) {
	//		uint32_t l;
	//		for (y = 0; y < 96; y++) {
	//			for (x = 0; x < 128; x++) {
	//				l = *(pMem + 0);
	//				l |= (*(pMem + 1)) << 8;
	//				l |= (*(pMem + 2)) << 16;
	//				pMem += 3;
	//				*((uint32_t *) (ps + y * 512 + x * 4)) = l;
	//			}
	//		}
	//	}

	g_draw.XPimage = XCreateImage(g_draw.display, g_draw.myvisual->visual,
			g_draw.depth, ZPixmap, 0, (char *) p, 128, 96,
			g_draw.depth > 16 ? 32 : 16, 0);
}

void DestroyPic(void) {
	if (g_draw.XPimage) { /*
	 XPutImage(display,window,hGC, XCimage,
	 0, 0, 0, 0, iResX, iResY);*/
		XDestroyImage(g_draw.XPimage);
		g_draw.XPimage = 0;
	}
}

void ShowGpuPic(void) {
}

void ShowTextGpuPic(void) {
}
