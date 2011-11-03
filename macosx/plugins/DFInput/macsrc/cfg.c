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

long DoConfiguration();
void DoAbout();

static void SetDefaultConfig() {
	memset(&g.cfg, 0, sizeof(g.cfg));

	g.cfg.Threaded = 1;

	g.cfg.PadDef[0].DevNum = 0;
	g.cfg.PadDef[1].DevNum = 1;

	g.cfg.PadDef[0].Type = PSE_PAD_TYPE_STANDARD;
	g.cfg.PadDef[1].Type = PSE_PAD_TYPE_STANDARD;

	// Pad1 keyboard
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].Key = 9;
	g.cfg.PadDef[0].KeyDef[DKEY_START].Key = 10;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].Key = 127;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].Key = 125;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].Key = 126;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].Key = 124;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].Key = 16;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].Key = 18;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].Key = 14;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].Key = 15;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].Key = 3;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].Key = 8;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].Key = 7;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].Key = 2;
	g.cfg.PadDef[0].KeyDef[DKEY_ANALOG].Key = 12;

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
	FILE		*fp;
	char		buf[256];
	int			current, a, b, c;

	SetDefaultConfig();

	sprintf(buf, "%s/Library/Preferences/net.pcsxr.DFInput.plist", getenv("HOME"));

	fp = fopen(buf, "r");
	if (fp == NULL) {
		return;
	}

	current = 0;

	while (fgets(buf, 256, fp) != NULL) {
		if (strncmp(buf, "Threaded=", 9) == 0) {
			g.cfg.Threaded = atoi(&buf[9]);
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
	char		buf[256];

	sprintf(buf, "%s/Library/Preferences/net.pcsxr.DFInput.plist", getenv("HOME"));

	fp = fopen(buf, "w");
	if (fp == NULL) {
		return;
	}

	fprintf(fp, "[CONFIG]\n");
	fprintf(fp, "Threaded=%d\n", g.cfg.Threaded);
	fprintf(fp, "\n");

	for (i = 0; i < 2; i++) {
		fprintf(fp, "[PAD%d]\n", i + 1);
		fprintf(fp, "DevNum=%d\n", g.cfg.PadDef[i].DevNum);
		fprintf(fp, "Type=%d\n", g.cfg.PadDef[i].Type);

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

	fclose(fp);
}

long PADconfigure(void) {
	if (SDL_WasInit(SDL_INIT_JOYSTICK)) return -1; // cannot change settings on the fly

	DoConfiguration();
	LoadPADConfig();
	return 0;
}

void PADabout(void) {
	DoAbout();
}

struct {
	uint16_t code;
	const char *desc;
} KeyString[] = {
	{ kVK_ANSI_A + 1, "A" },
	{ kVK_ANSI_B + 1, "B" },
	{ kVK_ANSI_C + 1, "C" },
	{ kVK_ANSI_D + 1, "D" },
	{ kVK_ANSI_E + 1, "E" },
	{ kVK_ANSI_F + 1, "F" },
	{ kVK_ANSI_G + 1, "G" },
	{ kVK_ANSI_H + 1, "H" },
	{ kVK_ANSI_I + 1, "I" },
	{ kVK_ANSI_J + 1, "J" },
	{ kVK_ANSI_K + 1, "K" },
	{ kVK_ANSI_L + 1, "L" },
	{ kVK_ANSI_M + 1, "M" },
	{ kVK_ANSI_N + 1, "N" },
	{ kVK_ANSI_O + 1, "O" },
	{ kVK_ANSI_P + 1, "P" },
	{ kVK_ANSI_Q + 1, "Q" },
	{ kVK_ANSI_R + 1, "R" },
	{ kVK_ANSI_S + 1, "S" },
	{ kVK_ANSI_T + 1, "T" },
	{ kVK_ANSI_U + 1, "U" },
	{ kVK_ANSI_V + 1, "V" },
	{ kVK_ANSI_W + 1, "W" },
	{ kVK_ANSI_X + 1, "X" },
	{ kVK_ANSI_Y + 1, "Y" },
	{ kVK_ANSI_Z + 1, "Z" },
	{ kVK_ANSI_LeftBracket + 1, "[" },
	{ kVK_ANSI_RightBracket + 1, "]" },
	{ kVK_ANSI_Semicolon + 1, ";" },
	{ kVK_ANSI_Quote + 1, "'" },
	{ kVK_ANSI_Comma + 1, "," },
	{ kVK_ANSI_Period + 1, "." },
	{ kVK_ANSI_Slash + 1, "/" },
	{ kVK_ANSI_Grave + 1, "`" },
	{ kVK_ANSI_1 + 1, "1" },
	{ kVK_ANSI_2 + 1, "2" },
	{ kVK_ANSI_3 + 1, "3" },
	{ kVK_ANSI_4 + 1, "4" },
	{ kVK_ANSI_5 + 1, "5" },
	{ kVK_ANSI_6 + 1, "6" },
	{ kVK_ANSI_7 + 1, "7" },
	{ kVK_ANSI_8 + 1, "8" },
	{ kVK_ANSI_9 + 1, "9" },
	{ kVK_ANSI_0 + 1, "0" },
	{ kVK_ANSI_Minus + 1, "-" },
	{ kVK_ANSI_Equal + 1, "=" },
	{ kVK_ANSI_Backslash + 1, "\\" },
	{ kVK_Tab + 1, "Tab" },
	{ kVK_Shift + 1, "Shift" },
	{ kVK_Option + 1, "Option" },
	{ kVK_Control + 1, "Control" },
	{ kVK_Command + 1, "Command" },
	{ kVK_Space + 1, "Spacebar" },
	{ kVK_Delete + 1, "Delete" },
	{ kVK_Return + 1, "Return" },
	{ kVK_UpArrow + 1, "Up" },
	{ kVK_DownArrow + 1, "Down" },
	{ kVK_LeftArrow + 1, "Left" },
	{ kVK_RightArrow + 1, "Right" },
	{ kVK_Help + 1, "Help" },
	{ kVK_ForwardDelete + 1, "Forward Delete" },
	{ kVK_Home + 1, "Home" },
	{ kVK_End + 1, "End" },
	{ kVK_PageUp + 1, "Page Up" },
	{ kVK_PageDown + 1, "Page Down" },
	{ kVK_ANSI_KeypadClear + 1, "Keypad Clear" },
	{ kVK_ANSI_KeypadDivide + 1, "Keypad /" },
	{ kVK_ANSI_KeypadMultiply + 1, "Keypad *" },
	{ kVK_ANSI_KeypadMinus + 1, "Keypad -" },
	{ kVK_ANSI_KeypadPlus + 1, "Keypad +" },
	{ kVK_ANSI_KeypadEnter + 1, "Keypad Enter" },
	{ kVK_ANSI_Keypad0 + 1, "Keypad 0" },
	{ kVK_ANSI_Keypad1 + 1, "Keypad 1" },
	{ kVK_ANSI_Keypad2 + 1, "Keypad 2" },
	{ kVK_ANSI_Keypad3 + 1, "Keypad 3" },
	{ kVK_ANSI_Keypad4 + 1, "Keypad 4" },
	{ kVK_ANSI_Keypad5 + 1, "Keypad 5" },
	{ kVK_ANSI_Keypad6 + 1, "Keypad 6" },
	{ kVK_ANSI_Keypad7 + 1, "Keypad 7" },
	{ kVK_ANSI_Keypad8 + 1, "Keypad 8" },
	{ kVK_ANSI_Keypad9 + 1, "Keypad 9" },
	{ kVK_ANSI_KeypadDecimal + 1, "Keypad ." },
	{ kVK_F1 + 1, "F1" },
	{ kVK_F2 + 1, "F2" },
	{ kVK_F3 + 1, "F3" },
	{ kVK_F4 + 1, "F4" },
	{ kVK_F5 + 1, "F5" },
	{ kVK_F6 + 1, "F6" },
	{ kVK_F7 + 1, "F7" },
	{ kVK_F8 + 1, "F8" },
	{ kVK_F9 + 1, "F9" },
	{ kVK_F10 + 1, "F10" },
	{ kVK_F11 + 1, "F11" },
	{ kVK_F12 + 1, "F12" },
	{ kVK_F13 + 1, "F13" },
	{ kVK_F14 + 1, "F14" },
	{ kVK_F15 + 1, "F15" },
	{ 0x00, NULL }
};

static const char *XKeysymToString(uint16_t key) {
	static char buf[64];
	int i = 0;

	while (KeyString[i].code != 0) {
		if (KeyString[i].code == key) {
			strcpy(buf, KeyString[i].desc);
			return buf;
		}
		i++;
	}

	sprintf(buf, "0x%.2X", key);
	return buf;
}

void GetKeyDescription(char *buf, int joynum, int key) {
	const char *hatname[16] = {"Centered", "Up", "Right", "Rightup",
		"Down", "", "Rightdown", "", "Left", "Leftup", "", "",
		"Leftdown", "", "", ""};

	switch (g.cfg.PadDef[joynum].KeyDef[key].JoyEvType) {
		case BUTTON:
			sprintf(buf, "Joystick: Button %d", g.cfg.PadDef[joynum].KeyDef[key].J.Button);
			break;

		case AXIS:
			sprintf(buf, "Joystick: Axis %d%c", abs(g.cfg.PadDef[joynum].KeyDef[key].J.Axis) - 1,
				g.cfg.PadDef[joynum].KeyDef[key].J.Axis > 0 ? '+' : '-');
			break;

		case HAT:
			sprintf(buf, "Joystick: Hat %d %s", (g.cfg.PadDef[joynum].KeyDef[key].J.Hat >> 8),
				hatname[g.cfg.PadDef[joynum].KeyDef[key].J.Hat & 0x0F]);
			break;

		case NONE:
		default:
			buf[0] = '\0';
			break;
	}

	if (g.cfg.PadDef[joynum].KeyDef[key].Key != 0) {
		if (buf[0] != '\0') {
			strcat(buf, " / ");
		}

		strcat(buf, "Keyboard:");
		strcat(buf, " ");
		strcat(buf, XKeysymToString(g.cfg.PadDef[joynum].KeyDef[key].Key));
	}
}

void GetAnalogDescription(char *buf, int joynum, int analognum, int dir) {
	const char *hatname[16] = {"Centered", "Up", "Right", "Rightup",
		"Down", "", "Rightdown", "", "Left", "Leftup", "", "",
		"Leftdown", "", "", ""};

	switch (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].JoyEvType) {
		case BUTTON:
			sprintf(buf, "Joystick: Button %d", g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Button);
			break;

		case AXIS:
			sprintf(buf, "Joystick: Axis %d%c", abs(g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Axis) - 1,
				g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Axis > 0 ? '+' : '-');
			break;

		case HAT:
			sprintf(buf, "Joystick: Hat %d %s", (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Hat >> 8),
				hatname[g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Hat & 0x0F]);
			break;

		case NONE:
		default:
			buf[0] = '\0';
			break;
	}

	if (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].Key != 0) {
		if (buf[0] != '\0') {
			strcat(buf, " / ");
		}

		strcat(buf, "Keyboard:");
		strcat(buf, " ");
		strcat(buf, XKeysymToString(g.cfg.PadDef[joynum].AnalogDef[analognum][dir].Key));
	}
}

