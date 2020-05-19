/***************************************************************************
                          key.c  -  description
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

#define _IN_KEY

#include "externals.h"
#include "menu.h"
#include "gpu.h"
#include "draw.h"
#include "key.h"

////////////////////////////////////////////////////////////////////////
// keyboard handler (UNIX)
////////////////////////////////////////////////////////////////////////
#ifdef _MACGL
#define VK_INSERT      114
#define VK_HOME        115
#define VK_PRIOR       116
#define VK_NEXT        121
#define VK_END         119
#define VK_DEL         117
#define VK_F5          96
#else
#define VK_INSERT      65379
#define VK_HOME        65360
#define VK_PRIOR       65365
#define VK_NEXT        65366
#define VK_END         65367
#define VK_DEL         65535
#define VK_F5          65474
#endif

void GPUmakeSnapshot(void);

unsigned long          ulKeybits=0;

void GPUkeypressed(int keycode)
{
 switch(keycode)
  {
   case 0xFFC9:			//X11 key: F12
   case ((1<<29) | 0xFF0D):	//special keycode from pcsx-df: alt-enter
       bChangeWinMode=TRUE;
       break;
#ifndef _MACGL
   case XK_section:	//special - accelerate
       iFastFwd = ( iFastFwd != 0 ? 0 : 1 );
       UseFrameLimit = ( UseFrameLimit != 0 ? 0 : 1 );
       break;
#endif
   case VK_F5:
       GPUmakeSnapshot();
       break;

   case VK_INSERT:
       if(iUseFixes) {iUseFixes=0;dwActFixes=0;}
       else          {iUseFixes=1;dwActFixes=dwCfgFixes;}
       SetFixes();
       if(iFrameLimit==2) SetAutoFrameCap();
       break;

   case VK_DEL:
       if(ulKeybits&KEY_SHOWFPS)
         ulKeybits&=~KEY_SHOWFPS;
       else
        {
         ulKeybits|=KEY_SHOWFPS;
         szDispBuf[0]=0;
         BuildDispMenu(0);
        }
       break;

   case VK_PRIOR: BuildDispMenu(-1);            break;
   case VK_NEXT:  BuildDispMenu( 1);            break;
   case VK_END:   SwitchDispMenu(1);            break;
   case VK_HOME:  SwitchDispMenu(-1);           break;
#ifndef _MACGL // 0x60 is VK_F5 in OSX, so I put this here until I figure something better -npepinpe
   case 0x60:
    {
     iFastFwd = 1 - iFastFwd;
     bSkipNextFrame = FALSE;
     UseFrameSkip = iFastFwd;
     BuildDispMenu(0);
     break;
    }
#else
   default: { void HandleKey(int keycode); HandleKey(keycode); }
#endif
  }
}

void SetKeyHandler(void)
{
}

void ReleaseKeyHandler(void)
{
}
