/*
 * Copyright (C) 2010 Benoit Gschwind
 * Inspired by original author : Pete Bernert
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "cfg.h"

#define DEFAULT_CFG_NAME "gxvideo.cfg"

static int get_int_value (char const * file_buffer, char const * name, int dflt) {
	char * p = strstr(file_buffer, name);
	if (p != NULL) {
		p += strlen(name);
		while ((*p == ' ') || (*p == '='))
			++p;
		if (*p != '\n')
			return atoi(p);
	}
	return dflt;
}

static double get_double_value(char const * file_buffer, char const * name,
		double dflt) {
	char * p = strstr(file_buffer, name);
	if (p != NULL) {
		p += strlen(name);
		while ((*p == ' ') || (*p == '='))
			++p;
		if (*p != '\n')
			return atof(p);
	}
	return dflt;
}

static void write_int_value (FILE * f, char const * name, int val) {
	fprintf(f, "%s = %d\n", name, val);
}

static void write_double_value (FILE * f, char const * name, double val) {
	fprintf(f, "%s = %.1f\n", name, val);
}

static void ReadConfigFile() {
	struct stat buf;
	FILE * f_in;
	char cfg_file_name[256];
	int len, size;
	char * file_buffer;

	if (g_file_name)
		strcpy(cfg_file_name, g_file_name);
	else {
		strcpy(cfg_file_name, DEFAULT_CFG_NAME);
		f_in = fopen(cfg_file_name, "rb");
		if (!f_in) {
			strcpy(cfg_file_name, "cfg/" DEFAULT_CFG_NAME);
			f_in = fopen(cfg_file_name, "rb");
			if (!f_in)
				snprintf(cfg_file_name, 255, "%s/.pcsx/plugins/" DEFAULT_CFG_NAME, getenv("HOME"));
			else
				fclose(f_in);
		} else
			fclose(f_in);
	}

	if (stat(cfg_file_name, &buf) == -1)
		return;
	size = buf.st_size;

	f_in = fopen(cfg_file_name, "rb");
	if (!f_in)
		return;

	file_buffer = (char *) malloc(size + 1);
	len = fread(file_buffer, 1, size, f_in);
	/* ensure end_of_string */
	file_buffer[len] = 0;
	fclose(f_in);

	g_cfg.ResX = get_int_value(file_buffer, "ResX", g_cfg.ResX);
	if (g_cfg.ResX < 20)
		g_cfg.ResX = 20;
	g_cfg.ResX = (g_cfg.ResX / 4) * 4;
	g_cfg.ResY = get_int_value(file_buffer, "ResY", g_cfg.ResY);
	if (g_cfg.ResY < 20)
		g_cfg.ResY = 20;
	g_cfg.ResY = (g_cfg.ResY / 4) * 4;
	g_cfg.Dithering = get_int_value(file_buffer, "Dithering", g_cfg.Dithering);
	g_cfg.FullScreen = get_int_value(file_buffer, "FullScreen", g_cfg.FullScreen);
	g_cfg.ShowFPS = get_int_value(file_buffer, "ShowFPS", g_cfg.ShowFPS);
	if (g_cfg.ShowFPS < 0)
		g_cfg.ShowFPS = 0;
	if (g_cfg.ShowFPS > 1)
		g_cfg.ShowFPS = 1;

	g_cfg.Maintain43 = get_int_value(file_buffer, "Maintain43", g_cfg.Maintain43);
	if (g_cfg.Maintain43 < 0)
		g_cfg.Maintain43 = 0;
	if (g_cfg.Maintain43 > 1)
		g_cfg.Maintain43 = 1;

	g_cfg.UseFrameLimit = get_int_value(file_buffer, "UseFrameLimit", g_cfg.UseFrameLimit);
	if (g_cfg.UseFrameLimit < 0)
		g_cfg.UseFrameLimit = 0;
	if (g_cfg.UseFrameLimit > 1)
		g_cfg.UseFrameLimit = 1;

	g_cfg.UseFrameSkip = 0;

	g_cfg.FPSDetection = get_int_value(file_buffer, "FPSDetection", g_cfg.FPSDetection);
	if (g_cfg.FPSDetection < 1)
		g_cfg.FPSDetection = 1;
	if (g_cfg.FPSDetection > 2)
		g_cfg.FPSDetection = 2;

	g_cfg.FrameRate = get_double_value(file_buffer, "FrameRate", g_cfg.FrameRate);
	g_cfg.FrameRate /= 10;
	if (g_cfg.FrameRate < 10.0f)
		g_cfg.FrameRate = 10.0f;
	if (g_cfg.FrameRate > 1000.0f)
		g_cfg.FrameRate = 1000.0f;

	g_cfg.CfgFixes = get_int_value(file_buffer, "CfgFixes", g_cfg.CfgFixes);

	g_cfg.UseFixes = get_int_value(file_buffer, "UseFixes", g_cfg.UseFixes);
	if (g_cfg.UseFixes < 0)
		g_cfg.UseFixes = 0;
	if (g_cfg.UseFixes > 1)
		g_cfg.UseFixes = 1;

	free(file_buffer);
}

