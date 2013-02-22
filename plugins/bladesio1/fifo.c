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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "typedefs.h"
#include "psemu_plugin_defs.h"

#include "sio1.h"
#include "fifo.h"
#include "connection.h"

/***************************************************************************/

static u8  _buf[8];
static s32 _indexw;
static s32 _indexr;
static s32 _employment;
static s32 _overrun;

/***************************************************************************/

void fifoOpen() {
	fifoReset();
	fifoResetErr();
}

void fifoClose() {
}

/***************************************************************************/

void fifoReset() {
	_indexw		= 0;
	_indexr		= 0;
	_employment = 0;
}

void fifoResetErr() {
	_overrun  = 0;
}

/***************************************************************************/

s32 fifoEmployment() {
	return _employment;
}

s32 fifoEmpty() {
	return (_employment == 0);
}

s32 fifoFull() {
	return (_employment == 8);
}

s32 fifoOverrun() {
	return _overrun;
}

/***************************************************************************/

void fifoPush(u8 data) {
	if(_employment > 8) {
		_overrun = 1;

		--_indexw;
		if(_indexw < 0)
			_indexw += 8;

		--_employment;
	}

	_buf[_indexw] = data;

	++_indexw;
	if(_indexw >= 8)
		_indexw -= 8;

	++_employment;
}

void fifoPeek(u8 *data) {
	if(_employment <= 0)
		*data = 0;
	else
		*data = _buf[_indexr];
}

void fifoPop(u8 *data) {
	if(_employment <= 0) {
		*data = 0;
	}
	else {
		*data = _buf[_indexr];

		++_indexr;
		if(_indexr >= 8)
			_indexr -= 8;

		--_employment;
	}
}

/***************************************************************************/
