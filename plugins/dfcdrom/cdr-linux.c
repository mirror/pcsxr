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

#ifdef __linux__

char *LibName = N_("CD-ROM Drive Reader");

static int handle = -1;

int OpenCdHandle(const char *dev) {
	char spindown;

	handle = open(dev, O_RDONLY);

	if (handle != -1) {
		ioctl(handle, CDROM_LOCKDOOR, 0);
//		ioctl(handle, CDROMSTART, NULL);

		spindown = (char)SpinDown;
		ioctl(handle, CDROMSETSPINDOWN, &spindown);

		ioctl(handle, CDROM_SELECT_SPEED, CdrSpeed);
	}

	return (handle == -1) ? -1 : 0;
}

void CloseCdHandle() {
	char spindown = SPINDOWN_VENDOR_SPECIFIC;
	ioctl(handle, CDROMSETSPINDOWN, &spindown);

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
	if (ioctl(handle, CDROMREADRAW, cr) == -1)
		return -1;

	return 0;
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
	struct cdrom_subchnl sc;
	int ret;
	char spindown;

	memset(stat, 0, sizeof(struct CdrStat));

	if (playing) { // return Time only if playing
		sc.cdsc_format = CDROM_MSF;
		if (ioctl(handle, CDROMSUBCHNL, &sc) != -1)
			memcpy(stat->Time, &sc.cdsc_absaddr.msf, 3);
	}

	ret = ioctl(handle, CDROM_DISC_STATUS);
	switch (ret) {
		case CDS_AUDIO:
			stat->Type = 0x02;
			break;
		case CDS_DATA_1:
		case CDS_DATA_2:
		case CDS_XA_2_1:
		case CDS_XA_2_2:
			stat->Type = 0x01;
			break;
	}
	ret = ioctl(handle, CDROM_DRIVE_STATUS);
	switch (ret) {
		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
			stat->Type = 0xff;
			stat->Status |= 0x10;
			break;
		default:
			spindown = (char)SpinDown;
			ioctl(handle, CDROMSETSPINDOWN, &spindown);
			ioctl(handle, CDROM_LOCKDOOR, 0);
			break;
	}

	switch (sc.cdsc_audiostatus) {
		case CDROM_AUDIO_PLAY:
			stat->Status |= 0x80;
			break;
	}

	return 0;
}

unsigned char *ReadSub(const unsigned char *time) {
	static struct SubQ subq;
	struct cdrom_subchnl subchnl;
	int ret;
	crdata cr;

	cr.msf.cdmsf_min0 = btoi(time[0]);
	cr.msf.cdmsf_sec0 = btoi(time[1]);
	cr.msf.cdmsf_frame0 = btoi(time[2]);

	if (ioctl(handle, CDROMSEEK, &cr.msf) == -1) {
		// will be slower, but there's no other way to make it accurate
		if (ioctl(handle, CDROMREADRAW, &cr) == -1) {
			return NULL;
		}
	}

	subchnl.cdsc_format = CDROM_MSF;
	ret = ioctl(handle, CDROMSUBCHNL, &subchnl);

	if (ret == -1) return NULL;

	subq.TrackNumber = subchnl.cdsc_trk;
	subq.IndexNumber = subchnl.cdsc_ind;
	subq.TrackRelativeAddress[0] = itob(subchnl.cdsc_reladdr.msf.minute);
	subq.TrackRelativeAddress[1] = itob(subchnl.cdsc_reladdr.msf.second);
	subq.TrackRelativeAddress[2] = itob(subchnl.cdsc_reladdr.msf.frame);
	subq.AbsoluteAddress[0] = itob(subchnl.cdsc_absaddr.msf.minute);
	subq.AbsoluteAddress[1] = itob(subchnl.cdsc_absaddr.msf.second);
	subq.AbsoluteAddress[2] = itob(subchnl.cdsc_absaddr.msf.frame);

	PRINTF("subq : %x,%x : %x,%x,%x : %x,%x,%x\n",
		   subchnl.cdsc_trk, subchnl.cdsc_ind,
		   itob(subchnl.cdsc_reladdr.msf.minute), itob(subchnl.cdsc_reladdr.msf.second), itob(subchnl.cdsc_reladdr.msf.frame),
		   itob(subchnl.cdsc_absaddr.msf.minute), itob(subchnl.cdsc_absaddr.msf.second), itob(subchnl.cdsc_absaddr.msf.frame));

	return (unsigned char *)&subq;
}

#endif
