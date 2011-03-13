/***************************************************************************
 *   Copyright (C) 2010 by Blade_Arma                                      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef _SIO1_H_
#define _SIO1_H_

#include "config.h"

/******************************************************************************/

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define  _(x) gettext(x)
#define N_(x) (x)
#else
#define  _(x) (x)
#define N_(x) (x)
#endif

#define CALLBACK

extern Settings settings;

/******************************************************************************/

#endif // _SIO1_H_
