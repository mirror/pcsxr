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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <sys/time.h>
#include <regex.h>

#include "Linux.h"

#include "../libpcsxcore/plugins.h"
#include "../libpcsxcore/cheat.h"

#include "MemcardDlg.h"
#include "ConfDlg.h"
#include "DebugMemory.h"
#include "AboutDlg.h"

// Functions Callbacks
void OnFile_RunCd();
void OnFile_RunBios();
void OnFile_RunExe();
void OnFile_RunImage();
void OnEmu_Run();
void OnEmu_Reset();
void OnEmu_Shutdown();
void OnEmu_SwitchImage();
void OnHelp_Help();
void OnHelp_About();
void OnDestroy();
void OnFile_Exit();

void on_states_load(GtkWidget *widget, gpointer user_data);
void on_states_load_other();
void on_states_save(GtkWidget *widget, gpointer user_data);
void on_states_save_other();

static GtkBuilder *builder;
GtkWidget *Window = NULL;

int destroy = 0;

#define MAX_SLOTS 9

/* TODO - If MAX_SLOTS changes, need to find a way to automatically set all positions */
int Slots[MAX_SLOTS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };

void ResetMenuSlots() {
	GtkWidget *widget;
	gchar *str;
	int i;

	if (CdromId[0] == '\0') {
		// disable state saving/loading if no CD is loaded
		for (i = 0; i < MAX_SLOTS; i++) {
			str = g_strdup_printf("GtkMenuItem_SaveSlot%d", i+1);
			widget = GTK_WIDGET(gtk_builder_get_object(builder, str));
			g_free(str);

			gtk_widget_set_sensitive(widget, FALSE);

			str = g_strdup_printf("GtkMenuItem_LoadSlot%d", i+1);
			widget = GTK_WIDGET(gtk_builder_get_object(builder, str));
			g_free(str);

			gtk_widget_set_sensitive(widget, FALSE);
		}

		// also disable certain menu/toolbar items
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "other1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "other2"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "run1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "reset1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "shutdown1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "search1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "SwitchImage"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "memorydump1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_run"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_switchimage"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "plugins_bios"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "graphics1"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "sound1"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "cdrom1"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "pad1"));
		gtk_widget_set_sensitive(widget, TRUE);
#ifdef ENABLE_SIO1API
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "sio1"));
		gtk_widget_set_sensitive(widget, TRUE);
#else
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "sio1"));
		gtk_widget_set_sensitive(widget, FALSE);	
#endif
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "net1"));
		gtk_widget_set_sensitive(widget, TRUE);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_switchimage"));
		gtk_widget_set_sensitive(widget, UsingIso());
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_graphics"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_sound"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_cdrom"));
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_controllers"));
		gtk_widget_set_sensitive(widget, TRUE);
		
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
		gtk_statusbar_pop(GTK_STATUSBAR(widget), 1);
		gtk_statusbar_push(GTK_STATUSBAR(widget), 1, _("Ready"));
	}
	else {
		for (i = 0; i < MAX_SLOTS; i++) {
			str = g_strdup_printf("GtkMenuItem_LoadSlot%d", i+1);
			widget = GTK_WIDGET(gtk_builder_get_object (builder, str));
			g_free (str);

			if (Slots[i] == -1) 
				gtk_widget_set_sensitive(widget, FALSE);
			else
				gtk_widget_set_sensitive(widget, TRUE);
		}

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "plugins_bios"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "graphics1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "sound1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "cdrom1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "pad1"));
		gtk_widget_set_sensitive(widget, FALSE);
#ifdef ENABLE_SIO1API
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "sio1"));
		gtk_widget_set_sensitive(widget, FALSE);
#endif
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "net1"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "SwitchImage"));
		gtk_widget_set_sensitive(widget, UsingIso());
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_switchimage"));
		gtk_widget_set_sensitive(widget, UsingIso());
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_graphics"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_sound"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_cdrom"));
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_controllers"));
		gtk_widget_set_sensitive(widget, FALSE);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
		gtk_statusbar_pop(GTK_STATUSBAR(widget), 1);
		gtk_statusbar_push(GTK_STATUSBAR(widget), 1, _("Emulation Paused."));
	}
}

