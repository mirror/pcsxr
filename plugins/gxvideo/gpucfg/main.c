/*
 * Copyright (C) 2010 Benoit Gschwind
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

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "globals.h"
#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define READBINARY "rb"
#define WRITEBINARY "wb"
#define CONFIG_FILENAME "dfxvideo.cfg"

enum {
	VIDMODE_320x200 = 0,
	VIDMODE_640x480,
	VIDMODE_800x600,
	VIDMODE_1024x768,
	VIDMODE_1152x864,
	VIDMODE_1280x960,
	VIDMODE_1600x1200
}; /* Video_modes */

typedef struct {
	GtkWidget * config_window;
	GtkWidget * resolution_combobox;
	GtkWidget * dithering_combobox;
	GtkWidget * maintain_ratio_checkbutton;
	GtkWidget * fullscreen_checkbutton;
	GtkWidget * show_fps_checkbutton;
	GtkWidget * set_fps_checkbutton;
	GtkWidget * auto_fps_limit_checkbutton;
	GtkWidget * fps_entry;
	GtkWidget * use_game_fixes_checkbutton;
	GtkWidget * save_button;
	GtkWidget * cancel_button;
	GtkWidget * fixes_table;
	GtkWidget * fix_checkbutton[11];
} cfg_window_t;

void save_config(cfg_window_t *);

static void on_about_clicked(GtkWidget * widget, gpointer user_data) {
	gtk_widget_destroy(widget);
	exit(0);
}

static void on_fullscreen_toggled(GtkWidget * widget, cfg_window_t * w) {
	gtk_widget_set_sensitive(w->resolution_combobox,
			!gtk_toggle_button_get_active(
					GTK_TOGGLE_BUTTON(w->fullscreen_checkbutton)));
}

static void update_fixes_stase(GtkWidget * ths, GtkWidget * w) {
	gtk_widget_set_sensitive(ths, gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(w)));
}

static void on_use_fixes_toggled(GtkWidget * widget, cfg_window_t * w) {
	/* Set the state of each of the fixes to the value of the use fixes toggle */
	gtk_container_foreach(GTK_CONTAINER (w->fixes_table),
			(GtkCallback) update_fixes_stase, w->use_game_fixes_checkbutton);
}

static void on_fps_toggled(GtkWidget * widget, cfg_window_t * w) {
	gboolean state_set_fps = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(w->set_fps_checkbutton));
	gboolean state_auto_fps_limit = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(w->auto_fps_limit_checkbutton));
	gtk_widget_set_sensitive(w->fps_entry, state_set_fps
			&& !state_auto_fps_limit);
	gtk_widget_set_sensitive(w->auto_fps_limit_checkbutton, state_set_fps);
}

static void on_destroy_window(GtkWidget * widget, cfg_window_t * w) {
	free(w);
}

static void on_click_save_button(GtkWidget * widget, cfg_window_t * w) {
	save_config(w);
	gtk_widget_destroy(GTK_WIDGET(w->config_window));
	gtk_exit(0);
}

static void on_click_cancel_button(GtkWidget * widget, cfg_window_t * w) {
	gtk_widget_destroy(GTK_WIDGET(w->config_window));
	gtk_exit(0);
}

int main(int argc, char *argv[]) {
	cfg_window_t * w = 0;
	GtkWidget * widget;
	GtkBuilder * builder;
	int i, val;
	char tempstr[50];

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	if (argc != 2) {
		printf("Usage: cfgGXVideo {ABOUT | CFG}\n");
		return 0;
	}
	if (strcmp(argv[1], "CFG") != 0 && strcmp(argv[1], "ABOUT") != 0) {
		printf("Usage: cfgGXVideo {ABOUT | CFG}\n");
		return 0;
	}

	gtk_set_locale();
	gtk_init(&argc, &argv);

	if (strcmp(argv[1], "ABOUT") == 0) {
		const char *authors[] = { "Pete Bernert and the P.E.Op.S. team",
				"Ryan Schultz", "Andrew Burton", NULL };
		widget = gtk_about_dialog_new();
		gtk_about_dialog_set_name(GTK_ABOUT_DIALOG (widget),
				"P.E.Op.S PCSX Video Plugin");
		gtk_about_dialog_set_version(GTK_ABOUT_DIALOG (widget), "1.17");
		gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG (widget), authors);
		gtk_about_dialog_set_website(GTK_ABOUT_DIALOG (widget),
				"http://pcsx-df.sourceforge.net/");

		g_signal_connect_data(GTK_OBJECT(widget), "response",
				G_CALLBACK(on_about_clicked), NULL, NULL, G_CONNECT_AFTER);

		gtk_widget_show(widget);
		gtk_main();

		return 0;
	}

	builder = gtk_builder_new();
	if (!builder) {
		g_warning("We could not load the interface!");
		return -1;
	}

	gtk_builder_add_from_file(builder, DATADIR "gxvideo.glade", NULL);
	w = (cfg_window_t *) malloc(sizeof(cfg_window_t));