static void ExecCfg(char const * arg) {
	char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgGXVideo");
	if (stat(cfg, &buf) != -1) {
		if (fork() == 0) {
			execl(cfg, "cfgGXVideo", arg, NULL);
			exit(0);
		}
		return;
	}

	strcpy(cfg, "./cfg/cfgGXVideo");
	if (stat(cfg, &buf) != -1) {
		if (fork() == 0) {
			execl(cfg, "cfgGXVideo", arg, NULL);
			exit(0);
		}
		return;
	}

	sprintf(cfg, "%s/.pcsx/plugins/cfg/cfgGXVideo", getenv("HOME"));
	if (stat(cfg, &buf) != -1) {
		if (fork() == 0) {
			execl(cfg, "cfgGXVideo", arg, NULL);
			exit(0);
		}
		return;
	}

	printf("ERROR: cfgGXVideo file not found!\n");
}

void SoftDlgProc(void) {
	ExecCfg("CFG");
}

void AboutDlgProc(void) {
	char args[256];
	sprintf(args, "ABOUT");
	ExecCfg(args);
}

void ReadConfig(void) {
	/* set defaults */
	g_cfg.ResX = 640;
	g_cfg.ResY = 480;
	g_cfg.NoStretch = 0;
	g_cfg.Dithering = 0;
	g_cfg.FullScreen = 0;
	g_cfg.ShowFPS = 0;
	g_cfg.Maintain43 = 0;
	g_cfg.UseFrameLimit = 1;
	g_cfg.UseFrameSkip = 0;
	g_cfg.FPSDetection = 1;
	g_cfg.FrameRate = 200.0;
	g_cfg.CfgFixes = 0;
	g_cfg.UseFixes = 0;

	// read sets
	ReadConfigFile();
}

void WriteConfig(void) {
	FILE *f_out;
	char cfg_file_name[256];

	if (g_file_name)
		strcpy(cfg_file_name, g_file_name);
	else {
		strcpy(cfg_file_name, DEFAULT_CFG_NAME);
		f_out = fopen(cfg_file_name, "rb");
		if (!f_out) {
			strcpy(cfg_file_name, "cfg/" DEFAULT_CFG_NAME);
			f_out = fopen(cfg_file_name, "rb");
			if (!f_out)
				snprintf(cfg_file_name, 255, "%s/.pcsx/plugins/" DEFAULT_CFG_NAME, getenv("HOME"));
			else
				fclose(f_out);
		} else
			fclose(f_out);
	}

	f_out = fopen(cfg_file_name, "wb");
	if (!f_out)
		return;

#define WRITE_VALUE(type, name) write_##type##_value(f_out, #name, g_cfg.name)

	WRITE_VALUE(int, ResX);
	WRITE_VALUE(int, ResY);
	WRITE_VALUE(int, NoStretch);
	WRITE_VALUE(int, Dithering);
	WRITE_VALUE(int, FullScreen);
	WRITE_VALUE(int, ShowFPS);
	WRITE_VALUE(int, Maintain43);
	WRITE_VALUE(int, UseFrameLimit);
	WRITE_VALUE(int, UseFrameSkip);
	WRITE_VALUE(int, FPSDetection);
	WRITE_VALUE(double, FrameRate);
	WRITE_VALUE(int, CfgFixes);
	WRITE_VALUE(int, UseFixes);

	fclose(f_out);
}
