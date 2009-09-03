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

#include "psxcommon.h"
#include "plugins.h"

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#define MSF2SECT(m, s, f)		(((m) * 60 + (s) - 2) * 75 + (f))
#define btoi(b)					((b) / 16 * 10 + (b) % 16) /* BCD to u_char */

#define CD_FRAMESIZE_RAW		2352
#define DATA_SIZE				(CD_FRAMESIZE_RAW - 12)

#define SUB_FRAMESIZE			96

FILE *cdHandle = NULL;
FILE *cddaHandle = NULL;
FILE *subHandle = NULL;

static unsigned char cdbuffer[DATA_SIZE];
static unsigned char subbuffer[SUB_FRAMESIZE];

static unsigned char sndbuffer[CD_FRAMESIZE_RAW * 10];

#ifdef _WIN32
static HANDLE threadid;
#else
static pthread_t threadid;
#endif
static int initial_offset = 0;
static int playing = 0;

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

// divide a string of xx:yy:zz into m, s, f
static void tok2msf(char *time, char *msf) {
	char *token;

	token = strtok(time, ":");
	if (token) {
		msf[0] = atoi(token);
	}
	else {
		msf[0] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[1] = atoi(token);
	}
	else {
		msf[1] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[2] = atoi(token);
	}
	else {
		msf[2] = 0;
	}
}

#ifndef _WIN32
static long GetTickCount(void) {
	static time_t		initial_time = 0;
	struct timeval		now;

	gettimeofday(&now, NULL);

	if (initial_time == 0) {
		initial_time = now.tv_sec;
	}

	return (now.tv_sec - initial_time) * 1000L + now.tv_usec / 1000L;
}
#endif

// this thread plays audio data
#ifdef _WIN32
static void playthread(void *param)
#else
static void *playthread(void *param)
#endif
{
	long		t = GetTickCount();
	long		d;

	while (playing) {
		d = t - (long)GetTickCount();
		if (d <= 0) {
			d = 1;
		}
#ifdef _WIN32
		Sleep(d);
#else
		usleep(d);
#endif

		t = GetTickCount() + 1000 * (sizeof(sndbuffer) / CD_FRAMESIZE_RAW) / 75;

		if ((d = fread(sndbuffer, 1, sizeof(sndbuffer), cddaHandle)) == 0) {
			playing = 0;
			fclose(cddaHandle);
			cddaHandle = NULL;
			initial_offset = 0;
			break;
		}

		SPU_playCDDAchannel((short *)sndbuffer, d);
	}

#ifdef _WIN32
	_endthread();
#else
	pthread_exit(0);
	return NULL;
#endif
}

// stop the CDDA playback
static void stopCDDA() {
	if (!playing) {
		return;
	}

	playing = 0;
#ifdef _WIN32
	WaitForSingleObject(threadid, INFINITE);
#else
	pthread_join(threadid, NULL);
#endif

	if (cddaHandle != NULL) {
		fclose(cddaHandle);
		cddaHandle = NULL;
	}

	initial_offset = 0;
}

// start the CDDA playback
static void startCDDA(unsigned int offset) {
	if (playing) {
		if (initial_offset == offset) {
			return;
		}
		stopCDDA();
	}

	cddaHandle = fopen(cdrfilename, "rb");
	if (cddaHandle == NULL) {
		return;
	}

	initial_offset = offset;
	fseek(cddaHandle, initial_offset, SEEK_SET);

	offset /= CD_FRAMESIZE_RAW;

	playing = 1;
#ifdef _WIN32
	threadid = (HANDLE)_beginthread(playthread, 0, NULL);
#else
	pthread_create(&threadid, NULL, playthread, NULL);
#endif
}