#define CFG_GET_WIDGET(name) w->name = GTK_WIDGET(gtk_builder_get_object(builder, #name))
	CFG_GET_WIDGET(config_window);
	CFG_GET_WIDGET(config_window);
	CFG_GET_WIDGET(resolution_combobox);
	CFG_GET_WIDGET(dithering_combobox);
	CFG_GET_WIDGET(maintain_ratio_checkbutton);
	CFG_GET_WIDGET(fullscreen_checkbutton);
	CFG_GET_WIDGET(show_fps_checkbutton);
	CFG_GET_WIDGET(set_fps_checkbutton);
	CFG_GET_WIDGET(auto_fps_limit_checkbutton);
	CFG_GET_WIDGET(fps_entry);
	CFG_GET_WIDGET(use_game_fixes_checkbutton);
	CFG_GET_WIDGET(save_button);
	CFG_GET_WIDGET(cancel_button);
	CFG_GET_WIDGET(fixes_table);

	for (i = 0; i < 11; ++i) {
		sprintf(tempstr, "fix%d_checkbutton", i);
		w->fix_checkbutton[i]
				= GTK_WIDGET(gtk_builder_get_object(builder, tempstr));
	}

	ReadConfig();

	switch (g_cfg.ResX) {
	case 1600:
		val = VIDMODE_1600x1200;
		break;
	case 1280:
		val = VIDMODE_1280x960;
		break;
	case 1152:
		val = VIDMODE_1152x864;
		break;
	case 1024:
		val = VIDMODE_1024x768;
		break;
	case 800:
		val = VIDMODE_800x600;
		break;
	case 640:
		val = VIDMODE_640x480;
		break;
	default:
		val = VIDMODE_320x200;
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX (w->resolution_combobox), val);
	gtk_combo_box_set_active(GTK_COMBO_BOX (w->dithering_combobox),
			g_cfg.Dithering);
	gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON (w->maintain_ratio_checkbutton), g_cfg.Maintain43);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w->fullscreen_checkbutton),
			g_cfg.FullScreen);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w->show_fps_checkbutton),
			g_cfg.ShowFPS);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w->set_fps_checkbutton),
			g_cfg.UseFrameLimit);

	gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON (w->auto_fps_limit_checkbutton),
			g_cfg.FPSDetection);

	sprintf(tempstr, "%.1f", g_cfg.FrameRate);
	gtk_entry_set_text(GTK_ENTRY(w->fps_entry), tempstr);

	gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON (w->use_game_fixes_checkbutton), g_cfg.UseFixes);

	for (i = 0; i < 11; ++i) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (w->fix_checkbutton[i]),
				(g_cfg.CfgFixes >> i) & 1);
	}

	g_signal_connect_data(GTK_OBJECT(w->config_window), "destroy",
			G_CALLBACK(on_destroy_window), w, NULL, 0);
	g_signal_connect_data(GTK_OBJECT(w->save_button), "clicked",
			G_CALLBACK(on_click_save_button), w, NULL, G_CONNECT_AFTER);
	g_signal_connect_data(GTK_OBJECT(w->cancel_button), "clicked",
			G_CALLBACK(on_click_cancel_button), w, NULL, G_CONNECT_AFTER);

	g_signal_connect_data(GTK_OBJECT(w->fullscreen_checkbutton), "clicked",
			G_CALLBACK(on_fullscreen_toggled), w, NULL, G_CONNECT_AFTER);

	g_signal_connect_data(GTK_OBJECT(w->use_game_fixes_checkbutton), "clicked",
			G_CALLBACK(on_use_fixes_toggled), w, NULL, G_CONNECT_AFTER);
	g_signal_connect_data(GTK_OBJECT(w->set_fps_checkbutton), "clicked",
			G_CALLBACK(on_fps_toggled), w, NULL, G_CONNECT_AFTER);

	g_signal_connect_data(GTK_OBJECT(w->auto_fps_limit_checkbutton), "clicked",
			G_CALLBACK(on_fps_toggled), w, NULL, G_CONNECT_AFTER);

	on_fullscreen_toggled(0, w);
	on_fps_toggled(0, w);
	on_use_fixes_toggled(0, w);

	gtk_widget_show_all(w->config_window);
	gtk_main();
	return 0;
}

void save_config(cfg_window_t * w) {
	int val;

	val = gtk_combo_box_get_active(GTK_COMBO_BOX (w->resolution_combobox));

	if (val == VIDMODE_320x200) {
		g_cfg.ResX = 320;
		g_cfg.ResY = 240;
	} else if (val == VIDMODE_640x480) {
		g_cfg.ResX = 640;
		g_cfg.ResY = 480;
	} else if (val == VIDMODE_800x600) {
		g_cfg.ResX = 800;
		g_cfg.ResY = 600;
	} else if (val == VIDMODE_1024x768) {
		g_cfg.ResX = 1024;
		g_cfg.ResY = 768;
	} else if (val == VIDMODE_1152x864) {
		g_cfg.ResX = 1152;
		g_cfg.ResY = 864;
	} else if (val == VIDMODE_1280x960) {
		g_cfg.ResX = 1280;
		g_cfg.ResY = 960;
	} else if (val == VIDMODE_1600x1200) {
		g_cfg.ResX = 1600;
		g_cfg.ResY = 1200;
	}

	val = gtk_combo_box_get_active(GTK_COMBO_BOX (w->dithering_combobox));
	g_cfg.Dithering = val;

	val = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON (w->maintain_ratio_checkbutton));
	g_cfg.Maintain43 = val;

	val = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON (w->fullscreen_checkbutton));
	g_cfg.FullScreen = val;

	val = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(w->show_fps_checkbutton));
	g_cfg.ShowFPS = val;

	val = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON (w->set_fps_checkbutton));
	g_cfg.UseFrameLimit = val;

	val = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON (w->auto_fps_limit_checkbutton));
	g_cfg.FPSDetection = val;

	//Framerate stored *10
	val = atof(gtk_entry_get_text(GTK_ENTRY(w->fps_entry))) * 10;
	g_cfg.FrameRate = val;

	val = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON (w->use_game_fixes_checkbutton));
	g_cfg.UseFixes = val;

	val = 0;
	int i;
	for (i = 0; i < 11; ++i) {
		if (gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON (w->fix_checkbutton[i])))
			val |= 1 << i;
	}
	g_cfg.CfgFixes = val;

	WriteConfig();
	free(w);
	// Close the window and exit control from the plugin
	gtk_exit(0);
}
