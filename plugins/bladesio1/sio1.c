/***************************************************************************
 *   Copyright (C) 2010 by Blade_Arma                                      *
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

#include "psxcommon.h"
#include "psemu_plugin_defs.h"

#include "settings.h"
#include "sio1.h"

/******************************************************************************/

static char *pluginName  = N_("Sio1 Driver");

static const unsigned char version  = 1;
static const unsigned char revision = 1;
static const unsigned char build    = 1;

static void (CALLBACK *irqCallback)() = 0;

Settings settings;

/* sio status flags.
 */
enum
{
	SR_TXRDY    = 0x0001,
	SR_RXRDY    = 0x0002,
    SR_TXU      = 0x0004,
    SR_PERROR   = 0x0008,
	SR_OE       = 0x0010,
	SR_FE       = 0x0020,
    SR_0040     = 0x0040, // ?
    SR_DSR      = 0x0080,
    SR_CTS      = 0x0100,
    SR_IRQ      = 0x0200
};

/* sio mode flags.
 */
enum
{
	MR_BR_1     = 0x0001,
	MR_BR_16    = 0x0002,
	MR_BR_64    = 0x0003,
	MR_CHLEN_5  = 0x0000,
	MR_CHLEN_6  = 0x0004,
	MR_CHLEN_7  = 0x0008,
	MR_CHLEN_8  = 0x000C,
	MR_PEN      = 0x0010,
	MR_P_EVEN   = 0x0020,
	MR_SB_00    = 0x0000,
	MR_SB_01    = 0x0040,
	MR_SB_10    = 0x0080,
	MR_SB_11    = 0x00C0
};

/* sio control flags.
 */
enum
{
	CR_TXEN     = 0x0001,
	CR_DTR      = 0x0002,
	CR_RXEN     = 0x0004,
	CR_0008     = 0x0008, // ?
	CR_ERRRST   = 0x0010,
	CR_RTS      = 0x0020,
	CR_RST      = 0x0040,
	CR_0080     = 0x0080, // HM?
	CR_BUFSZ_1  = 0x0000,
	CR_BUFSZ_2  = 0x0100,
	CR_BUFSZ_4  = 0x0200,
	CR_BUFSZ_8  = 0x0300,
	CR_TXIEN    = 0x0400,
	CR_RXIEN    = 0x0800,
	CR_DSRIEN   = 0x1000,
	CR_2000     = 0x2000 // CTSIEN?
};

static u8  dataReg[8];
static u16 statReg;
static u16 modeReg;
static u16 ctrlReg;
static u16 baudReg;

/******************************************************************************/

long CALLBACK SIO1init()
{
	printf("SIO1init()\n");
	return 0;
}

long CALLBACK SIO1shutdown()
{
	printf("SIO1shutdown()\n");
    return 0;
}

/******************************************************************************/

long CALLBACK SIO1open( unsigned long *gpuDisp )
{
	printf("SIO1open()\n");
	
    settingsRead();
	
	memset(dataReg, 0x00, sizeof(dataReg));
	statReg = SR_TXRDY | SR_TXU | SR_DSR | SR_CTS;
	modeReg = 0x0000;
	ctrlReg = 0x0000;
	baudReg = 0x0000;

	return 0;
}

long CALLBACK SIO1close()
{
	printf("SIO1close()\n");
	return 0;
}

/******************************************************************************/

void CALLBACK SIO1pause()
{
	printf("SIO1pause()\n");
}

void CALLBACK SIO1resume()
{
	printf("SIO1resume()\n");
}

/******************************************************************************/

long CALLBACK SIO1keypressed( int val )
{
    return 0;
}

/******************************************************************************/

/* Write.
 */

void CALLBACK SIO1writeData8(unsigned char val)
{
	printf("SIO1writeData8(%.2x)\n", val);
}

void CALLBACK SIO1writeData16(unsigned short val)
{
	printf("SIO1writeData16(%.4x)\n", val);
}

void CALLBACK SIO1writeData32(unsigned long val)
{
	printf("SIO1writeData32(%.8x)\n", val);
}

void CALLBACK SIO1writeStat16(unsigned short val)
{
	printf("SIO1writeStat16(%.4x)\n", val);
}

void CALLBACK SIO1writeStat32(unsigned long val)
{
	printf("SIO1writeStat32(%.8x)\n", val);
}

void CALLBACK SIO1writeMode16(unsigned short val)
{
	printf("SIO1writeMode16(%.4x)\n", val);
	modeReg = val;
}

void CALLBACK SIO1writeMode32(unsigned long val)
{
	printf("SIO1writeMode32(%.8x)\n", val);
	modeReg = val;
}

void CALLBACK SIO1writeCtrl16(unsigned short val)
{
	printf("SIO1writeCtrl16(%.4x)\n", val);
	ctrlReg = val;
}

void CALLBACK SIO1writeCtrl32(unsigned long val)
{
	printf("SIO1writeCtrl32(%.8x)\n", val);
	ctrlReg = val;
}

void CALLBACK SIO1writeBaud16(unsigned short val)
{
	printf("SIO1writeBaud16(%.4x)\n", val);
	baudReg = val;
}

void CALLBACK SIO1writeBaud32(unsigned long val)
{
	printf("SIO1writeBaud32(%.8x)\n", val);
	baudReg = val;
}

/* Read.
 */

unsigned char CALLBACK SIO1readData8()
{
	printf("SIO1readData8()\n");
    return 0;
}

unsigned short CALLBACK SIO1readData16()
{
	printf("SIO1readData16()\n");
    return 0;
}

unsigned long CALLBACK SIO1readData32()
{
	printf("SIO1readData32()\n");
    return 0;
}

unsigned short CALLBACK SIO1readStat16()
{
	printf("SIO1readStat16() : %.4x\n", statReg);
    return statReg;
}

unsigned long CALLBACK SIO1readStat32()
{
	printf("SIO1readStat32() : %.4x\n", statReg);
    return statReg;
}

unsigned short CALLBACK SIO1readMode16()
{
	printf("SIO1readMode16() : %.4x\n", modeReg);
    return modeReg;
}

unsigned long CALLBACK SIO1readMode32()
{
	printf("SIO1readMode32() : %.4x\n", modeReg);
    return modeReg;
}

unsigned short CALLBACK SIO1readCtrl16()
{
	printf("SIO1readCtrl16() : %.4x\n", ctrlReg);
    return ctrlReg;
}

unsigned long CALLBACK SIO1readCtrl32()
{
	printf("SIO1readCtrl32() : %.4x\n", ctrlReg);
    return ctrlReg;
}

unsigned short CALLBACK SIO1readBaud16()
{
	printf("SIO1readBaud16() : %.4x\n", baudReg);
    return baudReg;
}

unsigned long CALLBACK SIO1readBaud32()
{
	printf("SIO1readBaud32() : %.4x\n", baudReg);
    return baudReg;
}

/******************************************************************************/

void CALLBACK SIO1registerCallback(void (CALLBACK *callback)())
{
	irqCallback = callback;
}

/******************************************************************************/

unsigned long CALLBACK PSEgetLibType()
{
    return PSE_LT_SIO1;
}

char* CALLBACK PSEgetLibName()
{
    return _(pluginName);
}

unsigned long CALLBACK PSEgetLibVersion()
{
    return version << 16 | revision << 8 | build;
}

void CALLBACK SIO1about()
{
}

long CALLBACK SIO1test()
{
    return 0;
}

void CALLBACK SIO1configure()
{
}

/******************************************************************************/
