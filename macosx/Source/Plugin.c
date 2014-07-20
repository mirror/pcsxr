/*  Pcsxr - Pc Psx Emulator
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <CoreFoundation/CoreFoundation.h>
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
	//TODO: get these set up properly!
	GPU_keypressed(key);
#ifdef ENABLE_SIO1API
	SIO1_keypressed(key);
#endif
	if (Config.UseNet)
		NET_keypressed(key);
}

long PAD1__open(void) {
	return PAD1_open(&gpuDisp);
}

long PAD2__open(void) {
	return PAD2_open(&gpuDisp);
}

void SignalExit(int sig) {
	ClosePlugins();
	OnFile_Exit();
}

#define PARSEPATH(dst, src) \
	ptr = src + strlen(src); \
	while (*ptr != '\\' && ptr != src) ptr--; \
	if (ptr != src) { \
		strcpy(dst, ptr+1); \
	}

int _OpenPlugins() {
	long ret;

	GPU_clearDynarec(clearDynarec);

	ret = CDR_open();
	if (ret < 0) { SysMessage("%s", _("Error Opening CDR Plugin")); return -1; }
	ret = SPU_open();
	if (ret < 0) { SysMessage("%s", _("Error Opening SPU Plugin")); return -1; }
	SPU_registerCallback(SPUirq);
	ret = GPU_open(&gpuDisp, "PCSXR", NULL);
	if (ret < 0) { SysMessage("%s", _("Error Opening GPU Plugin")); return -1; }
	ret = PAD1_open(&gpuDisp);
	if (ret < 0) { SysMessage("%s", _("Error Opening PAD1 Plugin")); return -1; }
    PAD1_registerVibration(GPU_visualVibration);
    PAD1_registerCursor(GPU_cursor);
	ret = PAD2_open(&gpuDisp);
	if (ret < 0) { SysMessage("%s", _("Error Opening PAD2 Plugin")); return -1; }
    PAD2_registerVibration(GPU_visualVibration);
    PAD2_registerCursor(GPU_cursor);
#ifdef ENABLE_SIO1API
	ret = SIO1_open(&gpuDisp);
	if (ret < 0) { SysMessage("%s", _("Error opening SIO1 plugin!")); return -1; }
	SIO1_registerCallback(SIO1irq);
#endif

	if (Config.UseNet && !NetOpened) {
		netInfo info;
		char path[MAXPATHLEN];

		strcpy(info.EmuName, "PCSXR " PACKAGE_VERSION);
		strncpy(info.CdromID, CdromId, 9);
		strncpy(info.CdromLabel, CdromLabel, 11);
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
				Config.UseNet = FALSE;
			}
		} else {
			if (NET_queryPlayer() == 1) {
				if (SendPcsxInfo() == -1) Config.UseNet = FALSE;
			} else {
				if (RecvPcsxInfo() == -1) Config.UseNet = FALSE;
			}
		}
		NetOpened = TRUE;
	} else if (Config.UseNet) {
		NET_resume();
	}

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
	long ret;

	//signal(SIGINT, SIG_DFL);
	//signal(SIGPIPE, SIG_DFL);
	ret = CDR_close();
	if (ret < 0) { SysMessage("%s", _("Error Closing CDR Plugin")); return; }
	ret = SPU_close();
	if (ret < 0) { SysMessage("%s", _("Error Closing SPU Plugin")); return; }
	ret = PAD1_close();
	if (ret < 0) { SysMessage("%s", _("Error Closing PAD1 Plugin")); return; }
	ret = PAD2_close();
	if (ret < 0) { SysMessage("%s", _("Error Closing PAD2 Plugin")); return; }
	ret = GPU_close();
	if (ret < 0) { SysMessage("%s", _("Error Closing GPU Plugin")); return; }
#ifdef ENABLE_SIO1API
	ret = SIO1_close();
	if (ret < 0) { SysMessage("%s", _("Error closing SIO1 plugin!")); return; }
#endif

	if (Config.UseNet) {
		NET_pause();
	}
}

void ResetPlugins() {
	long ret;

	CDR_shutdown();
	GPU_shutdown();
	SPU_shutdown();
	PAD1_shutdown();
	PAD2_shutdown();
#ifdef ENABLE_SIO1API
	SIO1_shutdown();
#endif
	if (Config.UseNet) NET_shutdown();

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
#ifdef ENABLE_SIO1API
	ret = SIO1_init();
	if (ret < 0) { SysMessage(_("SIO1init error: %d!"), ret); return; }
#endif

	if (Config.UseNet) {
		ret = NET_init();
		if (ret < 0) { SysMessage(_("NETinit error: %d"), ret); return; }
	}

	NetOpened = FALSE;
}
