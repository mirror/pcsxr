/***************************************************************************
                          prim.h  -  description
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

#ifndef _PRIMDRAW_H_
#define _PRIMDRAW_H_

typedef struct {
	char bUsingTWin;
	gxv_win_t TWin;
	//unsigned long  clutid;                                 // global clut
	unsigned short usMirror;                             // sprite mirror
	int            iDither;
	int32_t        drawX;
	int32_t        drawY;
	int32_t        drawW;
	int32_t        drawH;
	uint32_t       dwCfgFixes;
	uint32_t       dwActFixes;
	uint32_t       dwEmuFixes;
	int            iUseFixes;
	int            iUseDither;
} gxv_prim_t;

extern void (*primTableJ[256])(unsigned char *);

void UploadScreen (long Position);
void PrepareFullScreenUpload (long Position);

#endif // _PRIMDRAW_H_
