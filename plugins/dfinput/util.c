/*
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * All Rights Reserved.
 *
 * Based on: Cdrom for Psemu Pro like Emulators
 * By: linuzappz <linuzappz@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#include "pad.h"
#include "util.h"

void grabCursor(Display *dpy, Window win, int grab)
{
    if(!grab)
    {
        XUngrabPointer(dpy, CurrentTime);
    }
    else
    {
        XGrabPointer(dpy, win, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, win, None, CurrentTime);
    }
}

void showCursor(Display *dpy, Window win, int show)
{
    if(!show)
    {
        Pixmap   bm_no;
        Colormap cmap;
        Cursor   no_ptr;
        XColor   black, dummy;

        char bm_no_data[] =
        {
            0, 0, 0, 0, 0, 0, 0, 0
        };

        cmap = DefaultColormap(dpy, DefaultScreen(dpy));
        XAllocNamedColor(dpy, cmap, "black", &black, &dummy);
        bm_no = XCreateBitmapFromData(dpy, win, bm_no_data, 8, 8);
        no_ptr = XCreatePixmapCursor(dpy, bm_no, bm_no, &black, &black, 0, 0);

        XDefineCursor(dpy, win, no_ptr);

        XFreeCursor(dpy, no_ptr);
        XFreePixmap(dpy, bm_no);
        XFreeColors(dpy, cmap, &black.pixel, 1, 0);
    }
    else
    {
        XDefineCursor(dpy, win, 0);
    }
}
