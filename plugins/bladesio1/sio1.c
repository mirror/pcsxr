/***************************************************************************
 *   Copyright (C) 2013 by Blade_Arma <edgbla@yandex.ru>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined _WINDOWS
#include "stdafx.h"
#include "cfg-winapi.h"
#elif defined _MACOSX
#include <sys/stat.h>
void AboutDlgProc();
void ConfDlgProc();
#else
#include <sys/stat.h>
#endif

#include "typedefs.h"
#include "psemu_plugin_defs.h"

#include "sio1.h"
#include "fifo.h"
#include "connection.h"

/***************************************************************************/

//#define SIO1_DEBUG 1

static char *pluginName	 = N_("sio1Blade");

static const unsigned char version	= 1;
static const unsigned char revision = 1;
static const unsigned char build	= 1;

static void (CALLBACK *irqCallback)() = 0;

Settings settings;

/* sio status flags.
 */
enum {
	SR_TXRDY	= 0x0001,
	SR_RXRDY	= 0x0002,
	SR_TXU		= 0x0004,
	SR_PERROR	= 0x0008,
	SR_OE		= 0x0010,
	SR_FE		= 0x0020,
	SR_0040		= 0x0040, // ?
	SR_DSR		= 0x0080,
	SR_CTS		= 0x0100,
	SR_IRQ		= 0x0200
};

/* sio mode flags.
 */
enum {
	MR_BR_1		= 0x0001,
	MR_BR_16	= 0x0002,
	MR_BR_64	= 0x0003,
	MR_CHLEN_5	= 0x0000,
	MR_CHLEN_6	= 0x0004,
	MR_CHLEN_7	= 0x0008,
	MR_CHLEN_8	= 0x000C,
	MR_PEN		= 0x0010,
	MR_P_EVEN	= 0x0020,
	MR_SB_00	= 0x0000,
	MR_SB_01	= 0x0040,
	MR_SB_10	= 0x0080,
	MR_SB_11	= 0x00C0
};

/* sio control flags.
 */
enum {
	CR_TXEN		= 0x0001,
	CR_DTR		= 0x0002,
	CR_RXEN		= 0x0004,
	CR_0008		= 0x0008, // ?
	CR_ERRRST	= 0x0010,
	CR_RTS		= 0x0020,
	CR_RST		= 0x0040,
	CR_0080		= 0x0080, // HM?
	CR_BUFSZ_1	= 0x0000,
	CR_BUFSZ_2	= 0x0100,
	CR_BUFSZ_4	= 0x0200,
	CR_BUFSZ_8	= 0x0300,
	CR_TXIEN	= 0x0400,
	CR_RXIEN	= 0x0800,
	CR_DSRIEN	= 0x1000,
	CR_2000		= 0x2000 // CTSIEN?
};

static u8  fifoIrqVals[] = {1, 2, 4, 8};
static s32 slaveDelay = 1;

static u16 statReg; // 0x1f801054: 0x185 SR_TXRDY | SR_TXU | SR_DSR | SR_CTS
static u16 modeReg; // 0x1f801058: 0x0
static u16 ctrlReg; // 0x1f80105A: 0x0
static u16 baudReg; // 0x1f80105E: 0x0

typedef struct EXC_DATA {
	u16 reg;
	u8 len;
	u8 data;
} EXC_DATA;

/***************************************************************************/

long CALLBACK SIO1init() {
	return 0;
}

long CALLBACK SIO1shutdown() {
	return 0;
}

/***************************************************************************/

long CALLBACK SIO1open(unsigned long *gpuDisp) {
	settingsRead();

	statReg = SR_TXRDY | SR_TXU | SR_DSR | SR_CTS;
	modeReg = 0x0000;
	ctrlReg = 0x0000;
	baudReg = 0x0000;

	fifoOpen();

	if(connectionOpen() < 0)
		settings.player = PLAYER_DISABLED;

	return 0;
}

long CALLBACK SIO1close() {
	fifoClose();
	connectionClose();

	return 0;
}