int match(const char *string, char *pattern) {
	int    status;
	regex_t    re;

	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		return 0;
	}
	status = regexec(&re, string, (size_t) 0, NULL, 0);
	regfree(&re);
	if (status != 0) {
		return 0;
	}

	return 1;
}

gchar* get_state_filename(int i) {
	gchar *state_filename;
	char SStateFile[64];
	char trimlabel[33];
	int j;

	strncpy(trimlabel, CdromLabel, 32);
	trimlabel[32] = 0;
	for (j = 31; j >= 0; j--)
		if (trimlabel[j] == ' ')
			trimlabel[j] = 0;
		else
			continue;

	sprintf(SStateFile, "%.32s-%.9s.%3.3d", trimlabel, CdromId, i);
	state_filename = g_build_filename (getenv("HOME"), STATES_DIR, SStateFile, NULL);

	return state_filename;
}

void UpdateMenuSlots() {
	gchar *str;
	int i;

	for (i = 0; i < MAX_SLOTS; i++) {
		str = get_state_filename (i);
		Slots[i] = CheckState(str);
		g_free (str);
	}
}

void StartGui() {
	GtkWidget *widget;

	/* If a plugin fails, the Window is not NULL, but is not initialised,
	   so the following causes a segfault
	if (Window != NULL) {
		gtk_window_present (GTK_WINDOW (Window));
		return;
	}*/
	builder = gtk_builder_new();
	
	if (!gtk_builder_add_from_file(builder, PACKAGE_DATA_DIR "pcsxr.ui", NULL)) {
		g_warning("Error: interface could not be loaded!");
		return;
	}
	
	Window = GTK_WIDGET(gtk_builder_get_object(builder, "MainWindow"));
	gtk_widget_show(GTK_WIDGET(Window));

	gtk_window_set_title(GTK_WINDOW(Window), "PCSXR");
	gtk_window_set_icon_from_file(GTK_WINDOW(Window), PIXMAPDIR "pcsxr-icon.png", NULL);
	gtk_window_set_default_icon_from_file(PIXMAPDIR "pcsxr-icon.png", NULL);
	ResetMenuSlots();

	// Set up callbacks
	g_signal_connect_data(G_OBJECT(Window), "delete-event",
			G_CALLBACK(OnDestroy), builder, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);

	// File menu
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "RunCd"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnFile_RunCd), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "RunBios"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnFile_RunBios), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "RunExe"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnFile_RunExe), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "RunImage"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnFile_RunImage), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "exit2"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnFile_Exit), NULL, NULL, G_CONNECT_AFTER);

	// States
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(0), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot2"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot3"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot4"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(3), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot5"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(4), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot6"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(5), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot7"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(6), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot8"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(7), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_LoadSlot9"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load), GINT_TO_POINTER(8), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "other1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_load_other), NULL, NULL, G_CONNECT_AFTER);			

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(0), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot2"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot3"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot4"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(3), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot5"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(4), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot6"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(5), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot7"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(6), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot8"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(7), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkMenuItem_SaveSlot9"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save), GINT_TO_POINTER(8), NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "other2"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(on_states_save_other), NULL, NULL, G_CONNECT_AFTER);

	// Emulation menu
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "run1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnEmu_Run), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "reset1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnEmu_Reset), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "shutdown1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnEmu_Shutdown), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "SwitchImage"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnEmu_SwitchImage), NULL, NULL, G_CONNECT_AFTER);

	// Configuration menu
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "plugins_bios"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(ConfigurePlugins), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "graphics1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Graphics), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "sound1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Sound), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "cdrom1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_CdRom), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "pad1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Pad), NULL, NULL, G_CONNECT_AFTER);
#ifdef ENABLE_SIO1API
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "sio1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Sio1), NULL, NULL, G_CONNECT_AFTER);
#endif
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "cpu1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Cpu), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "memory_cards1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Mcds), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "net1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnConf_Net), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "memorydump1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(RunDebugMemoryDialog), NULL, NULL, G_CONNECT_AFTER);

	// Cheat menu
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "browse1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(RunCheatListDialog), NULL, NULL, G_CONNECT_AFTER);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "search1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(RunCheatSearchDialog), NULL, NULL, G_CONNECT_AFTER);

	// Help menu
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "about_pcsxr1"));
	g_signal_connect_data(G_OBJECT(widget), "activate",
			G_CALLBACK(OnHelp_About), NULL, NULL, G_CONNECT_AFTER);

	// Toolbar
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_runcd"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnFile_RunCd), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_runimage"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnFile_RunImage), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_run"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnEmu_Run), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_switchimage"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnEmu_SwitchImage), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_memcards"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnConf_Mcds), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_graphics"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnConf_Graphics), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_sound"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnConf_Sound), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_cdrom"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnConf_CdRom), NULL, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "toolbutton_controllers"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnConf_Pad), NULL, NULL, G_CONNECT_AFTER);

	gtk_main();
}

