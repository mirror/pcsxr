/*
 * Cdrom for Psemu Pro like Emulators
 *
 * By: linuzappz <linuzappz@hotmail.com>
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#include "cdr.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

static inline int msf_to_lba(char m, char s, char f) {
	return (((m * CD_SECS) + s) * CD_FRAMES + f) - CD_MSF_OFFSET;
}

int initial_time = 0;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

long (*ReadTrackT[])() = {
	ReadNormal,
	ReadThreaded,
};

unsigned char* (*GetBufferT[])() = {
	GetBNormal,
	GetBThreaded,
};

long (*fReadTrack)();
unsigned char* (*fGetBuffer)();

void *CdrThread(void *arg);

char *LibName = N_("CD-ROM Drive Reader");

char *PSEgetLibName(void) {
	return _(LibName);
}

unsigned long PSEgetLibType(void) {
	return PSE_LT_CDR;
}

unsigned long PSEgetLibVersion(void) {
	return 1 << 16;
}

long CDRinit(void) {
	cdHandle = -1;
	thread = -1;

	return 0;
}

long CDRshutdown(void) {
	return 0;
}

long CDRopen(void) {
	LoadConf();

	if (cdHandle > 0)
		return 0;				/* it's already open */
	cdHandle = open(CdromDev, O_RDONLY);
	if (cdHandle != -1) {		// if we can't open the cdrom we'll works as a null plugin
		ioctl(cdHandle, CDROM_LOCKDOOR, 0);
//		ioctl(cdHandle, CDROMSTART, NULL);

		ioctl(cdHandle, CDROM_SELECT_SPEED, CdrSpeed);
	} else {
		fprintf(stderr, "CDR: Could not open %s\n", CdromDev);
	}

	fReadTrack = ReadTrackT[ReadMode];
	fGetBuffer = GetBufferT[ReadMode];

	if (ReadMode == THREADED) {
		cdcache = (CacheData *)malloc(CacheSize * sizeof(CacheData));
		if (cdcache == NULL) return -1;
		memset(cdcache, 0, CacheSize * sizeof(CacheData));
	} else {
		cdbuffer = cr.buf + 12; /* skip sync data */
	}

	if (ReadMode == THREADED) {
		pthread_attr_t attr;

		pthread_mutex_init(&mut, NULL);
		pthread_cond_init(&cond, NULL);
		locked = 0;

		pthread_attr_init(&attr);
		pthread_create(&thread, &attr, CdrThread, NULL);

		cacheaddr = -1;
	} else thread = -1;

	playing = 0;
	stopth = 0;
	initial_time = 0;

	return 0;
}

long CDRclose(void) {
	if (cdHandle < 1) return 0;

	if (playing) CDRstop();
	close(cdHandle);
	cdHandle = -1;

	if (thread != -1) {
		if (locked == 0) {
			stopth = 1;
			while (locked == 0) usleep(5000);
		}

		stopth = 2;
		pthread_mutex_lock(&mut);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mut);

		pthread_join(thread, NULL);
		pthread_mutex_destroy(&mut);
		pthread_cond_destroy(&cond);
	}

	if (ReadMode == THREADED) {
		free(cdcache);
	}

	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
