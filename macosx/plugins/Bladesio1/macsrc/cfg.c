/*
 * Copyright (c) 2010, Wei Mingzhi <whistler@openoffice.org>.
 * All Rights Reserved.
 *
 * Based on: Cdrom for Psemu Pro like Emulators
 * By: linuzappz <linuzappz@hotmail.com>
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

#include <string.h>
#include <stdint.h>
#include "typedefs.h"
#include "sio1.h"

void AboutDlgProc();
void ConfDlgProc();
void ReadConfig();

Settings settings;

void settingsRead() {
	settings.player = PLAYER_DISABLED;
	strcpy(settings.ip, "127.0.0.1");
	settings.port = 33307;

	ReadConfig();
}

#if 0
extern long SIO1configure() {
	ConfDlgProc();
	return 0;
}

extern void SIO1about() {
	AboutDlgProc();
}
#endif
