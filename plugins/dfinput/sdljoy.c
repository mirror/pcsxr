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

void JoyInitHaptic()
{
#if SDL_VERSION_ATLEAST(2,0,0)
	uint8_t i;
    //unsigned int haptic_query = 0;
	for (i = 0; i < 2; i++)
	{
		SDL_Joystick *curJoy = g.PadState[i].JoyDev;
		if (!curJoy && g.PadState[i].GCDev) {
			curJoy = SDL_GameControllerGetJoystick(g.PadState[i].GCDev);
		}
		if (SDL_JoystickIsHaptic(curJoy))
		{
			if (g.PadState[i].haptic != NULL)
			{
				SDL_HapticClose(g.PadState[i].haptic);
				g.PadState[i].haptic = NULL;
			}

			g.PadState[i].haptic = SDL_HapticOpenFromJoystick(curJoy);
			if (g.PadState[i].haptic == NULL)
				continue;

			if (SDL_HapticRumbleSupported(g.PadState[i].haptic) == SDL_FALSE)
			{
				printf("\nRumble not supported\n");
				g.PadState[i].haptic = NULL;
				continue;
			}

			if (SDL_HapticRumbleInit(g.PadState[i].haptic) != 0)
			{
				printf("\nFailed to initialize rumble: %s\n", SDL_GetError());
				g.PadState[i].haptic = NULL;
				continue;
			}
		}
	}
#endif
}

int JoyHapticRumble(int pad, uint32_t low, uint32_t high)
{
#if SDL_VERSION_ATLEAST(2,0,0)
  float mag;

  if (g.PadState[pad].haptic) {

    /* Stop the effect if it was playing. */
    SDL_HapticRumbleStop(g.PadState[pad].haptic);

    mag = ((high * 2 + low) / 6) / 127.5;
    //printf("low: %d high: %d mag: %f\n", low, high, mag);

    if(SDL_HapticRumblePlay(g.PadState[pad].haptic, mag, 500) != 0)
    {
      printf("\nFailed to play rumble: %s\n", SDL_GetError());
      return 1;
    }
  }
#endif
  return 0;
}

void InitSDLJoy() {
	uint8_t				i;
	g.PadState[0].JoyKeyStatus = 0xFFFF;
	g.PadState[1].JoyKeyStatus = 0xFFFF;

	for (i = 0; i < 2; i++) {
		if (g.cfg.PadDef[i].DevNum >= 0) {
#if SDL_VERSION_ATLEAST(2,0,0)
			if (g.cfg.PadDef[i].UseSDL2) {
				g.PadState[i].GCDev = SDL_GameControllerOpen(g.cfg.PadDef[i].DevNum);
			}
			
			if (!g.PadState[i].GCDev) {
				g.PadState[i].JoyDev = SDL_JoystickOpen(g.cfg.PadDef[i].DevNum);
			}
#else
			g.PadState[i].JoyDev = SDL_JoystickOpen(g.cfg.PadDef[i].DevNum);
#endif
			// Saves an extra call to SDL joystick open
			if (g.cfg.E.DevNum == g.cfg.PadDef[i].DevNum) {
				g.cfg.E.EmuKeyDev = g.PadState[i].JoyDev;
			}
		} else {
			g.PadState[i].JoyDev = NULL;
		}
#if !SDL_VERSION_ATLEAST(2,0,0) && defined(__linux__)
		g.PadState[i].VibrateDev = -1;
		g.PadState[i].VibrateEffect = -1;
#endif
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	if (has_haptic)
	{
		JoyInitHaptic();
	}
#endif

	if (g.cfg.E.EmuKeyDev == 0 && g.cfg.E.DevNum >= 0) {
		g.cfg.E.EmuKeyDev = SDL_JoystickOpen(g.cfg.E.DevNum);
	}

	SDL_JoystickEventState(SDL_IGNORE);
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_GameControllerEventState(SDL_IGNORE);
#endif

	InitAnalog();
}

void DestroySDLJoy() {
	uint8_t				i;

	if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
		for (i = 0; i < 2; i++) {
#if SDL_VERSION_ATLEAST(2,0,0)
			if (g.PadState[i].JoyDev != NULL || g.PadState[i].GCDev != NULL) {
				if (g.PadState[i].haptic != NULL) {
					SDL_HapticClose(g.PadState[i].haptic);
					g.PadState[i].haptic = NULL;
				}
				if (g.PadState[i].GCDev != NULL) {
					SDL_GameControllerClose(g.PadState[i].GCDev);
				} else {
					SDL_JoystickClose(g.PadState[i].JoyDev);
				}
			}
#else
			if (g.PadState[i].JoyDev != NULL) {
				SDL_JoystickClose(g.PadState[i].JoyDev);
			}
#endif
		}
	}

	for (i = 0; i < 2; i++) {
		g.PadState[i].JoyDev = NULL;
#if SDL_VERSION_ATLEAST(2,0,0)
		g.PadState[i].GCDev = NULL;
#endif
	}
	g.cfg.E.EmuKeyDev = NULL;
}

