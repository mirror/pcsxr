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

#ifndef _GPU_CFG_H_
#define _GPU_CFG_H_

#include <stdint.h>

typedef struct {
	int ResX;
	int ResY;
	int NoStretch;
	int Dithering;
	int FullScreen;
	int ShowFPS;
	int Maintain43;
	int UseFrameLimit;
	int UseFrameSkip;
	int FPSDetection;
	double FrameRate;
	int CfgFixes;
	int UseFixes;
	int video_output;
} gxv_cfg_t;

void ReadConfig(void);
void WriteConfig(void);
void ReadWinSizeConfig(void);

void SoftDlgProc(void);
void AboutDlgProc(void);

#endif
