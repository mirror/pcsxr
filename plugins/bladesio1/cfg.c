/***************************************************************************
 *   Copyright (C) 2013 by Blade_Arma <edgbla@yandex.ru>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "psxcommon.h"
#include "psemu_plugin_defs.h"

#include "sio1.h"

/***************************************************************************/

static const char configName[] = "bladesio1.cfg";

Settings settings;

/***************************************************************************/

void settingsRead() {
	FILE *file;

	file = fopen(configName, "rb");
	if(file) {
		fread(&settings, 1, sizeof(settings), file);
		fclose(file);
	}
	else {
		settings.player = PLAYER_DISABLED;
		strcpy(settings.ip, "127.0.0.1");
		settings.port = 33307;
	}
}

void settingsWrite() {
	FILE *file;

	file = fopen(configName, "wb");
	if(file) {
		fwrite(&settings, 1, sizeof(settings), file);
		fclose(file);
	}
}

/***************************************************************************/
