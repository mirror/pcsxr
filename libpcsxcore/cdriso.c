/***************************************************************************
 *   Copyright (C) 2007 PCSX-df Team                                       *
 *   Copyright (C) 2009 Wei Mingzhi                                        *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

// TODO: implement CDDA & subchannel support.

#include "psxcommon.h"
#include "plugins.h"

#define MSF2SECT(m, s, f)	(((m) * 60 + (s) - 2) * 75 + (f))
#define btoi(b)			((b) / 16 * 10 + (b) % 16) /* BCD to u_char */

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE		(CD_FRAMESIZE_RAW - 12)

FILE *cdHandle = NULL;
static unsigned char cdbuffer[CD_FRAMESIZE_RAW * 10];

char* CALLBACK CDR__getDriveLetter(void);
long CALLBACK CDR__configure(void);
long CALLBACK CDR__test(void);
void CALLBACK CDR__about(void);
long CALLBACK CDR__setfilename(char *filename);
long CALLBACK CDR__getStatus(struct CdrStat *stat);

extern void *hCDRDriver;

struct trackinfo {
	enum {DATA, CDDA} type;
	char start[3];		// MSF-format
	char length[3];		// MSF-format
	char gap[3];		// MSF-format
};

#define MAXTRACKS 100 /* How many tracks can a CD hold? */

static int numtracks = 0;
static struct trackinfo ti[MAXTRACKS];

// get a sector from a msf-array
static unsigned int msf2sec(char *msf) {
	return ((msf[0] * 60 + msf[1]) * 75) + msf[2];
}

static void sec2msf(unsigned int s, char *msf) {
	msf[0] = s / 75 / 60;
	s = s - msf[0] * 75 * 60;
	msf[1] = s / 75;
	s = s - msf[1] * 75;
	msf[2] = s;
}

// get size of a track given the sector
unsigned int ISOgetTrackLength(unsigned int s) {
	int i = 1;

	while ((msf2sec(ti[i].start) < s) && i <= numtracks)
		i++;

	return msf2sec(ti[--i].length);
}

// divide a string of xx:yy:zz into m, s, f
static void tok2msf(char *time, char *msf) {
	char *token;

	token = strtok(time, ":");
	if (token)
		msf[0] = atoi(token);
	else
		msf[0]=0;

	token = strtok(NULL, ":");
	if (token)
		msf[1] = atoi(token);
	else
		msf[1]=0;

	token = strtok(NULL, ":");
	if (token)
		msf[2] = atoi(token);
	else
		msf[2]=0;
}

// this function tries to get the .toc file of the given .bin
// the neccessary data is put into the ti (trackinformation)-array
static int parsetoc(char *isofile) {
	char tocname[MAXPATHLEN];
	FILE *fi;
	char linebuf[256], dummy[256];
	char *token;
	char name[256];
	char time[20], time2[20];
	unsigned int i, t;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .toc
	strncpy(tocname, isofile, sizeof(tocname));
	token = strstr(tocname, ".bin");
	if (token)
		sprintf((char *)token, ".toc");
	else
		return -1;

	if ((fi = fopen(tocname, "r")) == NULL) {
		SysPrintf(_("Could not open %s.\n"), tocname);
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	// parse the .toc file
	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		// search for tracks
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		// a new track is found
		if (!strcmp(token, "TRACK")) {
			// get type of track
			token = strtok(NULL, " ");

			numtracks++;

			if (!strcmp(token, "MODE2_RAW\n")) {
				ti[numtracks].type = DATA;
				sec2msf(2 * 75, ti[numtracks].start); // assume data track on 0:2:0
			}

			if (!strcmp(token, "AUDIO\n"))
				ti[numtracks].type = CDDA;
		}

		// interpretation of other lines
		if (!strcmp(token, "DATAFILE")) {
			sscanf(linebuf, "DATAFILE %s %s", name, time);
			tok2msf((char *)&time, (char *)&ti[numtracks].length);
		}

		if (!strcmp(token, "FILE")) {
			sscanf(linebuf, "FILE %s %s %s %s", name, dummy, time, time2);
			tok2msf((char *)&time, (char *)&ti[numtracks].start);
			tok2msf((char *)&time2, (char *)&ti[numtracks].length);
		}

		if (!strcmp(token, "START")) {
			sscanf(linebuf, "START %s", time);
			tok2msf((char *)&time, (char *)&ti[numtracks].gap);
		}
	} 

	fclose(fi);

	// calculate the true start of each track
	// start+gap+datasize (+2 secs of silence ? I dunno...)
	for(i = 2; i <= numtracks; i++) {
		t = msf2sec(ti[1].start) + msf2sec(ti[1].length) + msf2sec(ti[i].start) + msf2sec(ti[i].gap);
		sec2msf(t, ti[i].start);
	}

	return 0;
}

