/*
 * Copyright (c) 2010, Wei Mingzhi <whistler@openoffice.org>.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#include "pad.h"

GLOBALDATA			g;

static void SetDefaultConfig() {
	memset(&g.cfg, 0, sizeof(g.cfg));

	g.cfg.Threaded = 1;

	g.cfg.PadDef[0].DevNum = 0;
	g.cfg.PadDef[1].DevNum = 1;

	g.cfg.PadDef[0].Type = PSE_PAD_TYPE_STANDARD;
	g.cfg.PadDef[1].Type = PSE_PAD_TYPE_STANDARD;

/*	// Pad1 keyboard
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].Key = XK_c;
	g.cfg.PadDef[0].KeyDef[DKEY_START].Key = XK_v;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].Key = XK_Up;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].Key = XK_Right;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].Key = XK_Down;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].Key = XK_Left;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].Key = XK_e;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].Key = XK_t;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].Key = XK_w;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].Key = XK_r;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].Key = XK_d;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].Key = XK_x;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].Key = XK_z;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].Key = XK_s;*/

	// Pad1 joystick
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].J.Button = 8;
	g.cfg.PadDef[0].KeyDef[DKEY_START].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_START].J.Button = 9;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].J.Axis = -2;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].J.Axis = 1;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].J.Axis = 2;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].J.Axis = -1;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].J.Button = 4;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].J.Button = 6;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].J.Button = 5;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].J.Button = 7;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].J.Button = 0;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].J.Button = 1;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].J.Button = 2;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].J.Button = 3;

	// Pad2 joystick
	g.cfg.PadDef[1].KeyDef[DKEY_SELECT].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_SELECT].J.Button = 8;
	g.cfg.PadDef[1].KeyDef[DKEY_START].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_START].J.Button = 9;
	g.cfg.PadDef[1].KeyDef[DKEY_UP].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_UP].J.Axis = -2;
	g.cfg.PadDef[1].KeyDef[DKEY_RIGHT].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_RIGHT].J.Axis = 1;
	g.cfg.PadDef[1].KeyDef[DKEY_DOWN].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_DOWN].J.Axis = 2;
	g.cfg.PadDef[1].KeyDef[DKEY_LEFT].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_LEFT].J.Axis = -1;
	g.cfg.PadDef[1].KeyDef[DKEY_L2].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_L2].J.Button = 4;
	g.cfg.PadDef[1].KeyDef[DKEY_L1].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_L1].J.Button = 6;
	g.cfg.PadDef[1].KeyDef[DKEY_R2].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_R2].J.Button = 5;
	g.cfg.PadDef[1].KeyDef[DKEY_R1].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_R1].J.Button = 7;
	g.cfg.PadDef[1].KeyDef[DKEY_TRIANGLE].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_TRIANGLE].J.Button = 0;
	g.cfg.PadDef[1].KeyDef[DKEY_CIRCLE].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_CIRCLE].J.Button = 1;
	g.cfg.PadDef[1].KeyDef[DKEY_CROSS].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_CROSS].J.Button = 2;
	g.cfg.PadDef[1].KeyDef[DKEY_SQUARE].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_SQUARE].J.Button = 3;
}

void LoadPADConfig() {
	SetDefaultConfig();
}

long PADconfigure(void) {
	return PSE_PAD_ERR_SUCCESS;
}

void PADabout(void) {
}
