/*
 * Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
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

void InitAnalog() {
	g.PadState[0].AnalogStatus[ANALOG_LEFT][ANALOG_X] = 128;
	g.PadState[0].AnalogStatus[ANALOG_LEFT][ANALOG_Y] = 128;
	g.PadState[0].AnalogStatus[ANALOG_RIGHT][ANALOG_X] = 128;
	g.PadState[0].AnalogStatus[ANALOG_RIGHT][ANALOG_Y] = 128;
	g.PadState[1].AnalogStatus[ANALOG_LEFT][ANALOG_X] = 128;
	g.PadState[1].AnalogStatus[ANALOG_LEFT][ANALOG_Y] = 128;
	g.PadState[1].AnalogStatus[ANALOG_RIGHT][ANALOG_X] = 128;
	g.PadState[1].AnalogStatus[ANALOG_RIGHT][ANALOG_Y] = 128;
}

void CheckAnalog() {
	int			i, j, k, val;
	uint8_t		n;

	for (i = 0; i < 2; i++) {
		if (g.cfg.PadDef[i].Type != PSE_PAD_TYPE_ANALOGPAD) {
			continue;
		}

		for (j = 0; j < ANALOG_TOTAL; j++) {
			for (k = 0; k < 2; k++) {
				if (g.cfg.PadDef[i].AnalogDef[j][k] == 0) {
					continue;
				}

				n = abs(g.cfg.PadDef[i].AnalogDef[j][k]) - 1;

				val = SDL_JoystickGetAxis(g.PadState[i].JoyDev, n);

				val += 32768;
				val /= 256;

				if (g.cfg.PadDef[i].AnalogDef[j][k] < 0) {
					g.PadState[i].AnalogStatus[j][k] = 255 - val;
				} else {
					g.PadState[i].AnalogStatus[j][k] = val;
				}
			}
		}
	}
}