void OnDestroy() {
	if (!destroy) OnFile_Exit();
}

void destroy_main_window () {
	destroy = 1;
	gtk_widget_destroy(Window);
	Window = NULL;
	destroy = 0;
	gtk_main_quit();
	while (gtk_events_pending()) gtk_main_iteration();
}

void OnFile_RunExe() {
	GtkWidget *file_chooser;

	if (plugins_configured() == FALSE) {
		ConfigurePlugins();
	} else {
		file_chooser = gtk_file_chooser_dialog_new(_("Select PSX EXE File"),
			NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

		// Add file filters
		GtkFileFilter *exefilter = gtk_file_filter_new ();
		gtk_file_filter_add_pattern (exefilter, "*.exe");
		gtk_file_filter_add_pattern (exefilter, "*.psx");
		gtk_file_filter_add_pattern (exefilter, "*.cpe");
		gtk_file_filter_add_pattern (exefilter, "*.EXE");
		gtk_file_filter_add_pattern (exefilter, "*.PSX");
		gtk_file_filter_add_pattern (exefilter, "*.CPE");
		gtk_file_filter_set_name (exefilter, _("PlayStation Executable Files"));
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), exefilter);
		GtkFileFilter *allfilter = gtk_file_filter_new ();
		gtk_file_filter_add_pattern (allfilter, "*");
		gtk_file_filter_set_name (allfilter, _("All Files"));
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), allfilter);

		// Set this to the config object and retain it - maybe LastUsedDir
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), getenv("HOME"));

		if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
			gchar *file;

			/* TODO Need to validate the file */

			file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

			gtk_widget_destroy (file_chooser);
			destroy_main_window();

			SetIsoFile(NULL);
			LoadPlugins();
			NetOpened = FALSE;

			if (OpenPlugins() == -1) {
				g_free(file);
				SysRunGui();
			} else {
				// Auto-detect: get region first, then rcnt-bios reset
				SysReset();

				if (Load(file) == 0) {
					g_free(file);
					psxCpu->Execute();
				} else {
					g_free(file);
					ClosePlugins();
					SysErrorMessage(_("Not a valid PSX file"), _("The file does not appear to be a valid Playstation executable"));
					SysRunGui();
				}
			}
		} else
			gtk_widget_destroy(file_chooser);
	}
}

void OnFile_RunCd() {
	if (plugins_configured() == FALSE) { 
		ConfigurePlugins();
		return;
	}

	destroy_main_window();

	SetIsoFile(NULL);
	LoadPlugins();
	NetOpened = FALSE;

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	if (CheckCdrom() == -1) {
		/* Only check the CD if we are starting the console with a CD */
		ClosePlugins();
		SysErrorMessage (_("CD ROM failed"), _("The CD does not appear to be a valid Playstation CD"));
		SysRunGui();
		return;
	}

	// Auto-detect: get region first, then rcnt-bios reset
	SysReset();

	// Read main executable directly from CDRom and start it
	if (LoadCdrom() == -1) {
		ClosePlugins();
		SysErrorMessage(_("Could not load CD-ROM!"), _("The CD-ROM could not be loaded"));
		SysRunGui();
	}

	psxCpu->Execute();
}

