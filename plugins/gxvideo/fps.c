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

#include <unistd.h>
#include <sys/time.h>

#include "globals.h"
#include "fps.h"
#include "gpu.h"

#define TIMEBASE 100000

unsigned long time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, 0); // well, maybe there are better ways
	return tv.tv_sec * 100000 + tv.tv_usec / 10; // to do that, but at least it works
}

void frame_cap(int fps) {
	static unsigned int last_time = 0;
	unsigned int c = time_get_time();

	if (!fps) {
		last_time = c;
		return;
	}

	if (last_time + (100000/fps) < c) {
		last_time = c;
		return;
	}

	while (last_time + (100000/fps) - 20 > c) {
		usleep(((100000/fps) - 20 - (c - last_time)) * 10 );
		c = time_get_time();
	}

	last_time = c;

}

void compute_fps() {
	static int count = 0;
	static unsigned int last_time = 0;
	unsigned int c = time_get_time();
	++count;
	if((c - last_time) > 100000) {
		g_draw.fps = ((double)count) / ((double)(c - last_time)) * 100000;
		count = 0;
		last_time = c;
	}
}