// this function tries to get the .toc file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsetoc(const char *isofile) {
	char			tocname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256], dummy[256];
	char			*token;
	char			name[256];
	char			time[20], time2[20];
	unsigned int	i, t;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .toc
	strncpy(tocname, isofile, sizeof(tocname));
	tocname[MAXPATHLEN - 1] = '\0';
	if (strlen(tocname) >= 4) {
		strcpy(tocname + strlen(tocname) - 4, ".toc");
	}
	else {
		return -1;
	}

	if ((fi = fopen(tocname, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	// parse the .toc file
	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		// search for tracks
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (!strcmp(token, "TRACK")) {
			// get type of track
			token = strtok(NULL, " ");
			numtracks++;

			if (!strcmp(token, "MODE2_RAW\n")) {
				ti[numtracks].type = DATA;
				sec2msf(2 * 75, ti[numtracks].start); // assume data track on 0:2:0
			}
			else if (!strcmp(token, "AUDIO\n")) {
				ti[numtracks].type = CDDA;
			}
		}
		else if (!strcmp(token, "DATAFILE")) {
			sscanf(linebuf, "DATAFILE %s %8s", name, time);
			tok2msf((char *)&time, (char *)&ti[numtracks].length);
		}
		else if (!strcmp(token, "FILE")) {
			sscanf(linebuf, "FILE %s %s %8s %8s", name, dummy, time, time2);
			tok2msf((char *)&time, (char *)&ti[numtracks].start);
			tok2msf((char *)&time2, (char *)&ti[numtracks].length);
		}
		else if (!strcmp(token, "START")) {
			sscanf(linebuf, "START %8s", time);
			tok2msf((char *)&time, (char *)&ti[numtracks].gap);
		}
	}

	fclose(fi);

	// calculate the true start of each track
	// start+gap+datasize (+2 secs of silence ? I dunno...)
	for (i = 2; i <= numtracks; i++) {
		t = msf2sec(ti[1].start) + msf2sec(ti[1].length) + msf2sec(ti[i].start) + msf2sec(ti[i].gap);
		sec2msf(t, ti[i].start);
	}

	return 0;
}

// this function tries to get the .cue file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsecue(const char *isofile) {
	char			cuename[MAXPATHLEN];
	FILE			*fi;
	char			*token;
	char			time[20];
	char			*tmp;
	char			linebuf[256], dummy[256];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .cue
	strncpy(cuename, isofile, sizeof(cuename));
	cuename[MAXPATHLEN - 1] = '\0';
	if (strlen(cuename) >= 4) {
		strcpy(cuename + strlen(cuename) - 4, ".cue");
	}
	else {
		return -1;
	}

	if ((fi = fopen(cuename, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (!strcmp(token, "TRACK")){
			numtracks++;

			if (strstr(linebuf, "AUDIO") != NULL) {
				ti[numtracks].type = CDDA;
			}
			else if (strstr(linebuf, "MODE1/2352") != NULL || strstr(linebuf, "MODE2/2352") != NULL) {
				ti[numtracks].type = DATA;
			}
		}
		else if (!strcmp(token, "PREGAP")) {
			tmp = strstr(linebuf, "PREGAP");
			if (tmp != NULL) {
				tmp += strlen("PREGAP");
				while (*tmp == ' ') tmp++;
				if (*tmp != '\n') sscanf(tmp, "%8s", time);
			}
			tok2msf((char *)&time, (char *)&ti[numtracks].gap);
		}
		else if (!strcmp(token, "INDEX")) {
			tmp = strstr(linebuf, "INDEX");
			if (tmp != NULL) {
				tmp += strlen("INDEX") + 3; // 3 - space + numeric index
				while (*tmp == ' ') tmp++;
				if (*tmp != '\n') sscanf(tmp, "%8s", time);
			}

			tok2msf((char *)&time, (char *)&ti[numtracks].start);

			t = msf2sec(ti[numtracks].start) + 2 * 75 + msf2sec(ti[numtracks].gap);
			sec2msf(t, ti[numtracks].start);

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start) + msf2sec(ti[numtracks - 1].gap) - msf2sec(ti[numtracks].gap);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .ccd file of the given .img
// the necessary data is put into the ti (trackinformation)-array
static int parseccd(const char *isofile) {
	char			ccdname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .img to .ccd
	strncpy(ccdname, isofile, sizeof(ccdname));
	ccdname[MAXPATHLEN - 1] = '\0';
	if (strlen(ccdname) >= 4) {
		strcpy(ccdname + strlen(ccdname) - 4, ".ccd");
	}
	else {
		return -1;
	}

	if ((fi = fopen(ccdname, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		if (!strncmp(linebuf, "[TRACK", 6)){
			numtracks++;
		}
		else if (!strncmp(linebuf, "MODE=", 5)) {
			sscanf(linebuf, "MODE=%d", &t);
			ti[numtracks].type = ((t == 0) ? CDDA : DATA);
		}
		else if (!strncmp(linebuf, "INDEX 0=", 8)) {
			sscanf(linebuf, "INDEX 0=%d", &t);
			sec2msf(t, ti[numtracks].gap);
		}
		else if (!strncmp(linebuf, "INDEX 1=", 8)) {
			sscanf(linebuf, "INDEX 1=%d", &t);

			if (numtracks <= 1) {
				t += 2 * 75;
			}

			if (msf2sec(ti[numtracks].gap) != 0) {
				sec2msf(t - msf2sec(ti[numtracks].gap), ti[numtracks].gap);
			}

			t += msf2sec(ti[numtracks].gap);
			sec2msf(t, ti[numtracks].start);

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start) + msf2sec(ti[numtracks - 1].gap) - msf2sec(ti[numtracks].gap);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / 2352 - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .sub file of the given .img
static int opensubfile(const char *isoname) {
	char		subname[MAXPATHLEN];

	// copy name of the iso and change extension from .img to .sub
	strncpy(subname, isoname, sizeof(subname));
	subname[MAXPATHLEN - 1] = '\0';
	if (strlen(subname) >= 4) {
		strcpy(subname + strlen(subname) - 4, ".sub");
	}
	else {
		return -1;
	}

	subHandle = fopen(subname, "rb");
	if (subHandle == NULL) {
		return -1;
	}

	return 0;
}

static long CALLBACK ISOinit(void) {
	assert(cdHandle == NULL);
	assert(subHandle == NULL);

	return 0; // do nothing
}

static long CALLBACK ISOshutdown(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}
	stopCDDA();
	return 0;
}

// This function is invoked by the front-end when opening an ISO
// file for playback
static long CALLBACK ISOopen(void) {
	if (cdHandle != NULL) {
		return 0; // it's already open
	}

	cdHandle = fopen(cdrfilename, "rb");
	if (cdHandle == NULL) {
		return -1;
	}

	SysPrintf(_("Loaded CD Image: %s"), cdrfilename);

	if (parsetoc(cdrfilename) == 0) {
		SysPrintf("[+toc]");
	}
	else if (parsecue(cdrfilename) == 0) {
		SysPrintf("[+cue]");
	}
	else if (parseccd(cdrfilename) == 0) {
		SysPrintf("[+ccd]");
	}

	if (opensubfile(cdrfilename) == 0) {
		SysPrintf("[+sub]");
	}

	SysPrintf(".\n");

	return 0;
}

static long CALLBACK ISOclose(void) {
	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}
	stopCDDA();
	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
static long CALLBACK ISOgetTN(unsigned char *buffer) {
	buffer[0] = 1;

	if (numtracks > 0) {
		buffer[1] = numtracks;
	}
	else {
		buffer[1] = 1;
	}

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
	}
	else {
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
	if (cdHandle == NULL) {
		return -1;
	}

	fseek(cdHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * CD_FRAMESIZE_RAW + 12, SEEK_SET);
	fread(cdbuffer, 1, DATA_SIZE, cdHandle);

	if (subHandle != NULL) {
		fseek(subHandle, MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2])) * SUB_FRAMESIZE, SEEK_SET);
		fread(subbuffer, 1, SUB_FRAMESIZE, subHandle);
	}

	return 0;
}

// return readed track
static unsigned char * CALLBACK ISOgetBuffer(void) {
	return cdbuffer;
}

// plays cdda audio
// sector: byte 0 - minute; byte 1 - second; byte 2 - frame
// does NOT uses bcd format
static long CALLBACK ISOplay(unsigned char *time) {
	if (SPU_playCDDAchannel != NULL) {
		startCDDA(MSF2SECT(time[0], time[1], time[2]) * CD_FRAMESIZE_RAW);
	}
	return 0;
}

// stops cdda audio
static long CALLBACK ISOstop(void) {
	stopCDDA();
	return 0;
}

// gets subchannel data
unsigned char* CALLBACK ISOgetBufferSub(void) {
	if (subHandle != NULL) {
		return subbuffer;
	}

	return NULL;
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