int CheckKeyDown() {
	KeyMap theKeys;
	unsigned char *keybytes;
	int i;

	GetKeys(theKeys);
	keybytes = (unsigned char *) theKeys;

	for (i = 0; i < 128; i++) {
		if (i == kVK_CapsLock) continue; // Ignore capslock

		if (keybytes[i >> 3] & (1 << (i & 7)))
			return i + 1;
	}

	return 0;
}

static Sint16 InitialAxisPos[256], PrevAxisPos[256];

#define NUM_AXES(js) (SDL_JoystickNumAxes(js) > 256 ? 256 : SDL_JoystickNumAxes(js))

void InitAxisPos(int padnum) {
	int i;
	SDL_Joystick *js;

	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);
	} else return;

	SDL_JoystickUpdate();

	for (i = 0; i < NUM_AXES(js); i++) {
		InitialAxisPos[i] = PrevAxisPos[i] = SDL_JoystickGetAxis(js, i);
	}

	SDL_JoystickClose(js);
}

int ReadDKeyEvent(int padnum, int key) {
	SDL_Joystick *js;
	int i, changed = 0, t;
	Sint16 axis;

	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);
	} else {
		js = NULL;
	}

	for (t = 0; t < 1000000 / 1000; t++) {
		// check joystick events
		if (js != NULL) {
			SDL_JoystickUpdate();

			for (i = 0; i < SDL_JoystickNumButtons(js); i++) {
				if (SDL_JoystickGetButton(js, i)) {
					g.cfg.PadDef[padnum].KeyDef[key].JoyEvType = BUTTON;
					g.cfg.PadDef[padnum].KeyDef[key].J.Button = i;
					changed = 1;
					goto end;
				}
			}

			for (i = 0; i < NUM_AXES(js); i++) {
				axis = SDL_JoystickGetAxis(js, i);
				if (abs(axis) > 16383 && (abs(axis - PrevAxisPos[i]) > 4096 || abs(axis - InitialAxisPos[i]) > 4096)) {
					g.cfg.PadDef[padnum].KeyDef[key].JoyEvType = AXIS;
					g.cfg.PadDef[padnum].KeyDef[key].J.Axis = (i + 1) * (axis > 0 ? 1 : -1);
					changed = 1;
					goto end;
				}
				PrevAxisPos[i] = axis;
			}

			for (i = 0; i < SDL_JoystickNumHats(js); i++) {
				axis = SDL_JoystickGetHat(js, i);
				if (axis != SDL_HAT_CENTERED) {
					g.cfg.PadDef[padnum].KeyDef[key].JoyEvType = HAT;

					if (axis & SDL_HAT_UP) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_UP);
					} else if (axis & SDL_HAT_DOWN) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_DOWN);
					} else if (axis & SDL_HAT_LEFT) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_LEFT);
					} else if (axis & SDL_HAT_RIGHT) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_RIGHT);
					}

					changed = 1;
					goto end;
				}
			}
		}

		// check keyboard events
		i = CheckKeyDown();
		if (i != 0) {
			if (i != (kVK_Escape + 1)) g.cfg.PadDef[padnum].KeyDef[key].Key = i;
			changed = 1;
			goto end;
		}

		// check mouse events
		if (Button()) {
			changed = 2;
			goto end;
		}

		usleep(1000);
	}

