#ifndef __CDR_H__
#define __CDR_H__

#include <linux/cdrom.h>

typedef char HWND;

#include <stdint.h>
#include "psemu_plugin_defs.h"

extern char CdromDev[256];
extern long ReadMode;
extern long UseSubQ;
extern long CacheSize;
extern long CdrSpeed;
extern long SpinDown;

#define DEV_DEF		"/dev/cdrom"
#define NORMAL		0
#define THREADED	1
#define READ_MODES	2

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

#endif /* __CDR_H__ */