void OnFile_RunBios() {
	if (plugins_configured() == FALSE) { 
		ConfigurePlugins();
		return;
	}

	if (strcmp(Config.Bios, "HLE") == 0) {
		SysErrorMessage (_("Could not run BIOS"), _("Running BIOS is not supported with Internal HLE BIOS."));
		return;
	}

	destroy_main_window();

	SetIsoFile(NULL);
	LoadPlugins();
	NetOpened = FALSE;

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	SysReset();

	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	psxCpu->Execute();
}

static gchar *Open_Iso_Proc() {
	struct stat sb;
	GtkWidget *chooser;
	gchar *filename;
	GtkFileFilter *psxfilter, *allfilter;
	static char current_folder[MAXPATHLEN] = "";

	chooser = gtk_file_chooser_dialog_new (_("Open PSX Disc Image File"),
		NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	if (stat(Config.IsoImgDir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		strcpy(current_folder, Config.IsoImgDir);
	} else if (strlen(Config.IsoImgDir) <= 0) {
		strcpy(current_folder, getenv("HOME"));
	} else {
		/* Using static (recent) PATH */
	}

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (chooser), current_folder);

	psxfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(psxfilter, "*.bin");
	gtk_file_filter_add_pattern(psxfilter, "*.img");
	gtk_file_filter_add_pattern(psxfilter, "*.mdf");
	gtk_file_filter_add_pattern(psxfilter, "*.iso");
	gtk_file_filter_add_pattern(psxfilter, "*.cue");
	gtk_file_filter_add_pattern(psxfilter, "*.pbp");
	gtk_file_filter_add_pattern(psxfilter, "*.cbn");
	gtk_file_filter_add_pattern(psxfilter, "*.BIN");
	gtk_file_filter_add_pattern(psxfilter, "*.IMG");
	gtk_file_filter_add_pattern(psxfilter, "*.MDF");
	gtk_file_filter_add_pattern(psxfilter, "*.ISO");
	gtk_file_filter_add_pattern(psxfilter, "*.CUE");
	gtk_file_filter_add_pattern(psxfilter, "*.PBP");
	gtk_file_filter_add_pattern(psxfilter, "*.CBN");
	gtk_file_filter_set_name(psxfilter, _("PSX Image Files (*.bin, *.img, *.mdf, *.iso, *.cue, *.pbp, *.cbn)"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (chooser), psxfilter);

	allfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(allfilter, "*");
	gtk_file_filter_set_name(allfilter, _("All Files"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (chooser), allfilter);

	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK) {
		gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
		
		/* Workaround:
		for some reasons gtk_file_chooser_get_current_folder return NULL
		if a file is selected from "Recently Used" or "Searsh"*/
		if(path != NULL) {
		  strcpy(current_folder, path);
		  g_free(path);
		}
		
		GSList * l = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER (chooser));
		if(l) {
		filename = l->data;
		
		/* if the file was selected from "Recently Used" or "Searsh"
		we need to extract the path from the filename to set it to current_folder*/
		if(path == NULL) {
		  strncpy(current_folder, filename, strrchr(filename, '/') - filename);
		}

		/* If ISO path is NULL save current path. */
		if (!S_ISDIR(sb.st_mode)) {
		  strcpy(Config.IsoImgDir, current_folder);
		  SaveConfig();
		}

		/* free useless data */
		GSList * ll = l;
		while(l->next) {
			l = l->next;
			g_free(l->data);
		}
		g_slist_free(ll);

		gtk_widget_destroy(GTK_WIDGET(chooser));
		while (gtk_events_pending()) gtk_main_iteration();
		return filename;
		} else {
			gtk_widget_destroy (GTK_WIDGET(chooser));
			while (gtk_events_pending()) gtk_main_iteration();
			return NULL;
		}
	} else {
		gtk_widget_destroy (GTK_WIDGET(chooser));
		while (gtk_events_pending()) gtk_main_iteration();
		return NULL;
	}
}

void OnFile_RunImage() {
	gchar *filename;

	if (plugins_configured() == FALSE) { 
		ConfigurePlugins();
		return;
	}

	filename = Open_Iso_Proc();
	if (filename == NULL) {
		return;
	}

	destroy_main_window();

	SetIsoFile(filename);
	g_free(filename);

	LoadPlugins();
	NetOpened = FALSE;

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	if (CheckCdrom() == -1) {
		// Only check the CD if we are starting the console with a CD
		ClosePlugins();
		SysErrorMessage (_("CD ROM failed"), _("The CD does not appear to be a valid Playstation CD"));
		SysRunGui();
		return;
	}

	// Auto-detect: get region first, then rcnt-bios reset
	SysReset();

	// Read main executable directly from CDRom and start it
	if (LoadCdrom() == -1) {
		ClosePlugins();
		SysErrorMessage(_("Could not load CD-ROM!"), _("The CD-ROM could not be loaded"));
		SysRunGui();
	}

	psxCpu->Execute();
}

void OnEmu_Run() {
	if (plugins_configured() == FALSE) { 
		ConfigurePlugins();
		return;
	}

	destroy_main_window();

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	CheatSearchBackupMemory();
	psxCpu->Execute();
}

void OnEmu_Reset() {
	if (plugins_configured() == FALSE) { 
		ConfigurePlugins();
		return;
	}

	destroy_main_window();

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	if (CheckCdrom() != -1) {
		LoadCdrom();
	}

	// Auto-detect: get region first, then rcnt-bios reset
	SysReset();

	psxCpu->Execute();
}

void OnEmu_Shutdown() {
	ReleasePlugins();
	SetIsoFile(NULL);
	CdromId[0] = '\0';
	CdromLabel[0] = '\0';
	ResetMenuSlots();
}

void OnEmu_SwitchImage() {
	gchar *filename;

	if (plugins_configured() == FALSE) { 
		ConfigurePlugins();
		return;
	}

	filename = Open_Iso_Proc();
	if (filename == NULL) {
		return;
	}

	destroy_main_window();

	SetIsoFile(filename);
	g_free(filename);

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	SetCdOpenCaseTime(time(NULL) + 2);
	LidInterrupt(); // causes CD lid open event

	CheatSearchBackupMemory();
	psxCpu->Execute();
}

void OnFile_Exit() {
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	gchar *plugin = NULL;
	gchar *dotdir;

	dotdir = g_build_filename(getenv("HOME"), PLUGINS_DIR, NULL);

	// with this the problem with plugins that are linked with the pthread
	// library is solved

	dir = opendir(dotdir);
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			plugin = g_build_filename(dotdir, ent->d_name, NULL);

			if (strstr(plugin, ".so") == NULL && strstr(plugin, ".dylib") == NULL)
				continue;
			Handle = dlopen(plugin, RTLD_NOW);
			if (Handle == NULL)
				continue;

			g_free(plugin);
		}
	}
	g_free(dotdir);

	bind_textdomain_codeset(PACKAGE_NAME, "");
	if (UseGui)
		gtk_main_quit();
	SysClose();
	exit(0);
}

