/***************************************************************************
                          draw.c  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
    email                : BlackDove@addcom.de
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

#define _IN_DRAW

#include "externals.h"
#include "gpu.h"
#include "draw.h"
#include "prim.h"
#include "menu.h"
#include "interp.h"
#include "swap.h"

// misc globals
int            iResX;
int            iResY;
long           lLowerpart;
BOOL           bIsFirstFrame = TRUE;
BOOL           bCheckMask = FALSE;
unsigned short sSetMask = 0;
unsigned long  lSetMask = 0;
int            iDesktopCol = 16;
int            iShowFPS = 0;
int            iWinSize;
int            iMaintainAspect = 0;
int            iUseNoStretchBlt = 0;
int            iFastFwd = 0;
int            iDebugMode = 0;
int            iFVDisplay = 0;
PSXPoint_t     ptCursorPoint[8];
unsigned short usCursorActive = 0;

// This could be used to select specific mode known to work.
// TODO implement as a cfg param if needed for wider compatibility among GPU cards
unsigned int uOverrideMode = 0x59565955U;

//unsigned int   LUT16to32[65536];
//unsigned int   RGBtoYUV[65536];

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XShm.h>
int xv_port = -1;
int xv_id = -1;
int use_yuv = False;
int xv_vsync = False;

XShmSegmentInfo shminfo;
int finalw,finalh;


Screen*  screen;
//extern XvImage  *XvShmCreateImage(Display*, XvPortID, int, char*, int, int, XShmSegmentInfo*);

#include <time.h>

// prototypes
void hq2x_32( unsigned char * srcPtr, DWORD srcPitch, unsigned char * dstPtr, int width, int height);
void hq3x_32( unsigned char * srcPtr,  DWORD srcPitch, unsigned char * dstPtr, int width, int height);

////////////////////////////////////////////////////////////////////////
// generic 2xSaI helpers
////////////////////////////////////////////////////////////////////////

void *         pSaISmallBuff=NULL;
void *         pSaIBigBuff=NULL;

#define GET_RESULT(A, B, C, D) ((A != C || A != D) - (B != C || B != D))

static __inline int GetResult1(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
 int x = 0;
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r+=1;
 if (y <= 1) r-=1;
 return r;
}

static __inline int GetResult2(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
 int x = 0;
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r-=1;
 if (y <= 1) r+=1;
 return r;
}

/* Convert RGB to YUV */
__inline uint32_t rgb_to_yuv(uint8_t R, uint8_t G, uint8_t B) {
    uint8_t Y = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);
    uint8_t U = min(abs(R * -1214 + G * -2384 + B * 3598 + 4096 + 1048576) >> 13, 240);
    uint8_t V = min(abs(R * 3598 + G * -3013 + B * -585 + 4096 + 1048576) >> 13, 240);

#ifdef __BIG_ENDIAN__
    return Y << 24 | U << 16 | Y << 8 | V;
#else
    return Y << 24 | V << 16 | Y << 8 | U;
#endif
}

#define colorMask8     0x00FEFEFE
#define lowPixelMask8  0x00010101
#define qcolorMask8    0x00FCFCFC
#define qlowpixelMask8 0x00030303

#define INTERPOLATE8(A, B) ((((A & colorMask8) >> 1) + ((B & colorMask8) >> 1) + (A & B & lowPixelMask8)))
#define Q_INTERPOLATE8(A, B, C, D) (((((A & qcolorMask8) >> 2) + ((B & qcolorMask8) >> 2) + ((C & qcolorMask8) >> 2) + ((D & qcolorMask8) >> 2) \
	+ ((((A & qlowpixelMask8) + (B & qlowpixelMask8) + (C & qlowpixelMask8) + (D & qlowpixelMask8)) >> 2) & qlowpixelMask8))))


