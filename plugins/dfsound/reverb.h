/***************************************************************************
                          reverb.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
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

extern void SetREVERB(unsigned short val);
extern void InitREVERB(void);
extern void StartREVERB(int ch);
extern void StoreREVERB(int ch,int ns);
extern void StoreREVERB_CD(int left, int right,int ns);
extern int  MixREVERBLeft(int ns);
extern int  MixREVERBRight(void);
