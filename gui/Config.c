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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Linux.h"

/* TODO escaping/unescaping would be nice, as would maxchars */
static void GetValue(char *src, char *name, char *outvar) {
	char *tmp;

	*outvar = 0;
	tmp = strstr(src, name);
	if (tmp == NULL) return;

	tmp += strlen(name); 
	while ((*tmp == ' ') || (*tmp == '=')) tmp++;

	while (*tmp != '\n' && *tmp != 0)
		*outvar++ = *tmp++;

	*outvar = 0;
	return;
}

static long GetValuel(char *src, char *name) {
	char *tmp = strstr(src, name);
	if (tmp != NULL) {
		tmp += strlen(name);
		while ((*tmp == ' ') || (*tmp == '=')) tmp++;
		if (*tmp != '\n') return atol(tmp);
	}
	return 0;
}

static boolean GetValueb(char *src, char *name) {
	char *tmp = strstr(src, name);
	if (tmp != NULL) {
		tmp += strlen(name);
		while ((*tmp == ' ') || (*tmp == '=')) tmp++;
		if (*tmp != '\n') return (atoi(tmp) != 0);
	}
	return FALSE;
}

static long GetValuef(char *src, char *name) {
    char *tmp = strstr(src, name);
    if (tmp != NULL) {
        tmp += strlen(name);
        while ((*tmp == ' ') || (*tmp == '=')) tmp++;
        if (*tmp != '\n') return atof(tmp);
    }
    return 0.0f;
}

#define SetValue(name, var) \
	fprintf(f, "%s = %s\n", name, var);

#define SetValuel(name, var) \
	fprintf(f, "%s = %li\n", name, var);

#define SetValueb(name, var) \
	fprintf(f, "%s = %d\n", name, (var) ? 1 : 0);

#define SetValuef(name, var) \
    fprintf(f, "%s = %f\n", name, var);

int LoadConfig(PcsxConfig *Conf) {
	struct stat buf;
	FILE *f;
	int size;
	char *data;

	/* TODO local var called cfgfile */

	// Ryan says: use dotdir, dotdir is GOOD
	// No giant homedir names
	strncpy(cfgfile, getenv("HOME"), 200);
	strcat(cfgfile, PCSXR_DOT_DIR);

	// proceed to load the cfg file
	// append its name
	strcat(cfgfile, cfgfile_basename);

	// file is  now ~/.pcsxr/pcsxr.cfg (or whatever cfgfile_basename is)
	if (stat(cfgfile, &buf) == -1) {
		// the config file doesn't exist!
		/* TODO Error checking? */
		printf("Configuration file %s couldn't be found\n", cfgfile);
		return -1;
	}

	size = buf.st_size;

	/* TODO Error checking for the next two lines, and at least log failures */
	f = fopen(cfgfile, "r");
	if (f == NULL) return -1;

	data = (char *)malloc(size + 1);
	if (data == NULL) {
		fclose(f);
		return -1;
	}

	fread(data, 1, buf.st_size, f);
	fclose(f);

	data[size] = '\0';

	GetValue(data, "Bios", Config.Bios);
	GetValue(data, "Gpu",  Config.Gpu);
	GetValue(data, "Spu",  Config.Spu);
	GetValue(data, "Cdr",  Config.Cdr);
#ifdef ENABLE_SIO1API
	GetValue(data, "Sio1", Config.Sio1);
#endif
	GetValue(data, "Pad1", Config.Pad1);
	GetValue(data, "Pad2", Config.Pad2);
	GetValue(data, "Net",  Config.Net);
	GetValue(data, "Mcd1", Config.Mcd1);
	GetValue(data, "Mcd2", Config.Mcd2);
	GetValue(data, "BiosDir",    Config.BiosDir);
	GetValue(data, "PluginsDir",    Config.PluginsDir);
	GetValue(data, "IsoImgDir",  Config.IsoImgDir);

	Config.Xa      = GetValueb(data, "Xa");
	Config.SioIrq  = GetValueb(data, "SioIrq");
	Config.Mdec    = GetValueb(data, "Mdec");
	Config.PsxAuto = GetValueb(data, "PsxAuto");
	Config.Cdda    = GetValuel(data, "Cdda");
	Config.SlowBoot= GetValueb(data, "SlowBoot");
	Config.Debug   = GetValueb(data, "Dbg");
	Config.PsxOut  = (Config.PsxOut || GetValueb(data, "PsxOut"));
	Config.SpuIrq  = GetValueb(data, "SpuIrq");
	Config.RCntFix = GetValueb(data, "RCntFix");
	Config.VSyncWA = GetValueb(data, "VSyncWA");
	Config.NoMemcard = GetValueb(data, "NoMemcard");
	Config.Widescreen = GetValueb(data, "Widescreen");
    Config.PerGameMcd = GetValueb(data, "PerGameMcd");
    Config.MemHack = GetValuel(data, "MemHack");
    Config.OverClock = GetValueb(data, "OverClock");

	Config.Cpu     = GetValuel(data, "Cpu");
	Config.PsxType = GetValuel(data, "PsxType");
    Config.PsxClock = GetValuef(data, "PsxClock");

    Config.PGXP_GTE = GetValueb(data, "PGXP_GTE");
    Config.PGXP_Cache = GetValueb(data, "PGXP_Cache");
    Config.PGXP_Texture = GetValueb(data, "PGXP_Texture");
    Config.PGXP_Mode = GetValuel(data, "PGXP_Mode");

    Config.RewindCount = GetValuel(data, "RewindCount");
	Config.RewindInterval = GetValuel(data, "RewindInterval");

	Config.AltSpeed1 = GetValuel(data, "AltSpeed1");
	if (!Config.AltSpeed1)
		Config.AltSpeed1 = 50;
	Config.AltSpeed2 = GetValuel(data, "AltSpeed2");
	if (!Config.AltSpeed2)
		Config.AltSpeed2 = 250;

	Config.HackFix = GetValuel(data, "HackFix");

	free(data);

	return 0;
}

