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

#ifndef __CDR_H__
#define __CDR_H__

//#define DEBUG 1

#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "psemu_plugin_defs.h"

#if defined (__linux__)

#include <linux/cdrom.h>
#ifndef CDROMSETSPINDOWN
#define CDROMSETSPINDOWN 0x531e
#endif
#define DEV_DEF		"/dev/cdrom"

#elif defined (__sun)

#include <sys/cdio.h>

/* The CD-ROM device name seems to vary on different computers on Solaris, so
   let user set this. */
#define DEV_DEF		""

#else

struct cdrom_msf {
	unsigned char cdmsf_min0;     /* start minute */
	unsigned char cdmsf_sec0;     /* start second */
	unsigned char cdmsf_frame0;   /* start frame */
	unsigned char cdmsf_min1;     /* end minute */
	unsigned char cdmsf_sec1;     /* end second */
	unsigned char cdmsf_frame1;   /* end frame */
};

#define CD_SECS			60
#define CD_FRAMES		75
#define CD_FRAMESIZE_RAW	2352
#define CD_FRAMESIZE_SUB	96
#define CD_MSF_OFFSET		150

#if defined (__FreeBSD__)

#include <sys/ata.h>
#include <sys/cdio.h>
#include <sys/cdrio.h>
#include <sys/disklabel.h>

#define DEV_DEF		"/dev/acd0"

#else

#define DEV_DEF		""
#define USE_NULL	1

#endif

#endif

extern char CdromDev[256];
extern long ReadMode;
extern long UseSubQ;
extern long CacheSize;
extern long CdrSpeed;
extern long SpinDown;

#define NORMAL		0
#define THREADED	1
#define READ_MODES	2

#ifndef CD_FRAMESIZE_RAW
#define CD_FRAMESIZE_RAW 2352
#endif

#define DATA_SIZE	(CD_FRAMESIZE_RAW - 12)

// spindown codes
#define SPINDOWN_VENDOR_SPECIFIC	0x00
#define SPINDOWN_125MS				0x01
#define SPINDOWN_250MS				0x02
#define SPINDOWN_500MS				0x03
#define SPINDOWN_1S					0x04
#define SPINDOWN_2S					0x05
#define SPINDOWN_4S					0x06
#define SPINDOWN_8S					0x07
#define SPINDOWN_16S				0x08
#define SPINDOWN_32S				0x09
#define SPINDOWN_1MIN				0x0A
#define SPINDOWN_2MIN				0x0B
#define SPINDOWN_4MIN				0x0C
#define SPINDOWN_8MIN				0x0D
#define SPINDOWN_16MIN				0x0E
#define SPINDOWN_32MIN				0x0F

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

typedef union {
	struct cdrom_msf msf;
	unsigned char buf[CD_FRAMESIZE_RAW];
} crdata;

typedef struct {
	crdata cr;
	int ret;
} CacheData;

struct CdrStat {
	unsigned long Type;
	unsigned long Status;
	unsigned char Time[3];		// current playing time
};

struct SubQ {
	char res0[12];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	unsigned char CRC[2];
	char res1[72];
};

long ReadNormal();
long ReadThreaded();
unsigned char* GetBNormal();
unsigned char* GetBThreaded();

long CDRstop(void);

void LoadConf();
void SaveConf();

#ifdef DEBUG
#define PRINTF printf
#else
#define PRINTF(...) /* */
#endif

unsigned int msf_to_lba(char m, char s, char f);
void lba_to_msf(unsigned int s, char *msf);

int OpenCdHandle();
void CloseCdHandle();
int IsCdHandleOpen();
long GetTN(unsigned char *buffer);
long GetTD(unsigned char track, unsigned char *buffer);
long GetTE(unsigned char track, unsigned char *m, unsigned char *s, unsigned char *f);
long ReadSector(crdata *cr);
long PlayCDDA(unsigned char *sector);
long StopCDDA();
long GetStatus(int playing, struct CdrStat *stat);
unsigned char *ReadSub(const unsigned char *time);

#endif