/***************************************************************************/

void CALLBACK SIO1pause() {
}

void CALLBACK SIO1resume() {
}

/***************************************************************************/

long CALLBACK SIO1keypressed(int key) {
	return 0;
}

/***************************************************************************/

void Exchange(s32 data) {
	EXC_DATA exc_data_send, exc_data_recv;

	if(settings.player == PLAYER_DISABLED)
		return;

	if(slaveDelay && settings.player == PLAYER_SLAVE) {
		EXC_DATA exc_data_empty;
		s32 count;

		memset(&exc_data_empty, 0x00, sizeof(exc_data_empty));
		exc_data_empty.reg = CR_DTR | CR_RTS;

		for(count = 0; count < 4; ++count) {
			connectionRecv((u8*)&exc_data_recv, sizeof(exc_data_recv));
			connectionSend((u8*)&exc_data_empty, sizeof(exc_data_empty));
		}

		slaveDelay = 0;
		return;
	}

	memset(&exc_data_send, 0x00, sizeof(exc_data_send));
	memset(&exc_data_recv, 0x00, sizeof(exc_data_recv));

	exc_data_send.reg = ctrlReg;
	exc_data_send.len = 0;

	if(data >= 0) {
		statReg |= SR_TXRDY | SR_TXU;

		exc_data_send.len = 1;
		exc_data_send.data = data;

		if(ctrlReg & CR_TXIEN) {
			if(!(statReg & SR_IRQ)) {
#if defined SIO1_DEBUG
			printf("irqCallback() : CR_TXIEN\n");
#endif
				irqCallback();
				statReg |= SR_IRQ;
			}
		}
	}

	if(settings.player == PLAYER_MASTER) {
		connectionSend((u8*)&exc_data_send, sizeof(exc_data_send));
		connectionRecv((u8*)&exc_data_recv, sizeof(exc_data_recv));
	}
	else {
		connectionRecv((u8*)&exc_data_recv, sizeof(exc_data_recv));
		connectionSend((u8*)&exc_data_send, sizeof(exc_data_send));
	}

	if(exc_data_recv.reg & CR_DTR)
		statReg |= SR_DSR;
	else
		statReg &= ~SR_DSR;

	if(exc_data_recv.reg & CR_RTS)
		statReg |= SR_CTS;
	else
		statReg &= ~SR_CTS;

	if(exc_data_recv.len) {
		fifoPush(exc_data_recv.data);

#if defined SIO1_DEBUG
		printf("data recieved : %.2x (%i)\n", exc_data_recv.data, fifoEmployment());
#endif
	}

	if(ctrlReg & CR_RXIEN) {
		if(fifoEmployment() == fifoIrqVals[(ctrlReg >> 8) & 0x03]) {
			if(!(statReg & SR_IRQ)) {
#if defined SIO1_DEBUG
			printf("irqCallback() : CR_RXIEN\n");
#endif
				irqCallback();
				statReg |= SR_IRQ;
			}
		}
	}

	if(fifoOverrun()) {
#if defined SIO1_DEBUG
		printf("Overrun\n");
#endif
		statReg |= SR_OE;
	}

	// FIXME:
	if(fifoEmpty())
		statReg &= ~SR_RXRDY;
	else
		statReg |= SR_RXRDY;

	if(ctrlReg & CR_DSRIEN) {
		if(statReg & SR_DSR) {
			if(!(statReg & SR_IRQ)) {
#if defined SIO1_DEBUG
			printf("irqCallback() : CR_DSRIEN\n");
#endif
				irqCallback();
				statReg |= SR_IRQ;
			}
		}
	}
}

/***************************************************************************/

/* Write.
 */

void CALLBACK SIO1writeData8(u8 data) {
#if defined SIO1_DEBUG
	printf("SIO1writeData8(%.2x)\n", data);
#endif
	Exchange(data);
}

