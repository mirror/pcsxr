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

#include "typedefs.h"
#include "psemu_plugin_defs.h"

#ifndef CALLBACK
#define CALLBACK
#endif

/***************************************************************************/

static char *pluginName	 = "sio1Null";

static const unsigned char version	= 1;
static const unsigned char revision = 1;
static const unsigned char build	= 1;

static void (CALLBACK *irqCallback)() = 0;

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

static u16 statReg; // 0x1f801054: 0x185 SR_TXRDY | SR_TXU | SR_DSR | SR_CTS
static u16 modeReg; // 0x1f801058: 0x0
static u16 ctrlReg; // 0x1f80105A: 0x0
static u16 baudReg; // 0x1f80105E: 0x0

/***************************************************************************/

long CALLBACK SIO1init() {
	return 0;
}

long CALLBACK SIO1shutdown() {
	return 0;
}

/***************************************************************************/

long CALLBACK SIO1open(unsigned long *gpuDisp) {
	statReg = SR_TXRDY | SR_TXU | SR_DSR | SR_CTS;
	modeReg = 0x0000;
	ctrlReg = 0x0000;
	baudReg = 0x0000;
	return 0;
}

long CALLBACK SIO1close() {
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

/* Write.
 */

void CALLBACK SIO1writeData8(u8 data) {
}

void CALLBACK SIO1writeData16(u16 data) {
}

void CALLBACK SIO1writeData32(u32 data) {
}

void CALLBACK SIO1writeStat16(u16 stat) {
}

void CALLBACK SIO1writeStat32(u32 stat) {
	SIO1writeStat16(stat);
}

void CALLBACK SIO1writeMode16(u16 mode) {
	modeReg = mode;
}

void CALLBACK SIO1writeMode32(u32 mode) {
	SIO1writeMode16(mode);
}

void CALLBACK SIO1writeCtrl16(u16 ctrl) {
	u16 ctrlSaved = ctrlReg;

	ctrlReg = ctrl;

	if(ctrlReg & CR_ERRRST) {
		ctrlReg &= ~CR_ERRRST;
		statReg &= ~(SR_PERROR | SR_OE | SR_FE | SR_IRQ);
	}

	if(ctrlReg & CR_RST) {
		statReg &= ~SR_IRQ;
		statReg |= SR_TXRDY | SR_TXU;
		modeReg = 0;
		ctrlReg = 0;
		baudReg = 0;
	}
	
	if(ctrlReg & CR_TXIEN) {
		if(!(statReg & SR_IRQ)) {
			irqCallback();
			statReg |= SR_IRQ;
		}
	}
}

void CALLBACK SIO1writeCtrl32(u32 ctrl) {
	SIO1writeCtrl16(ctrl);
}

void CALLBACK SIO1writeBaud16(u16 baud) {
	baudReg = baud;
}

void CALLBACK SIO1writeBaud32(u32 baud) {
	SIO1writeBaud16(baud);
}

/* Read.
 */

u8 CALLBACK SIO1readData8() {
	u8 data[1] = {0};
	return *(u8*)data;
}

u16 CALLBACK SIO1readData16() {
	u8 data[2] = {0, 0};
	return *(u16*)data;
}

u32 CALLBACK SIO1readData32() {
	u8 data[4] = {0, 0, 0, 0};
	return *(u32*)data;
}

u16 CALLBACK SIO1readStat16() {
	return statReg;
}

u32 CALLBACK SIO1readStat32() {
	return statReg;
}

u16 CALLBACK SIO1readMode16() {
	return modeReg;
}

u32 CALLBACK SIO1readMode32() {
	return modeReg;
}

u16 CALLBACK SIO1readCtrl16() {
	return ctrlReg;
}

u32 CALLBACK SIO1readCtrl32() {
	return ctrlReg;
}

u16 CALLBACK SIO1readBaud16() {
	return baudReg;
}

u32 CALLBACK SIO1readBaud32() {
	return baudReg;
}

/***************************************************************************/

void CALLBACK SIO1update(uint32_t t) {
}

void CALLBACK SIO1registerCallback(void (CALLBACK *callback)()) {
	irqCallback = callback;
}

/***************************************************************************/

unsigned long CALLBACK PSEgetLibType() {
	return PSE_LT_SIO1;
}

char* CALLBACK PSEgetLibName() {
	return pluginName;
}

unsigned long CALLBACK PSEgetLibVersion() {
	return version << 16 | revision << 8 | build;
}

long CALLBACK SIO1test() {
	return 0;
}

void CALLBACK SIO1about() {
}

void CALLBACK SIO1configure() {
}

/***************************************************************************/
