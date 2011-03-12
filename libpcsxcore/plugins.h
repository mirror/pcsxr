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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"

#include "plugin_common.h"

#define PLUGINEXTERN typedef
#define PLUGINCALL(x) (CALLBACK* x)

#include "psemu_plugin_defs.h"

#include "spu.h"

int LoadPlugins();
void ReleasePlugins();
int OpenPlugins();
void ClosePlugins();

// GPU function pointers
extern GPUupdateLace    GPU_updateLace;
extern GPUinit          GPU_init;
extern GPUshutdown      GPU_shutdown;
extern GPUconfigure     GPU_configure;
extern GPUtest          GPU_test;
extern GPUabout         GPU_about;
extern GPUopen          GPU_open;
extern GPUclose         GPU_close;
extern GPUreadStatus    GPU_readStatus;
extern GPUreadData      GPU_readData;
extern GPUreadDataMem   GPU_readDataMem;
extern GPUwriteStatus   GPU_writeStatus;
extern GPUwriteData     GPU_writeData;
extern GPUwriteDataMem  GPU_writeDataMem;
extern GPUdmaChain      GPU_dmaChain;
extern GPUkeypressed    GPU_keypressed;
extern GPUdisplayText   GPU_displayText;
extern GPUmakeSnapshot  GPU_makeSnapshot;
extern GPUfreeze        GPU_freeze;
extern GPUgetScreenPic  GPU_getScreenPic;
extern GPUshowScreenPic GPU_showScreenPic;
extern GPUclearDynarec  GPU_clearDynarec;
extern GPUvBlank        GPU_vBlank;
extern GPUregisterCallback GPU_registerCallback;
extern GPUidle          GPU_idle;
extern GPUvisualVibration GPU_visualVibration;
extern GPUcursor        GPU_cursor;

// CD-ROM function pointers
extern CDRinit               CDR_init;
extern CDRshutdown           CDR_shutdown;
extern CDRopen               CDR_open;
extern CDRclose              CDR_close;
extern CDRtest               CDR_test;
extern CDRgetTN              CDR_getTN;
extern CDRgetTD              CDR_getTD;
extern CDRreadTrack          CDR_readTrack;
extern CDRgetBuffer          CDR_getBuffer;
extern CDRgetBufferSub       CDR_getBufferSub;
extern CDRplay               CDR_play;
extern CDRstop               CDR_stop;
extern CDRgetStatus          CDR_getStatus;
extern CDRgetDriveLetter     CDR_getDriveLetter;
extern CDRconfigure          CDR_configure;
extern CDRabout              CDR_about;
extern CDRsetfilename        CDR_setfilename;
extern CDRreadCDDA           CDR_readCDDA;
extern CDRgetTE              CDR_getTE;

// SPU function pointers
extern SPUconfigure        SPU_configure;
extern SPUabout            SPU_about;
extern SPUinit             SPU_init;
extern SPUshutdown         SPU_shutdown;
extern SPUtest             SPU_test;
extern SPUopen             SPU_open;
extern SPUclose            SPU_close;
extern SPUplaySample       SPU_playSample;
extern SPUwriteRegister    SPU_writeRegister;
extern SPUreadRegister     SPU_readRegister;
extern SPUwriteDMA         SPU_writeDMA;
extern SPUreadDMA          SPU_readDMA;
extern SPUwriteDMAMem      SPU_writeDMAMem;
extern SPUreadDMAMem       SPU_readDMAMem;
extern SPUplayADPCMchannel SPU_playADPCMchannel;
extern SPUfreeze           SPU_freeze;
extern SPUregisterCallback SPU_registerCallback;
extern SPUasync            SPU_async;
extern SPUplayCDDAchannel  SPU_playCDDAchannel;