void SaveConfig() {
	FILE *f;

	/* TODO Error checking for the next two lines, and at least log
	   failures - suggest a file dialog to specify a new file or
	   create a new file */
	f = fopen(cfgfile, "w");
	if (f == NULL) return;

	SetValue("Bios", Config.Bios);
	SetValue("Gpu",  Config.Gpu);
	SetValue("Spu",  Config.Spu);
	SetValue("Cdr",  Config.Cdr);
#ifdef ENABLE_SIO1API
	SetValue("Sio1", Config.Sio1);
#endif
	SetValue("Net",  Config.Net);
	SetValue("Pad1", Config.Pad1);
	SetValue("Pad2", Config.Pad2);
	SetValue("Mcd1", Config.Mcd1);
	SetValue("Mcd2", Config.Mcd2);
	SetValue("BiosDir",    Config.BiosDir);
	SetValue("PluginsDir",    Config.PluginsDir);
	SetValue("IsoImgDir",  Config.IsoImgDir);

	SetValueb("Xa",      Config.Xa);
	SetValueb("SioIrq",  Config.SioIrq);
	SetValueb("Mdec",    Config.Mdec);
	SetValueb("PsxAuto", Config.PsxAuto);
	SetValuel("Cdda",    (long)Config.Cdda);
	SetValueb("SlowBoot",Config.SlowBoot);
	SetValueb("Dbg",     Config.Debug);
	SetValueb("PsxOut",  Config.PsxOut);
	SetValueb("SpuIrq",  Config.SpuIrq);
	SetValueb("RCntFix", Config.RCntFix);
	SetValueb("VSyncWA", Config.VSyncWA);
	SetValueb("NoMemcard", Config.NoMemcard);
	SetValueb("Widescreen", Config.Widescreen);
    SetValueb("PerGameMcd", Config.PerGameMcd);
    SetValuel("MemHack", (long)Config.MemHack);
    SetValueb("OverClock", Config.OverClock);

	SetValuel("Cpu",     (long)Config.Cpu);
    SetValuel("PsxType", (long)Config.PsxType);
    SetValuef("PsxClock", Config.PsxClock);

    SetValueb("PGXP_GTE", Config.PGXP_GTE);
    SetValueb("PGXP_Cache", Config.PGXP_Cache);
    SetValueb("PGXP_Texture", Config.PGXP_Texture);
    SetValuel("PGXP_Mode", (long)Config.PGXP_Mode);

    SetValuel("RewindCount", (long)Config.RewindCount);
    SetValuel("RewindInterval", (long)Config.RewindInterval);

    SetValuel("AltSpeed1", (long)Config.AltSpeed1);
    SetValuel("AltSpeed2", (long)Config.AltSpeed2);

    SetValuel("HackFix", (long)Config.HackFix);

	fclose(f);
}