void CALLBACK SIO1writeData16(u16 data) {
#if defined SIO1_DEBUG
	printf("SIO1writeData16(%.4x)\n", data);
#endif
	Exchange(data & 0xff);
}

void CALLBACK SIO1writeData32(u32 data) {
#if defined SIO1_DEBUG
	printf("SIO1writeData32(%.8x)\n", data);
#endif
	Exchange(data & 0xff);
}

void CALLBACK SIO1writeStat16(u16 stat) {
#if defined SIO1_DEBUG
	printf("SIO1writeStat16(%.4x)\n", stat);
#endif
	Exchange(-1);
}

void CALLBACK SIO1writeStat32(u32 stat) {
#if defined SIO1_DEBUG
	printf("SIO1writeStat32(%.8x)\n", stat);
#endif
	SIO1writeStat16(stat);
}

void CALLBACK SIO1writeMode16(u16 mode) {
#if defined SIO1_DEBUG
	printf("SIO1writeMode16(%.4x)\n", mode);
#endif
	modeReg = mode;
	Exchange(-1);
}

void CALLBACK SIO1writeMode32(u32 mode) {
#if defined SIO1_DEBUG
	printf("SIO1writeMode32(%.8x)\n", mode);
#endif
	SIO1writeMode16(mode);
}

void CALLBACK SIO1writeCtrl16(u16 ctrl) {
#if defined SIO1_DEBUG
	printf("SIO1writeCtrl16(%.4x)\n", ctrl);
#endif
	u16 ctrlSaved = ctrlReg;

	ctrlReg = ctrl;

	if(ctrlReg & CR_ERRRST) {
		ctrlReg &= ~CR_ERRRST;
		statReg &= ~(SR_PERROR | SR_OE | SR_FE | SR_IRQ);
		
		fifoResetErr();
	}

	if(ctrlReg & CR_RST) {
		statReg &= ~SR_IRQ;
		statReg |= SR_TXRDY | SR_TXU;
		modeReg = 0;
		ctrlReg = 0;
		baudReg = 0;
	}
	
	// FIXME: buffer not cleared and overrun possible, but flag must be cleared.
	//if(!(ctrlReg & CR_RXEN))
	//	statReg &= ~SR_RXRDY;
	
	// FIXME: ugly hack for C&C: RA and C&C: RAR.
	if(((ctrlReg >> 8) & 0x03) != ((ctrlSaved >> 8) & 0x03))
		fifoReset();
	
	// FIXME: move to Exchange.
	if(ctrlReg & CR_TXIEN) {
		if(!(statReg & SR_IRQ)) {
#if defined SIO1_DEBUG
		printf("irqCallback() : ctrl CR_TXIEN\n");
#endif
			irqCallback();
			statReg |= SR_IRQ;
		}
	}
	
	Exchange(-1);
}

void CALLBACK SIO1writeCtrl32(u32 ctrl) {
#if defined SIO1_DEBUG
	printf("SIO1writeCtrl32(%.8x)\n", ctrl);
#endif
	SIO1writeCtrl16(ctrl);
}

void CALLBACK SIO1writeBaud16(u16 baud) {
#if defined SIO1_DEBUG
	printf("SIO1writeBaud16(%.4x)\n", baud);
#endif
	baudReg = baud;
	Exchange(-1);
}

void CALLBACK SIO1writeBaud32(u32 baud) {
#if defined SIO1_DEBUG
	printf("SIO1writeBaud32(%.8x)\n", baud);
#endif
	SIO1writeBaud16(baud);
}

/* Read.
 */

u8 CALLBACK SIO1readData8() {
	u8 data[1];

	fifoPop(&data[0]);
	Exchange(-1);

#if defined SIO1_DEBUG
	printf("SIO1readData8() : %.2x\n", data[0]);
#endif

	return *(u8*)data;
}

u16 CALLBACK SIO1readData16() {
	u8 data[2];

	fifoPop(&data[0]);
	fifoPeek(&data[1]);
	Exchange(-1);

#if defined SIO1_DEBUG
	printf("SIO1readData16() : %.2x %.2x\n", data[0], data[1]);
#endif

	return *(u16*)data;
}