void Super2xSaI_ex8(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch        = srcPitch<<1;
 DWORD srcPitchHalf    = srcPitch>>1;
 int   finWidth        = srcPitch>>2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 finalw=width<<1;
 finalh=height<<1;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==finWidth) iXA=0;
       else                 iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0)  {iYA=0;}
       else         {iYA=finWidth;}
       if(height>4) {iYB=finWidth;iYC=srcPitchHalf;}
       else
       if(height>3) {iYB=finWidth;iYC=finWidth;}
       else         {iYB=0;iYC=0;}

       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE8(color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE8 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE8 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE8 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE8 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE8 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE8 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE8(color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE8(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE8(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE8(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(srcPitchHalf))=product2a;
       *(dP+1+(srcPitchHalf))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

////////////////////////////////////////////////////////////////////////

void Std2xSaI_ex8(unsigned char *srcPtr, DWORD srcPitch,
                  unsigned char *dstBitmap, int width, int height)
{
 DWORD dstPitch        = srcPitch<<1;
 DWORD srcPitchHalf    = srcPitch>>1;
 int   finWidth        = srcPitch>>2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;

 finalw=width<<1;
 finalh=height<<1;

 DWORD colorA, colorB;
 DWORD colorC, colorD,
       colorE, colorF, colorG, colorH,
       colorI, colorJ, colorK, colorL,
       colorM, colorN, colorO, colorP;
 DWORD product, product1, product2;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
       if(finish==finWidth) iXA=0;
       else                 iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0)  {iYA=0;}
       else         {iYA=finWidth;}
       if(height>4) {iYB=finWidth;iYC=srcPitchHalf;}
       else
       if(height>3) {iYB=finWidth;iYC=finWidth;}
       else         {iYB=0;iYC=0;}

       colorI = *(bP- iYA - iXA);
       colorE = *(bP- iYA);
       colorF = *(bP- iYA + iXB);
       colorJ = *(bP- iYA + iXC);

       colorG = *(bP  - iXA);
       colorA = *(bP);
       colorB = *(bP  + iXB);
       colorK = *(bP + iXC);

       colorH = *(bP  + iYB  - iXA);
       colorC = *(bP  + iYB);
       colorD = *(bP  + iYB  + iXB);
       colorL = *(bP  + iYB  + iXC);

       colorM = *(bP + iYC - iXA);
       colorN = *(bP + iYC);
       colorO = *(bP + iYC + iXB);
       colorP = *(bP + iYC + iXC);


       if((colorA == colorD) && (colorB != colorC))
        {
         if(((colorA == colorE) && (colorB == colorL)) ||
            ((colorA == colorC) && (colorA == colorF) &&
             (colorB != colorE) && (colorB == colorJ)))
          {
           product = colorA;
          }
         else
          {
           product = INTERPOLATE8(colorA, colorB);
          }

         if(((colorA == colorG) && (colorC == colorO)) ||
            ((colorA == colorB) && (colorA == colorH) &&
             (colorG != colorC) && (colorC == colorM)))
          {
           product1 = colorA;
          }
         else
          {
           product1 = INTERPOLATE8(colorA, colorC);
          }
         product2 = colorA;
        }
       else
       if((colorB == colorC) && (colorA != colorD))
        {
         if(((colorB == colorF) && (colorA == colorH)) ||
            ((colorB == colorE) && (colorB == colorD) &&
             (colorA != colorF) && (colorA == colorI)))
          {
           product = colorB;
          }
         else
          {
           product = INTERPOLATE8(colorA, colorB);
          }

         if(((colorC == colorH) && (colorA == colorF)) ||
            ((colorC == colorG) && (colorC == colorD) &&
             (colorA != colorH) && (colorA == colorI)))
          {
           product1 = colorC;
          }
         else
          {
           product1=INTERPOLATE8(colorA, colorC);
          }
         product2 = colorB;
        }
       else
       if((colorA == colorD) && (colorB == colorC))
        {
         if (colorA == colorB)
          {
           product = colorA;
           product1 = colorA;
           product2 = colorA;
          }
         else
          {
           register int r = 0;
           product1 = INTERPOLATE8(colorA, colorC);
           product = INTERPOLATE8(colorA, colorB);

           r += GetResult1 (colorA&0x00FFFFFF, colorB&0x00FFFFFF, colorG&0x00FFFFFF, colorE&0x00FFFFFF, colorI&0x00FFFFFF);
           r += GetResult2 (colorB&0x00FFFFFF, colorA&0x00FFFFFF, colorK&0x00FFFFFF, colorF&0x00FFFFFF, colorJ&0x00FFFFFF);
           r += GetResult2 (colorB&0x00FFFFFF, colorA&0x00FFFFFF, colorH&0x00FFFFFF, colorN&0x00FFFFFF, colorM&0x00FFFFFF);
           r += GetResult1 (colorA&0x00FFFFFF, colorB&0x00FFFFFF, colorL&0x00FFFFFF, colorO&0x00FFFFFF, colorP&0x00FFFFFF);

           if (r > 0)
            product2 = colorA;
           else
           if (r < 0)
            product2 = colorB;
           else
            {
             product2 = Q_INTERPOLATE8(colorA, colorB, colorC, colorD);
            }
          }
        }
       else
        {
         product2 = Q_INTERPOLATE8(colorA, colorB, colorC, colorD);

         if ((colorA == colorC) && (colorA == colorF) &&
             (colorB != colorE) && (colorB == colorJ))
          {
           product = colorA;
          }
         else
         if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
          {
           product = colorB;
          }
         else
          {
           product = INTERPOLATE8(colorA, colorB);
          }

         if ((colorA == colorB) && (colorA == colorH) &&
             (colorG != colorC) && (colorC == colorM))
          {
           product1 = colorA;
          }
         else
         if ((colorC == colorG) && (colorC == colorD) &&
             (colorA != colorH) && (colorA == colorI))
          {
           product1 = colorC;
          }
         else
          {
           product1 = INTERPOLATE8(colorA, colorC);
          }
        }

//////////////////////////

       *dP=colorA;
       *(dP+1)=product;
       *(dP+(srcPitchHalf))=product1;
       *(dP+1+(srcPitchHalf))=product2;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

////////////////////////////////////////////////////////////////////////

void SuperEagle_ex8(unsigned char *srcPtr, DWORD srcPitch,
	                unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch        = srcPitch<<1;
 DWORD srcPitchHalf    = srcPitch>>1;
 int   finWidth        = srcPitch>>2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA1, colorA2,
       colorB1, colorB2,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 finalw=width<<1;
 finalh=height<<1;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
       if(finish==finWidth) iXA=0;
       else                 iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0)  {iYA=0;}
       else         {iYA=finWidth;}
       if(height>4) {iYB=finWidth;iYC=srcPitchHalf;}
       else
       if(height>3) {iYB=finWidth;iYC=finWidth;}
       else         {iYB=0;iYC=0;}

       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);

       if(color2 == color6 && color5 != color3)
        {
         product1b = product2a = color2;
         if((color1 == color2) ||
            (color6 == colorB2))
          {
           product1a = INTERPOLATE8(color2, color5);
           product1a = INTERPOLATE8(color2, product1a);
          }
         else
          {
           product1a = INTERPOLATE8(color5, color6);
          }

         if((color6 == colorS2) ||
            (color2 == colorA1))
          {
           product2b = INTERPOLATE8(color2, color3);
           product2b = INTERPOLATE8(color2, product2b);
          }
         else
          {
           product2b = INTERPOLATE8(color2, color3);
          }
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1a = color5;

         if ((colorB1 == color5) ||
             (color3 == colorS1))
          {
           product1b = INTERPOLATE8(color5, color6);
           product1b = INTERPOLATE8(color5, product1b);
          }
         else
          {
           product1b = INTERPOLATE8(color5, color6);
          }

         if ((color3 == colorA2) ||
             (color4 == color5))
          {
           product2a = INTERPOLATE8(color5, color2);
           product2a = INTERPOLATE8(color5, product2a);
          }
         else
          {
           product2a = INTERPOLATE8(color2, color3);
          }
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          {
           product1b = product2a = color2;
           product1a = product2b = INTERPOLATE8(color5, color6);
          }
         else
         if (r < 0)
          {
           product2b = product1a = color5;
           product1b = product2a = INTERPOLATE8(color5, color6);
          }
         else
          {
           product2b = product1a = color5;
           product1b = product2a = color2;
          }
        }
       else
        {
         product2b = product1a = INTERPOLATE8(color2, color6);
         product2b = Q_INTERPOLATE8(color3, color3, color3, product2b);
         product1a = Q_INTERPOLATE8(color5, color5, color5, product1a);

         product2a = product1b = INTERPOLATE8(color5, color3);
         product2a = Q_INTERPOLATE8(color2, color2, color2, product2a);
         product1b = Q_INTERPOLATE8(color6, color6, color6, product1b);
        }

////////////////////////////////

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(srcPitchHalf))=product2a;
       *(dP+1+(srcPitchHalf))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

/////////////////////////

//#include <assert.h>

static __inline void scale2x_32_def_whole(uint32_t*  dst0, uint32_t* dst1, const uint32_t* src0, const uint32_t* src1, const uint32_t* src2, unsigned count)
{

	//assert(count >= 2);

	// first pixel
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		dst0[0] = src1[0] == src0[0] ? src0[0] : src1[0];
		dst0[1] = src1[1] == src0[0] ? src0[0] : src1[0];
		dst1[0] = src1[0] == src2[0] ? src2[0] : src1[0];
		dst1[1] = src1[1] == src2[0] ? src2[0] : src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
	}
	++src0;
	++src1;
	++src2;
	dst0 += 2;
	dst1 += 2;

	// central pixels
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			dst0[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
			dst0[1] = src1[1] == src0[0] ? src0[0] : src1[0];
			dst1[0] = src1[-1] == src2[0] ? src2[0] : src1[0];
			dst1[1] = src1[1] == src2[0] ? src2[0] : src1[0];
		} else {
			dst0[0] = src1[0];
			dst0[1] = src1[0];
			dst1[0] = src1[0];
			dst1[1] = src1[0];
		}

		++src0;
		++src1;
		++src2;
		dst0 += 2;
		dst1 += 2;
		--count;
	}

	// last pixel
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		dst0[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
		dst0[1] = src1[0] == src0[0] ? src0[0] : src1[0];
		dst1[0] = src1[-1] == src2[0] ? src2[0] : src1[0];
		dst1[1] = src1[0] == src2[0] ? src2[0] : src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
	}
}

