/*
 * Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#define CONFIG_FILE		"dfinput.cfg"

GLOBALDATA			g;

static void SetDefaultConfig() {
	memset(&g.cfg, 0, sizeof(g.cfg));

	g.cfg.Threaded = 1;
	g.cfg.HideCursor = 0;
	g.cfg.PreventScrSaver = 0u;

	g.cfg.PadDef[0].DevNum = 0;
	g.cfg.PadDef[1].DevNum = 1;

	g.cfg.PadDef[0].Type = PSE_PAD_TYPE_STANDARD;
	g.cfg.PadDef[1].Type = PSE_PAD_TYPE_STANDARD;

	g.cfg.PadDef[0].VisualVibration = 0;
	g.cfg.PadDef[1].VisualVibration = 0;
    g.cfg.PadDef[0].PhysicalVibration = 1;
    g.cfg.PadDef[1].PhysicalVibration = 1;

	// Pad1 keyboard
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
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].Key = XK_s;
	g.cfg.PadDef[0].KeyDef[DKEY_ANALOG].Key = XK_b;

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

	// Emu special
	g.cfg.E.DevNum = -1;
	g.cfg.E.EmuKeyDev = 0;

	g.cfg.E.EmuDef[EMU_SAVESTATE].EmuKeyEvent = XK_F1;
	g.cfg.E.EmuDef[EMU_FASTFORWARDS].EmuKeyEvent = XK_section;
	g.cfg.E.EmuDef[EMU_LOADSTATE].EmuKeyEvent = XK_F3;
	g.cfg.E.EmuDef[EMU_INCREMENTSTATE].EmuKeyEvent = XK_F2;
	g.cfg.E.EmuDef[EMU_SCREENSHOT].EmuKeyEvent = XK_F8;
	g.cfg.E.EmuDef[EMU_ESCAPE].EmuKeyEvent = XK_Escape;
	g.cfg.E.EmuDef[EMU_REWIND].EmuKeyEvent = XK_BackSpace;
	g.cfg.E.EmuDef[EMU_ALTSPEED1].EmuKeyEvent = XK_bracketleft;
	g.cfg.E.EmuDef[EMU_ALTSPEED2].EmuKeyEvent = XK_bracketright;
}

void LoadPADConfig() {
	FILE		*fp;
	char		buf[256];
	int			current, a, b, c;

	SetDefaultConfig();

	fp = fopen(CONFIG_FILE, "r");
	if (fp == NULL) {
		return;
	}

	current = 0;

	while (fgets(buf, 256, fp) != NULL) {
		if (strncmp(buf, "Threaded=", 9) == 0) {
			g.cfg.Threaded = atoi(&buf[9]);
		} else if (strncmp(buf, "HideCursor=", 11) == 0) {
			g.cfg.HideCursor = atoi(&buf[11]);
		} else if (strncmp(buf, "PreventScrSaver=", 16) == 0) {
			g.cfg.PreventScrSaver = atoi(&buf[16]);
		} else if (strncmp(buf, "[PAD", 4) == 0) {
			current = atoi(&buf[4]) - 1;
			if (current < 0) {
				current = 0;
			} else if (current > 1) {
				current = 1;
			}
		} else if (strncmp(buf, "DevNum=", 7) == 0) {
			g.cfg.PadDef[current].DevNum = atoi(&buf[7]);
		} else if (strncmp(buf, "Type=", 5) == 0) {
			g.cfg.PadDef[current].Type = atoi(&buf[5]);
		} else if (strncmp(buf, "VisualVibration=", 16) == 0) {
			g.cfg.PadDef[current].VisualVibration = atoi(&buf[16]);
        } else if (strncmp(buf, "PhysicalVibration=", 18) == 0) {
            g.cfg.PadDef[current].PhysicalVibration = atoi(&buf[18]);
		} else if (strncmp(buf, "EmuDev=", 7) == 0) {
			g.cfg.E.DevNum = atoi(&buf[5]);
		} else if (strncmp(buf, "EMU_FASTFORWARDS=", 17) == 0) {
			sscanf(buf, "EMU_FASTFORWARDS=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_FASTFORWARDS].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_FASTFORWARDS].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_FASTFORWARDS].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_SAVESTATE=", 14) == 0) {
			sscanf(buf, "EMU_SAVESTATE=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_SAVESTATE].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_SAVESTATE].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_SAVESTATE].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_LOADSTATE=", 14) == 0) {
			sscanf(buf, "EMU_LOADSTATE=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_LOADSTATE].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_LOADSTATE].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_LOADSTATE].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_SCREENSHOT=", 15) == 0) {
			sscanf(buf, "EMU_SCREENSHOT=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_SCREENSHOT].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_SCREENSHOT].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_SCREENSHOT].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_INCREMENTSTATE=", 19) == 0) {
			sscanf(buf, "EMU_INCREMENTSTATE=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_INCREMENTSTATE].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_INCREMENTSTATE].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_INCREMENTSTATE].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_ESCAPE=", 11) == 0) {
			sscanf(buf, "EMU_ESCAPE=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_ESCAPE].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_ESCAPE].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_ESCAPE].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_REWIND=", 11) == 0) {
			sscanf(buf, "EMU_REWIND=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_REWIND].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_REWIND].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_REWIND].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_ALTSPEED1=", 14) == 0) {
			sscanf(buf, "EMU_ALTSPEED1=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_ALTSPEED1].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_ALTSPEED1].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_ALTSPEED1].Mapping.J.d = c;
		} else if (strncmp(buf, "EMU_ALTSPEED2=", 14) == 0) {
			sscanf(buf, "EMU_ALTSPEED2=%d,%d,%d", &a, &b, &c);
			g.cfg.E.EmuDef[EMU_ALTSPEED2].Mapping.Key = a;
			g.cfg.E.EmuDef[EMU_ALTSPEED2].Mapping.JoyEvType = b;
			g.cfg.E.EmuDef[EMU_ALTSPEED2].Mapping.J.d = c;
		} else if (strncmp(buf, "Select=", 7) == 0) {
			sscanf(buf, "Select=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_SELECT].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_SELECT].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_SELECT].J.d = c;
		} else if (strncmp(buf, "L3=", 3) == 0) {
			sscanf(buf, "L3=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_L3].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_L3].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_L3].J.d = c;
		} else if (strncmp(buf, "R3=", 3) == 0) {
			sscanf(buf, "R3=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_R3].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_R3].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_R3].J.d = c;
		} else if (strncmp(buf, "Analog=", 7) == 0) {
			sscanf(buf, "Analog=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_ANALOG].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_ANALOG].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_ANALOG].J.d = c;
		} else if (strncmp(buf, "Start=", 6) == 0) {
			sscanf(buf, "Start=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_START].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_START].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_START].J.d = c;
		} else if (strncmp(buf, "Up=", 3) == 0) {
			sscanf(buf, "Up=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_UP].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_UP].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_UP].J.d = c;
		} else if (strncmp(buf, "Right=", 6) == 0) {
			sscanf(buf, "Right=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_RIGHT].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_RIGHT].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_RIGHT].J.d = c;
		} else if (strncmp(buf, "Down=", 5) == 0) {
			sscanf(buf, "Down=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_DOWN].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_DOWN].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_DOWN].J.d = c;
		} else if (strncmp(buf, "Left=", 5) == 0) {
			sscanf(buf, "Left=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_LEFT].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_LEFT].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_LEFT].J.d = c;
		} else if (strncmp(buf, "L2=", 3) == 0) {
			sscanf(buf, "L2=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_L2].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_L2].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_L2].J.d = c;
		} else if (strncmp(buf, "R2=", 3) == 0) {
			sscanf(buf, "R2=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_R2].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_R2].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_R2].J.d = c;
		} else if (strncmp(buf, "L1=", 3) == 0) {
			sscanf(buf, "L1=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_L1].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_L1].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_L1].J.d = c;
		} else if (strncmp(buf, "R1=", 3) == 0) {
			sscanf(buf, "R1=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_R1].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_R1].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_R1].J.d = c;
		} else if (strncmp(buf, "Triangle=", 9) == 0) {
			sscanf(buf, "Triangle=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_TRIANGLE].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_TRIANGLE].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_TRIANGLE].J.d = c;
		} else if (strncmp(buf, "Circle=", 7) == 0) {
			sscanf(buf, "Circle=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_CIRCLE].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_CIRCLE].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_CIRCLE].J.d = c;
		} else if (strncmp(buf, "Cross=", 6) == 0) {
			sscanf(buf, "Cross=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_CROSS].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_CROSS].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_CROSS].J.d = c;
		} else if (strncmp(buf, "Square=", 7) == 0) {
			sscanf(buf, "Square=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].KeyDef[DKEY_SQUARE].Key = a;
			g.cfg.PadDef[current].KeyDef[DKEY_SQUARE].JoyEvType = b;
			g.cfg.PadDef[current].KeyDef[DKEY_SQUARE].J.d = c;
		} else if (strncmp(buf, "LeftAnalogXP=", 13) == 0) {
			sscanf(buf, "LeftAnalogXP=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XP].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XP].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XP].J.d = c;
		} else if (strncmp(buf, "LeftAnalogXM=", 13) == 0) {
			sscanf(buf, "LeftAnalogXM=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XM].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XM].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XM].J.d = c;
		} else if (strncmp(buf, "LeftAnalogYP=", 13) == 0) {
			sscanf(buf, "LeftAnalogYP=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YP].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YP].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YP].J.d = c;
		} else if (strncmp(buf, "LeftAnalogYM=", 13) == 0) {
			sscanf(buf, "LeftAnalogYM=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YM].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YM].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YM].J.d = c;
		} else if (strncmp(buf, "RightAnalogXP=", 14) == 0) {
			sscanf(buf, "RightAnalogXP=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XP].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XP].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XP].J.d = c;
		} else if (strncmp(buf, "RightAnalogXM=", 14) == 0) {
			sscanf(buf, "RightAnalogXM=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XM].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XM].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XM].J.d = c;
		} else if (strncmp(buf, "RightAnalogYP=", 14) == 0) {
			sscanf(buf, "RightAnalogYP=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YP].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YP].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YP].J.d = c;
		} else if (strncmp(buf, "RightAnalogYM=", 14) == 0) {
			sscanf(buf, "RightAnalogYM=%d,%d,%d", &a, &b, &c);
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YM].Key = a;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YM].JoyEvType = b;
			g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YM].J.d = c;
		}
	}

	fclose(fp);
}

void SavePADConfig() {
	FILE		*fp;
	int			i;

	fp = fopen(CONFIG_FILE, "w");
	if (fp == NULL) {
		return;
	}

	fprintf(fp, "[CONFIG]\n");
	fprintf(fp, "Threaded=%d\n", g.cfg.Threaded);
	fprintf(fp, "HideCursor=%d\n", g.cfg.HideCursor);
	fprintf(fp, "PreventScrSaver=%d\n", g.cfg.PreventScrSaver);
	fprintf(fp, "\n");

	for (i = 0; i < 2; i++) {
		fprintf(fp, "[PAD%d]\n", i + 1);
		fprintf(fp, "DevNum=%d\n", g.cfg.PadDef[i].DevNum);
		fprintf(fp, "Type=%d\n", g.cfg.PadDef[i].Type);
		fprintf(fp, "VisualVibration=%d\n", g.cfg.PadDef[i].VisualVibration);
        fprintf(fp, "PhysicalVibration=%d\n", g.cfg.PadDef[i].PhysicalVibration);

		fprintf(fp, "Select=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_SELECT].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_SELECT].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_SELECT].J.d);
		fprintf(fp, "L3=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_L3].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_L3].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_L3].J.d);
		fprintf(fp, "R3=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_R3].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_R3].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_R3].J.d);
		fprintf(fp, "Analog=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_ANALOG].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_ANALOG].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_ANALOG].J.d);
		fprintf(fp, "Start=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_START].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_START].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_START].J.d);
		fprintf(fp, "Up=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_UP].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_UP].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_UP].J.d);
		fprintf(fp, "Right=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_RIGHT].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_RIGHT].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_RIGHT].J.d);
		fprintf(fp, "Down=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_DOWN].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_DOWN].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_DOWN].J.d);
		fprintf(fp, "Left=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_LEFT].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_LEFT].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_LEFT].J.d);
		fprintf(fp, "L2=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_L2].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_L2].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_L2].J.d);
		fprintf(fp, "R2=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_R2].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_R2].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_R2].J.d);
		fprintf(fp, "L1=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_L1].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_L1].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_L1].J.d);
		fprintf(fp, "R1=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_R1].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_R1].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_R1].J.d);
		fprintf(fp, "Triangle=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_TRIANGLE].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_TRIANGLE].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_TRIANGLE].J.d);
		fprintf(fp, "Circle=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_CIRCLE].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_CIRCLE].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_CIRCLE].J.d);
		fprintf(fp, "Cross=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_CROSS].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_CROSS].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_CROSS].J.d);
		fprintf(fp, "Square=%d,%d,%d\n", g.cfg.PadDef[i].KeyDef[DKEY_SQUARE].Key,
			g.cfg.PadDef[i].KeyDef[DKEY_SQUARE].JoyEvType, g.cfg.PadDef[i].KeyDef[DKEY_SQUARE].J.d);
		fprintf(fp, "LeftAnalogXP=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_XP].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_XP].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_XP].J.d);
		fprintf(fp, "LeftAnalogXM=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_XM].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_XM].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_XM].J.d);
		fprintf(fp, "LeftAnalogYP=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_YP].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_YP].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_YP].J.d);
		fprintf(fp, "LeftAnalogYM=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_YM].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_YM].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_LEFT][ANALOG_YM].J.d);
		fprintf(fp, "RightAnalogXP=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_XP].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_XP].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_XP].J.d);
		fprintf(fp, "RightAnalogXM=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_XM].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_XM].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_XM].J.d);
		fprintf(fp, "RightAnalogYP=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_YP].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_YP].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_YP].J.d);
		fprintf(fp, "RightAnalogYM=%d,%d,%d\n", g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_YM].Key,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_YM].JoyEvType,
			g.cfg.PadDef[i].AnalogDef[ANALOG_RIGHT][ANALOG_YM].J.d);

		fprintf(fp, "\n");
	}

	// Emulator keys
	fprintf(fp, "[EMU]\n");
	fprintf(fp, "EmuDev=%d\n", g.cfg.E.DevNum);
	fprintf(fp, "EMU_SAVESTATE=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_SAVESTATE].Mapping.Key,
			g.cfg.E.EmuDef[EMU_SAVESTATE].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_SAVESTATE].Mapping.J.d);
	fprintf(fp, "EMU_LOADSTATE=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_LOADSTATE].Mapping.Key,
			g.cfg.E.EmuDef[EMU_LOADSTATE].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_LOADSTATE].Mapping.J.d);
	fprintf(fp, "EMU_INCREMENTSTATE=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_INCREMENTSTATE].Mapping.Key,
			g.cfg.E.EmuDef[EMU_INCREMENTSTATE].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_INCREMENTSTATE].Mapping.J.d);
	fprintf(fp, "EMU_FASTFORWARDS=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_FASTFORWARDS].Mapping.Key,
			g.cfg.E.EmuDef[EMU_FASTFORWARDS].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_FASTFORWARDS].Mapping.J.d);
	fprintf(fp, "EMU_SCREENSHOT=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_SCREENSHOT].Mapping.Key,
			g.cfg.E.EmuDef[EMU_SCREENSHOT].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_SCREENSHOT].Mapping.J.d);
	fprintf(fp, "EMU_ESCAPE=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_ESCAPE].Mapping.Key,
			g.cfg.E.EmuDef[EMU_ESCAPE].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_ESCAPE].Mapping.J.d);
	fprintf(fp, "EMU_REWIND=%d,%d,%d\n", g.cfg.E.EmuDef[EMU_REWIND].Mapping.Key,
			g.cfg.E.EmuDef[EMU_REWIND].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_REWIND].Mapping.J.d);
	fprintf(fp, "EMU_ALTSPEED1=%d,%d,%d\n",
			g.cfg.E.EmuDef[EMU_ALTSPEED1].Mapping.Key,
			g.cfg.E.EmuDef[EMU_ALTSPEED1].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_ALTSPEED1].Mapping.J.d);
	fprintf(fp, "EMU_ALTSPEED2=%d,%d,%d\n",
			g.cfg.E.EmuDef[EMU_ALTSPEED2].Mapping.Key,
			g.cfg.E.EmuDef[EMU_ALTSPEED2].Mapping.JoyEvType,
			g.cfg.E.EmuDef[EMU_ALTSPEED2].Mapping.J.d);
	fclose(fp);
}