long CDRgetTN(unsigned char *buffer) {
	struct cdrom_tochdr toc;

	if (cdHandle < 1) {
		buffer[0] = 1;
		buffer[1] = 1;
		return 0;
	}

	if (ioctl(cdHandle, CDROMREADTOCHDR, &toc) == -1)
		return -1;

	buffer[0] = toc.cdth_trk0;	// start track
	buffer[1] = toc.cdth_trk1;	// end track

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
long CDRgetTD(unsigned char track, unsigned char *buffer) {
	struct cdrom_tocentry entry;

	if (cdHandle < 1) {
		memset(buffer + 1, 0, 3);
		return 0;
	}

	if (track == 0)
		track = 0xaa;			// total time
	entry.cdte_track = track;
	entry.cdte_format = CDROM_MSF;

	if (ioctl(cdHandle, CDROMREADTOCENTRY, &entry) == -1)
		return -1;

	buffer[0] = entry.cdte_addr.msf.frame;	/* frame */
	buffer[1] = entry.cdte_addr.msf.second;	/* second */
	buffer[2] = entry.cdte_addr.msf.minute;	/* minute */

	return 0;
}

// normal reading
long ReadNormal() {
	if (ioctl(cdHandle, CDROMREADRAW, &cr) == -1)
		return -1;

	return 0;
}

unsigned char* GetBNormal() {
	return cdbuffer;
}

// threaded reading (with cache)
long ReadThreaded() {
	int addr = msf_to_lba(cr.msf.cdmsf_min0, cr.msf.cdmsf_sec0, cr.msf.cdmsf_frame0);
	int i;

	if (addr >= cacheaddr && addr < (cacheaddr + CacheSize) &&
		cacheaddr != -1) {
		i = addr - cacheaddr;
//		printf("found %d\n", (addr - cacheaddr));
		cdbuffer = cdcache[i].cr.buf + 12;
		while (btoi(cdbuffer[0]) != cr.msf.cdmsf_min0 ||
			   btoi(cdbuffer[1]) != cr.msf.cdmsf_sec0 ||
			   btoi(cdbuffer[2]) != cr.msf.cdmsf_frame0) {
			if (locked == 1) {
				if (cdcache[i].ret == 0) break;
				return -1;
			}
			usleep(5000);
		}
//		printf("%x:%x:%x, %p, %p\n", cdbuffer[0], cdbuffer[1], cdbuffer[2], cdbuffer, cdcache);
		found = 1;

		return 0;
	} else found = 0;

	if (locked == 0) {
		stopth = 1;
		while (locked == 0) { usleep(5000); }
		stopth = 0;
	}

	// not found in cache
	locked = 0;
	pthread_mutex_lock(&mut);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mut);

	return 0;
}

unsigned char* GetBThreaded() {
//	printf("threadc %d\n", found);
	if (found == 1) { found = 0; return cdbuffer; }
	cdbuffer = cdcache[0].cr.buf + 12;
	while (btoi(cdbuffer[0]) != cr.msf.cdmsf_min0 ||
		   btoi(cdbuffer[1]) != cr.msf.cdmsf_sec0 ||
		   btoi(cdbuffer[2]) != cr.msf.cdmsf_frame0) {
		if (locked == 1) return NULL;
		usleep(5000);
	}
	if (cdcache[0].ret == -1) return NULL;

	return cdbuffer;
}


// read track
// time:
//  byte 0 - minute
//  byte 1 - second
//  byte 2 - frame
// uses bcd format
long CDRreadTrack(unsigned char *time) {
	if (cdHandle < 1) {
		memset(cr.buf, 0, DATA_SIZE);
		return 0;
	}

//	printf("CDRreadTrack %d:%d:%d\n", btoi(time[0]), btoi(time[1]), btoi(time[2]));

	if (UseSubQ) memcpy(lastTime, time, 3);
	subqread = 0;

	cr.msf.cdmsf_min0 = btoi(time[0]);
	cr.msf.cdmsf_sec0 = btoi(time[1]);
	cr.msf.cdmsf_frame0 = btoi(time[2]);

	return fReadTrack();
}