void Scale2x_ex8(unsigned char *srcPtr, DWORD srcPitch,
				 unsigned char  *dstPtr, int width, int height)
{
	//const int srcpitch = srcPitch;
	const int dstPitch = srcPitch<<1;

	int count = height;

	finalw=width<<1;
	finalh=height<<1;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + (dstPitch >> 2);

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	scale2x_32_def_whole(dst0, dst1, src0, src0, src1, width);

	count -= 2;
	while(count) {
		dst0 += dstPitch >> 1;
		dst1 += dstPitch >> 1;
		scale2x_32_def_whole(dst0, dst1, src0, src0, src1, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstPitch >> 1;
	dst1 += dstPitch >> 1;
	scale2x_32_def_whole(dst0, dst1, src0, src1, src1, width);

}

////////////////////////////////////////////////////////////////////////

static __inline void scale3x_32_def_whole(uint32_t* dst0, uint32_t* dst1, uint32_t* dst2, const uint32_t* src0, const uint32_t* src1, const uint32_t* src2, unsigned count)
{
	//assert(count >= 2);

	//first pixel
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		dst0[0] = src1[0];
		dst0[1] = (src1[0] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[0]) ? src0[0] : src1[0];
		dst0[2] = src1[1] == src0[0] ? src1[1] : src1[0];
		dst1[0] = (src1[0] == src0[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src0[0]) ? src1[0] : src1[0];
		dst1[1] = src1[0];
		dst1[2] = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
		dst2[0] = src1[0];
		dst2[1] = (src1[0] == src2[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src2[0]) ? src2[0] : src1[0];
		dst2[2] = src1[1] == src2[0] ? src1[1] : src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst0[2] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
		dst1[2] = src1[0];
		dst2[0] = src1[0];
		dst2[1] = src1[0];
		dst2[2] = src1[0];
	}
	++src0;
	++src1;
	++src2;
	dst0 += 3;
	dst1 += 3;
	dst2 += 3;

	//central pixels
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			dst0[0] = src1[-1] == src0[0] ? src1[-1] : src1[0];
			dst0[1] = (src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
			dst0[2] = src1[1] == src0[0] ? src1[1] : src1[0];
			dst1[0] = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
			dst1[1] = src1[0];
			dst1[2] = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
			dst2[0] = src1[-1] == src2[0] ? src1[-1] : src1[0];
			dst2[1] = (src1[-1] == src2[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src2[-1]) ? src2[0] : src1[0];
			dst2[2] = src1[1] == src2[0] ? src1[1] : src1[0];
		} else {
			dst0[0] = src1[0];
			dst0[1] = src1[0];
			dst0[2] = src1[0];
			dst1[0] = src1[0];
			dst1[1] = src1[0];
			dst1[2] = src1[0];
			dst2[0] = src1[0];
			dst2[1] = src1[0];
			dst2[2] = src1[0];
		}

		++src0;
		++src1;
		++src2;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
		--count;
	}

	// last pixel
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		dst0[0] = src1[-1] == src0[0] ? src1[-1] : src1[0];
		dst0[1] = (src1[-1] == src0[0] && src1[0] != src0[0]) || (src1[0] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
		dst0[2] = src1[0];
		dst1[0] = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
		dst1[1] = src1[0];
		dst1[2] = (src1[0] == src0[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src0[0]) ? src1[0] : src1[0];
		dst2[0] = src1[-1] == src2[0] ? src1[-1] : src1[0];
		dst2[1] = (src1[-1] == src2[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src2[-1]) ? src2[0] : src1[0];
		dst2[2] = src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst0[2] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
		dst1[2] = src1[0];
		dst2[0] = src1[0];
		dst2[1] = src1[0];
		dst2[2] = src1[0];
	}
}


void Scale3x_ex8(unsigned char *srcPtr, DWORD srcPitch,
				 unsigned char  *dstPtr, int width, int height)
{
	int count = height;

	int dstPitch = srcPitch*3;
	int dstRowPixels = dstPitch>>2;

	finalw=width*3;
	finalh=height*3;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + dstRowPixels;
	uint32_t  *dst2 = dst1 + dstRowPixels;

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	scale3x_32_def_whole(dst0, dst1, dst2, src0, src0, src2, width);

	count -= 2;
	while(count) {
		dst0 += dstRowPixels*3;
		dst1 += dstRowPixels*3;
		dst2 += dstRowPixels*3;

		scale3x_32_def_whole(dst0, dst1, dst2, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}

	dst0 += dstRowPixels*3;
	dst1 += dstRowPixels*3;
	dst2 += dstRowPixels*3;

	scale3x_32_def_whole(dst0, dst1, dst2, src0, src1, src1, width);
}


////////////////////////////////////////////////////////////////////////

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif


////////////////////////////////////////////////////////////////////////
// X STUFF :)
////////////////////////////////////////////////////////////////////////


static Cursor        cursor;
XVisualInfo          vi;
static XVisualInfo   *myvisual;
Display              *display;
static Colormap      colormap;
Window        window;
static GC            hGC;
static XImage      * Ximage;
static XvImage     * XCimage;
static XImage      * XPimage=0;
char *               Xpixels;
char *               pCaptionText;

static int fx=0;


static Atom xv_intern_atom_if_exists( Display *display, char const * atom_name )
{
  XvAttribute * attributes;
  int attrib_count,i;
  Atom xv_atom = None;

  attributes = XvQueryPortAttributes( display, xv_port, &attrib_count );
  if( attributes!=NULL )
  {
    for ( i = 0; i < attrib_count; ++i )
    {
      if ( strcmp(attributes[i].name, atom_name ) == 0 )
      {
        xv_atom = XInternAtom( display, atom_name, False );
        break; // found what we want, break out
      }
    }
    XFree( attributes );
  }

  return xv_atom;
}



// close display

void DestroyDisplay(void)
{
 if(display)
  {
   XFreeColormap(display, colormap);
   if(hGC)
    {
     XFreeGC(display,hGC);
     hGC = 0;
    }
   if(Ximage)
    {
     XDestroyImage(Ximage);
     Ximage=0;
    }
   if(XCimage)
    {
     XFree(XCimage);
     XCimage=0;
    }

	XShmDetach(display,&shminfo);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid,IPC_RMID,NULL);

  Atom atom_vsync = xv_intern_atom_if_exists(display, "XV_SYNC_TO_VBLANK");
  if (atom_vsync != None) {
	XvSetPortAttribute(display, xv_port, atom_vsync, xv_vsync);
  }

   XSync(display,False);

   XCloseDisplay(display);
  }
}

static int depth=0;
int root_window_id=0;


// Create display

void CreateDisplay(void)
{
 XSetWindowAttributes winattr;
 int                  myscreen;
 XEvent               event;
 XSizeHints           hints;
 XWMHints             wm_hints;
 MotifWmHints         mwmhints;
 Atom                 mwmatom;

 Atom			delwindow;

 XGCValues            gcv;
 int i;

 int ret, j, p;
 int formats;
 unsigned int p_num_adaptors=0, p_num_ports=0;

 XvAdaptorInfo		*ai;
 XvImageFormatValues	*fo;

 XClassHint* classHint;

 int yuv_port = -1, yuv_id = -1;
 int rgb_port = -1, rgb_id = -1;
 int xv_depth = 0;

 // Open display
 display = XOpenDisplay(NULL);

 if (!display)
  {
   fprintf (stderr,"Failed to open display!!!\n");
   DestroyDisplay();
   return;
  }


 // desktop fullscreen switch
 if (!iWindowMode) fx = 1;

 screen=DefaultScreenOfDisplay(display);
 myscreen=DefaultScreen(display);
 root_window_id=RootWindow(display, myscreen);

 //Look for an Xvideo RGB port
 ret = XvQueryAdaptors(display, root_window_id, &p_num_adaptors, &ai);
 if (ret != Success) {
   if (ret == XvBadExtension)
     printf("XvBadExtension returned at XvQueryExtension.\n");
   else
     if (ret == XvBadAlloc)
   printf("XvBadAlloc returned at XvQueryExtension.\n");
     else
   printf("other error happaned at XvQueryAdaptors.\n");

   exit(-1);
 }

 depth = DefaultDepth(display, myscreen);

 for (i = 0; i < p_num_adaptors; i++) {
   p_num_ports = ai[i].base_id + ai[i].num_ports;
   for (p = ai[i].base_id; p < p_num_ports; p++) {
     fo = XvListImageFormats(display, p, &formats);
     for (j = 0; j < formats; j++) {
       //Check for compatible YUV modes for backup
       //hmm, should I bother check guid == 55595659-0000-0010-8000-00aa00389b71?
       //and check byte order?   fo[j].byte_order == LSBFirst
#ifdef __BIG_ENDIAN__
       if ( fo[j].type == XvYUV && fo[j].bits_per_pixel == 16 && fo[j].format == XvPacked && strncmp("YUYV", fo[j].component_order, 5) == 0 )
#else
       if ( fo[j].type == XvYUV && fo[j].bits_per_pixel == 16 && fo[j].format == XvPacked && strncmp("UYVY", fo[j].component_order, 5) == 0 )
#endif
       {
         yuv_port = p;
         yuv_id = fo[j].id;
       }
       if (fo[j].type == XvRGB && fo[j].bits_per_pixel == 32)
       {
         rgb_port = p;
         rgb_id = fo[j].id;
         xv_depth = fo[j].depth;
         printf("RGB mode found.  id: %x, depth: %d\n", xv_id, xv_depth);
         
         if (xv_depth != depth) {
           printf("Warning: Depth does not match screen depth (%d)\n", depth);
         }
         else {
           //break out of loops
           j = formats;
           p = p_num_ports;
           i = p_num_adaptors;
         }
       }

       // Are we searching for a specific mode?
       /*
       if ( fo[j].id == uOverrideMode) {
         if (fo[j].type == XvYUV) {
           xv_id = yuv_id = fo[j].id;
           xv_port = yuv_port = p;
           use_yuv = True;
         } else if (fo[j].type == XvRGB) {
           xv_id = rgb_id = fo[j].id;
           xv_port = rgb_port = p;
           //xv_depth = fo[j].depth;
           use_yuv = False;
         }
         // Get out
         j = formats;
         p = p_num_ports;
         i = p_num_adaptors;
         //break;
       }*/
     }
     if (fo)
         XFree(fo);
   }
   if (yuv_port != -1) i = p_num_adaptors; // TODO: at least intel adapters >0 just display black image
 }
 if (p_num_adaptors > 0)
   XvFreeAdaptorInfo(ai);
 if (xv_port == -1 && rgb_port == -1 && yuv_port == -1)
 {
   printf("RGB or YUV not available for this adapter. See xvinfo. Quitting.\n");
   exit(-1);
 }
 else if (xv_port != -1) {
     printf("Using explicit mode id = %x.\n", uOverrideMode);
 }
 else if (rgb_port == -1 && yuv_port != -1)
 {
   use_yuv = True;
   printf("RGB not found. Using YUV.\n");
   xv_port = yuv_port;
   xv_id = yuv_id;
 }
 else if (xv_depth && xv_depth != depth && yuv_port != -1)
 {
   use_yuv = True;
   printf("Acceptable RGB mode not found.  Using YUV.\n");
   xv_port = yuv_port;
   xv_id = yuv_id;
 }
 else if (rgb_port != -1) {
   xv_port = rgb_port;
   xv_id = rgb_id;
 }

 if ((dwActFixes&0x800)) { // Try to use Xv's sync
   Atom atom_vsync = xv_intern_atom_if_exists(display, "XV_SYNC_TO_VBLANK");
   if (atom_vsync != None) {
       XvGetPortAttribute(display, xv_port, atom_vsync, &xv_vsync);
       XvSetPortAttribute(display, xv_port, atom_vsync, 0);
   }
 }

 myvisual = 0;

 if(XMatchVisualInfo(display,myscreen, depth, TrueColor, &vi))
   myvisual = &vi;

 if (!myvisual)
 {
   fprintf(stderr,"Failed to obtain visual!\n");
   DestroyDisplay();
   return;
 }

 if(myvisual->red_mask==0x00007c00 &&
    myvisual->green_mask==0x000003e0 &&
    myvisual->blue_mask==0x0000001f)
     {iColDepth=15;}
 else
 if(myvisual->red_mask==0x0000f800 &&
    myvisual->green_mask==0x000007e0 &&
    myvisual->blue_mask==0x0000001f)
     {iColDepth=16;}
 else
 if(myvisual->red_mask==0x00ff0000 &&
    myvisual->green_mask==0x0000ff00 &&
    myvisual->blue_mask==0x000000ff)
     {iColDepth=32;}
 else
  {
   iColDepth=0;
/*   fprintf(stderr,"COLOR DEPTH NOT SUPPORTED!\n");
   fprintf(stderr,"r: %08lx\n",myvisual->red_mask);
   fprintf(stderr,"g: %08lx\n",myvisual->green_mask);
   fprintf(stderr,"b: %08lx\n",myvisual->blue_mask);
   DestroyDisplay();
   return;*/
  }

 // pffff... much work for a simple blank cursor... oh, well...
 if(iWindowMode) cursor=XCreateFontCursor(display,XC_left_ptr);
 else
  {
   Pixmap p1,p2;
   XImage * img;
   XColor b,w;
   char * idata;
   XGCValues GCv;
   GC        GCc;

   memset(&b,0,sizeof(XColor));
   memset(&w,0,sizeof(XColor));
   idata=(char *)malloc(8);
   memset(idata,0,8);

   p1=XCreatePixmap(display,RootWindow(display,myvisual->screen),8,8,1);
   p2=XCreatePixmap(display,RootWindow(display,myvisual->screen),8,8,1);

   img = XCreateImage(display,myvisual->visual,
                      1,XYBitmap,0,idata,8,8,8,1);

   GCv.function   = GXcopy;
   GCv.foreground = ~0;
   GCv.background =  0;
   GCv.plane_mask = AllPlanes;
   GCc = XCreateGC(display,p1,
                   (GCFunction|GCForeground|GCBackground|GCPlaneMask),&GCv);

   XPutImage(display, p1,GCc,img,0,0,0,0,8,8);
   XPutImage(display, p2,GCc,img,0,0,0,0,8,8);
   XFreeGC(display, GCc);

   cursor = XCreatePixmapCursor(display,p1,p2,&b,&w,0,0);

   XFreePixmap(display,p1);
   XFreePixmap(display,p2);
   XDestroyImage(img); // will free idata as well
  }

 colormap=XCreateColormap(display,root_window_id,
                          myvisual->visual,AllocNone);

 winattr.background_pixel=BlackPixelOfScreen(screen);
 winattr.border_pixel=WhitePixelOfScreen(screen);
 winattr.bit_gravity=ForgetGravity;
 winattr.win_gravity=NorthWestGravity;
 winattr.backing_store=NotUseful;

 winattr.override_redirect=False;
 winattr.save_under=False;
 winattr.event_mask=ExposureMask |
                    VisibilityChangeMask |
                    FocusChangeMask |
                    KeyPressMask | KeyReleaseMask |
                    ButtonPressMask | ButtonReleaseMask |
                    PointerMotionMask;
 winattr.do_not_propagate_mask=0;
 winattr.colormap=colormap;
 winattr.cursor=None;

 window=XCreateWindow(display,root_window_id,
                      0,0,iResX,iResY,
                      0,myvisual->depth,
                      InputOutput,myvisual->visual,
                      CWBorderPixel | CWBackPixel |
                      CWEventMask | CWDontPropagate |
                      CWColormap | CWCursor | CWEventMask,
                      &winattr);

 if(!window)
  {
   fprintf(stderr,"Failed in XCreateWindow()!!!\n");
   DestroyDisplay();
   return;
  }

 delwindow = XInternAtom(display,"WM_DELETE_WINDOW",0);
 XSetWMProtocols(display, window, &delwindow, 1);

 hints.flags=USPosition|USSize;
 hints.base_width = iResX;
 hints.base_height = iResY;

 wm_hints.input=1;
 wm_hints.flags=InputHint;

 XSetWMHints(display,window,&wm_hints);
 XSetWMNormalHints(display,window,&hints);

 if(!pCaptionText)
     pCaptionText = "P.E.Op.S SoftX PSX Gpu";

 // set the WM_NAME and WM_CLASS of the window

 // set the titlebar name
 XStoreName(display, window, pCaptionText);

 // set the name and class hints for the window manager to use
 classHint = XAllocClassHint();
 if(classHint)
 {
   classHint->res_name = pCaptionText;
   classHint->res_class = pCaptionText;
 }

 XSetClassHint(display, window, classHint);
 XFree(classHint);

 XDefineCursor(display,window,cursor);

 // hack to get rid of window title bar
 if (fx)
  {
   mwmhints.flags=MWM_HINTS_DECORATIONS;
   mwmhints.decorations=0;
   mwmatom=XInternAtom(display,"_MOTIF_WM_HINTS",0);
   XChangeProperty(display,window,mwmatom,mwmatom,32,
                   PropModeReplace,(unsigned char *)&mwmhints,4);
  }

 XMapRaised(display,window);
 XClearWindow(display,window);
 XWindowEvent(display,window,ExposureMask,&event);

 if (fx) // fullscreen
  {
   XResizeWindow(display,window,screen->width,screen->height);

   hints.min_width   = hints.max_width = hints.base_width = screen->width;
   hints.min_height= hints.max_height = hints.base_height = screen->height;

   XSetWMNormalHints(display,window,&hints);

   // set the window layer for GNOME
   {
    XEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = 1;
    xev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", 0);
    xev.xclient.window = window;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent(display, root_window_id, 0,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);
   }
  }

 gcv.foreground = 0x0000FF00; // green letters for the FPS bar; do we need to take care of endianess?
 gcv.background = 0x00000000;
 gcv.graphics_exposures = False;
 hGC = XCreateGC(display,window,
                 GCGraphicsExposures | GCForeground | GCBackground, &gcv);
 if(!hGC)
  {
   fprintf(stderr,"No gfx context!!!\n");
   DestroyDisplay();
  }

 uint32_t color;

 /* fix the green back ground in YUV mode */
 if(use_yuv)
	 color = rgb_to_yuv(0x00, 0x00, 0x00);
 else
	 color = 0;

 Xpixels = (char *)malloc(8*8*4);
 for(i = 0; i < 8*8; ++i)
	 ((uint32_t *)Xpixels)[i] = color;

 XCimage = XvCreateImage(display,xv_port,xv_id,
                      (char *)Xpixels, 8, 8);


/*
Allocate max that could be needed:
Big(est?) PSX res: 640x512
32bpp (times 4)
2xsai func= 3xwidth,3xheight
= approx 11.8mb
*/
shminfo.shmid = shmget(IPC_PRIVATE, 640*512*4*3*3, IPC_CREAT | 0777);
shminfo.shmaddr = shmat(shminfo.shmid, 0, 0);
shminfo.readOnly = 0;

 if (!XShmAttach(display, &shminfo)) {
    printf("XShmAttach failed !\n");
    exit (-1);
 }

 {
   uint32_t *pShmaddr = (uint32_t *)shminfo.shmaddr;
   for(i = 0; i < 640*512*3*3; ++i)
	 pShmaddr[i] = color;
 }
}

void (*p2XSaIFunc) (unsigned char *, DWORD, unsigned char *, int, int);
unsigned char *pBackBuffer = 0;

void BlitScreen32(unsigned char *surf, int32_t x, int32_t y)
{
 unsigned char *pD;
 unsigned int startxy;
 uint32_t lu;
 unsigned short s;
 unsigned short row, column;
 unsigned short dx = PreviousPSXDisplay.Range.x1;
 unsigned short dy = PreviousPSXDisplay.DisplayMode.y;

 int32_t lPitch = PSXDisplay.DisplayMode.x << 2;

 uint32_t *destpix;

 if (PreviousPSXDisplay.Range.y0) // centering needed?
  {
   memset(surf, 0, (PreviousPSXDisplay.Range.y0 >> 1) * lPitch);

   dy -= PreviousPSXDisplay.Range.y0;
   surf += (PreviousPSXDisplay.Range.y0 >> 1) * lPitch;

   memset(surf + dy * lPitch,
          0, ((PreviousPSXDisplay.Range.y0 + 1) >> 1) * lPitch);
  }

 if (PreviousPSXDisplay.Range.x0)
  {
   for (column = 0; column < dy; column++)
    {
     destpix = (uint32_t *)(surf + (column * lPitch));
     memset(destpix, 0, PreviousPSXDisplay.Range.x0 << 2);
    }
   surf += PreviousPSXDisplay.Range.x0 << 2;
  }

 if (PSXDisplay.RGB24)
  {
   for (column = 0; column < dy; column++)
    {
     startxy = ((1024) * (column + y)) + x;
     pD = (unsigned char *)&psxVuw[startxy];
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       lu = *((uint32_t *)pD);
       destpix[row] =
          0xff000000 | (RED(lu) << 16) | (GREEN(lu) << 8) | (BLUE(lu));
       pD += 3;
      }
    }
  }
 else
  {
   for (column = 0;column<dy;column++)
    {
     startxy = (1024 * (column + y)) + x;
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       s = GETLE16(&psxVuw[startxy++]);
       destpix[row] =
          (((s << 19) & 0xf80000) | ((s << 6) & 0xf800) | ((s >> 7) & 0xf8)) | 0xff000000;
      }
    }
  }
}



void BlitToYUV(unsigned char * surf,int32_t x,int32_t y)
{
 unsigned char * pD;
 unsigned int startxy;
 uint32_t lu;unsigned short s;
 unsigned short row,column;
 unsigned short dx = PreviousPSXDisplay.Range.x1;
 unsigned short dy = PreviousPSXDisplay.DisplayMode.y;
 int R,G,B;

 int32_t lPitch = PSXDisplay.DisplayMode.x << 2;
 uint32_t *destpix;

 if (PreviousPSXDisplay.Range.y0) // centering needed?
  {
   for (column = 0; column < (PreviousPSXDisplay.Range.y0 >> 1); column++)
    {
     destpix = (uint32_t *)(surf + column * lPitch);
     for (row = 0; row < dx; row++)
     {
      destpix[row] = (4 << 24) | (128 << 16) | (4 << 8) | 128;
     }
    }

   dy -= PreviousPSXDisplay.Range.y0;
   surf += (PreviousPSXDisplay.Range.y0 >> 1) * lPitch;

   for (column = 0; column < (PreviousPSXDisplay.Range.y0 + 1) >> 1; column++)
    {
     destpix = (uint32_t *)(surf + (dy + column) * lPitch);
     for (row = 0; row < dx; row++)
     {
      destpix[row] = (4 << 24) | (128 << 16) | (4 << 8) | 128;
     }
    }
  }

 if (PreviousPSXDisplay.Range.x0)
  {
   for (column = 0; column < dy; column++)
    {
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < PreviousPSXDisplay.Range.x0; row++)
      {
       destpix[row] = (4 << 24) | (128 << 16) | (4 << 8) | 128;
      }
    }
   surf += PreviousPSXDisplay.Range.x0 << 2;
  }

 if (PSXDisplay.RGB24)
  {
   for (column = 0; column < dy; column++)
    {
     startxy = (1024 * (column + y)) + x;
     pD = (unsigned char *)&psxVuw[startxy];
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       lu = *((uint32_t *)pD);

       R = RED(lu);
       G = GREEN(lu);
       B = BLUE(lu);

       destpix[row] = rgb_to_yuv(R, G, B);

       pD += 3;
      }
    }
  }
 else
  {
   for (column = 0; column < dy; column++)
    {
     startxy = (1024 * (column + y)) + x;
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       s = GETLE16(&psxVuw[startxy++]);

       R = (s << 3) &0xf8;
       G = (s >> 2) &0xf8;
       B = (s >> 7) &0xf8;

       destpix[row] = rgb_to_yuv(R, G, B);

      }
    }
  }
}

