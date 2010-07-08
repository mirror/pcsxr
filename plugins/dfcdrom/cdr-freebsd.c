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

#include "cdr.h"

#ifdef __FreeBSD__

char *LibName = N_("CD-ROM Drive Reader");

int OpenCdHandle(const char *dev) {
	int h, parameter;
	char spindown;

	h = open(dev, O_RDONLY);

	if (h != -1) {
		if (SpinDown != SPINDOWN_VENDOR_SPECIFIC) {
			if (SpinDown > SPINDOWN_1S) {
				parameter = ((int)SpinDown << (SpinDown - SPINDOWN_1S));
			} else {
				parameter = 1;
			}

			ioctl(h, IOCATASSPINDOWN, &parameter);

			parameter = CdrSpeed * 177;
			if (parameter == 0) parameter = CDR_MAX_SPEED;

			ioctl(h, CDRIOCREADSPEED, &parameter);

			parameter = CD_FRAMESIZE_RAW;
			ioctl(h, CDRIOCSETBLOCKSIZE, &parameter);
		}
	}

	return h;
}

void CloseCdHandle(int handle) {
	int parameter;

	parameter = 0;
	ioctl(handle, IOCATASSPINDOWN, &parameter);

	parameter = CDR_MAX_SPEED;
	ioctl(h, CDRIOCREADSPEED, &parameter);

	close(handle);
}

long GetTN(int handle, unsigned char *buffer) {
	buffer[0] = 0;
	buffer[1] = 0;
	return 0;
}

long GetTD(int handle, unsigned char track, unsigned char *buffer) {
	memset(buffer + 1, 0, 3);
	return 0;
}

long GetTE(int handle, unsigned char track, unsigned char *m, unsigned char *s, unsigned char *f) {
	return -1;
}

long ReadSector(int handle, crdata *cr) {
	unsigned int lba = msf_to_lba(cr->msf.cdmsf_min0, cr->msf.cdmsf_sec0,
								  cr->msf.cdmsf_frame0);

	if (pread(handle, (void *)cr->buf, CD_FRAMESIZE_RAW, lba * CD_FRAMESIZE_RAW) != CD_FRAMESIZE_RAW)
		return -1;

	return 0;
}

long PlayCDDA(int handle, unsigned char *sector) {
	return -1;
}

long StopCDDA(int handle) {
	return -1;
}

long GetStatus(int handle, int playing, struct CdrStat *stat) {
	return -1;
}

unsigned char *ReadSub(int handle, const unsigned char *time) {
	return NULL;
}

#endif