u32 CALLBACK SIO1readData32() {
	u8 data[4];

	fifoPop(&data[0]);
	fifoPop(&data[1]);
	fifoPop(&data[2]);
	fifoPop(&data[3]);
	Exchange(-1);

#if defined SIO1_DEBUG
	printf("SIO1readData32() : %.2x %.2x %.2x %.2x\n", data[0], data[1], data[2], data[3]);
#endif

	return *(u32*)data;
}

u16 CALLBACK SIO1readStat16() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readStat16() : %.4x\n", statReg);
#endif
	return statReg;
}

u32 CALLBACK SIO1readStat32() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readStat32() : %.4x\n", statReg);
#endif
	return statReg;
}

u16 CALLBACK SIO1readMode16() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readMode16() : %.4x\n", modeReg);
#endif
	return modeReg;
}

u32 CALLBACK SIO1readMode32() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readMode32() : %.4x\n", modeReg);
#endif
	return modeReg;
}

u16 CALLBACK SIO1readCtrl16() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readCtrl16() : %.4x\n", ctrlReg);
#endif
	return ctrlReg;
}

u32 CALLBACK SIO1readCtrl32() {
	Exchange(-1);
#if defined SIO1_DEBU
	printf("SIO1readCtrl32() : %.4x\n", ctrlReg);
#endif
	return ctrlReg;
}

u16 CALLBACK SIO1readBaud16() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readBaud16() : %.4x\n", baudReg);
#endif
	return baudReg;
}

u32 CALLBACK SIO1readBaud32() {
	Exchange(-1);
#if defined SIO1_DEBUG
	printf("SIO1readBaud32() : %.4x\n", baudReg);
#endif
	return baudReg;
}

/***************************************************************************/

void CALLBACK SIO1update(uint32_t t) {
	Exchange(-1);
}

void CALLBACK SIO1registerCallback(void (CALLBACK *callback)()) {
	irqCallback = callback;
}

/***************************************************************************/

unsigned long CALLBACK PSEgetLibType() {
	return PSE_LT_SIO1;
}

char* CALLBACK PSEgetLibName() {
	return _(pluginName);
}

unsigned long CALLBACK PSEgetLibVersion() {
	return version << 16 | revision << 8 | build;
}

long CALLBACK SIO1test() {
	return 0;
}

#if defined _WINDOWS
#elif defined _MACOSX
#else
void ExecCfg(char *arg) {
	char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgBladeSio1");
	if (stat(cfg, &buf) != -1) {
		int pid = fork();
		if (pid == 0) {
			if (fork() == 0) {
				execl(cfg, "cfgBladeSio1", arg, NULL);
			}
			exit(0);
		} else if (pid > 0) {
			waitpid(pid, NULL, 0);
		}
		return;
	}

	strcpy(cfg, "./cfg/cfgBladeSio1");
	if (stat(cfg, &buf) != -1) {
		int pid = fork();
		if (pid == 0) {
			if (fork() == 0) {
				execl(cfg, "cfgBladeSio1", arg, NULL);
			}
			exit(0);
		} else if (pid > 0) {
			waitpid(pid, NULL, 0);
		}
		return;
	}

	fprintf(stderr, "cfgBladeSio1 file not found!\n");
}
#endif

void CALLBACK SIO1about() {
#if defined _WINDOWS
	DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT), GetActiveWindow(),(DLGPROC)AboutDlgProc);
#elif defined _MACOSX
	AboutDlgProc();
#else
	ExecCfg("about");
#endif
}

void CALLBACK SIO1configure() {
#if defined _WINDOWS
	DialogBox(hInst,MAKEINTRESOURCE(IDD_CFGDLG), GetActiveWindow(),(DLGPROC)Sio1DlgProc);
#elif defined _MACOSX
	ConfDlgProc();
#else
	ExecCfg("configure");
#endif
}

/***************************************************************************/
