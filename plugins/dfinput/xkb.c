/*
 * Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * All Rights Reserved.
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

static Atom wmprotocols, wmdelwindow;
static int g_currentMouse_X;
static int g_currentMouse_Y;
static Window window;
static uint8_t resumeScrSaver = 0;

void InitKeyboard() {
    int revert_to;

    wmprotocols = XInternAtom(g.Disp, "WM_PROTOCOLS", 0);
    wmdelwindow = XInternAtom(g.Disp, "WM_DELETE_WINDOW", 0);

    // Hide cursor and lock cursor to window if type is mouse
    XkbSetDetectableAutoRepeat(g.Disp, 1, NULL);
    XGetInputFocus(g.Disp, &window, &revert_to);
    if (g.cfg.PadDef[0].Type == PSE_PAD_TYPE_MOUSE ||
        g.cfg.PadDef[1].Type == PSE_PAD_TYPE_MOUSE) {
        grabCursor(g.Disp, window, 1);
        showCursor(g.Disp, window, 0);
    } else if (g.cfg.HideCursor) {
        showCursor(g.Disp, window, 0);
    }

    // Disable screensaver - this could be in different place
    resumeScrSaver = 0;
    if (g.cfg.PreventScrSaver) {
        char buf[64];
        snprintf(buf, 64, "xdg-screensaver suspend 0x%x > /dev/null 2>&1", window);
        if (pclose(popen(buf, "r")) == 0) {
            resumeScrSaver = 1;
            printf("Suspending Window ID 0x%x of activating screensaver.\n", window);
        } else {
            //resumeScrSaver = 0;
            fprintf(stderr, "Failed to execute xdg-screensaver (maybe not installed?)\n");
        }
    }

    g_currentMouse_X = 0;
    g_currentMouse_Y = 0;

    g.PadState[0].KeyStatus = 0xFFFF;
    g.PadState[1].KeyStatus = 0xFFFF;
}

void DestroyKeyboard() {
	XkbSetDetectableAutoRepeat(g.Disp, 0, NULL);

    // Enable cursor and revert grab cursor if mouse
    if (g.cfg.PadDef[0].Type == PSE_PAD_TYPE_MOUSE ||
        g.cfg.PadDef[1].Type == PSE_PAD_TYPE_MOUSE) {
        grabCursor(g.Disp, window, 0);
        showCursor(g.Disp, window, 1);
    } else if (g.cfg.HideCursor) {
        showCursor(g.Disp, window, 1);
    }

    // Enable screensaver if it was disabled - this could be in different place
    if (resumeScrSaver) {
        char buf[64];
        printf("Resuming Window ID 0x%x to activate screensaver.\n", window);
        snprintf(buf, 64, "xdg-screensaver resume 0x%x", window);
        FILE *phandle = popen(buf, "r");
        pclose(phandle);
    }
}

static void bdown(int pad, int bit)
{
	if(bit < 16)
		g.PadState[pad].KeyStatus &= ~(1 << bit);
	else if(bit == DKEY_ANALOG)
		g.PadState[pad].PadModeSwitch = 1;
}

static void bup(int pad, int bit)
{
	if(bit < 16)
		g.PadState[pad].KeyStatus |= (1 << bit);
}

void CheckKeyboard() {
	uint8_t					i, j, found;
	XEvent					evt;
	XClientMessageEvent		*xce;
	uint16_t				Key;

	while (XPending(g.Disp)) {
		XNextEvent(g.Disp, &evt);
		switch (evt.type) {
            case ButtonPress:
                for(i = 0; i < 2; ++i) {
                    if(g.cfg.PadDef[i].Type == PSE_PAD_TYPE_MOUSE) {
                            switch(evt.xbutton.button) {
                                case 1:
                                    bdown(i, 11);
                                    break;
                                case 3:
                                    bdown(i, 10);
                                    break;
                            }
                        }
                    }
                break;
            case ButtonRelease:
                for(i = 0; i < 2; ++i) {
                    if(g.cfg.PadDef[i].Type == PSE_PAD_TYPE_MOUSE) {
                            switch(evt.xbutton.button) {
                                case 1:
                                    bup(i, 11);
                                    break;
                                case 3:
                                    bup(i, 10);
                                    break;
                            }
                        }
                    }
                break;
            case MotionNotify:
                g_currentMouse_X = evt.xmotion.x - 160;
                g_currentMouse_Y = evt.xmotion.y - 120;
                if( g_currentMouse_X < -128) g_currentMouse_X = -128;
                if( g_currentMouse_X > 127) g_currentMouse_X = 127;
                if( g_currentMouse_Y < -128) g_currentMouse_Y = -128;
                if( g_currentMouse_Y > 127) g_currentMouse_Y = 127;
                break;
			case KeyPress:
				Key = XLookupKeysym((XKeyEvent *)&evt, 0);
				found = 0;
				for (i = 0; i < 2; i++) {
					for (j = 0; j < DKEY_TOTAL; j++) {
						if (g.cfg.PadDef[i].KeyDef[j].Key == Key) {
							found = 1;
							bdown(i, j);
						}
					}
				}
				if (!found && !AnalogKeyPressed(Key)) {
					for (i=0 ; i < EMU_TOTAL ; i++) {
						if (Key == g.cfg.E.EmuDef[i].Mapping.Key /*&& g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending == 0*/) {
							//printf("press %x %x and %x\n", Key, g.cfg.E.EmuDef[i].Mapping.Key, g.cfg.E.EmuDef[i].EmuKeyEvent);
							Key = g.cfg.E.EmuDef[i].EmuKeyEvent;
							//g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 1; // joypad sends immediately release if enabled here
							i=EMU_TOTAL;
						}
					}
					g.KeyLeftOver = Key;
				}
				break;
			case KeyRelease:
				Key = XLookupKeysym((XKeyEvent *)&evt, 0);
				found = 0;
				for (i = 0; i < 2; i++) {
					for (j = 0; j < DKEY_TOTAL; j++) {
						if (g.cfg.PadDef[i].KeyDef[j].Key == Key) {
							found = 1;
							bup(i, j);
						}
					}
				}
				if (!found && !AnalogKeyReleased(Key)) {
					for (i=0 ; i < EMU_TOTAL ; i++) {
						if (Key == g.cfg.E.EmuDef[i].Mapping.Key) {
							//printf("release %x and %x\n", Key, g.cfg.E.EmuDef[i].EmuKeyEvent);
							Key = g.cfg.E.EmuDef[i].EmuKeyEvent;
							//g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 0;
							i=EMU_TOTAL;
						}
					}
					g.KeyLeftOver = (long) ( Key | 0x40000000l );
				}
				break;
			case ClientMessage:
				xce = (XClientMessageEvent *)&evt;
				if (xce->message_type == wmprotocols && (Atom)xce->data.l[0] == wmdelwindow) {
					// Fake an ESC key if user clicked the close button on window
					g.KeyLeftOver = XK_Escape;
					return;
				}
				break;
		}
    }

	g.PadState[0].MouseAxis[0][0] = g_currentMouse_X;
    g.PadState[0].MouseAxis[0][1] = g_currentMouse_Y;
    g_currentMouse_X *= 0.7;
    g_currentMouse_Y *= 0.7;

    if (g.cfg.PadDef[0].Type == PSE_PAD_TYPE_MOUSE ||
        g.cfg.PadDef[1].Type == PSE_PAD_TYPE_MOUSE) {
        XWarpPointer(g.Disp, None, window, 0, 0, 0, 0, 160, 120);
    }
}
