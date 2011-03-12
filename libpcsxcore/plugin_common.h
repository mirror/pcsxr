/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef __PLUGIN_COMMON_H__
#define __PLUGIN_COMMON_H__

//#define ENABLE_SIO1API 1

#ifndef _WIN32

typedef void* HWND;
#define CALLBACK

#else

#include <windows.h>

#endif

#include "decode_xa.h"

typedef struct {
	uint32_t ulFreezeVersion;
	uint32_t ulStatus;
	uint32_t ulControl[256];
	unsigned char psxVRam[1024*1024*2];
} GPUFreeze_t;

#ifdef Status /* defined by X11 includes */
#undef Status
#endif

struct CdrStat {
	uint32_t Type;
	uint32_t Status;
	unsigned char Time[3];
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

typedef struct {
	char PluginName[8];
	uint32_t PluginVersion;
	uint32_t Size;
	unsigned char SPUPorts[0x200];
	unsigned char SPURam[0x80000];
	xa_decode_t xa;
	unsigned char *SPUInfo;
} SPUFreeze_t;

typedef long (CALLBACK* GPUshowScreenPicCB)(unsigned char *);
typedef void (CALLBACK* GPUdisplayTextCB)(char *);
typedef void (CALLBACK* PADsetSensitiveCB)(int);

typedef struct {
	char EmuName[32];
	char CdromID[9];	// ie. 'SCPH12345', no \0 trailing character
	char CdromLabel[11];
	void *psxMem;
	GPUshowScreenPicCB GPU_showScreenPic;
	GPUdisplayTextCB GPU_displayText;
	PADsetSensitiveCB PAD_setSensitive;
	char GPUpath[256];	// paths must be absolute
	char SPUpath[256];
	char CDRpath[256];
	char MCD1path[256];
	char MCD2path[256];
	char BIOSpath[256];	// 'HLE' for internal bios
	char Unused[1024];
} netInfo;


#endif