//dst will have half the pitch (32bit to 16bit)
void RGB2YUV(uint32_t *s, int width, int height, uint32_t *d)
{
	int x,y;
	int R,G,B, Y1,Y2,U,V;

	for (y=0; y<height; y++) {
		for(x=0; x<width>>1; x++) {
			R = (*s >> 16) & 0xff;
			G = (*s >> 8) & 0xff;
			B = *s & 0xff;
			s++;

			Y1 = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);
			U = min(abs(R * -1214 + G * -2384 + B * 3598 + 4096 + 1048576) >> 13, 240);
			V = min(abs(R * 3598 + G * -3013 + B * -585 + 4096 + 1048576) >> 13, 240);

			R = (*s >> 16) & 0xff;
			G = (*s >> 8) & 0xff;
			B = *s & 0xff;
			s++;

			Y2 = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);

#ifdef __BIG_ENDIAN__
			*d = V | Y2 << 8 | U << 16 | Y1 << 24;
#else
			*d = U | Y1 << 8 | V << 16 | Y2 << 24;
#endif
			d++;
		}
	}
}

extern time_t tStart;

/* compute the position and the size of output screen
 * The aspect of the psx output mode is preserved.
 * Note: dest dx,dy,dw,dh are both input and output variables
 */
__inline void MaintainAspect(uint32_t * dx, uint32_t * dy, uint32_t * dw, uint32_t * dh)
{

	double ratio_x = ((double)*dw) / ((double)PSXDisplay.DisplayMode.x) ;
	double ratio_y = ((double)*dh) / ((double)PSXDisplay.DisplayMode.y);

	double ratio;
	if (ratio_x < ratio_y) {
		ratio = ratio_x;
	} else {
		ratio = ratio_y;
	}

	uint32_t tw = (uint32_t) floor(PSXDisplay.DisplayMode.x * ratio);
	uint32_t th = (uint32_t) floor(PSXDisplay.DisplayMode.y * ratio);

	*dx = (uint32_t) floor((*dw - tw) / 2.0);
	*dy = (uint32_t) floor((*dh - th) / 2.0);
	*dw = tw;
	*dh = th;
}