static long CALLBACK ISOinit(void) {
	assert(cdHandle == NULL);
	return 0; // do nothing
}

static long CALLBACK ISOshutdown(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	return 0;
}

// This function is invoked by the front-end when opening an ISO
// file for playback
static long CALLBACK ISOopen(void) {
	if (cdHandle != NULL)
		return 0; // it's already open

	cdHandle = fopen(cdrfilename, "rb");
	if (cdHandle == NULL)
		return -1;

	parsetoc(cdrfilename);
	return 0;
}

static long CALLBACK ISOclose(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
static long CALLBACK ISOgetTN(unsigned char *buffer) {
	buffer[0] = 1;

	if (numtracks > 0)
		buffer[1] = numtracks;
	else
		buffer[1] = 1;

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
static long CALLBACK ISOgetTD(unsigned char track, unsigned char *buffer) {
	if (numtracks > 0 && track <= numtracks) {
		buffer[2] = ti[track].start[0];
		buffer[1] = ti[track].start[1];
		buffer[0] = ti[track].start[2];
	} else {
		buffer[2] = 0;
		buffer[1] = 2;
		buffer[0] = 0;
	}

	return 0;
}

// read track
// time: byte 0 - minute; byte 1 - second; byte 2 - frame
// uses bcd format
static long CALLBACK ISOreadTrack(unsigned char *time) {
	if (cdHandle == NULL)
		return -1;

	fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * CD_FRAMESIZE_RAW + 12, SEEK_SET);
	fread(cdbuffer, 1, DATA_SIZE, cdHandle);

	return 0;
}

// return readed track
static unsigned char * CALLBACK ISOgetBuffer(void) {
	return (unsigned char *)&cdbuffer;
}

// plays cdda audio
// sector: byte 0 - minute; byte 1 - second; byte 2 - frame
// does NOT uses bcd format
static long CALLBACK ISOplay(unsigned char *time) {
	return 0; // TODO
}

// stops cdda audio
static long CALLBACK ISOstop(void) {
	return 0; // TODO
}

// gets subchannel data
unsigned char* CALLBACK ISOgetBufferSub(void) {
	return NULL; // TODO
}

void imageReaderInit(void) {
	assert(hCDRDriver == NULL);

	CDR_init = ISOinit;
	CDR_shutdown = ISOshutdown;
	CDR_open = ISOopen;
	CDR_close = ISOclose;
	CDR_getTN = ISOgetTN;
	CDR_getTD = ISOgetTD;
	CDR_readTrack = ISOreadTrack;
	CDR_getBuffer = ISOgetBuffer;
	CDR_play = ISOplay;
	CDR_stop = ISOstop;
	CDR_getBufferSub = ISOgetBufferSub;

	CDR_getStatus = CDR__getStatus;
	CDR_getDriveLetter = CDR__getDriveLetter;
	CDR_configure = CDR__configure;
	CDR_test = CDR__test;
	CDR_about = CDR__about;
	CDR_setfilename = CDR__setfilename;

	numtracks = 0;
}
