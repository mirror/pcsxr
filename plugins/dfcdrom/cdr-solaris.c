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

#ifdef __sun

char *LibName = N_("CD-ROM Device Reader");

int handle = -1;

int OpenCdHandle(const char *dev) {
	char spindown;

	handle = open(dev, O_RDONLY);

	if (handle != -1) {
//		ioctl(handle, CDROM_LOCKDOOR, 0);

		spindown = (char)SpinDown;
//		ioctl(handle, CDROMSETSPINDOWN, &spindown);

//		ioctl(handle, CDROM_SELECT_SPEED, CdrSpeed);
	}

	return (handle == -1) ? -1 : 0;
}

void CloseCdHandle() {
	char spindown = SPINDOWN_VENDOR_SPECIFIC;
//	ioctl(handle, CDROMSETSPINDOWN, &spindown);

	close(handle);

	handle = -1;
}

int IsCdHandleOpen() {
	return (handle != -1);
}

long GetTN(unsigned char *buffer) {
	struct cdrom_tochdr toc;

	if (ioctl(handle, CDROMREADTOCHDR, &toc) == -1)
		return -1;

	buffer[0] = toc.cdth_trk0;	// start track
	buffer[1] = toc.cdth_trk1;	// end track

	return 0;
}

long GetTD(unsigned char track, unsigned char *buffer) {
	struct cdrom_tocentry entry;

	if (track == 0)
		track = 0xaa;			// total time
	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;

	if (ioctl(handle, CDROMREADTOCENTRY, &entry) == -1)
		return -1;

	buffer[0] = entry.cdte_addr.msf.frame;	/* frame */
	buffer[1] = entry.cdte_addr.msf.second;	/* second */
	buffer[2] = entry.cdte_addr.msf.minute;	/* minute */

	return 0;
}

long GetTE(unsigned char track, unsigned char *m, unsigned char *s, unsigned char *f) {
	struct cdrom_tocentry entry;
	char msf[3];

	entry.cdte_track = track + 1;
	entry.cdte_format = CDROM_MSF;

	if (ioctl(handle, CDROMREADTOCENTRY, &entry) == -1)
		return -1;

	lba_to_msf(msf_to_lba(entry.cdte_addr.msf.minute, entry.cdte_addr.msf.second, entry.cdte_addr.msf.frame) - CD_MSF_OFFSET, msf);

	*m = msf[0];
	*s = msf[1];
	*f = msf[2];

	return 0;
}

long ReadSector(crdata *cr) {
	return -1;
}

long PlayCDDA(unsigned char *sector) {
	struct cdrom_msf addr;
	unsigned char ptmp[4];

	// 0 is the last track of every cdrom, so play up to there
	if (GetTD(0, ptmp) == -1)
		return -1;

	addr.cdmsf_min0 = sector[0];
	addr.cdmsf_sec0 = sector[1];
	addr.cdmsf_frame0 = sector[2];
	addr.cdmsf_min1 = ptmp[2];
	addr.cdmsf_sec1 = ptmp[1];
	addr.cdmsf_frame1 = ptmp[0];

	if (ioctl(handle, CDROMPLAYMSF, &addr) == -1)
		return -1;

	return 0;
}

long StopCDDA() {
	struct cdrom_subchnl sc;

	sc.cdsc_format = CDROM_MSF;
	if (ioctl(handle, CDROMSUBCHNL, &sc) == -1)
		return -1;

	switch (sc.cdsc_audiostatus) {
		case CDROM_AUDIO_PAUSED:
		case CDROM_AUDIO_PLAY:
			ioctl(handle, CDROMSTOP);
			break;
	}

	return 0;
}

long GetStatus(int playing, struct CdrStat *stat) {
	return -1;
}

unsigned char *ReadSub(const unsigned char *time) {
	return NULL;
}

#endif
