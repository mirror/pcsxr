/***************************************************************************
 *   Debugger-Interface for PCSX-DF                                        *
 *                                                                         *
 *   Copyright (C) 2008 Stefan Sikora                                      *
 *   hoshy['at']schrauberstube['dot']de                                    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

#ifndef __HDBUG_H__
#define __HDBUG_H__

// 0 = run cpu, 1 = pause cpu, 2 = trace cpu
int hdb_pause;
unsigned long hdb_break;

void hdb_start();
void hdb_auto_pause ();

#endif /* __HDBUG_H__ */
