/***************************************************************************
 *   Copyright (C) 2013 by Blade_Arma <edgbla@yandex.ru>                   *
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

#ifndef _CONNECTION_H_
#define _CONNECTION_H_

/***************************************************************************/

extern int connectionOpen();
extern void connectionClose();

/***************************************************************************/

extern int connectionSend(u8 *pdata, s32 size);
extern int connectionRecv(u8 *pdata, s32 size);

/***************************************************************************/

#endif // _CONNECTION_H_