// PAD function pointers
extern PADconfigure        PAD1_configure;
extern PADabout            PAD1_about;
extern PADinit             PAD1_init;
extern PADshutdown         PAD1_shutdown;
extern PADtest             PAD1_test;
extern PADopen             PAD1_open;
extern PADclose            PAD1_close;
extern PADquery            PAD1_query;
extern PADreadPort1        PAD1_readPort1;
extern PADkeypressed       PAD1_keypressed;
extern PADstartPoll        PAD1_startPoll;
extern PADpoll             PAD1_poll;
extern PADsetSensitive     PAD1_setSensitive;
extern PADregisterVibration PAD1_registerVibration;
extern PADregisterCursor   PAD1_registerCursor;
extern PADconfigure        PAD2_configure;
extern PADabout            PAD2_about;
extern PADinit             PAD2_init;
extern PADshutdown         PAD2_shutdown;
extern PADtest             PAD2_test;
extern PADopen             PAD2_open;
extern PADclose            PAD2_close;
extern PADquery            PAD2_query;
extern PADreadPort2        PAD2_readPort2;
extern PADkeypressed       PAD2_keypressed;
extern PADstartPoll        PAD2_startPoll;
extern PADpoll             PAD2_poll;
extern PADsetSensitive     PAD2_setSensitive;
extern PADregisterVibration PAD2_registerVibration;
extern PADregisterCursor   PAD2_registerCursor;

// NET function pointers
extern NETinit               NET_init;
extern NETshutdown           NET_shutdown;
extern NETopen               NET_open;
extern NETclose              NET_close;
extern NETtest               NET_test;
extern NETconfigure          NET_configure;
extern NETabout              NET_about;
extern NETpause              NET_pause;
extern NETresume             NET_resume;
extern NETqueryPlayer        NET_queryPlayer;
extern NETsendData           NET_sendData;
extern NETrecvData           NET_recvData;
extern NETsendPadData        NET_sendPadData;
extern NETrecvPadData        NET_recvPadData;
extern NETsetInfo            NET_setInfo;
extern NETkeypressed         NET_keypressed;

#ifdef ENABLE_SIO1API

// SIO1 function pointers
extern SIO1init               SIO1_init;
extern SIO1shutdown           SIO1_shutdown;
extern SIO1open               SIO1_open;
extern SIO1close              SIO1_close;
extern SIO1test               SIO1_test;
extern SIO1configure          SIO1_configure;
extern SIO1about              SIO1_about;
extern SIO1pause              SIO1_pause;
extern SIO1resume             SIO1_resume;
extern SIO1keypressed         SIO1_keypressed;
extern SIO1writeData8         SIO1_writeData8;
extern SIO1writeData16        SIO1_writeData16;
extern SIO1writeData32        SIO1_writeData32;
extern SIO1writeStat16        SIO1_writeStat16;
extern SIO1writeStat32        SIO1_writeStat32;
extern SIO1writeMode16        SIO1_writeMode16;
extern SIO1writeMode32        SIO1_writeMode32;
extern SIO1writeCtrl16        SIO1_writeCtrl16;
extern SIO1writeCtrl32        SIO1_writeCtrl32;
extern SIO1writeBaud16        SIO1_writeBaud16;
extern SIO1writeBaud32        SIO1_writeBaud32;
extern SIO1readData8          SIO1_readData8;
extern SIO1readData16         SIO1_readData16;
extern SIO1readData32         SIO1_readData32;
extern SIO1readStat16         SIO1_readStat16;
extern SIO1readStat32         SIO1_readStat32;
extern SIO1readMode16         SIO1_readMode16;
extern SIO1readMode32         SIO1_readMode32;
extern SIO1readCtrl16         SIO1_readCtrl16;
extern SIO1readCtrl32         SIO1_readCtrl32;
extern SIO1readBaud16         SIO1_readBaud16;
extern SIO1readBaud32         SIO1_readBaud32;
extern SIO1registerCallback   SIO1_registerCallback;

#endif

void CALLBACK clearDynarec(void);

void CALLBACK GPUbusy( int ticks );

void SetIsoFile(const char *filename);
const char *GetIsoFile(void);
boolean UsingIso(void);
void SetCdOpenCaseTime(s64 time);

char* CALLBACK CDR__getDriveLetter(void);
long CALLBACK CDR__configure(void);
long CALLBACK CDR__test(void);
void CALLBACK CDR__about(void);
long CALLBACK CDR__setfilename(char *filename);
long CALLBACK CDR__getStatus(struct CdrStat *stat);


#ifdef __cplusplus
}
#endif
#endif