void state_load(gchar *state_filename) {
	int ret;
	char Text[MAXPATHLEN + 20];
	FILE *fp;

	// check if the state file actually exists
	fp = fopen(state_filename, "rb");
	if (fp == NULL) {
		// file does not exist
		return;
	}

	fclose(fp);

	// If the window exists, then we are loading the state from within
	// within the PCSXR GUI. We need to initialise the plugins first
	if (Window) {
		destroy_main_window();

		if (OpenPlugins() == -1) {
			SysRunGui();
			return;
		}
	}

	ret = CheckState(state_filename);

	if (ret == 0) {
		// Check the CD-ROM is valid
		if (CheckCdrom() == -1) {
			ClosePlugins();
			SysRunGui();
			return;
		}

		// Auto-detect: region first, then rcnt reset
		SysReset();
		ret = LoadState(state_filename);

		sprintf(Text, _("Loaded state %s."), state_filename);
		GPU_displayText(Text);
	} else {
		sprintf(Text, _("Error loading state %s!"), state_filename);
		GPU_displayText(Text);
	}
}

void state_save(gchar *state_filename) {
	char Text[MAXPATHLEN + 20];

	GPU_updateLace();

	if (SaveState(state_filename) == 0)
		sprintf(Text, _("Saved state %s."), state_filename);
	else
		sprintf(Text, _("Error saving state %s!"), state_filename);

	GPU_displayText(Text);
}

