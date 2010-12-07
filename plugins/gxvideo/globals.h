/*
 * Copyright (C) 2010 Benoit Gschwind
 * Inspired by original author : Pete Bernert
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "cfg.h"
#include "fps.h"
#include "gpu.h"
#include "draw.h"
#include "prim.h"
#include "soft.h"

extern char * g_file_name;
extern gxv_cfg_t g_cfg;
extern gxv_gpu_t g_gpu;
extern gxv_draw_t g_draw;
extern gxv_prim_t g_prim;
extern gxv_soft_t g_soft;

#endif /* GLOBALS_H_ */