end:
	if (js != NULL) {
		SDL_JoystickClose(js);
	}

	return changed;
}

int ReadAnalogEvent(int padnum, int analognum, int analogdir) {
	SDL_Joystick *js;
	int i, changed = 0, t;
	Sint16 axis;

	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);
	} else {
		js = NULL;
	}

	for (t = 0; t < 1000000 / 1000; t++) {
		// check joystick events
		if (js != NULL) {
			SDL_JoystickUpdate();

			for (i = 0; i < SDL_JoystickNumButtons(js); i++) {
				if (SDL_JoystickGetButton(js, i)) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = BUTTON;
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Button = i;
					changed = 1;
					goto end;
				}
			}

			for (i = 0; i < NUM_AXES(js); i++) {
				axis = SDL_JoystickGetAxis(js, i);
				if (abs(axis) > 16383 && (abs(axis - PrevAxisPos[i]) > 4096 || abs(axis - InitialAxisPos[i]) > 4096)) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = AXIS;
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Axis = (i + 1) * (axis > 0 ? 1 : -1);
					changed = 1;
					goto end;
				}
				PrevAxisPos[i] = axis;
			}

			for (i = 0; i < SDL_JoystickNumHats(js); i++) {
				axis = SDL_JoystickGetHat(js, i);
				if (axis != SDL_HAT_CENTERED) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = HAT;

					if (axis & SDL_HAT_UP) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_UP);
					} else if (axis & SDL_HAT_DOWN) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_DOWN);
					} else if (axis & SDL_HAT_LEFT) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_LEFT);
					} else if (axis & SDL_HAT_RIGHT) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_RIGHT);
					}

					changed = 1;
					goto end;
				}
			}
		}

		// check keyboard events
		i = CheckKeyDown();
		if (i != 0) {
			if (i != (kVK_Escape + 1)) g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].Key = i;
			changed = 1;
			goto end;
		}

		// check mouse events
		if (Button()) {
			changed = 2;
			goto end;
		}

		usleep(1000);
	}

end:
	if (js != NULL) {
		SDL_JoystickClose(js);
	}

	return changed;
}
