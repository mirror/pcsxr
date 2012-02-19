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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#ifndef GPU_UTILS_H_
#define GPU_UTILS_H_

#include <stdint.h>

typedef struct {
	uint16_t rgb16;
} __attribute__((__packed__)) gxv_rgb16;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} __attribute__((__packed__)) gxv_rgb24;

/* for fast recast ^^ */
typedef union {
	uint8_t * u8;
	int8_t * s8;
	uint16_t * u16;
	int16_t * s16;
	uint32_t * u32;
	int32_t * s32;
	gxv_rgb16 * rgb16;
	gxv_rgb24 * rgb24;
} gxv_pointer_t;

typedef struct {
	int32_t x;
	int32_t y;
} gxv_point_t;

typedef struct {
	int16_t x;
	int16_t y;
} gxv_spoint_t;

typedef struct {
	int16_t x0;
	int16_t x1;
	int16_t y0;
	int16_t y1;
} gxv_rect_t;

typedef struct {
	gxv_point_t position;
	gxv_point_t mode;
	gxv_point_t DrawOffset;
	gxv_rect_t range;
} gxv_display_t;

typedef struct {
	gxv_rect_t Position;
} gxv_win_t;

#endif /* GPU_UTILS_H_ */