void on_states_load (GtkWidget *widget, gpointer user_data) {
	(void)widget; // unused

	gchar *state_filename;
	gint state = GPOINTER_TO_INT(user_data);

	state_filename = get_state_filename(state);

	state_load(state_filename);

	g_free(state_filename);

	psxCpu->Execute();
}

void on_states_save (GtkWidget *widget, gpointer user_data) {
	(void)widget; // unused

	gchar *state_filename;
	gint state = GPOINTER_TO_INT(user_data);

	state_filename = get_state_filename(state);

	state_save(state_filename);

	g_free(state_filename);
}

void on_states_load_other() {
	GtkWidget *file_chooser;
	gchar *SStateFile;

	SStateFile = g_strconcat(getenv("HOME"), STATES_DIR, NULL);

	file_chooser = gtk_file_chooser_dialog_new(_("Select State File"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (file_chooser), SStateFile);
	g_free(SStateFile);

	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		gtk_widget_destroy(file_chooser);

		state_load(filename);

		g_free(filename);

		psxCpu->Execute();
	} else
		gtk_widget_destroy(file_chooser);
} 

void on_states_save_other() {
	GtkWidget *file_chooser;
	gchar *SStateFile;

	SStateFile = g_strconcat (getenv("HOME"), STATES_DIR, NULL);

	file_chooser = gtk_file_chooser_dialog_new(_("Select State File"),
			NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_OK,
			NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), SStateFile);
	g_free(SStateFile);

	if (gtk_dialog_run (GTK_DIALOG(file_chooser)) == GTK_RESPONSE_OK) {
		gchar *filename;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
		gtk_widget_destroy(file_chooser);

		state_save(filename);

		g_free(filename);
	}
	else
		gtk_widget_destroy(file_chooser);
} 

void OnHelp_About(GtkWidget *widget, gpointer user_data) {
	RunAboutDialog();
}

void SysMessage(const char *fmt, ...) {
	GtkWidget *Txt, *MsgDlg;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg) - 1] == '\n')
		msg[strlen(msg) - 1] = 0;

	if (!UseGui) {
		fprintf(stderr, "%s\n", msg);
		return;
	}

	MsgDlg = gtk_dialog_new_with_buttons(_("Notice"), NULL,
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);

	gtk_window_set_position (GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);

	Txt = gtk_label_new (msg);
	gtk_label_set_line_wrap(GTK_LABEL(Txt), TRUE);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(MsgDlg))), Txt);

	gtk_widget_show (Txt);
	gtk_widget_show_all (MsgDlg);
	gtk_dialog_run (GTK_DIALOG(MsgDlg));
	gtk_widget_destroy (MsgDlg);
}

void SysErrorMessage(gchar *primary, gchar *secondary) {
	GtkWidget *message_dialog;	
	if (!UseGui)
		printf ("%s - %s\n", primary, secondary);
	else {
		message_dialog = gtk_message_dialog_new(NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				primary,
				NULL);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(message_dialog),
				secondary);

		gtk_dialog_run(GTK_DIALOG(message_dialog));
		gtk_widget_destroy(message_dialog);
	}
}

void SysInfoMessage(gchar *primary, gchar *secondary) {
	GtkWidget *message_dialog;	
	if (!UseGui)
		printf ("%s - %s\n", primary, secondary);
	else {
		message_dialog = gtk_message_dialog_new(NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_CLOSE,
				primary,
				NULL);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(message_dialog),
				secondary);

		gtk_dialog_run(GTK_DIALOG(message_dialog));
		gtk_widget_destroy(message_dialog);
	}
}
