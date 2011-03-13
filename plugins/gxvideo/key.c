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

#include "menu.h"
#include "gpu.h"
#include "draw.h"
#include "key.h"

#define VK_INSERT      65379
#define VK_HOME        65360
#define VK_PRIOR       65365
#define VK_NEXT        65366
#define VK_END         65367
#define VK_DEL         65535
#define VK_F5          65474

void GPUmakeSnapshot(void);

unsigned long ulKeybits = 0;

void GPUkeypressed(int keycode) {

}

void SetKeyHandler(void) {
}

void ReleaseKeyHandler(void) {
}
