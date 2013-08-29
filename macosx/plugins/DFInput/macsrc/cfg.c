/*
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
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
#include "cfg.h"

GLOBALDATA g;

long DoConfiguration();
void DoAbout();

long PADconfigure(void) {
	if (SDL_WasInit(SDL_INIT_JOYSTICK)) return PSE_ERR_FATAL; // cannot change settings on the fly
	
	DoConfiguration();
	LoadPADConfig();
	return PSE_ERR_SUCCESS;
}

void PADabout(void) {
	DoAbout();
}

struct {
	uint16_t code;
	const char *desc;
} KeyString[] = {
	{ kVK_ANSI_A + 1, "A" },
	{ kVK_ANSI_B + 1, "B" },
	{ kVK_ANSI_C + 1, "C" },
	{ kVK_ANSI_D + 1, "D" },
	{ kVK_ANSI_E + 1, "E" },
	{ kVK_ANSI_F + 1, "F" },
	{ kVK_ANSI_G + 1, "G" },
	{ kVK_ANSI_H + 1, "H" },
	{ kVK_ANSI_I + 1, "I" },
	{ kVK_ANSI_J + 1, "J" },
	{ kVK_ANSI_K + 1, "K" },
	{ kVK_ANSI_L + 1, "L" },
	{ kVK_ANSI_M + 1, "M" },
	{ kVK_ANSI_N + 1, "N" },
	{ kVK_ANSI_O + 1, "O" },
	{ kVK_ANSI_P + 1, "P" },
	{ kVK_ANSI_Q + 1, "Q" },
	{ kVK_ANSI_R + 1, "R" },
	{ kVK_ANSI_S + 1, "S" },
	{ kVK_ANSI_T + 1, "T" },
	{ kVK_ANSI_U + 1, "U" },
	{ kVK_ANSI_V + 1, "V" },
	{ kVK_ANSI_W + 1, "W" },
	{ kVK_ANSI_X + 1, "X" },
	{ kVK_ANSI_Y + 1, "Y" },
	{ kVK_ANSI_Z + 1, "Z" },
	{ kVK_ANSI_LeftBracket + 1, "[" },
	{ kVK_ANSI_RightBracket + 1, "]" },
	{ kVK_ANSI_Semicolon + 1, ";" },
	{ kVK_ANSI_Quote + 1, "'" },
	{ kVK_ANSI_Comma + 1, "," },
	{ kVK_ANSI_Period + 1, "." },
	{ kVK_ANSI_Slash + 1, "/" },
	{ kVK_ANSI_Grave + 1, "`" },
	{ kVK_ANSI_1 + 1, "1" },
	{ kVK_ANSI_2 + 1, "2" },
	{ kVK_ANSI_3 + 1, "3" },
	{ kVK_ANSI_4 + 1, "4" },
	{ kVK_ANSI_5 + 1, "5" },
	{ kVK_ANSI_6 + 1, "6" },
	{ kVK_ANSI_7 + 1, "7" },
	{ kVK_ANSI_8 + 1, "8" },
	{ kVK_ANSI_9 + 1, "9" },
	{ kVK_ANSI_0 + 1, "0" },
	{ kVK_ANSI_Minus + 1, "-" },
	{ kVK_ANSI_Equal + 1, "=" },
	{ kVK_ANSI_Backslash + 1, "\\" },
	{ kVK_Tab + 1, "Tab" },
	{ kVK_Shift + 1, "Shift" },
	{ kVK_Option + 1, "Option" },
	{ kVK_Control + 1, "Control" },
	{ kVK_Command + 1, "Command" },
	{ kVK_Space + 1, "Spacebar" },
	{ kVK_Delete + 1, "Delete" },
	{ kVK_Return + 1, "Return" },
	{ kVK_UpArrow + 1, "Up" },
	{ kVK_DownArrow + 1, "Down" },
	{ kVK_LeftArrow + 1, "Left" },
	{ kVK_RightArrow + 1, "Right" },
	{ kVK_Help + 1, "Help" },
	{ kVK_ForwardDelete + 1, "Forward Delete" },
	{ kVK_Home + 1, "Home" },
	{ kVK_End + 1, "End" },
	{ kVK_PageUp + 1, "Page Up" },
	{ kVK_PageDown + 1, "Page Down" },
	{ kVK_ANSI_KeypadClear + 1, "Keypad Clear" },
	{ kVK_ANSI_KeypadDivide + 1, "Keypad /" },
	{ kVK_ANSI_KeypadMultiply + 1, "Keypad *" },
	{ kVK_ANSI_KeypadMinus + 1, "Keypad -" },
	{ kVK_ANSI_KeypadPlus + 1, "Keypad +" },
	{ kVK_ANSI_KeypadEnter + 1, "Keypad Enter" },
	{ kVK_ANSI_Keypad0 + 1, "Keypad 0" },
	{ kVK_ANSI_Keypad1 + 1, "Keypad 1" },
	{ kVK_ANSI_Keypad2 + 1, "Keypad 2" },
	{ kVK_ANSI_Keypad3 + 1, "Keypad 3" },
	{ kVK_ANSI_Keypad4 + 1, "Keypad 4" },
	{ kVK_ANSI_Keypad5 + 1, "Keypad 5" },
	{ kVK_ANSI_Keypad6 + 1, "Keypad 6" },
	{ kVK_ANSI_Keypad7 + 1, "Keypad 7" },
	{ kVK_ANSI_Keypad8 + 1, "Keypad 8" },
	{ kVK_ANSI_Keypad9 + 1, "Keypad 9" },
	{ kVK_ANSI_KeypadDecimal + 1, "Keypad ." },
	{ kVK_F1 + 1, "F1" },
	{ kVK_F2 + 1, "F2" },
	{ kVK_F3 + 1, "F3" },
	{ kVK_F4 + 1, "F4" },
	{ kVK_F5 + 1, "F5" },
	{ kVK_F6 + 1, "F6" },
	{ kVK_F7 + 1, "F7" },
	{ kVK_F8 + 1, "F8" },
	{ kVK_F9 + 1, "F9" },
	{ kVK_F10 + 1, "F10" },
	{ kVK_F11 + 1, "F11" },
	{ kVK_F12 + 1, "F12" },
	{ kVK_F13 + 1, "F13" },
	{ kVK_F14 + 1, "F14" },
	{ kVK_F15 + 1, "F15" },
	{ 0x00, NULL }
};

static const char *XKeysymToString(uint16_t key) {
	static char buf[64];
	int i = 0;
	
	while (KeyString[i].code != 0) {
		if (KeyString[i].code == key) {
			strcpy(buf, KeyString[i].desc);
			return buf;
		}
		i++;
	}
	
	sprintf(buf, "0x%.2X", key);
	return buf;
}

static const char *hatname[16] = {"Centered", "Up", "Right", "Rightup",
	"Down", "", "Rightdown", "", "Left", "Leftup", "", "",
	"Leftdown", "", "", ""};

void GetKeyDescription(char *buf, int joynum, int key)
{
	switch (g.cfg.PadDef[joynum].KeyDef[key].JoyEvType) {
		case BUTTON:
			sprintf(buf, _("Joystick: Button %d"), g.cfg.PadDef[joynum].KeyDef[key].J.Button);
			break;
			
		case AXIS:
			sprintf(buf, _("Joystick: Axis %d%c"), abs(g.cfg.PadDef[joynum].KeyDef[key].J.Axis) - 1,
					g.cfg.PadDef[joynum].KeyDef[key].J.Axis > 0 ? '+' : '-');
			break;
			
		case HAT:
			sprintf(buf, _("Joystick: Hat %d %s"), (g.cfg.PadDef[joynum].KeyDef[key].J.Hat >> 8),
					hatname[g.cfg.PadDef[joynum].KeyDef[key].J.Hat & 0x0F]);
			break;
			
		case NONE:
		default:
			buf[0] = '\0';
			break;
	}
	
	if (g.cfg.PadDef[joynum].KeyDef[key].Key != 0) {
		if (buf[0] != '\0') {
			strcat(buf, " / ");
		}
		char keyboardBuf[64] = {0};
		
		snprintf(keyboardBuf, 63, _("Keyboard: %s"), XKeysymToString(g.cfg.PadDef[joynum].KeyDef[key].Key));
		strcat(buf, keyboardBuf);
	}
}

void GetAnalogDescription(char *buf, int joynum, int analognum, int dir)
{
	switch (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].JoyEvType) {
		case BUTTON:
			sprintf(buf, _("Joystick: Button %d"), g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Button);
			break;
			
		case AXIS:
			sprintf(buf, _("Joystick: Axis %d%c"), abs(g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Axis) - 1,
					g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Axis > 0 ? '+' : '-');
			break;
			
		case HAT:
			sprintf(buf, _("Joystick: Hat %d %s"), (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Hat >> 8),
					hatname[g.cfg.PadDef[joynum].AnalogDef[analognum][dir].J.Hat & 0x0F]);
			break;
			
		case NONE:
		default:
			buf[0] = '\0';
			break;
	}
	
	if (g.cfg.PadDef[joynum].AnalogDef[analognum][dir].Key != 0) {
		if (buf[0] != '\0') {
			strcat(buf, " / ");
		}
		char keyboardBuf[64] = {0};
		
		snprintf(keyboardBuf, 63, _("Keyboard: %s"), XKeysymToString(g.cfg.PadDef[joynum].AnalogDef[analognum][dir].Key));
		strcat(buf, keyboardBuf);
	}
}

int CheckKeyDown() {
	KeyMap theKeys;
	unsigned char *keybytes;
	int i;
	
	GetKeys(theKeys);
	keybytes = (unsigned char *) theKeys;
	
	for (i = 0; i < 128; i++) {
		if (i == kVK_CapsLock) continue; // Ignore capslock
		
		if (keybytes[i >> 3] & (1 << (i & 7)))
			return i + 1;
	}
	
	return 0;
}

static Sint16 InitialAxisPos[256], PrevAxisPos[256];

#define NUM_AXES(js) (SDL_JoystickNumAxes(js) > 256 ? 256 : SDL_JoystickNumAxes(js))

void InitAxisPos(int padnum)
{
	int i;
	SDL_Joystick *js;
	
	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);
	} else return;
	
	SDL_JoystickUpdate();
	
	for (i = 0; i < NUM_AXES(js); i++) {
		InitialAxisPos[i] = PrevAxisPos[i] = SDL_JoystickGetAxis(js, i);
	}
	
	SDL_JoystickClose(js);
}

int ReadDKeyEvent(int padnum, int key)
{
	SDL_Joystick *js;
	int i, changed = 0, t;
	Sint16 axis;
	
	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);
	} else {
		js = NULL;
	}
	
	for (t = 0; t < 1000000 / 1000; t++) {
		// check joystick events
		if (js != NULL) {
			SDL_JoystickUpdate();
			
			for (i = 0; i < SDL_JoystickNumButtons(js); i++) {
				if (SDL_JoystickGetButton(js, i)) {
					g.cfg.PadDef[padnum].KeyDef[key].JoyEvType = BUTTON;
					g.cfg.PadDef[padnum].KeyDef[key].J.Button = i;
					changed = 1;
					goto end;
				}
			}
			
			for (i = 0; i < NUM_AXES(js); i++) {
				axis = SDL_JoystickGetAxis(js, i);
				if (abs(axis) > 16383 && (abs(axis - PrevAxisPos[i]) > 4096 || abs(axis - InitialAxisPos[i]) > 4096)) {
					g.cfg.PadDef[padnum].KeyDef[key].JoyEvType = AXIS;
					g.cfg.PadDef[padnum].KeyDef[key].J.Axis = (i + 1) * (axis > 0 ? 1 : -1);
					changed = 1;
					goto end;
				}
				PrevAxisPos[i] = axis;
			}
			
			for (i = 0; i < SDL_JoystickNumHats(js); i++) {
				axis = SDL_JoystickGetHat(js, i);
				if (axis != SDL_HAT_CENTERED) {
					g.cfg.PadDef[padnum].KeyDef[key].JoyEvType = HAT;
					
					if (axis & SDL_HAT_UP) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_UP);
					} else if (axis & SDL_HAT_DOWN) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_DOWN);
					} else if (axis & SDL_HAT_LEFT) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_LEFT);
					} else if (axis & SDL_HAT_RIGHT) {
						g.cfg.PadDef[padnum].KeyDef[key].J.Hat = ((i << 8) | SDL_HAT_RIGHT);
					}
					
					changed = 1;
					goto end;
				}
			}
		}
		
		// check keyboard events
		i = CheckKeyDown();
		if (i != 0) {
			if (i != (kVK_Escape + 1)) g.cfg.PadDef[padnum].KeyDef[key].Key = i;
			changed = 1;
			goto end;
		}
		
		// check mouse events
		if (GetCurrentButtonState()) {
			changed = 2;
			goto end;
		}
		
		usleep(1000);
	}
	
end:
	if (js != NULL) {
		SDL_JoystickClose(js);
	}
	
	return changed;
}

int ReadAnalogEvent(int padnum, int analognum, int analogdir)
{
	SDL_Joystick *js;
	int i, changed = 0, t;
	Sint16 axis;
	
	if (g.cfg.PadDef[padnum].DevNum >= 0) {
		js = SDL_JoystickOpen(g.cfg.PadDef[padnum].DevNum);
		SDL_JoystickEventState(SDL_IGNORE);
	} else {
		js = NULL;
	}
	
	for (t = 0; t < 1000000 / 1000; t++) {
		// check joystick events
		if (js != NULL) {
			SDL_JoystickUpdate();
			
			for (i = 0; i < SDL_JoystickNumButtons(js); i++) {
				if (SDL_JoystickGetButton(js, i)) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = BUTTON;
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Button = i;
					changed = 1;
					goto end;
				}
			}
			
			for (i = 0; i < NUM_AXES(js); i++) {
				axis = SDL_JoystickGetAxis(js, i);
				if (abs(axis) > 16383 && (abs(axis - PrevAxisPos[i]) > 4096 || abs(axis - InitialAxisPos[i]) > 4096)) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = AXIS;
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Axis = (i + 1) * (axis > 0 ? 1 : -1);
					changed = 1;
					goto end;
				}
				PrevAxisPos[i] = axis;
			}
			
			for (i = 0; i < SDL_JoystickNumHats(js); i++) {
				axis = SDL_JoystickGetHat(js, i);
				if (axis != SDL_HAT_CENTERED) {
					g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].JoyEvType = HAT;
					
					if (axis & SDL_HAT_UP) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_UP);
					} else if (axis & SDL_HAT_DOWN) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_DOWN);
					} else if (axis & SDL_HAT_LEFT) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_LEFT);
					} else if (axis & SDL_HAT_RIGHT) {
						g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].J.Hat = ((i << 8) | SDL_HAT_RIGHT);
					}
					
					changed = 1;
					goto end;
				}
			}
		}
		
		// check keyboard events
		i = CheckKeyDown();
		if (i != 0) {
			if (i != (kVK_Escape + 1)) g.cfg.PadDef[padnum].AnalogDef[analognum][analogdir].Key = i;
			changed = 1;
			goto end;
		}
		
		// check mouse events
		if (GetCurrentButtonState()) {
			changed = 2;
			goto end;
		}
		
		usleep(1000);
	}
	
end:
	if (js != NULL) {
		SDL_JoystickClose(js);
	}
	
	return changed;
}
