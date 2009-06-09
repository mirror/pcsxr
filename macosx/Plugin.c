/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#import <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "psxcommon.h"
#include "plugins.h"
#include "spu.h"

void OnFile_Exit();

unsigned long gpuDisp;

long SPU__open(void) {
	return SPU_open();
}

int StatesC = 0;
extern int UseGui;
int ShowPic=0;

void gpuShowPic() {
}

void PADhandleKey(int key) {
/*	char Text[255];
	int ret;

	switch (key) {
		case 0: break;
		case XK_F1:
			sprintf(Text, "sstates/%10.10s.%3.3d", CdromLabel, StatesC);
			GPU_freeze(2, (GPUFreeze_t *)&StatesC);
			ret = SaveState(Text);
			if (ret == 0)
				 sprintf(Text, _("*PCSX*: Saved State %d"), StatesC+1);
			else sprintf(Text, _("*PCSX*: Error Saving State %d"), StatesC+1);
			GPU_displayText(Text);
			if (ShowPic) { ShowPic = 0; gpuShowPic(); }
			break;
		case XK_F2:
			if (StatesC < 4) StatesC++;
			else StatesC = 0;
			GPU_freeze(2, (GPUFreeze_t *)&StatesC);
			if (ShowPic) { ShowPic = 0; gpuShowPic(); }
			break;
		case XK_F3:			
			sprintf (Text, "sstates/%10.10s.%3.3d", CdromLabel, StatesC);
			ret = LoadState(Text);
			if (ret == 0)
				 sprintf(Text, _("*PCSX*: Loaded State %d"), StatesC+1);
			else sprintf(Text, _("*PCSX*: Error Loading State %d"), StatesC+1);
			GPU_displayText(Text);
			break;
		case XK_F4:
			gpuShowPic();
			break;
		case XK_F5:
			Config.Sio ^= 0x1;
			if (Config.Sio)
				 sprintf(Text, _("*PCSX*: Sio Irq Always Enabled"));
			else sprintf(Text, _("*PCSX*: Sio Irq Not Always Enabled"));
			GPU_displayText(Text);
			break;
		case XK_F6:
			Config.Mdec ^= 0x1;
			if (Config.Mdec)
				 sprintf(Text, _("*PCSX*: Black&White Mdecs Only Enabled"));
			else sprintf(Text, _("*PCSX*: Black&White Mdecs Only Disabled"));
			GPU_displayText(Text);
			break;
		case XK_F7:
			Config.Xa ^= 0x1;
			if (Config.Xa == 0)
				 sprintf (Text, _("*PCSX*: Xa Enabled"));
			else sprintf (Text, _("*PCSX*: Xa Disabled"));
			GPU_displayText(Text);
			break;
		case XK_F8:
			GPU_makeSnapshot();
			break;
		case XK_F9:
			cdOpenCase = 1;
			break;
		case XK_F10:
			cdOpenCase = 0;
			break;
		case XK_Escape:
			ClosePlugins();
			UpdateMenuSlots();
			if (!UseGui) OnFile_Exit();
			RunGui();
			break;
		default:
			GPU_keypressed(key);
			if (Config.UseNet) NET_keypressed(key);
	}*/
}

long PAD1__open(void) {
	return PAD1_open(&gpuDisp);
}

long PAD2__open(void) {
	return PAD2_open(&gpuDisp);
}

void OnFile_Exit();

void SignalExit(int sig) {
	ClosePlugins();
	OnFile_Exit();
}

void SPUirq(void);

int NetOpened = 0;

#define PARSEPATH(dst, src) \
	ptr = src + strlen(src); \
	while (*ptr != '\\' && ptr != src) ptr--; \
	if (ptr != src) { \
		strcpy(dst, ptr+1); \
	}

int _OpenPlugins() {
	static char path[1024];
	CFURLRef pathUrl;
	int ret;

	//signal(SIGINT, SignalExit);
	//signal(SIGPIPE, SignalExit);

	GPU_clearDynarec(clearDynarec);
	
	pathUrl = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("gpuPeopsSoftX.cfg"), NULL, NULL);
	if (pathUrl)
		CFURLGetFileSystemRepresentation(pathUrl, true, path, 1024);
	
	ret = CDR_open();
	if (ret < 0) { SysMessage(_("Error Opening CDR Plugin")); return -1; }
	ret = SPU_open();
	if (ret < 0) { SysMessage(_("Error Opening SPU Plugin")); return -1; }
	SPU_registerCallback(SPUirq);
	ret = GPU_open(&gpuDisp, "PCSX", /*pathUrl ? path :*/ NULL);
	if (ret < 0) { SysMessage(_("Error Opening GPU Plugin")); return -1; }
	ret = PAD1_open(&gpuDisp);
	if (ret < 0) { SysMessage(_("Error Opening PAD1 Plugin")); return -1; }
	ret = PAD2_open(&gpuDisp);
	if (ret < 0) { SysMessage(_("Error Opening PAD2 Plugin")); return -1; }