void DoBufferSwap(void)
{
	Window _dw;
	XvImage *xvi;
	unsigned int dstx, dsty;
	unsigned int _d, _w, _h;	//don't care about _d

	finalw = PSXDisplay.DisplayMode.x;
	finalh = PSXDisplay.DisplayMode.y;

	if (finalw == 0 || finalh == 0)
		return;

	XSync(display,False);

	if(use_yuv) {
		if (iUseNoStretchBlt==0 || finalw > 320 || finalh > 256) {
			BlitToYUV((unsigned char *)shminfo.shmaddr, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
			finalw <<= 1;
		} else {
			BlitScreen32((unsigned char *)pBackBuffer, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
			p2XSaIFunc(pBackBuffer, finalw<<2, (unsigned char *)pSaIBigBuff,finalw,finalh);
			RGB2YUV( (uint32_t*)pSaIBigBuff, finalw, finalh, (uint32_t*)shminfo.shmaddr);
		}
	} else if(iUseNoStretchBlt==0 || finalw > 320 || finalh > 256) {
		BlitScreen32((unsigned char *)shminfo.shmaddr, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
	} else {
		BlitScreen32((unsigned char *)pBackBuffer, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
		p2XSaIFunc(pBackBuffer, finalw<<2, (unsigned char *)shminfo.shmaddr,finalw,finalh);
	}

	XGetGeometry(display, window, &_dw, (int *)&_d, (int *)&_d, &_w, &_h, &_d, &_d);
	xvi = XvShmCreateImage(display, xv_port, xv_id, 0, finalw, finalh, &shminfo);

	xvi->data = shminfo.shmaddr;

	if (!screen) screen=DefaultScreenOfDisplay(display);
	//screennum = DefaultScreen(display);

	if (!iWindowMode) {
		_w = screen->width;
		_h = screen->height;
	}

	dstx = 0;
	dsty = 0;

	if (iMaintainAspect)
		MaintainAspect(&dstx, &dsty, &_w, &_h);

/*Whistler: too slow/laggy so commented out for now
    if(iRumbleTime)
    {
       dstx += (rand() % iRumbleVal) - iRumbleVal / 2;
       _w -= (rand() % iRumbleVal) - iRumbleVal / 2;
       dsty += (rand() % iRumbleVal) - iRumbleVal / 2;
       _h -= (rand() % iRumbleVal) - iRumbleVal / 2;
       iRumbleTime--;
    }
*/
	XvShmPutImage(display, xv_port, window, hGC, xvi,
		0,0,		//src x,y
		finalw,finalh,	//src w,h
		dstx,dsty,	//dst x,y
		_w, _h,		//dst w,h
		1
		);

	if(ulKeybits&KEY_SHOWFPS) //DisplayText();   c            // paint menu text
	{
		if(szDebugText[0] && ((time(NULL) - tStart) < 2))
		{
			strcpy(szDispBuf,szDebugText);
		}
		else
		{
			szDebugText[0]=0;
			strcat(szDispBuf,szMenuBuf);
		}

		//XPutImage(display,window,hGC, XFimage,
		//          0, 0, 0, 0, 220,15);

		XDrawImageString(display,window,hGC,2,13,szDispBuf,strlen(szDispBuf));
	}

	//if(XPimage) DisplayPic();

	XFree(xvi);
}

void DoClearScreenBuffer(void)                         // CLEAR DX BUFFER
{
 Window _dw;
 unsigned int _d, _w, _h;	//don't care about _d

 XGetGeometry(display, window, &_dw, (int *)&_d, (int *)&_d, &_w, &_h, &_d, &_d);

 XvPutImage(display, xv_port, window, hGC, XCimage,
           0, 0, 8, 8, 0, 0, _w, _h);
 //XSync(display,False);
}

void DoClearFrontBuffer(void)                          // CLEAR DX BUFFER
{/*
 XPutImage(display,window,hGC, XCimage,
           0, 0, 0, 0, iResX, iResY);
 XSync(display,False);*/
}

int Xinitialize()
{
   iDesktopCol=32;


 if(iUseNoStretchBlt>0)
  {
   pBackBuffer=(unsigned char *)malloc(640*512*sizeof(uint32_t));
   memset(pBackBuffer,0,640*512*sizeof(uint32_t));
   if (use_yuv) {
    pSaIBigBuff=malloc(640*512*4*3*3);
    memset(pSaIBigBuff,0,640*512*4*3*3);
   }
  }

 p2XSaIFunc=NULL;

 if(iUseNoStretchBlt==1)
  {
   p2XSaIFunc=Std2xSaI_ex8;
  }

 if(iUseNoStretchBlt==2)
  {
   p2XSaIFunc=Super2xSaI_ex8;
  }

 if(iUseNoStretchBlt==3)
  {
   p2XSaIFunc=SuperEagle_ex8;
  }

 if(iUseNoStretchBlt==4)
  {
   p2XSaIFunc=Scale2x_ex8;
  }
 if(iUseNoStretchBlt==5)
  {
   p2XSaIFunc=Scale3x_ex8;
  }
 if(iUseNoStretchBlt==6)
  {
   p2XSaIFunc=hq2x_32;
  }
 if(iUseNoStretchBlt==7)
  {
   p2XSaIFunc=hq3x_32;
  }

 bUsingTWin=FALSE;

 InitMenu();

 bIsFirstFrame = FALSE;                                // done

 if(iShowFPS)
  {
   iShowFPS=0;
   ulKeybits|=KEY_SHOWFPS;
   szDispBuf[0]=0;
   BuildDispMenu(0);
  }

 return 0;
}

void Xcleanup()                                        // X CLEANUP
{
 CloseMenu();

 if(iUseNoStretchBlt>0)
  {
   if(pBackBuffer)  free(pBackBuffer);
   pBackBuffer=0;
   if(pSaIBigBuff) free(pSaIBigBuff);
   pSaIBigBuff=0;
  }
}

unsigned long ulInitDisplay(void)
{
 CreateDisplay();                                      // x stuff
 Xinitialize();                                        // init x
 return (unsigned long)display;
}

void CloseDisplay(void)
{
 Xcleanup();                                           // cleanup dx
 DestroyDisplay();
}

void CreatePic(unsigned char * pMem)
{
 unsigned char * p=(unsigned char *)malloc(128*96*4);
 unsigned char * ps; int x,y;

 ps=p;

 if(iDesktopCol==16)
  {
   unsigned short s;
   for(y=0;y<96;y++)
    {
     for(x=0;x<128;x++)
      {
       s=(*(pMem+0))>>3;
       s|=((*(pMem+1))&0xfc)<<3;
       s|=((*(pMem+2))&0xf8)<<8;
       pMem+=3;
       *((unsigned short *)(ps+y*256+x*2))=s;
      }
    }
  }
 else
 if(iDesktopCol==15)
  {
   unsigned short s;
   for(y=0;y<96;y++)
    {
     for(x=0;x<128;x++)
      {
       s=(*(pMem+0))>>3;
       s|=((*(pMem+1))&0xfc)<<2;
       s|=((*(pMem+2))&0xf8)<<7;
       pMem+=3;
       *((unsigned short *)(ps+y*256+x*2))=s;
      }
    }
  }
 else
 if(iDesktopCol==32)
  {
   uint32_t l;
   for(y=0;y<96;y++)
    {
     for(x=0;x<128;x++)
      {
       l=  *(pMem+0);
       l|=(*(pMem+1))<<8;
       l|=(*(pMem+2))<<16;
       pMem+=3;
       *((uint32_t *)(ps+y*512+x*4))=l;
      }
    }
  }

 XPimage = XCreateImage(display,myvisual->visual,
                        depth, ZPixmap, 0,
                        (char *)p,
                        128, 96,
                        depth>16 ? 32 : 16,
                        0);
}

void DestroyPic(void)
{
 if(XPimage)
  { /*
   XPutImage(display,window,hGC, XCimage,
	  0, 0, 0, 0, iResX, iResY);*/
   XDestroyImage(XPimage);
   XPimage=0;
  }
}

void DisplayPic(void)
{
 XPutImage(display,window,hGC, XPimage,
           0, 0, iResX-128, 0,128,96);
}

void ShowGpuPic(void)
{
}

void ShowTextGpuPic(void)
{
}

static void hq2x_32_def(uint32_t * dst0, uint32_t * dst1, const uint32_t * src0, const uint32_t * src1, const uint32_t * src2, unsigned count)
{
	static unsigned char cache_vert_mask[640];
	unsigned char cache_horiz_mask = 0;

	unsigned i;
	unsigned char mask;
	uint32_t  c[9];

	if (src0 == src1)	//processing first row
		memset(cache_vert_mask, 0, count);

	for(i=0;i<count;++i) {
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		mask = 0;

		mask |= interp_32_diff(c[0], c[4]) << 0;
		mask |= cache_vert_mask[i];
		mask |= interp_32_diff(c[2], c[4]) << 2;
		mask |= cache_horiz_mask;
		cache_horiz_mask = interp_32_diff(c[5], c[4]) << 3;
		mask |= cache_horiz_mask << 1;	// << 3 << 1 == << 4
		mask |= interp_32_diff(c[6], c[4]) << 5;
		cache_vert_mask[i] = interp_32_diff(c[7], c[4]) << 1;
		mask |= cache_vert_mask[i] << 5; // << 1 << 5 == << 6
		mask |= interp_32_diff(c[8], c[4]) << 7;

#define P0 dst0[0]
#define P1 dst0[1]
#define P2 dst1[0]
#define P3 dst1[1]
#define MUR interp_32_diff(c[1], c[5])
#define MDR interp_32_diff(c[5], c[7])
#define MDL interp_32_diff(c[7], c[3])
#define MUL interp_32_diff(c[3], c[1])
#define IC(p0) c[p0]
#define I11(p0,p1) interp_32_11(c[p0], c[p1])
#define I211(p0,p1,p2) interp_32_211(c[p0], c[p1], c[p2])
#define I31(p0,p1) interp_32_31(c[p0], c[p1])
#define I332(p0,p1,p2) interp_32_332(c[p0], c[p1], c[p2])
#define I431(p0,p1,p2) interp_32_431(c[p0], c[p1], c[p2])
#define I521(p0,p1,p2) interp_32_521(c[p0], c[p1], c[p2])
#define I53(p0,p1) interp_32_53(c[p0], c[p1])
#define I611(p0,p1,p2) interp_32_611(c[p0], c[p1], c[p2])
#define I71(p0,p1) interp_32_71(c[p0], c[p1])
#define I772(p0,p1,p2) interp_32_772(c[p0], c[p1], c[p2])
#define I97(p0,p1) interp_32_97(c[p0], c[p1])
#define I1411(p0,p1,p2) interp_32_1411(c[p0], c[p1], c[p2])
#define I151(p0,p1) interp_32_151(c[p0], c[p1])

		switch (mask) {
#include "hq2x.h"
		}

#undef P0
#undef P1
#undef P2
#undef P3
#undef MUR
#undef MDR
#undef MDL
#undef MUL
#undef IC
#undef I11
#undef I211
#undef I31
#undef I332
#undef I431
#undef I521
#undef I53
#undef I611
#undef I71
#undef I772
#undef I97
#undef I1411
#undef I151

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 2;
		dst1 += 2;
	}
}

void hq2x_32( unsigned char * srcPtr,  DWORD srcPitch, unsigned char * dstPtr, int width, int height)
{
	const int dstPitch = srcPitch<<1;

	int count = height;

	finalw=width*2;
	finalh=height*2;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + (dstPitch >> 2);

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	hq2x_32_def(dst0, dst1, src0, src0, src1, width);


	count -= 2;
	while(count) {
		dst0 += dstPitch >> 1;		//next 2 lines (dstPitch / 4 char per int * 2)
		dst1 += dstPitch >> 1;
		hq2x_32_def(dst0, dst1, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstPitch >> 1;
	dst1 += dstPitch >> 1;
	hq2x_32_def(dst0, dst1, src0, src1, src1, width);
}

static void hq3x_32_def(uint32_t*  dst0, uint32_t*  dst1, uint32_t*  dst2, const uint32_t* src0, const uint32_t* src1, const uint32_t* src2, unsigned count)
{
	static unsigned char cache_vert_mask[640];
	unsigned char cache_horiz_mask = 0;

	unsigned i;
	unsigned char mask;
	uint32_t  c[9];

	if (src0 == src1)	//processing first row
		memset(cache_vert_mask, 0, count);

	for(i=0;i<count;++i) {
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		mask = 0;

		mask |= interp_32_diff(c[0], c[4]) << 0;
		mask |= cache_vert_mask[i];
		mask |= interp_32_diff(c[2], c[4]) << 2;
		mask |= cache_horiz_mask;
		cache_horiz_mask = interp_32_diff(c[5], c[4]) << 3;
		mask |= cache_horiz_mask << 1;	// << 3 << 1 == << 4
		mask |= interp_32_diff(c[6], c[4]) << 5;
		cache_vert_mask[i] = interp_32_diff(c[7], c[4]) << 1;
		mask |= cache_vert_mask[i] << 5; // << 1 << 5 == << 6
		mask |= interp_32_diff(c[8], c[4]) << 7;

#define P0 dst0[0]
#define P1 dst0[1]
#define P2 dst0[2]
#define P3 dst1[0]
#define P4 dst1[1]
#define P5 dst1[2]
#define P6 dst2[0]
#define P7 dst2[1]
#define P8 dst2[2]
#define MUR interp_32_diff(c[1], c[5])
#define MDR interp_32_diff(c[5], c[7])
#define MDL interp_32_diff(c[7], c[3])
#define MUL interp_32_diff(c[3], c[1])
#define IC(p0) c[p0]
#define I11(p0,p1) interp_32_11(c[p0], c[p1])
#define I211(p0,p1,p2) interp_32_211(c[p0], c[p1], c[p2])
#define I31(p0,p1) interp_32_31(c[p0], c[p1])
#define I332(p0,p1,p2) interp_32_332(c[p0], c[p1], c[p2])
#define I431(p0,p1,p2) interp_32_431(c[p0], c[p1], c[p2])
#define I521(p0,p1,p2) interp_32_521(c[p0], c[p1], c[p2])
#define I53(p0,p1) interp_32_53(c[p0], c[p1])
#define I611(p0,p1,p2) interp_32_611(c[p0], c[p1], c[p2])
#define I71(p0,p1) interp_32_71(c[p0], c[p1])
#define I772(p0,p1,p2) interp_32_772(c[p0], c[p1], c[p2])
#define I97(p0,p1) interp_32_97(c[p0], c[p1])
#define I1411(p0,p1,p2) interp_32_1411(c[p0], c[p1], c[p2])
#define I151(p0,p1) interp_32_151(c[p0], c[p1])

		switch (mask) {
#include "hq3x.h"
		}

#undef P0
#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7
#undef P8
#undef MUR
#undef MDR
#undef MDL
#undef MUL
#undef IC
#undef I11
#undef I211
#undef I31
#undef I332
#undef I431
#undef I521
#undef I53
#undef I611
#undef I71
#undef I772
#undef I97
#undef I1411
#undef I151

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
	}
}

void hq3x_32( unsigned char * srcPtr,  DWORD srcPitch, unsigned char * dstPtr, int width, int height)
{
	int count = height;

	int dstPitch = srcPitch*3;
	int dstRowPixels = dstPitch>>2;

	finalw=width*3;
	finalh=height*3;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + dstRowPixels;
	uint32_t  *dst2 = dst1 + dstRowPixels;

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	hq3x_32_def(dst0, dst1, dst2, src0, src0, src2, width);

	count -= 2;
	while(count) {
		dst0 += dstRowPixels * 3;
		dst1 += dstRowPixels * 3;
		dst2 += dstRowPixels * 3;

		hq3x_32_def(dst0, dst1, dst2, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstRowPixels * 3;
	dst1 += dstRowPixels * 3;
	dst2 += dstRowPixels * 3;

	hq3x_32_def(dst0, dst1, dst2, src0, src1, src1, width);

}
