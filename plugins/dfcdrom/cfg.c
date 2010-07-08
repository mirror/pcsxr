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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "cdr.h"

char CdromDev[256];
long ReadMode;
long UseSubQ;
long CacheSize;
long CdrSpeed;
long SpinDown;

void LoadConf() {
	FILE *f;
	char cfg[255];

	strcpy(CdromDev, DEV_DEF);
	ReadMode = THREADED;
	UseSubQ = 0;
	CacheSize = 64;
	CdrSpeed = 0;
	SpinDown = SPINDOWN_VENDOR_SPECIFIC;

	sprintf(cfg, "dfcdrom.cfg");
	f = fopen(cfg, "r");
	if (f == NULL) return;

	fscanf(f, "CdromDev = %s\n", CdromDev);
	fscanf(f, "ReadMode = %ld\n", &ReadMode);
	fscanf(f, "UseSubQ = %ld\n", &UseSubQ);
	fscanf(f, "CacheSize = %ld\n", &CacheSize);
	fscanf(f, "CdrSpeed = %ld\n", &CdrSpeed);
	fscanf(f, "SpinDown = %ld\n", &SpinDown);
	fclose(f);

	if (ReadMode >= READ_MODES) ReadMode = THREADED;
	if (CacheSize <= 0) CacheSize = 32;
	if (CacheSize > 2048) CacheSize = 2048;
	if (SpinDown <= 0) SpinDown = SPINDOWN_VENDOR_SPECIFIC;
	if (SpinDown > SPINDOWN_32MIN) SpinDown = SPINDOWN_32MIN;
}

void SaveConf() {
	FILE *f;
	char cfg[255];

	sprintf(cfg, "dfcdrom.cfg");
	f = fopen(cfg, "w");
	if (f == NULL)
		return;
	fprintf(f, "CdromDev = %s\n", CdromDev);
	fprintf(f, "ReadMode = %ld\n", ReadMode);
	fprintf(f, "UseSubQ = %ld\n", UseSubQ);
	fprintf(f, "CacheSize = %ld\n", CacheSize);
	fprintf(f, "CdrSpeed = %ld\n", CdrSpeed);
	fprintf(f, "SpinDown = %ld\n", SpinDown);
	fclose(f);
}
