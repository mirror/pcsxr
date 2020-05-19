/***************************************************************************
                       record.c  -  description
                             -------------------
    begin                : Fri Nov 09 2001
    copyright            : (C) 2001 by Darko Matesic
    email                : thedarkma@ptt.yu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "externals.h"
#include "record.h"
#include "gpu.h"
#include <direct.h>

extern BOOL			RECORD_RECORDING = FALSE;
BITMAPINFOHEADER	RECORD_BI = {40,0,0,1,16,0,0,2048,2048,0,0};
unsigned char		RECORD_BUFFER[1600*1200*3];
unsigned long		RECORD_INDEX;
unsigned long		RECORD_RECORDING_MODE;
unsigned long		RECORD_VIDEO_SIZE;
unsigned long		RECORD_RECORDING_WIDTH;
unsigned long		RECORD_RECORDING_HEIGHT;
unsigned long		RECORD_FRAME_RATE_SCALE;
unsigned long		RECORD_COMPRESSION_MODE;
COMPVARS			RECORD_COMPRESSION1;
unsigned char		RECORD_COMPRESSION_STATE1[4096];
COMPVARS			RECORD_COMPRESSION2;
unsigned char		RECORD_COMPRESSION_STATE2[4096];


PCOMPVARS			pCompression = NULL;
AVISTREAMINFO		strhdr;
PAVIFILE			pfile = NULL;
PAVISTREAM			ps = NULL;
PAVISTREAM			psCompressed = NULL;
AVICOMPRESSOPTIONS	opts;

unsigned long		frame;
unsigned long		skip;

//--------------------------------------------------------------------

BOOL RECORD_Start()
{
static FILE *data;
static char filename[255];
RECORD_INDEX = 0;
if(HIWORD(VideoForWindowsVersion())<0x010a) {MessageBox(NULL,"Video for Windows version is too old !\nAbording Recording.","Error", MB_OK|MB_ICONSTOP);return FALSE;}
mkdir("demo");
while(TRUE)
	{
	sprintf(filename,"demo\\demo%04d.AVI",RECORD_INDEX++);
	if((data=fopen(filename,"rb"))==NULL) break;
	fclose(data);
	if(RECORD_INDEX>9999) goto error;
	}
if((data=fopen(filename,"wb"))==NULL) goto error;
if(RECORD_RECORDING_MODE==0)
	{
	switch(RECORD_VIDEO_SIZE)
		{
		case 0:
			RECORD_BI.biWidth	= iResX;
			RECORD_BI.biHeight	= iResY;
			break;
		case 1:
			RECORD_BI.biWidth	= iResX/2;
			RECORD_BI.biHeight	= iResY/2;
			break;
		default:
			RECORD_BI.biWidth	= iResX/4;
			RECORD_BI.biHeight	= iResY/4;
			break;
		}
	}
else
	{
	RECORD_BI.biWidth	= RECORD_RECORDING_WIDTH;
	RECORD_BI.biHeight	= RECORD_RECORDING_HEIGHT;
	}
if(RECORD_COMPRESSION_MODE==0)
	{
	RECORD_BI.biBitCount = 16;
	RECORD_BI.biSizeImage = RECORD_BI.biWidth*RECORD_BI.biHeight*2;
	pCompression = &RECORD_COMPRESSION1;
	}
else
	{
	RECORD_BI.biBitCount = 24;
	RECORD_BI.biSizeImage = RECORD_BI.biWidth*RECORD_BI.biHeight*3;
	pCompression = &RECORD_COMPRESSION2;
	}
AVIFileInit();
if(AVIFileOpen(&pfile,filename,OF_WRITE | OF_CREATE,NULL)!=AVIERR_OK) goto error;
memset(&strhdr,0,sizeof(strhdr));
strhdr.fccType                = streamtypeVIDEO;
strhdr.fccHandler             = pCompression->fccHandler;
strhdr.dwScale                = RECORD_FRAME_RATE_SCALE + 1;
strhdr.dwRate                 = (PSXDisplay.PAL)?50:60;// FPS
strhdr.dwSuggestedBufferSize  = RECORD_BI.biSizeImage;
SetRect(&strhdr.rcFrame,0,0,RECORD_BI.biWidth,RECORD_BI.biHeight);
if(AVIFileCreateStream(pfile,&ps,&strhdr)!=AVIERR_OK) goto error;

opts.fccType			= pCompression->fccType;
opts.fccHandler			= pCompression->fccHandler;
opts.dwKeyFrameEvery	= pCompression->lKey;
opts.dwQuality			= pCompression->lQ;
opts.dwBytesPerSecond	= pCompression->lDataRate;
opts.dwFlags			= AVICOMPRESSF_DATARATE|AVICOMPRESSF_KEYFRAMES|AVICOMPRESSF_VALID;
opts.lpFormat			= &RECORD_BI;
opts.cbFormat			= sizeof(RECORD_BI);
opts.lpParms			= pCompression->lpState;
opts.cbParms			= pCompression->cbState;
opts.dwInterleaveEvery	= 0;

if(AVIMakeCompressedStream(&psCompressed,ps,&opts,NULL)!=AVIERR_OK) goto error;

//if(AVIStreamSetFormat(psCompressed,0,&RECORD_BI,RECORD_BI.biSizeImage)!=AVIERR_OK) goto error;
// fixed:
if(AVIStreamSetFormat(psCompressed,0,&RECORD_BI,sizeof(RECORD_BI))!=AVIERR_OK) goto error;

frame = 0;
skip = RECORD_FRAME_RATE_SCALE;
return TRUE;
error:
RECORD_Stop();
RECORD_RECORDING = FALSE;
return FALSE;
}

//--------------------------------------------------------------------

void RECORD_Stop()
{
if(ps) AVIStreamClose(ps);
if(psCompressed) AVIStreamClose(psCompressed);
if(pfile) AVIFileClose(pfile);
AVIFileExit();
}

//--------------------------------------------------------------------

BOOL RECORD_WriteFrame()
{
if(skip) {skip--;return TRUE;}
skip=RECORD_FRAME_RATE_SCALE;
if(!RECORD_GetFrame()) return FALSE;
if(FAILED(AVIStreamWrite(psCompressed,frame,1,RECORD_BUFFER,RECORD_BI.biSizeImage,AVIIF_KEYFRAME,NULL,NULL)))
	{RECORD_Stop();return FALSE;}
frame++;
return TRUE;
}

//--------------------------------------------------------------------

BOOL RECORD_GetFrame()
{
static unsigned short *srcs,*src,*dests,cs;
static unsigned char *srcc,*destc;
static long x,y,cx,cy,ax,ay;
static unsigned long cl;

if(PSXDisplay.Disabled)
	{
	memset(RECORD_BUFFER,0,RECORD_BI.biSizeImage);
	return TRUE;
	}

srcs = (unsigned short*)&psxVuw[PSXDisplay.DisplayPosition.x+(PSXDisplay.DisplayPosition.y<<10)];
dests = (unsigned short*)RECORD_BUFFER;
destc = (unsigned char*)RECORD_BUFFER;
ax = (PSXDisplay.DisplayMode.x*65535L)/RECORD_BI.biWidth;
ay = (PSXDisplay.DisplayMode.y*65535L)/RECORD_BI.biHeight;
cy = (PSXDisplay.DisplayMode.y-1)<<16;
if(RECORD_BI.biBitCount==16)
	{
	if(PSXDisplay.RGB24)
		{
			for(y=0;y<RECORD_BI.biHeight;y++)
				{
				srcc = (unsigned char*)&srcs[(cy&0xffff0000)>>6];
				cx = 0;
				for(x=0;x<RECORD_BI.biWidth;x++)
					{
					cl = *((unsigned long*)&srcc[(cx>>16)*3]);
					*(dests++) = (unsigned short)(((cl&0xf8)<<7)|((cl&0xf800)>>6)|((cl&0xf80000)>>19));
					cx += ax;
					}
				cy -= ay;if(cy<0) cy=0;
				}
		}
	else
		{
		for(y=0;y<RECORD_BI.biHeight;y++)
			{
			src = &srcs[(cy&0xffff0000)>>6];
			cx = 0;
			for(x=0;x<RECORD_BI.biWidth;x++)
				{
				cs = src[cx>>16];
				*(dests++) = ((cs&0x7c00)>>10)|(cs&0x03e0)|((cs&0x001f)<<10);
				cx += ax;
				}
			cy -= ay;if(cy<0) cy=0;
			}
		}
	}
else if(RECORD_BI.biBitCount==24)
	{
	if(PSXDisplay.RGB24)
		{
			for(y=0;y<RECORD_BI.biHeight;y++)
				{
				srcc = (unsigned char*)&srcs[(cy&0xffff0000)>>6];
				cx = 0;
				for(x=0;x<RECORD_BI.biWidth;x++)
					{
					cl = *((unsigned long*)&srcc[(cx>>16)*3]);
					*(destc++) = (unsigned char)((cl&0xff0000)>>16);
					*(destc++) = (unsigned char)((cl&0xff00)>>8);
					*(destc++) = (unsigned char)(cl&0xff);
					cx += ax;
					}
				cy -= ay;if(cy<0) cy=0;
				}
		}
	else
		{
		for(y=0;y<RECORD_BI.biHeight;y++)
			{
			src = &srcs[(cy&0xffff0000)>>6];
			cx = 0;
			for(x=0;x<RECORD_BI.biWidth;x++)
				{
				cs = src[cx>>16];
				*(destc++) = (unsigned char)((cs&0x7c00)>>7);
				*(destc++) = (unsigned char)((cs&0x03e0)>>2);
				*(destc++) = (unsigned char)((cs&0x001f)<<3);
				cx += ax;
				}
			cy -= ay;if(cy<0) cy=0;
			}
		}
	}
else
	memset(RECORD_BUFFER,0,RECORD_BI.biSizeImage);

return TRUE;
}
