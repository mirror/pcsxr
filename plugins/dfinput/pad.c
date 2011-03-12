/*
 * Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
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

static void (*gpuVisualVibration)(uint32_t, uint32_t) = NULL;

char *PSEgetLibName(void) {
	return _("Gamepad/Keyboard Input");
}

unsigned long CALLBACK PSEgetLibType(void) {
	return PSE_LT_PAD;
}

unsigned long CALLBACK PSEgetLibVersion(void) {
	return (1 << 16) | (1 << 8);
}

static void PADsetMode(const int pad, const int mode) {
	g.PadState[pad].PadMode = mode;
	g.PadState[pad].PadID = mode ? 0x73 : 0x41;

	g.PadState[pad].Vib0 = 0;
	g.PadState[pad].Vib1 = 0;
	g.PadState[pad].VibF[0] = 0;
	g.PadState[pad].VibF[1] = 0;
}

long PADinit(long flags) {
	LoadPADConfig();

	PADsetMode(0, 0);
	PADsetMode(1, 0);

	gpuVisualVibration = NULL;

	return PSE_PAD_ERR_SUCCESS;
}

long PADshutdown(void) {
	PADclose();
	return PSE_PAD_ERR_SUCCESS;
}

static pthread_t			ThreadID;
static volatile uint8_t		TerminateThread = 0;

static void *JoyThread(void *param) {
	while (!TerminateThread) {
		CheckJoy();
		usleep(1000);
	}
	pthread_exit(0);
	return NULL;
}

long PADopen(unsigned long *Disp) {
	g.Disp = (Display *)*Disp;

	if (!g.Opened) {
		if (SDL_WasInit(SDL_INIT_EVERYTHING)) {
			if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
				return PSE_PAD_ERR_FAILURE;
			}
		} else if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) == -1) {
			return PSE_PAD_ERR_FAILURE;
		}

		InitSDLJoy();
		InitKeyboard();

		g.KeyLeftOver = 0;

		if (g.cfg.Threaded) {
			TerminateThread = 0;

			if (pthread_create(&ThreadID, NULL, JoyThread, NULL) != 0) {
				// thread creation failed, fallback to polling
				g.cfg.Threaded = 0;
			}
		}
	}

	g.Opened = 1;

	return PSE_PAD_ERR_SUCCESS;
}

long PADclose(void) {
	if (g.Opened) {
		if (g.cfg.Threaded) {
			TerminateThread = 1;
			pthread_join(ThreadID, NULL);
		}

		DestroySDLJoy();
		DestroyKeyboard();

		if (SDL_WasInit(SDL_INIT_EVERYTHING & ~SDL_INIT_JOYSTICK)) {
			SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		} else {
			SDL_Quit();
		}
	}

	g.Opened = 0;

	return PSE_PAD_ERR_SUCCESS;
}

long PADquery(void) {
	return PSE_PAD_USE_PORT1 | PSE_PAD_USE_PORT2;
}

static void UpdateInput(void) {
	int pad;
	if (!g.cfg.Threaded) CheckJoy();
	for(pad = 0; pad < 2; pad++) {
		if(g.PadState[pad].PadModeSwitch) {
			g.PadState[pad].PadModeSwitch = 0;
			PADsetMode(pad, 1 - g.PadState[pad].PadMode);
		}
	}
	CheckKeyboard();
}

static uint8_t stdpar[2][8] = {
	{0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80},
	{0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80}
};

static uint8_t unk46[2][8] = {
	{0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A},
	{0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A}
};

static uint8_t unk47[2][8] = {
	{0xFF, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00}
};

static uint8_t unk4c[2][8] = {
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static uint8_t unk4d[2][8] = { 
	{0xFF, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0xFF, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

static uint8_t stdcfg[2][8]   = { 
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static uint8_t stdmode[2][8]  = { 
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static uint8_t stdmodel[2][8] = { 
	{0xFF,
	 0x5A,
	 0x01, // 03 - dualshock2, 01 - dualshock
	 0x02, // number of modes
	 0x01, // current mode: 01 - analog, 00 - digital
	 0x02,
	 0x01,
	 0x00},
	{0xFF, 
	 0x5A,
	 0x01, // 03 - dualshock2, 01 - dualshock
	 0x02, // number of modes
	 0x01, // current mode: 01 - analog, 00 - digital
	 0x02,
	 0x01,
	 0x00}
};

static uint8_t CurPad = 0, CurByte = 0, CurCmd = 0, CmdLen = 0;

unsigned char PADstartPoll(int pad) {
	CurPad = pad - 1;
	CurByte = 0;

	return 0xFF;
}

unsigned char PADpoll(unsigned char value) {
	static uint8_t		*buf = NULL;
	uint16_t			n;

	if (CurByte == 0) {
		CurByte++;

		// Don't enable Analog/Vibration for a standard pad
		if (g.cfg.PadDef[CurPad].Type != PSE_PAD_TYPE_ANALOGPAD) {
			CurCmd = CMD_READ_DATA_AND_VIBRATE;
		} else {
			CurCmd = value;
		}

		switch (CurCmd) {
			case CMD_CONFIG_MODE:
				CmdLen = 8;
				buf = stdcfg[CurPad];
				if (stdcfg[CurPad][3] == 0xFF) return 0xF3;
				else return g.PadState[CurPad].PadID;

			case CMD_SET_MODE_AND_LOCK:
				CmdLen = 8;
				buf = stdmode[CurPad];
				return 0xF3;

			case CMD_QUERY_MODEL_AND_MODE:
				CmdLen = 8;
				buf = stdmodel[CurPad];
				buf[4] = g.PadState[CurPad].PadMode;
				return 0xF3;

			case CMD_QUERY_ACT:
				CmdLen = 8;
				buf = unk46[CurPad];
				return 0xF3;

			case CMD_QUERY_COMB:
				CmdLen = 8;
				buf = unk47[CurPad];
				return 0xF3;

			case CMD_QUERY_MODE:
				CmdLen = 8;
				buf = unk4c[CurPad];
				return 0xF3;

			case CMD_VIBRATION_TOGGLE:
				CmdLen = 8;
				buf = unk4d[CurPad];
				return 0xF3;

			case CMD_READ_DATA_AND_VIBRATE:
			default:
				UpdateInput();

				n = g.PadState[CurPad].KeyStatus;
				n &= g.PadState[CurPad].JoyKeyStatus;

				stdpar[CurPad][2] = n & 0xFF;
				stdpar[CurPad][3] = n >> 8;

				if (g.PadState[CurPad].PadMode == 1) {
					CmdLen = 8;

					stdpar[CurPad][4] = g.PadState[CurPad].AnalogStatus[ANALOG_RIGHT][0];
					stdpar[CurPad][5] = g.PadState[CurPad].AnalogStatus[ANALOG_RIGHT][1];
					stdpar[CurPad][6] = g.PadState[CurPad].AnalogStatus[ANALOG_LEFT][0];
					stdpar[CurPad][7] = g.PadState[CurPad].AnalogStatus[ANALOG_LEFT][1];
				} else {
					CmdLen = 4;
				}

				buf = stdpar[CurPad];
				return g.PadState[CurPad].PadID;
		}
	}

	switch (CurCmd) {
		case CMD_READ_DATA_AND_VIBRATE:
			if (g.cfg.PadDef[CurPad].Type == PSE_PAD_TYPE_ANALOGPAD) {
				if (CurByte == g.PadState[CurPad].Vib0) {
					g.PadState[CurPad].VibF[0] = value;

					if (gpuVisualVibration != NULL && (g.PadState[CurPad].VibF[0] != 0 || g.PadState[CurPad].VibF[1] != 0)) {
						gpuVisualVibration(g.PadState[CurPad].VibF[0], g.PadState[CurPad].VibF[1]);
					}
				}

				if (CurByte == g.PadState[CurPad].Vib1) {
					g.PadState[CurPad].VibF[1] = value;

					if (gpuVisualVibration != NULL && (g.PadState[CurPad].VibF[0] != 0 || g.PadState[CurPad].VibF[1] != 0)) {
						gpuVisualVibration(g.PadState[CurPad].VibF[0], g.PadState[CurPad].VibF[1]);
					}
				}
			}
			break;

		case CMD_CONFIG_MODE:
			if (CurByte == 2) {
				switch (value) {
					case 0:
						buf[2] = 0;
						buf[3] = 0;
						break;

					case 1:
						buf[2] = 0xFF;
						buf[3] = 0xFF;
						break;
				}
			}
			break;

		case CMD_SET_MODE_AND_LOCK:
			if (CurByte == 2) {
				PADsetMode(CurPad, value);
			}
			break;

		case CMD_QUERY_ACT:
			if (CurByte == 2) {
				switch (value) {
					case 0: // default
						buf[5] = 0x02;
						buf[6] = 0x00;
						buf[7] = 0x0A;
						break;

					case 1: // Param std conf change
						buf[5] = 0x01;
						buf[6] = 0x01;
						buf[7] = 0x14;
						break;
				}
			}
			break;

		case CMD_QUERY_MODE:
			if (CurByte == 2) {
				switch (value) {
					case 0: // mode 0 - digital mode
						buf[5] = PSE_PAD_TYPE_STANDARD;
						break;

					case 1: // mode 1 - analog mode
						buf[5] = PSE_PAD_TYPE_ANALOGPAD;
						break;
				}
			}
			break;

		case CMD_VIBRATION_TOGGLE:
			if (CurByte >= 2 && CurByte < CmdLen) {
				if (CurByte == g.PadState[CurPad].Vib0) {
					buf[CurByte] = 0;
				}
				if (CurByte == g.PadState[CurPad].Vib1) {
					buf[CurByte] = 1;
				}

				if (value == 0) {
					g.PadState[CurPad].Vib0 = CurByte;
					if ((g.PadState[CurPad].PadID & 0x0f) < (CurByte - 1) / 2) {
						g.PadState[CurPad].PadID = (g.PadState[CurPad].PadID & 0xf0) + (CurByte - 1) / 2;
					}
				} else if (value == 1) {
					g.PadState[CurPad].Vib1 = CurByte;
					if ((g.PadState[CurPad].PadID & 0x0f) < (CurByte - 1) / 2) {
						g.PadState[CurPad].PadID = (g.PadState[CurPad].PadID & 0xf0) + (CurByte - 1) / 2;
					}
				}
			}
			break;
	}

	if (CurByte >= CmdLen) return 0;
	return buf[CurByte++];
}

static long PADreadPort(int num, PadDataS *pad) {
	UpdateInput();

	pad->buttonStatus = (g.PadState[num].KeyStatus & g.PadState[num].JoyKeyStatus);

	// ePSXe different from pcsx, swap bytes
	pad->buttonStatus = (pad->buttonStatus >> 8) | (pad->buttonStatus << 8);

	switch (g.cfg.PadDef[num].Type) {
		case PSE_PAD_TYPE_ANALOGPAD: // Analog Controller SCPH-1150
			pad->controllerType = PSE_PAD_TYPE_ANALOGPAD;
			pad->rightJoyX = g.PadState[num].AnalogStatus[ANALOG_RIGHT][0];
			pad->rightJoyY = g.PadState[num].AnalogStatus[ANALOG_RIGHT][1];
			pad->leftJoyX = g.PadState[num].AnalogStatus[ANALOG_LEFT][0];
			pad->leftJoyY = g.PadState[num].AnalogStatus[ANALOG_LEFT][1];
			break;

		case PSE_PAD_TYPE_STANDARD: // Standard Pad SCPH-1080, SCPH-1150
		default:
			pad->controllerType = PSE_PAD_TYPE_STANDARD;
			break;
	}

	return PSE_PAD_ERR_SUCCESS;
}

long PADreadPort1(PadDataS *pad) {
	return PADreadPort(0, pad);
}

long PADreadPort2(PadDataS *pad) {
	return PADreadPort(1, pad);
}

long PADkeypressed(void) {
	long s;

	CheckKeyboard();

	s = g.KeyLeftOver;
	g.KeyLeftOver = 0;

	return s;
}

void PADregisterVibration(void (*callback)(uint32_t, uint32_t)) {
	gpuVisualVibration = callback;
}

#ifndef _MACOSX

long PADconfigure(void) {
	if (fork() == 0) {
		execl("cfg/cfgDFInput", "cfgDFInput", NULL);
		exit(0);
	}
	return PSE_PAD_ERR_SUCCESS;
}

void PADabout(void) {
	if (fork() == 0) {
		execl("cfg/cfgDFInput", "cfgDFInput", "-about", NULL);
		exit(0);
	}
}

#endif

long PADtest(void) {
	return PSE_PAD_ERR_SUCCESS;
}
