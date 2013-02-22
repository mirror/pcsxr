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

#ifndef _SIO1_H_
#define _SIO1_H_

/***************************************************************************/

#ifdef ENABLE_NLS
#define _(s) dgettext(GETTEXT_PACKAGE, s)
#define N_(s) (s)
#else
#define _(s) (s)
#define N_(s) (s)
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

enum {
	PLAYER_DISABLED = 0,
	PLAYER_MASTER,
	PLAYER_SLAVE
};

typedef struct Settings {
	s32 enabled;
	s32 player;
	char ip[32];
	u16 port;
} Settings;

/******************************************************************************/

extern void settingsRead();
extern void settingsWrite();

extern Settings settings;

/***************************************************************************/

#endif // _SIO1_H_
