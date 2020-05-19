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

#include "stdafx.h"
#include "typedefs.h"
#include "cfg-winapi.h"
#include "sio1.h"

/***************************************************************************/

void settingsRead() {
	HKEY myKey;
	DWORD temp;
	DWORD type;
	DWORD size;

	settings.player = PLAYER_DISABLED;
	strcpy(settings.ip, "127.0.0.1");
	settings.port = 33307;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\SIO1\\bladesio1",0,KEY_ALL_ACCESS,&myKey) == ERROR_SUCCESS) {
	   size = 4;
	   if(RegQueryValueEx(myKey, "player", 0, &type, (LPBYTE)&temp, &size) == ERROR_SUCCESS)
		   settings.player = (int)temp;
	   size = sizeof(settings.ip);
	   RegQueryValueEx(myKey, "ip", 0, &type, (LPBYTE)settings.ip, &size);
	   size = 4;
	   if(RegQueryValueEx(myKey, "port", 0, &type, (LPBYTE)&temp, &size) == ERROR_SUCCESS)
		   settings.port = (int)temp;

	   RegCloseKey(myKey);
	}
}

void settingsWrite() {
	HKEY myKey;
	DWORD myDisp;
	DWORD temp;

	RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Vision Thing\\PSEmu Pro\\SIO1\\bladesio1", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &myKey, &myDisp);
	temp = settings.player;
	RegSetValueEx(myKey, "player", 0, REG_DWORD, (LPBYTE)&temp, sizeof(temp));
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\SIO1\\bladesio1", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &myKey, &myDisp);
	RegSetValueEx(myKey, "ip", 0, REG_SZ, (BYTE*)settings.ip, strlen(settings.ip));
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\SIO1\\bladesio1", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &myKey, &myDisp);
	temp = settings.port;
	RegSetValueEx(myKey, "port", 0, REG_DWORD,(LPBYTE) &temp,sizeof(temp));

	RegCloseKey(myKey);
}

/***************************************************************************/

BOOL OnInitSio1Dialog(HWND hW) {
	char str[32];

	settingsRead();

	CheckRadioButton(hW, IDC_DISABLED, IDC_CLIENT, IDC_DISABLED + settings.player);
	SetDlgItemText(hW, IDC_IP, settings.ip);
	sprintf(str, "%i", settings.port);
	SetDlgItemText(hW, IDC_PORT, str);

	return TRUE;
}

void OnSio1OK(HWND hW) {
	char str[32];

	if(IsDlgButtonChecked(hW,IDC_DISABLED))
		settings.player = 0;
	if(IsDlgButtonChecked(hW,IDC_SERVER))
		settings.player = 1;
	if(IsDlgButtonChecked(hW,IDC_CLIENT))
		settings.player = 2;
	GetDlgItemText(hW,IDC_IP, settings.ip, sizeof(settings.ip));
	GetDlgItemText(hW,IDC_PORT, str, sizeof(str));
	settings.port = atoi(str);

	settingsWrite();

	EndDialog(hW,TRUE);
}

void OnSio1Cancel(HWND hW) {
	EndDialog(hW,FALSE);
}

BOOL CALLBACK Sio1DlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return OnInitSio1Dialog(hW);
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDCANCEL: 
					OnSio1Cancel(hW);
					return TRUE;
				case IDOK:
					OnSio1OK(hW);
					return TRUE;
			}
		}
	}

	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW,TRUE);
					return TRUE;
			}
		}
	}

	return FALSE;
}

/***************************************************************************/
