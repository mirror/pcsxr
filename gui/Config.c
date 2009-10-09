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
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Linux.h"

/* TODO escaping/unescaping would be nice, as would maxchars */
void GetValue(char *src, char *name, char *outvar) {
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

void GetValuel(char *src, char *name, long *var) {
	char *tmp = strstr(src, name); 
	if (tmp != NULL) { 
		tmp+=strlen(name); 
		while ((*tmp == ' ') || (*tmp == '=')) tmp++; 
		if (*tmp != '\n') sscanf(tmp, "%lx", var); 
	}
	return;
}

#define SetValue(name, var) \
	fprintf (f,"%s = %s\n", name, var);

#define SetValuel(name, var) \
	fprintf (f,"%s = %lx\n", name, var);

/* TODO Use glib functions for filename handling, use constants, glib-key-value-file-parser */
int LoadConfig(PcsxConfig *Conf) {
	struct stat buf;
	FILE *f;
	int size;
	char *data;

	/* TODO local var called cfgfile */

	// Ryan says: use dotdir, dotdir is GOOD
	// No giant homedir names
	strncpy(cfgfile, getenv("HOME"), 200);
	strcat(cfgfile, PCSX_DOT_DIR);

	// proceed to load the cfg file
	// append its name
	strcat(cfgfile, cfgfile_basename);

	// file is  now ~/.pcsx/pcsx.cfg (or whatever cfgfile_basename is)
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

	data = (char*)malloc(size + 1);
	if (data == NULL) return -1;

	fread(data, 1, buf.st_size, f);
	fclose(f);

	data[size] = '\0';

	GetValue(data, "Bios", Config.Bios);
	GetValue(data, "Gpu",  Config.Gpu);
	GetValue(data, "Spu",  Config.Spu);
	GetValue(data, "Cdr",  Config.Cdr);
	GetValue(data, "Pad1", Config.Pad1);
	GetValue(data, "Pad2", Config.Pad2);
	GetValue(data, "Net",  Config.Net);
	GetValue(data, "Mcd1", Config.Mcd1);
	GetValue(data, "Mcd2", Config.Mcd2);
	GetValue(data, "BiosDir",    Config.BiosDir);
	GetValue(data, "PluginsDir",    Config.PluginsDir);
	GetValuel(data, "Xa",      &Config.Xa);
	GetValuel(data, "Sio",     &Config.Sio);
	GetValuel(data, "Mdec",    &Config.Mdec);
	GetValuel(data, "PsxAuto", &Config.PsxAuto);
	GetValuel(data, "PsxType", &Config.PsxType);
	GetValuel(data, "Cdda",    &Config.Cdda);
	GetValuel(data, "Cpu",     &Config.Cpu);
	GetValuel(data, "Dbg",     &Config.Debug);
	GetValuel(data, "PsxOut",  &Config.PsxOut);
	GetValuel(data, "SpuIrq",  &Config.SpuIrq);
	GetValuel(data, "RCntFix", &Config.RCntFix);
	GetValuel(data, "VSyncWA", &Config.VSyncWA);

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
	SetValue("Net",  Config.Net);
	SetValue("Pad1", Config.Pad1);
	SetValue("Pad2", Config.Pad2);
	SetValue("Mcd1", Config.Mcd1);
	SetValue("Mcd2", Config.Mcd2);
	SetValue("BiosDir",    Config.BiosDir);
	SetValue("PluginsDir",    Config.PluginsDir);
	SetValuel("Xa",      Config.Xa);
	SetValuel("Sio",     Config.Sio);
	SetValuel("Mdec",    Config.Mdec);
	SetValuel("PsxAuto", Config.PsxAuto);
	SetValuel("PsxType", Config.PsxType);
	SetValuel("Cdda",    Config.Cdda);
	SetValuel("Cpu",     Config.Cpu);
	SetValuel("Dbg",     Config.Debug);
	SetValuel("PsxOut",  Config.PsxOut);
	SetValuel("SpuIrq",  Config.SpuIrq);
	SetValuel("RCntFix", Config.RCntFix);
	SetValuel("VSyncWA", Config.VSyncWA);

	fclose(f);
}
