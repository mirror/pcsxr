/***************************************************************************
    PlugCD.h
    CDDeviceInterface
  
    Created by Gil Pedersen on Fri July 18 2003.
    Copyright (c) 2003,2004 Gil Pedersen.
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

#ifndef _PLUGCD_H_
#define _PLUGCD_H_

#include <stdio.h>

#define CHAR_LEN 256

// 2352 is a sector size
#define BUFFER_SECTORS 50
#define BUFFER_SIZE BUFFER_SECTORS*2352
#define BZIP_BUFFER_SECTORS 10

//  74 minutes * 60 sex/min * 75 frames/sec * 96 bytes needed per frame
#define TOTAL_CD_LENGTH 74*60*75
#define BYTES_PER_SUBCHANNEL_FRAME 96
#define MAX_SUBCHANNEL_DATA TOTAL_CD_LENGTH*BYTES_PER_SUBCHANNEL_FRAME

typedef struct {
	char	dn[128];
	char	fn[128];
} cd_conf;

cd_conf CDConfiguration;

int rc;

enum TrackType
{
   unknown, Mode1, Mode2, Audio, Pregap = 0x80
};

typedef struct
{
   enum TrackType type;
   char num;
   unsigned char start[3];
   unsigned char end[3];
} Track;

struct
{   
   int cd;
   FILE* cdda;
   int numtracks;
   long bufferPos;
   long bufferSize;
   long sector;
   long sectorType;
   long status;
   Track* tl;
   unsigned char buffer[BUFFER_SIZE];
} CD;

void CDDAclose(void);

// function headers for cdreader.c
char getNumTracks();
void seekSector(const unsigned char m, const unsigned char s, const unsigned char f);
unsigned char* getSector();
void newCD(const char * filename);
void readit();


// subtracts two times in integer format (non-BCD) ->  l - r = a
#define sub(l, r, a)\
   a[1] = 0;\
   a[0] = 0;\
   a[2] = l[2] - r[2];\
   if ((char)a[2] < 0)\
   {\
      a[2] += 75;\
      a[1] -= 1;\
   }\
   a[1] += l[1] - r[1];\
   if ((char)a[1] < 0)\
   {\
      a[1] += 60;\
      a[0] -= 1;\
   }\
   a[0] += l[0] - r[0];\

// converts a time like 17:61:00  to 18:01:00
#define normalizeTime(c)\
   while(c[2] > 75)\
   {\
      c[2] -= 75;\
      c[1] += 1;\
   }\
   while(c[1] > 60)\
   {\
      c[1] -= 60;\
      c[0] += 1;\
   }

// converts uchar in c to BCD character
#define intToBCD(c) (unsigned char)((c%10) | ((c/10)<<4))

// converts BCD number in c to uchar
#define BCDToInt(c) (unsigned char)((c & 0x0F) + 10 * ((c & 0xF0) >> 4))

#endif