void *CdrThread(void *arg) {
	unsigned char curTime[3];
	int i;

	for (;;) {
		locked = 1;
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&cond, &mut);

		if (stopth == 2) pthread_exit(NULL);
		// refill the buffer
		cacheaddr = msf_to_lba(cr.msf.cdmsf_min0, cr.msf.cdmsf_sec0, cr.msf.cdmsf_frame0);

		memcpy(curTime, &cr.msf, 3);

//		printf("start thc %d:%d:%d\n", curTime[0], curTime[1], curTime[2]);

		for (i=0; i<CacheSize; i++) {
			memcpy(&cdcache[i].cr.msf, curTime, 3);
//			printf("reading %d:%d:%d\n", crp.msf.cdmsf_min0, crp.msf.cdmsf_sec0, crp.msf.cdmsf_frame0);
			cdcache[i].ret = ioctl(cdHandle, CDROMREADRAW, &cdcache[i].cr);

//			printf("readed %x:%x:%x\n", crd.buf[12], crd.buf[13], crd.buf[14]);
			if (cdcache[i].ret == -1) break;

			curTime[2]++;
			if (curTime[2] == 75) {
				curTime[2] = 0;
				curTime[1]++;
				if (curTime[1] == 60) {
					curTime[1] = 0;
					curTime[0]++;
				}
			}

			if (stopth) break;
		}

		pthread_mutex_unlock(&mut);
	}

	return NULL;
}

// return readed track
unsigned char *CDRgetBuffer(void) {
	return fGetBuffer();
}

// plays cdda audio
// sector:
//  byte 0 - minute
//  byte 1 - second
//  byte 2 - frame
// does NOT uses bcd format
long CDRplay(unsigned char *sector) {
	struct cdrom_msf addr;
	unsigned char ptmp[4];

	if (cdHandle < 1)
		return 0;

	// If play was called with the same time as the previous call,
	// don't restart it. Of course, if play is called with a different
	// track, stop playing the current stream.
	if (playing)
	{
		if (msf_to_lba(sector[0], sector[1], sector[2]) == initial_time)
			return 0;
		else
			CDRstop();
	}
	initial_time = msf_to_lba(sector[0], sector[1], sector[2]);

	// 0 is the last track of every cdrom, so play up to there
	if (CDRgetTD(0, ptmp) == -1)
		return -1;
	addr.cdmsf_min0 = sector[0];
	addr.cdmsf_sec0 = sector[1];
	addr.cdmsf_frame0 = sector[2];
	addr.cdmsf_min1 = ptmp[2];
	addr.cdmsf_sec1 = ptmp[1];
	addr.cdmsf_frame1 = ptmp[0];

	if (ioctl(cdHandle, CDROMPLAYMSF, &addr) == -1)
		return -1;

	playing = 1;

	return 0;
}

// stops cdda audio
long CDRstop(void) {
	struct cdrom_subchnl sc;

	if (cdHandle < 1)
		return 0;

	sc.cdsc_format = CDROM_MSF;
	if (ioctl(cdHandle, CDROMSUBCHNL, &sc) == -1)
		return -1;

	switch (sc.cdsc_audiostatus) {
		case CDROM_AUDIO_PAUSED:
		case CDROM_AUDIO_PLAY:
			ioctl(cdHandle, CDROMSTOP);
			break;
	}

	playing = 0;
	initial_time = 0;

	return 0;
}

struct CdrStat {
	unsigned long Type;
	unsigned long Status;
	unsigned char Time[3];		// current playing time
};

struct CdrStat ostat;

// reads cdr status
// type:
//  0x00 - unknown
//  0x01 - data
//  0x02 - audio
//  0xff - no cdrom
// status: (only shell open supported)
//  0x00 - unknown
//  0x01 - error
//  0x04 - seek error
//  0x10 - shell open
//  0x20 - reading
//  0x40 - seeking
//  0x80 - playing
// time:
//  byte 0 - minute
//  byte 1 - second
//  byte 2 - frame

