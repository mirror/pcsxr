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

#include "psemu_plugin_defs.h"

#include "settings.h"
#include "sio1.h"

/******************************************************************************/

static char *pluginName  = N_("Sio1 Driver");

static const unsigned char version  = 1;
static const unsigned char revision = 1;
static const unsigned char build    = 1;

Settings settings;

/******************************************************************************/

long CALLBACK SIO1init()
{
	return 0;
}

long CALLBACK SIO1shutdown()
{
    return 0;
}

/******************************************************************************/

long CALLBACK SIO1open( unsigned long *gpuDisp )
{
    settingsRead();

	return 0;
}

long CALLBACK SIO1close()
{
	return 0;
}

/******************************************************************************/

void CALLBACK SIO1pause()
{
}

void CALLBACK SIO1resume()
{
}

/******************************************************************************/

long CALLBACK SIO1keypressed( int val )
{
    return 0;
}

/******************************************************************************/

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
}

void CALLBACK SIO1writeMode32(unsigned long val)
{
	printf("SIO1writeMode32(%.8x)\n", val);
}

void CALLBACK SIO1writeCtrl16(unsigned short val)
{
	printf("SIO1writeCtrl16(%.4x)\n", val);
}

void CALLBACK SIO1writeCtrl32(unsigned long val)
{
	printf("SIO1writeCtrl32(%.8x)\n", val);
}

void CALLBACK SIO1writeBaud16(unsigned short val)
{
	printf("SIO1writeBaud16(%.4x)\n", val);
}

void CALLBACK SIO1writeBaud32(unsigned long val)
{
	printf("SIO1writeBaud32(%.8x)\n", val);
}

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
	printf("SIO1readStat16()\n");
    return 0;
}

unsigned long CALLBACK SIO1readStat32()
{
	printf("SIO1readStat32()\n");
    return 0;
}

unsigned short CALLBACK SIO1readMode16()
{
	printf("SIO1readMode16()\n");
    return 0;
}

unsigned long CALLBACK SIO1readMode32()
{
	printf("SIO1readMode32()\n");
    return 0;
}

unsigned short CALLBACK SIO1readCtrl16()
{
	printf("SIO1readCtrl16()\n");
    return 0;
}

unsigned long CALLBACK SIO1readCtrl32()
{
	printf("SIO1readCtrl32()\n");
    return 0;
}

unsigned short CALLBACK SIO1readBaud16()
{
	printf("SIO1readBaud16()\n");
    return 0;
}

unsigned long CALLBACK SIO1readBaud32()
{
	printf("SIO1readBaud32()\n");
    return 0;
}


/******************************************************************************/

void CALLBACK SIO1registerCallback(void (CALLBACK *callback)())
{
}

/******************************************************************************/
/*
long CALLBACK SIO1queryPlayer()
{
	return settings.player;
}
*/
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
