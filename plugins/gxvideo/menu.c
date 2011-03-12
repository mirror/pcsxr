/***************************************************************************
                         menu.c  -  description
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

#include "draw.h"
#include "menu.h"
#include "gpu.h"

unsigned long dwCoreFlags = 0;

// create lists/stuff for fonts (actually there are no more lists, but I am too lazy to change the func names ;)
void InitMenu(void)
{
}

// kill existing lists/fonts
void CloseMenu(void)
{
 DestroyPic();
}

// DISPLAY FPS/MENU TEXT

#include <time.h>

int iMPos=0;                                           // menu arrow pos

void DisplayText(void)                                 // DISPLAY TEXT
{

}

// Build Menu buffer (== Dispbuffer without FPS)...
void BuildDispMenu(int iInc)
{

}

// Some menu action...
void SwitchDispMenu(int iStep)                         // SWITCH DISP MENU
{
                                  // update info
}