/*	if (Config.UseNet && NetOpened == 0) {
		netInfo info;
		char path[256];

		strcpy(info.EmuName, "PCSX v" PCSX_VERSION);
		strncpy(info.CdromID, CdromId, 9);
		strncpy(info.CdromLabel, CdromLabel, 9);
		info.psxMem = psxM;
		info.GPU_showScreenPic = GPU_showScreenPic;
		info.GPU_displayText = GPU_displayText;
		info.GPU_showScreenPic = GPU_showScreenPic;
		info.PAD_setSensitive = PAD1_setSensitive;
		sprintf(path, "%s%s", Config.BiosDir, Config.Bios);
		strcpy(info.BIOSpath, path);
		strcpy(info.MCD1path, Config.Mcd1);
		strcpy(info.MCD2path, Config.Mcd2);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Gpu);
		strcpy(info.GPUpath, path);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Spu);
		strcpy(info.SPUpath, path);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Cdr);
		strcpy(info.CDRpath, path);
		NET_setInfo(&info);

		ret = NET_open(&gpuDisp);
		if (ret < 0) {
			if (ret == -2) {
				// -2 is returned when something in the info
				// changed and needs to be synced
				char *ptr;

				PARSEPATH(Config.Bios, info.BIOSpath);
				PARSEPATH(Config.Gpu,  info.GPUpath);
				PARSEPATH(Config.Spu,  info.SPUpath);
				PARSEPATH(Config.Cdr,  info.CDRpath);

				strcpy(Config.Mcd1, info.MCD1path);
				strcpy(Config.Mcd2, info.MCD2path);
				return -2;
			} else {
				Config.UseNet = 0;
			}
		} else {
			if (NET_queryPlayer() == 1) {
				if (SendPcsxInfo() == -1) Config.UseNet = 0;
			} else {
				if (RecvPcsxInfo() == -1) Config.UseNet = 0;
			}
		}
		NetOpened = 1;
	} else if (Config.UseNet) {
		NET_resume();
	}
*/
	return 0;
}

int OpenPlugins() {
	int ret;

	while ((ret = _OpenPlugins()) == -2) {
		ReleasePlugins();
		LoadMcds(Config.Mcd1, Config.Mcd2);
		if (LoadPlugins() == -1) return -1;
	}
	return ret;	
}

void ClosePlugins() {
	int ret;

	//signal(SIGINT, SIG_DFL);
	//signal(SIGPIPE, SIG_DFL);
	ret = CDR_close();
	if (ret < 0) { SysMessage(_("Error Closing CDR Plugin")); return; }
	ret = SPU_close();
	if (ret < 0) { SysMessage(_("Error Closing SPU Plugin")); return; }
	ret = PAD1_close();
	if (ret < 0) { SysMessage(_("Error Closing PAD1 Plugin")); return; }
	ret = PAD2_close();
	if (ret < 0) { SysMessage(_("Error Closing PAD2 Plugin")); return; }
	ret = GPU_close();
	if (ret < 0) { SysMessage(_("Error Closing GPU Plugin")); return; }

	/*if (Config.UseNet) {
		NET_pause();
	}*/
}

void ResetPlugins() {
	int ret;

	CDR_shutdown();
	GPU_shutdown();
	SPU_shutdown();
	PAD1_shutdown();
	PAD2_shutdown();
	//if (Config.UseNet) NET_shutdown(); 

	ret = CDR_init();
	if (ret < 0) { SysMessage(_("CDRinit error: %d"), ret); return; }
	ret = GPU_init();
	if (ret < 0) { SysMessage(_("GPUinit error: %d"), ret); return; }
	ret = SPU_init();
	if (ret < 0) { SysMessage(_("SPUinit error: %d"), ret); return; }
	ret = PAD1_init(1);
	if (ret < 0) { SysMessage(_("PAD1init error: %d"), ret); return; }
	ret = PAD2_init(2);
	if (ret < 0) { SysMessage(_("PAD2init error: %d"), ret); return; }
	/*if (Config.UseNet) {
		ret = NET_init();
		if (ret < 0) { SysMessage(_("NETinit error: %d"), ret); return; }
	}*/

	NetOpened = 0;
}

