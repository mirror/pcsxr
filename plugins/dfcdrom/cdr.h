#ifndef __CDR_H__
#define __CDR_H__

#ifdef __linux__
#include <linux/cdrom.h>
#endif

typedef char HWND;

#include <stdint.h>
#include "psemu_plugin_defs.h"

char CdromDev[256];
long ReadMode;
long UseSubQ;
long CacheSize;
long CdrSpeed;

#ifdef __linux__

#define DEV_DEF		"/dev/cdrom"
#define NORMAL		0
#define THREADED	1
#define READ_MODES	2

#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

typedef union {
	struct cdrom_msf msf;
	unsigned char buf[CD_FRAMESIZE_RAW];
} crdata;

crdata cr;

typedef struct {
	crdata cr;
	int ret;
} CacheData;

CacheData *cdcache;
unsigned char *cdbuffer;
int cacheaddr;

unsigned char lastTime[3];
int cdHandle;
pthread_t thread;
int subqread, stopth;
int found, locked;
int playing;

long ReadNormal();
long ReadThreaded();
unsigned char* GetBNormal();
unsigned char* GetBThreaded();

long CDRstop(void);

void LoadConf();
void SaveConf();

#endif

#endif /* __CDR_H__ */