long CDRgetStatus(struct CdrStat *stat) {
	struct cdrom_subchnl sc;
	int ret;
	static time_t to;

	if (cdHandle < 1)
		return -1;

	if (!playing) { // if not playing update stat only once in a second
		if (to < time(NULL)) {
			to = time(NULL);
		} else {
			memcpy(stat, &ostat, sizeof(struct CdrStat));
			return 0;
		}
	}

	memset(stat, 0, sizeof(struct CdrStat));

	if (playing) { // return Time only if playing
		sc.cdsc_format = CDROM_MSF;
		if (ioctl(cdHandle, CDROMSUBCHNL, &sc) != -1)
			memcpy(stat->Time, &sc.cdsc_absaddr.msf, 3);
	}

	ret = ioctl(cdHandle, CDROM_DISC_STATUS);
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
	ret = ioctl(cdHandle, CDROM_DRIVE_STATUS);
	switch (ret) {
		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
			stat->Type = 0xff;
			stat->Status |= 0x10;
			break;
		default:
			ioctl(cdHandle, CDROM_LOCKDOOR, 0);
			break;
	}

	switch (sc.cdsc_audiostatus) {
		case CDROM_AUDIO_PLAY:
			stat->Status |= 0x80;
			break;
	}

	memcpy(&ostat, stat, sizeof(struct CdrStat));

	return 0;
}

struct SubQ {
	char res0[12];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	char res1[72];
};

struct SubQ subq;

unsigned char *CDRgetBufferSub(void) {
	struct cdrom_subchnl subchnl;
	int ret;

	if (!UseSubQ) return NULL;

	if (subqread) return (unsigned char *)&subq;

	cr.msf.cdmsf_min0 = btoi(lastTime[0]);
	cr.msf.cdmsf_sec0 = btoi(lastTime[1]);
	cr.msf.cdmsf_frame0 = btoi(lastTime[2]);
	if (ioctl(cdHandle, CDROMSEEK, &cr.msf) == -1) {
		// will be slower, but there's no other way to make it accurate
		if (ioctl(cdHandle, CDROMREADRAW, &cr) == -1) return NULL;
	}

	subchnl.cdsc_format = CDROM_MSF;
	ret = ioctl(cdHandle, CDROMSUBCHNL, &subchnl);
	if (ret == -1) return NULL;

	subqread = 1;

	subq.TrackNumber = subchnl.cdsc_trk;
	subq.IndexNumber = subchnl.cdsc_ind;
	subq.TrackRelativeAddress[0] = itob(subchnl.cdsc_reladdr.msf.minute);
	subq.TrackRelativeAddress[1] = itob(subchnl.cdsc_reladdr.msf.second);
	subq.TrackRelativeAddress[2] = itob(subchnl.cdsc_reladdr.msf.frame);
	subq.AbsoluteAddress[0] = itob(subchnl.cdsc_absaddr.msf.minute);
	subq.AbsoluteAddress[1] = itob(subchnl.cdsc_absaddr.msf.second);
	subq.AbsoluteAddress[2] = itob(subchnl.cdsc_absaddr.msf.frame);

#if 0
	printf("subq : %x,%x : %x,%x,%x : %x,%x,%x\n",
		   subchnl.cdsc_trk, subchnl.cdsc_ind,
		   itob(subchnl.cdsc_reladdr.msf.minute), itob(subchnl.cdsc_reladdr.msf.second), itob(subchnl.cdsc_reladdr.msf.frame),
		   itob(subchnl.cdsc_absaddr.msf.minute), itob(subchnl.cdsc_absaddr.msf.second), itob(subchnl.cdsc_absaddr.msf.frame));
#endif

	return (unsigned char *)&subq;
}

void ExecCfg(char *arg) {
	char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgDFCdrom");
	if (stat(cfg, &buf) != -1) {
		if (fork() == 0) {
			execl(cfg, "cfgDFCdrom", arg, NULL);
			exit(0);
		}
		return;
	}

	strcpy(cfg, "./cfg/DFCdrom");
	if (stat(cfg, &buf) != -1) {
		if (fork() == 0) {
			execl(cfg, "cfgDFCdrom", arg, NULL);
			exit(0);
		}
		return;
	}

	printf("cfgDFCdrom file not found!\n");
}

long CDRconfigure() {
	ExecCfg("configure");

	return 0;
}

void CDRabout() {
	ExecCfg("about");
}

long CDRtest(void) {
	cdHandle = open(CdromDev, O_RDONLY);
	if (cdHandle == -1)
		return -1;
	close(cdHandle);
	cdHandle = -1;
	return 0;
}