static void bdown(int pad, int bit)
{
	if(bit < 16)
		g.PadState[pad].JoyKeyStatus &= ~(1 << bit);
	else if(bit == DKEY_ANALOG) {
		if(++g.PadState[pad].PadModeKey == 10)
			g.PadState[pad].PadModeSwitch = 1;
		else if(g.PadState[pad].PadModeKey > 10)
			g.PadState[pad].PadModeKey = 11;
	}
}

static void bup(int pad, int bit)
{
	if(bit < 16)
		g.PadState[pad].JoyKeyStatus |= (1 << bit);
	else if(bit == DKEY_ANALOG)
		g.PadState[pad].PadModeKey = 0;
}

void CheckJoy() {
	uint8_t				i, j, n;
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_GameControllerUpdate();
#endif

	SDL_JoystickUpdate();

	for (i = 0; i < 2; i++) {
		if (g.PadState[i].JoyDev != NULL) {
		g.PadState[i].JoyKeyStatus = ~0;
		for (j = 0; j < DKEY_TOTAL; j++) {
			switch (g.cfg.PadDef[i].KeyDef[j].JoyEvType) {
				case AXIS:
					n = abs(g.cfg.PadDef[i].KeyDef[j].J.Axis) - 1;

					if (g.cfg.PadDef[i].KeyDef[j].J.Axis > 0) {
						if (SDL_JoystickGetAxis(g.PadState[i].JoyDev, n) > 16383) {
							bdown(i, j);
						} else {
							bup(i, j);
						}
					} else if (g.cfg.PadDef[i].KeyDef[j].J.Axis < 0) {
						if (SDL_JoystickGetAxis(g.PadState[i].JoyDev, n) < -16383) {
							bdown(i, j);
						} else {
							bup(i, j);
						}
					}
					break;

				case HAT:
					n = (g.cfg.PadDef[i].KeyDef[j].J.Hat >> 8);

					if (SDL_JoystickGetHat(g.PadState[i].JoyDev, n) & (g.cfg.PadDef[i].KeyDef[j].J.Hat & 0xFF)) {
						bdown(i, j);
					} else {
						bup(i, j);
					}
					break;

				case BUTTON:
					if (SDL_JoystickGetButton(g.PadState[i].JoyDev, g.cfg.PadDef[i].KeyDef[j].J.Button)) {
						bdown(i, j);
					} else {
						bup(i, j);
					}
					break;

				default:
					break;
			}
		}
	}
		
#if SDL_VERSION_ATLEAST(2,0,0)
		if (g.PadState[i].GCDev != NULL) {
			g.PadState[i].JoyKeyStatus = ~0;
			for (j = 0; j < DKEY_TOTAL; j++) {
				Sint16 axis2;
				switch (j) {
					case DKEY_SELECT:
					case DKEY_L3:
					case DKEY_R3:
					case DKEY_START:
					case DKEY_UP:
					case DKEY_RIGHT:
					case DKEY_DOWN:
					case DKEY_LEFT:
					case DKEY_L1:
					case DKEY_R1:
					case DKEY_TRIANGLE:
					case DKEY_CIRCLE:
					case DKEY_CROSS:
					case DKEY_SQUARE:
					case DKEY_ANALOG:

						if (SDL_GameControllerGetButton(g.PadState[i].GCDev, controllerMap[j])) {
							bdown(i, j);
						} else {
							bup(i, j);
						}
						break;
						
					case DKEY_L2:
						axis2 = SDL_GameControllerGetAxis(g.PadState[i].GCDev, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
						if (axis2 > 0) {
							bdown(i, j);
						} else {
							bup(i, j);
						}
						
						break;
						
					case DKEY_R2:
						axis2 = SDL_GameControllerGetAxis(g.PadState[i].GCDev, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
						if (axis2 > 0) {
							bdown(i, j);
						} else {
							bup(i, j);
						}
						
						break;
						
					default:
						break;
				}
			}
		}
#endif
	}

	// Check for emulator button states
	for (i=(g.cfg.E.EmuKeyDev == NULL ? EMU_TOTAL : 0) ; i < EMU_TOTAL ; i++) {
		switch (g.cfg.E.EmuDef[i].Mapping.JoyEvType) {
			case BUTTON:
				if (SDL_JoystickGetButton(g.cfg.E.EmuKeyDev, g.cfg.E.EmuDef[i].Mapping.J.Button)) {
					if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending == 0) {
						//printf("%x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Button, g.cfg.E.EmuDef[i].EmuKeyEvent);
						g.KeyLeftOver = g.cfg.E.EmuDef[i].EmuKeyEvent;
						g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 1;
					}
				} else if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending) {
					//printf("%x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Button, g.cfg.E.EmuDef[i].EmuKeyEvent);
					g.KeyLeftOver = ( g.cfg.E.EmuDef[i].EmuKeyEvent | 0x40000000l );
					g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 0;
				}
				break;
			case HAT:
				n = (g.cfg.E.EmuDef[i].Mapping.J.Hat >> 8);
				if (SDL_JoystickGetHat(g.cfg.E.EmuKeyDev, n) & (g.cfg.E.EmuDef[i].Mapping.J.Hat & 0xFF)) {
					if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending == 0) {
						//printf("%x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Button, g.cfg.E.EmuDef[i].EmuKeyEvent);
						g.KeyLeftOver = g.cfg.E.EmuDef[i].EmuKeyEvent;
						g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 1;
					}
				} else if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending) {
					//printf("%x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Button, g.cfg.E.EmuDef[i].EmuKeyEvent);
					g.KeyLeftOver = ( g.cfg.E.EmuDef[i].EmuKeyEvent | 0x40000000l );
					g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 0;
				}
				break;
			case AXIS:
				n = abs(g.cfg.E.EmuDef[i].Mapping.J.Axis) - 1;

				if (g.cfg.E.EmuDef[i].Mapping.J.Axis > 0) {
					if (SDL_JoystickGetAxis(g.cfg.E.EmuKeyDev, n) > 16383) {
						if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending == 0) {
							//printf("push1 %x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Axis, g.cfg.E.EmuDef[i].EmuKeyEvent);
							g.KeyLeftOver = g.cfg.E.EmuDef[i].EmuKeyEvent;
							g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 1;
						}
					} else if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending) {
						//printf("rel1 %x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Button, g.cfg.E.EmuDef[i].EmuKeyEvent);
						g.KeyLeftOver = ( g.cfg.E.EmuDef[i].EmuKeyEvent | 0x40000000l );
						g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 0;
					}
				} else if (g.cfg.E.EmuDef[i].Mapping.J.Axis < 0) {
					if (SDL_JoystickGetAxis(g.cfg.E.EmuKeyDev, n) < -16383) {
						if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending == 0) {
							//printf("push2 %x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Axis, g.cfg.E.EmuDef[i].EmuKeyEvent);
							g.KeyLeftOver = g.cfg.E.EmuDef[i].EmuKeyEvent;
							g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 1;
						}
					} else if (g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending) {
						//printf("rel2 %x %x %x %x\n", g.cfg.E.EmuKeyDev, i, g.cfg.E.EmuDef[i].Mapping.J.Button, g.cfg.E.EmuDef[i].EmuKeyEvent);
						g.KeyLeftOver = ( g.cfg.E.EmuDef[i].EmuKeyEvent | 0x40000000l );
						g.cfg.E.EmuDef[i].Mapping.ReleaseEventPending = 0;
					}
				}
				break;
			default:
				break;

		}
	}

	CheckAnalog();

	for (i = 0; i < 2; i++) {
		if(!g.PadState[i].PadMode) {
			if(g.PadState[i].AnalogStatus[ANALOG_LEFT][0] < 64)
				bdown(i, DKEY_LEFT);
			else if(g.PadState[i].AnalogStatus[ANALOG_LEFT][0] > 127 + 64)
				bdown(i, DKEY_RIGHT);
			if(g.PadState[i].AnalogStatus[ANALOG_LEFT][1] < 64)
				bdown(i, DKEY_UP);
			else if(g.PadState[i].AnalogStatus[ANALOG_LEFT][1] > 127 + 64)
				bdown(i, DKEY_DOWN);
		}
	}
}
