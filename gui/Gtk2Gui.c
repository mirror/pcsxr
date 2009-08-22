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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <signal.h>
#include <sys/time.h>
#include <regex.h>

#include "Linux.h"

#include "hdebug.h"

#include "../libpcsxcore/plugins.h"
#include "../libpcsxcore/sio.h"
#include "../libpcsxcore/cheat.h"

extern int UseGui;

PSEgetLibType		PSE_getLibType = NULL;
PSEgetLibVersion	PSE_getLibVersion = NULL;
PSEgetLibName		PSE_getLibName = NULL;

// Helper Functions
void UpdatePluginsBIOS();
void UpdatePluginsBIOS_UpdateGUI(GladeXML *xml);
void FindNetPlugin(GladeXML *xml);
void copy_memcard_data (char *from, char *to, gint *i, gchar *str);

void OnNet_Conf(GtkWidget *widget, gpointer user_data);
void OnNet_About(GtkWidget *widget, gpointer user_data);

// Functions Callbacks
/*make them getting the right params, will be used later*/
void OnFile_RunCd();
void OnFile_RunBios();
void OnFile_RunExe();
void OnFile_RunImage();
void OnEmu_Run();
void OnEmu_Reset();
void OnEmu_SwitchImage();
void OnConf_Mcds();
void OnConf_Cpu();
void OnConf_Net();
void OnHelp_Help();
void OnHelp_About();
void OnDestroy();
void OnFile_Exit();

void OnBiosPath_Changed(GtkWidget *wdg, gpointer data);
void OnConf_Clicked (GtkDialog *dialog, gint arg1, gpointer user_data);
void OnPluginPath_Changed(GtkWidget *wdg, gpointer data);
void OnConfConf_Pad1About(GtkWidget *widget, gpointer user_data);
void OnConfConf_Pad2About(GtkWidget *widget, gpointer user_data);
void OnConfConf_Pad1Conf(GtkWidget *widget, gpointer user_data);
void OnConfConf_Pad2Conf(GtkWidget *widget, gpointer user_data);

void on_states_load(GtkWidget *widget, gpointer user_data);
void on_states_load_other();
void on_states_save(GtkWidget *widget, gpointer user_data);
void on_states_save_other();

void on_configure_plugin(GtkWidget *widget, gpointer user_data);
void on_about_plugin(GtkWidget *widget, gpointer user_data);

GtkWidget *Window = NULL;
GtkWidget *ConfDlg = NULL;

//GtkAccelGroup *AccelGroup;

GtkWidget *controlwidget;

int destroy = 0;

#define MAX_SLOTS 5
#define MAX_MEMCARD_BLOCKS 15

#define FindComboText(combo, list, conf) \
	if (strlen(conf) > 0) { \
		int i; \
		for (i = 2; i < 255; i += 2) { \
			if (!strcmp(conf, list[i - 2])) { \
				gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i / 2 - 1); \
				break; \
			} \
		} \
	}

#define GetComboText(combo, list, conf) \
	{ \
		int row; \
		row = gtk_combo_box_get_active(GTK_COMBO_BOX(combo)); \
		strcpy(conf, (char *)list[row * 2]); \
	}

/* TODO - If MAX_SLOTS changes, need to find a way to automatically set all positions */
int Slots[MAX_SLOTS] = { -1, -1, -1, -1, -1 };

void ResetMenuSlots(GladeXML *xml) {
	GtkWidget *widget;
	gchar *str;
	int i;

	if (CdromId[0] == '\0') {
		/* disable state saving/loading if no CD is loaded */
		for (i = 0; i < MAX_SLOTS; i++) {
			str = g_strdup_printf("GtkMenuItem_SaveSlot%d", i+1);
			widget = glade_xml_get_widget(xml, str);
			g_free(str);

			gtk_widget_set_sensitive(widget, FALSE);

			str = g_strdup_printf("GtkMenuItem_LoadSlot%d", i+1);
			widget = glade_xml_get_widget (xml, str);
			g_free (str);

			gtk_widget_set_sensitive(widget, FALSE);
		}

		/* also disable certain menu items */
		widget = glade_xml_get_widget (xml, "other1");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "other2");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "run1");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "reset1");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "search1");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "SwitchImage");
		gtk_widget_set_sensitive(widget, FALSE);
	}
	else {
		for (i = 0; i < MAX_SLOTS; i++) {
			str = g_strdup_printf("GtkMenuItem_LoadSlot%d", i+1);
			widget = glade_xml_get_widget (xml, str);
			g_free (str);

			if (Slots[i] == -1) 
				gtk_widget_set_sensitive(widget, FALSE);
			else
				gtk_widget_set_sensitive(widget, TRUE);
		}

		widget = glade_xml_get_widget (xml, "plugins_bios");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "net1");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = glade_xml_get_widget (xml, "SwitchImage");
		gtk_widget_set_sensitive(widget, cdrfilename[0]);
	}
}

int match(const char *string, char *pattern) {
	int    status;
	regex_t    re;

	if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
		return 0;
	}
	status = regexec(&re, string, (size_t) 0, NULL, 0);
	regfree(&re);
	if (status != 0) {
		return 0;
	}

	return 1;
}

gchar* get_state_filename (int i) {
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
	GladeXML *xml;
	GtkWidget *widget;

	/* If a plugin fails, the Window is not NULL, but is not initialised,
	   so the following causes a segfault
	if (Window != NULL) {
		gtk_window_present (GTK_WINDOW (Window));
		return;
	}*/

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "MainWindow", NULL);

	if (!xml) {
		g_warning("We could not load the interface!");
		return;
	}

	Window = glade_xml_get_widget(xml, "MainWindow");
	gtk_window_set_title(GTK_WINDOW(Window), "PCSX");
	gtk_window_set_icon_from_file(GTK_WINDOW(Window), PIXMAPDIR "pcsx-icon.png", NULL);
	gtk_window_set_default_icon_from_file(PIXMAPDIR "pcsx-icon.png", NULL);
	ResetMenuSlots(xml);

	/* Set up callbacks */
	g_signal_connect_data(GTK_OBJECT(Window), "delete-event",
			GTK_SIGNAL_FUNC(OnDestroy), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);

	/* File menu */	
	widget = glade_xml_get_widget(xml, "RunCd");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnFile_RunCd), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "RunBios");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnFile_RunBios), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "RunExe");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnFile_RunExe), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "RunImage");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnFile_RunImage), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "SwitchImage");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnEmu_SwitchImage), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "exit2");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnFile_Exit), NULL, NULL, G_CONNECT_AFTER);

	/* States */
	widget = glade_xml_get_widget(xml, "GtkMenuItem_LoadSlot1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_load), (gpointer) 0, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_LoadSlot2");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_load), (gpointer) 1, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_LoadSlot3");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_load), (gpointer) 2, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_LoadSlot4");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_load), (gpointer) 3, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_LoadSlot5");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_load), (gpointer) 4, NULL, G_CONNECT_AFTER);	
	widget = glade_xml_get_widget(xml, "other1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_load_other), NULL, NULL, G_CONNECT_AFTER);			

	widget = glade_xml_get_widget(xml, "GtkMenuItem_SaveSlot1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_save), (gpointer) 0, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_SaveSlot2");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_save), (gpointer) 1, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_SaveSlot3");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_save), (gpointer) 2, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_SaveSlot4");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_save), (gpointer) 3, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "GtkMenuItem_SaveSlot5");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_save), (gpointer) 4, NULL, G_CONNECT_AFTER);	
	widget = glade_xml_get_widget(xml, "other2");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(on_states_save_other), NULL, NULL, G_CONNECT_AFTER);

	/* Emulation menu */
	widget = glade_xml_get_widget(xml, "run1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnEmu_Run), NULL, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "reset1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnEmu_Reset), NULL, NULL, G_CONNECT_AFTER);

	/* Configuration menu */
	widget = glade_xml_get_widget(xml, "plugins_bios");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(ConfigurePlugins), NULL, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "cpu1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnConf_Cpu), NULL, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "memory_cards1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnConf_Mcds), NULL, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "net1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnConf_Net), NULL, NULL, G_CONNECT_AFTER);

	/* Cheat menu */
	widget = glade_xml_get_widget(xml, "browse1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(RunCheatListDialog), NULL, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget(xml, "search1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(RunCheatSearchDialog), NULL, NULL, G_CONNECT_AFTER);

	/* Help menu */
	widget = glade_xml_get_widget(xml, "about_pcsx1");
	g_signal_connect_data(GTK_OBJECT(widget), "activate",
			GTK_SIGNAL_FUNC(OnHelp_About), NULL, NULL, G_CONNECT_AFTER);

	gtk_main();
}

void OnDestroy() {
	if (!destroy) OnFile_Exit();
}

void ConfigurePlugins() {
	if (!UseGui) {
		/* How do we get here if we're not running the GUI? */
		/* Ryan: we're going to imagine that someday, there will be a way
		 * to configure plugins from the commandline */
		printf("ERROR: Plugins cannot be configured without the GUI.");
		return;
	}

	GladeXML *xml;
	GtkWidget *widget;

	gchar *path;

	UpdatePluginsBIOS();

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "ConfDlg", NULL);

	if (!xml) {
		g_warning(_("Error: Glade interface could not be loaded!"));
		return;
	}

	UpdatePluginsBIOS_UpdateGUI(xml);

	ConfDlg = glade_xml_get_widget(xml, "ConfDlg");

	gtk_window_set_title(GTK_WINDOW(ConfDlg), _("Configure PCSX"));

	/* Set the paths in the file choosers to be based on the saved configurations */
	widget = glade_xml_get_widget(xml, "GtkFileChooser_Bios");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget), 
			Config.BiosDir);

	widget = glade_xml_get_widget(xml, "GtkFileChooser_Plugin");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget), 
			Config.PluginsDir);

	if (strlen(Config.PluginsDir) == 0) {
		if((path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget))) != NULL) {
			strcpy(Config.PluginsDir, path);
			g_free(path);
		}
	}

	widget = glade_xml_get_widget(xml, "btn_ConfGpu");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_configure_plugin), (gpointer) PSE_LT_GPU, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_ConfSpu");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_configure_plugin), (gpointer) PSE_LT_SPU, NULL, G_CONNECT_AFTER);

	/* ADB TODO Does pad 1 and 2 need to be different? */
	widget = glade_xml_get_widget(xml, "btn_ConfPad1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnConfConf_Pad1Conf), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_ConfPad2");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnConfConf_Pad2Conf), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_ConfCdr");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_configure_plugin), (gpointer) PSE_LT_CDR, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_AboutGpu");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_about_plugin), (gpointer) PSE_LT_GPU, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_AboutSpu");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_about_plugin), (gpointer) PSE_LT_SPU, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_AboutPad1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnConfConf_Pad1About), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_AboutPad2");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnConfConf_Pad2About), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_AboutCdr");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_about_plugin), (gpointer) PSE_LT_CDR, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkFileChooser_Bios");
	g_signal_connect_data(GTK_OBJECT(widget), "current_folder_changed",
			GTK_SIGNAL_FUNC(OnBiosPath_Changed), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkFileChooser_Plugin");
	g_signal_connect_data(GTK_OBJECT(widget), "current_folder_changed",
			GTK_SIGNAL_FUNC(OnPluginPath_Changed), xml, NULL, G_CONNECT_AFTER);

	g_signal_connect_data(GTK_OBJECT(ConfDlg), "response",
			GTK_SIGNAL_FUNC(OnConf_Clicked), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);
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
		file_chooser = gtk_file_chooser_dialog_new (_("Select PSX EXE File"),
				NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);

		/* Add file filter */
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

		/* Set this to the config object and retain it - maybe LastUsedDir */
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (file_chooser), getenv("HOME"));

		if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
			gchar *file;

			/* TODO Need to validate the file */

			file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

			gtk_widget_destroy (file_chooser);
			destroy_main_window();

			cdrfilename[0] = '\0';
			LoadPlugins();
			NetOpened = 0;

			if (OpenPlugins() == -1) {
				g_free(file);
				SysRunGui();
			} else {
				SysReset();

				if (Load(file) == 0) {
					g_free(file);
					if (Config.Dbg) hdb_start();
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

	cdrfilename[0] = '\0';
	LoadPlugins();
	NetOpened = 0;

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	SysReset();

	if (CheckCdrom() == -1) {
		/* Only check the CD if we are starting the console with a CD */
		ClosePlugins();
		SysErrorMessage (_("CD ROM failed"), _("The CD does not appear to be a valid Playstation CD"));
		SysRunGui();
		return;
	}

	/* Read main executable directly from CDRom and start it */
	if (LoadCdrom() == -1) {
		ClosePlugins();
		SysErrorMessage(_("Could not load CD-ROM!"), _("The CD ROM could not be loaded"));
		SysRunGui();
	}

	if (Config.Dbg) hdb_start();
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

	cdrfilename[0] = '\0';
	LoadPlugins();
	NetOpened = 0;

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	SysReset();

	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	if (Config.Dbg) hdb_start();
	psxCpu->Execute();
}

static gchar *Open_Iso_Proc() {
	GtkWidget *chooser;
	gchar *filename;
	GtkFileFilter *psxfilter, *allfilter;
	static char current_folder[MAXPATHLEN] = "";

	chooser = gtk_file_chooser_dialog_new (_("Open PSX Disc Image File"),
		NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);

	if (current_folder[0] == '\0') {
		strcpy(current_folder, getenv("HOME"));
	}

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (chooser), current_folder);

	psxfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(psxfilter, "*.bin");
	gtk_file_filter_add_pattern(psxfilter, "*.img");
	gtk_file_filter_add_pattern(psxfilter, "*.mdf");
	gtk_file_filter_add_pattern(psxfilter, "*.iso");
	gtk_file_filter_add_pattern(psxfilter, "*.BIN");
	gtk_file_filter_add_pattern(psxfilter, "*.IMG");
	gtk_file_filter_add_pattern(psxfilter, "*.MDF");
	gtk_file_filter_add_pattern(psxfilter, "*.ISO");
	gtk_file_filter_set_name(psxfilter, _("PSX Image Files (*.bin, *.img, *.mdf, *.iso)"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (chooser), psxfilter);

	allfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(allfilter, "*");
	gtk_file_filter_set_name(allfilter, _("All Files"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (chooser), allfilter);

	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK) {
		gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
		strcpy(current_folder, path);
		g_free(path);
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chooser));
		gtk_widget_destroy(GTK_WIDGET(chooser));
		while (gtk_events_pending()) gtk_main_iteration();
		return filename;
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

	strcpy(cdrfilename, filename);
	g_free(filename);

	LoadPlugins();
	NetOpened = 0;

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	SysReset();

	if (CheckCdrom() == -1) {
		/* Only check the CD if we are starting the console with a CD */
		ClosePlugins();
		SysErrorMessage (_("CD ROM failed"), _("The CD does not appear to be a valid Playstation CD"));
		SysRunGui();
		return;
	}

	/* Read main executable directly from CDRom and start it */
	if (LoadCdrom() == -1) {
		ClosePlugins();
		SysErrorMessage(_("Could not load CD-ROM!"), "The CD ROM could not be loaded");
		SysRunGui();
	}

	if (Config.Dbg) hdb_start();
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

	if (Config.Dbg) hdb_start();
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

	SysReset();

	if (CheckCdrom() != -1) {
		LoadCdrom();
	}

	if (Config.Dbg) hdb_start();
	psxCpu->Execute();
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

	strcpy(cdrfilename, filename);
	g_free(filename);

	if (OpenPlugins() == -1) {
		SysRunGui();
		return;
	}

	cdOpenCase = time(NULL) + 2;

	if (Config.Dbg) hdb_start();
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
	if (UseGui)
		gtk_exit (0);
	else
		exit(0);
}

void state_load (gchar *state_filename) {
	int ret;
	char Text[MAXPATHLEN+20];
	FILE *fp;

	/* check if the state file actually exists */
	fp = fopen(state_filename, "rb");
	if (fp == NULL) {
		/* file does not exist */
		return;
	}

	fclose(fp);

	/* If the window exists, then we are loading the state from within
	   within the PCSX GUI. We need to initialise the plugins first */
	if (Window) {
		destroy_main_window();

		if (OpenPlugins() == -1) {
			/* TODO Error message */
			SysRunGui();
			return;
		}
	}

	SysReset();

	ret = LoadState(state_filename);
	if (ret == 0) {
		/* Check the CD ROM is valid */
		if (CheckCdrom() == -1) {
			/* TODO Error message */
			ClosePlugins ();
			SysRunGui();
			return;
		}

#if 0 /* Whistler: this will cause crash when using the "Load Other" option */
		/* Check that the currently loaded CD ROM ID matches that of the CD
		   used when saving the state file. The latter is stored in the filename */
		gchar *cmp = g_strrstr (g_path_get_basename (state_filename), "-");
		cmp++;
		if (g_ascii_strncasecmp (cmp, CdromId, 9) != 0) {
			ClosePlugins ();
			gchar *error_desc, *label;
			gint pos;
			label = g_strdup_printf("%.9s", state_filename);
			error_desc = g_strdup_printf ("The Playstation CD that is currently in use is %s. It is not the same CD as that used when saving the state file. The state file is looking for %s.",
									label,
									CdromLabel);
			SysErrorMessage ("The CD does not match the state file",
							 error_desc);
			g_free (error_desc);
			g_free (label);
			SysRunGui();
			return;
		}
#endif
		sprintf(Text, _("Loaded state %s."), state_filename);
		GPU_displayText(Text);
		if (Config.Dbg) hdb_start();
		psxCpu->Execute();
	} else {
		sprintf(Text, _("Error loading state %s!"), state_filename);
		GPU_displayText(Text);
	}
}

void state_save (gchar *state_filename) {
	char Text[MAXPATHLEN + 20];

	GPU_updateLace();

	if (SaveState(state_filename) == 0)
		sprintf(Text, _("Saved state %s."), state_filename);
	else
		sprintf(Text, _("Error saving state %s!"), state_filename);

	GPU_displayText(Text);
}

void on_states_load (GtkWidget *widget, gpointer user_data) {
	gchar *state_filename;
	gint state = (int) user_data;

	state_filename = get_state_filename (state);

	state_load (state_filename);

	g_free (state_filename);
}

void on_states_save (GtkWidget *widget, gpointer user_data) {
	gchar *state_filename;
	gint state = (int) user_data;

	state_filename = get_state_filename (state);

	state_save (state_filename);

	g_free (state_filename);
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

	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
		gtk_widget_destroy(file_chooser);

		state_load(filename);

		g_free(filename);
	} else
		gtk_widget_destroy(file_chooser);

	g_free(SStateFile);
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
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), SStateFile);

	if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_OK) {
		gchar *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
		gtk_widget_destroy(file_chooser);

		state_save (filename);

		g_free (filename);
	}
	else
		gtk_widget_destroy (file_chooser);

	g_free (SStateFile);
} 

int all_config_set () {
	int retval;

	if ((strlen(Config.Gpu) != 0) &&
	    (strlen(Config.Spu) != 0) &&
	    (strlen(Config.Cdr) != 0) &&
	    (strlen(Config.Pad1) != 0) &&
	    (strlen(Config.Pad2) != 0))
		retval = TRUE;
	else
		retval = FALSE;

	return retval;
}

GtkWidget *NetDlg;

void OnNet_Clicked (GtkDialog *dialog, gint arg1, gpointer user_data) {
	if (arg1 == GTK_RESPONSE_OK) {
		GetComboText(NetConfS.Combo, NetConfS.plist, Config.Net);
		SaveConfig();
	}

	gtk_widget_destroy(GTK_WIDGET (dialog));
	NetDlg = NULL;
}

void OnConf_Net() {
	GladeXML *xml;
	GtkWidget *widget;

	if (NetDlg != NULL) {
		gtk_window_present (GTK_WINDOW (NetDlg));
		return;
	}

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "NetDlg", NULL);

	if (!xml) {
		g_warning(_("Error: Glade interface could not be loaded!"));
		return;
	}

	NetDlg = glade_xml_get_widget(xml, "NetDlg");

	FindNetPlugin(xml);

	/* Setup a handler for when Close or Cancel is clicked */
	g_signal_connect_data(GTK_OBJECT(NetDlg), "response",
			GTK_SIGNAL_FUNC(OnNet_Clicked), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_ConfNet");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnNet_Conf), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_AboutNet");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnNet_About), xml, NULL, G_CONNECT_AFTER);
}

McdBlock Blocks[2][MAX_MEMCARD_BLOCKS];	/* Assuming 2 cards, 15 blocks? */
int IconC[2][MAX_MEMCARD_BLOCKS];
enum {
    CL_ICON,
    CL_TITLE,
    CL_STAT,
    CL_ID,
    CL_NAME,
    NUM_CL
};

GtkWidget *GtkCList_McdList1, *GtkCList_McdList2;
GtkTreeSelection *sel1, *sel2;
gint mcd1_row, mcd2_row;

static void add_columns (GtkTreeView *treeview) {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* column for icon */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Icon"),
			renderer, "pixbuf", CL_ICON, NULL);
	gtk_tree_view_append_column (treeview, column);

	/* column for title */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Title"),
			renderer, "text", CL_TITLE, NULL);
	gtk_tree_view_append_column (treeview, column);

	/* column for status */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Status"),
			renderer, "text", CL_STAT, NULL);
	gtk_tree_view_append_column (treeview, column);

	/* column for id */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("ID"),
			renderer, "text", CL_ID, NULL);
	gtk_tree_view_append_column (treeview, column);

	/* column for Name */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
			renderer, "text", CL_NAME, NULL);
	gtk_tree_view_append_column (treeview, column);
}

GdkPixbuf *SetIcon(GtkWidget *dialog, short *icon, int i) {
	GdkPixmap *pixmap;
	GdkImage  *image;
	GdkVisual *visual;
	GdkGC     *gc;
	int x, y, c;

	visual = gdk_window_get_visual(dialog->window);

	if (visual->depth == 8) return NULL;

	image = gdk_image_new(GDK_IMAGE_NORMAL, visual, 16, 16);

	for (y=0; y<16; y++) {
		for (x=0; x<16; x++) {
			c = icon[y*16+x];
			c = ((c&0x001f) << 10) | ((c&0x7c00) >> 10) | (c&0x03e0);
			if (visual->depth == 16)
				c = (c&0x001f) | ((c&0x7c00) << 1) | ((c&0x03e0) << 1);
			else if (visual->depth == 24 || visual->depth == 32)
				c = ((c&0x001f) << 3) | ((c&0x03e0) << 6) | ((c&0x7c00) << 9);
				
			gdk_image_put_pixel(image, x, y, c);
		}
	}

	pixmap = gdk_pixmap_new(dialog->window, 16, 16, visual->depth);

	gc = gdk_gc_new(pixmap);
	gdk_draw_image(pixmap, gc, image, 0, 0, 0, 0, 16, 16);
	gdk_gc_destroy(gc);
	gdk_image_destroy(image);
	
	return gdk_pixbuf_get_from_drawable    (NULL,
	                                        GDK_PIXMAP (pixmap),
		                                NULL,
						0,0,0,0,-1,-1);
}

void LoadListItems(int mcd, GtkWidget *List, GtkWidget *dialog) {
	int i;

	GtkListStore *store= gtk_list_store_new (NUM_CL,GDK_TYPE_PIXBUF, G_TYPE_STRING,
					     G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		McdBlock *Info;
		gchar *state;

		Info = &Blocks[mcd-1][i];
		IconC[mcd-1][i] = 0;

		if ((Info->Flags & 0xF0) == 0xA0) {
			if ((Info->Flags & 0xF) >= 1 &&
				(Info->Flags & 0xF) <= 3) {
				state = _("Deleted");
			} else
				state = _("Free");
		} else if ((Info->Flags & 0xF0) == 0x50)
			state = _("Used");
		else
			state = _("Free");

//		if (Info->IconCount == 0) continue;

		pixbuf = SetIcon(dialog, Info->Icon, i+1);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				CL_ICON, pixbuf,
				CL_TITLE, Info->Title,
				CL_STAT, state,
				CL_NAME, Info->Name,
				CL_ID, Info->ID,
				-1);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (List), GTK_TREE_MODEL (store));
	g_object_unref (G_OBJECT (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (List), TRUE);
	gtk_widget_show(List);
}

void UpdateMcdDlg(GtkWidget *widget) {
	int i;
	GladeXML *xml;
	GtkWidget *list1, *list2, *dialog;

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		GetMcdBlockInfo(1, i+1, &Blocks[0][i]);
		GetMcdBlockInfo(2, i+1, &Blocks[1][i]);
	}

	xml = glade_get_widget_tree (widget);
	list1 = glade_xml_get_widget (xml, "GtkCList_McdList1");
	list2 = glade_xml_get_widget (xml, "GtkCList_McdList2");
	dialog = glade_xml_get_widget (xml, "McdsDlg");
	LoadListItems(1, list1, dialog);
	LoadListItems(2, list2, dialog);
}

void OnMcd_Clicked (GtkDialog *dialog, gint arg1, gpointer user_data) {
	GladeXML *xml = user_data;

	if (arg1 == GTK_RESPONSE_CLOSE) {
		gchar *tmp;
		GtkWidget *widget;

		widget = glade_xml_get_widget (xml, "GtkMcd1FSButton");
		if ((tmp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget))) != NULL) {
			strcpy(Config.Mcd1, tmp);
			g_free (tmp);
		}

		widget = glade_xml_get_widget (xml, "GtkMcd2FSButton");
		if ((tmp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget))) != NULL) {
			strcpy(Config.Mcd2, tmp);
			g_free (tmp);
		}

		SaveConfig();
	} 

	LoadMcds(Config.Mcd1, Config.Mcd2);
	gtk_widget_destroy(GTK_WIDGET (dialog));
}

void on_memcard_file_changed (GtkWidget *widget, gpointer user_data) {
	gint memcard = (int) user_data;
	gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));

	if (filename != NULL) {
		LoadMcd(memcard, filename);
		UpdateMcdDlg(widget);
		gtk_widget_set_sensitive (widget, TRUE);
	}

	g_free (filename);
}

void OnMcd_Format(GtkWidget *widget, gpointer user_data) {
	GladeXML *xml;
	GtkWidget *memcard_fs;
	GtkWidget *message_dialog;
	gint result;
	gchar *str;

	gint memcard = (int) user_data;

	/* TODO Check if the memory card actually has data on it before displaying this */

	message_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		_("Format this Memory Card?"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message_dialog),
		_("If you format the memory card, the card will be empty, and any existing data overwritten."));
	gtk_dialog_add_buttons (GTK_DIALOG (message_dialog),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		_("Format card"), GTK_RESPONSE_YES,
		NULL);

	result = gtk_dialog_run (GTK_DIALOG (message_dialog));
	gtk_widget_destroy (message_dialog);

	if (result == GTK_RESPONSE_YES) {
		xml = glade_get_widget_tree (widget);
		if (memcard == 1)
			memcard_fs = glade_xml_get_widget (xml, "GtkMcd1FSButton");
		else
			memcard_fs = glade_xml_get_widget (xml, "GtkMcd2FSButton");

		str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (memcard_fs));

		CreateMcd(str);
		LoadMcd(memcard, str);
		UpdateMcdDlg(widget);
		g_free (str);
	}
}

void OnMcd_Reload (GtkWidget *widget, gpointer user_data) {
	GladeXML *xml;
	GtkWidget *memcard_fs;
	gchar *str;

	gint memcard = (int) user_data;
	
	xml = glade_get_widget_tree (widget);

	if (memcard == 1) {
		memcard_fs = glade_xml_get_widget (xml, "GtkMcd1FSButton");
		str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (memcard_fs));
	} else {
		memcard_fs = glade_xml_get_widget (xml, "GtkMcd2FSButton");
		str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (memcard_fs));
	}

	LoadMcd (memcard, str);
	UpdateMcdDlg(widget);

	g_free (str);
}

static int copy = 0, copymcd = 0;

static int get_free_memcard_slot (int target_card) {
	McdBlock *Info;
	gboolean found = FALSE;

	int i = 0;
	while (i < 15  && found == FALSE) {
		Info = &Blocks[target_card][i];
		if (g_ascii_strcasecmp (Info->Title, "") == 0) {
			found = TRUE;
		} else {
			i++;
		}
	}

	if (found == TRUE)
		return i;

	// no free slots, try to find a deleted one
	i = 0;
	while (i < 15  && found == FALSE) {
		Info = &Blocks[target_card][i];
		if ((Info->Flags & 0xF0) != 0x50) {
			found = TRUE;
		} else {
			i++;
		}
	}

	if (found == TRUE)
		return i;

	return -1;
}

void OnMcd_CopyTo(GtkWidget *widget, gpointer user_data) {
	gint mcd = (gint)user_data;

	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	gint *i;
	GladeXML *xml;
	GtkTreeSelection *treesel;
	gchar *str;
	char *source, *destination;

	int first_free_slot;

	xml = glade_get_widget_tree(widget);

	if (mcd == 1)
		treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(GtkCList_McdList1));
	else
		treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(GtkCList_McdList2));

	/* If the item selected is not reported as a 'Free' slot */
	if (gtk_tree_selection_get_selected(treesel, &model, &iter)) {
		path = gtk_tree_model_get_path(model, &iter);
		i = gtk_tree_path_get_indices(path);
		copy    = *i;
		copymcd = mcd;
		gtk_tree_path_free(path);
	}

	/* Determine the first free slot in the target memory card */
	first_free_slot = get_free_memcard_slot(mcd - 1);
	if (first_free_slot == -1) {
		/* No free slots available on the destination card */
		SysMessage(_("No space available in the target memory card!"));
		return;
	}
	xml = glade_get_widget_tree (GtkCList_McdList1);

	if (mcd == 1) {
		str = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(glade_xml_get_widget (xml, "GtkMcd1FSButton")));
		source = Mcd2Data;
		copy = mcd2_row;
		destination = Mcd1Data;
//		printf("Copying from card 2 to card 1 from slot %d into slot %d\n", mcd2_row, first_free_slot);
	} else {
		str = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(glade_xml_get_widget (xml, "GtkMcd2FSButton")));
		source = Mcd1Data;
		copy = mcd1_row;
		destination = Mcd2Data;
//		printf("Copying from card 1 to card 2 from slot %d into slot %d\n", mcd1_row, first_free_slot);
	}

	copy_memcard_data (source, destination, &first_free_slot, str);
	UpdateMcdDlg(widget);
}

void copy_memcard_data (char *from, char *to, gint *i, gchar *str) {
	memcpy(to + (*i+1) * 128, from + (copy+1) * 128, 128);
	SaveMcd((char*)str, to, (*i+1) * 128, 128);
	memcpy(to + (*i+1) * 1024 * 8, from + (copy+1) * 1024 * 8, 1024 * 8);
	SaveMcd((char*)str, to, (*i+1) * 1024 * 8, 1024 * 8);
}

void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data) {
	GladeXML *xml;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	
	gboolean selected;
	int i;
	char *data;

	gint memcard = (int) user_data;

	if (memcard == 1) {
		selected = gtk_tree_selection_get_selected (selection, &model, &iter);
		data = Mcd1Data;
	} else {
		selected = gtk_tree_selection_get_selected (selection, &model, &iter);
		data = Mcd2Data;
	}

	if (selected) {
		path = gtk_tree_model_get_path (model, &iter);
		i = *gtk_tree_path_get_indices (path);

		/* If a row was selected, and the row is not blank, we can now enable
		   some of the disabled widgets */
		/* Check LoadListItems for determining state */
		xml = glade_get_widget_tree (GtkCList_McdList1);
		GtkWidget *disable, *enable;
		if (memcard == 1) {
			mcd1_row = i;
			enable = glade_xml_get_widget (xml, "GtkButton_CopyTo2");
			disable = glade_xml_get_widget (xml, "GtkButton_CopyTo1");
		} else {
			mcd2_row = i;
			enable = glade_xml_get_widget (xml, "GtkButton_CopyTo1");
			disable = glade_xml_get_widget (xml, "GtkButton_CopyTo2");
		}
		gtk_widget_set_sensitive (enable, TRUE);
		gtk_widget_set_sensitive (disable, FALSE);
	}
}

void on_memcard_delete (GtkWidget *widget, gpointer user_data) {
	McdBlock *Info;
	int i, xor = 0, j;
	char *data, *ptr;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	gchar *filename;
	GladeXML *xml;
	gboolean selected;
	GtkWidget *tree;
	GtkTreeSelection *sel;

	gint memcard = (int) user_data;

	xml = glade_get_widget_tree (widget);
	if (memcard == 1) {
		tree = glade_xml_get_widget (xml, "GtkCList_McdList1");
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
		selected = gtk_tree_selection_get_selected (sel, &model, &iter);
		data = Mcd1Data;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (glade_xml_get_widget (xml, "GtkMcd1FSButton")));
	} else {
		tree = glade_xml_get_widget (xml, "GtkCList_McdList2");
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
		selected = gtk_tree_selection_get_selected (sel, &model, &iter);
		data = Mcd2Data;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (glade_xml_get_widget (xml, "GtkMcd2FSButton")));
	}
//	printf("Will delete from memory card %s\n", filename);
	if (selected) {
		path = gtk_tree_model_get_path (model, &iter);
		i = *gtk_tree_path_get_indices (path);

		i++;
		ptr = data + i * 128;
		Info = &Blocks[memcard-1][i-1];

		if ((Info->Flags & 0xF0) == 0xA0) {
			if ((Info->Flags & 0xF) >= 1 &&
				(Info->Flags & 0xF) <= 3) { // deleted
				*ptr = 0x50 | (Info->Flags & 0xF);
			} else return;
		} else if ((Info->Flags & 0xF0) == 0x50) { // used
				*ptr = 0xA0 | (Info->Flags & 0xF);
		} else { return; }

		for (j=0; j<127; j++) xor^=*ptr++;
		*ptr = xor;

		SaveMcd((char*)filename, data, i * 128, 128);
		UpdateMcdDlg(widget);
	}
}

void OnConf_Mcds() {
	GladeXML *xml;
	GtkWidget *dialog;
	GtkWidget *widget;
	GtkTreeSelection *treesel1, *treesel2;
	gchar *str;
	int i;

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "McdsDlg", NULL);

	if (!xml) {
		g_warning("We could not load the interface!");
		return;
	}

	dialog = glade_xml_get_widget(xml, "McdsDlg");

	gtk_window_set_title(GTK_WINDOW(dialog), _("Memory Card Manager"));

	/* Assign default memory cards */
	if (!strlen(Config.Mcd1)) {
		str = g_strconcat (getenv("HOME"), DEFAULT_MEM_CARD_1, NULL);
		strcpy(Config.Mcd1, str);
		g_free (str);
	}

	if (!strlen(Config.Mcd2)) {
		str = g_strconcat (getenv("HOME"), DEFAULT_MEM_CARD_2, NULL);
		strcpy(Config.Mcd2, str);
		g_free (str);
	}

	GtkCList_McdList1 = glade_xml_get_widget(xml, "GtkCList_McdList1");
	add_columns(GTK_TREE_VIEW (GtkCList_McdList1));
	GtkCList_McdList2 = glade_xml_get_widget(xml, "GtkCList_McdList2");
	add_columns(GTK_TREE_VIEW (GtkCList_McdList2));

	treesel1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (GtkCList_McdList1));
	gtk_tree_selection_set_mode (treesel1, GTK_SELECTION_SINGLE);
	g_signal_connect_data (G_OBJECT (treesel1), "changed",
						   G_CALLBACK (tree_selection_changed_cb),
						   (gpointer) 1, NULL,
						   G_CONNECT_AFTER);

	treesel2 = gtk_tree_view_get_selection (GTK_TREE_VIEW (GtkCList_McdList2));
	gtk_tree_selection_set_mode (treesel2, GTK_SELECTION_SINGLE);
	g_signal_connect_data (G_OBJECT (treesel2), "changed",
						   G_CALLBACK (tree_selection_changed_cb),
						   (gpointer) 2, NULL,
						   G_CONNECT_AFTER);

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		GetMcdBlockInfo (1, i+1, &Blocks[0][i]);
		GetMcdBlockInfo (2, i+1, &Blocks[1][i]);
	}

	LoadListItems(1, GtkCList_McdList1, dialog);
	LoadListItems(2, GtkCList_McdList2, dialog);

	/* Setup a handler for when Close or Cancel is clicked */
	g_signal_connect_data(GTK_OBJECT(dialog), "response",
			GTK_SIGNAL_FUNC(OnMcd_Clicked), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkButton_Format1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnMcd_Format), (gpointer) 1, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkButton_Format2");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnMcd_Format), (gpointer) 2, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkMcd1FSButton");
	g_signal_connect_data(GTK_OBJECT(widget), "selection-changed",
			GTK_SIGNAL_FUNC(on_memcard_file_changed), (gpointer) 1, NULL, G_CONNECT_AFTER);
	gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (widget), Config.Mcd1);

	widget = glade_xml_get_widget(xml, "GtkMcd2FSButton");
	g_signal_connect_data(GTK_OBJECT(widget), "selection-changed",
			GTK_SIGNAL_FUNC(on_memcard_file_changed), (gpointer) 2, NULL, G_CONNECT_AFTER);
	gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (widget), Config.Mcd2);

	widget = glade_xml_get_widget(xml, "GtkButton_Reload1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnMcd_Reload), (gpointer) 1, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkButton_Reload2");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnMcd_Reload), (gpointer) 2, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkButton_CopyTo1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnMcd_CopyTo), (gpointer) 1, NULL, G_CONNECT_AFTER);
	gtk_widget_set_sensitive (GTK_WIDGET (widget), FALSE);

	widget = glade_xml_get_widget(xml, "GtkButton_CopyTo2");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnMcd_CopyTo), (gpointer) 2, NULL, G_CONNECT_AFTER);
	gtk_widget_set_sensitive (GTK_WIDGET (widget), FALSE);

	widget = glade_xml_get_widget(xml, "GtkButton_Delete1");
	g_signal_connect_data (GTK_OBJECT (widget), "clicked",
			GTK_SIGNAL_FUNC (on_memcard_delete), (gpointer) 1, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "GtkButton_Delete2");
	g_signal_connect_data (GTK_OBJECT (widget), "clicked",
			GTK_SIGNAL_FUNC (on_memcard_delete), (gpointer) 2, NULL, G_CONNECT_AFTER);

	while (gtk_events_pending()) gtk_main_iteration();
}

GtkWidget *CpuDlg;
GtkWidget *PsxCombo;
GList *psxglist;
char *psxtypes[] = {
	"NTSC",
	"PAL"
};

/* When the auto-detect CPU type is selected, disable the NTSC/PAL selection */
static void OnCpu_PsxAutoClicked (GtkWidget *widget, gpointer user_data) {
	GtkWidget *combo;
	GladeXML *xml = user_data;
	combo = glade_xml_get_widget(xml, "GtkCombo_PsxType");

	gtk_widget_set_sensitive (combo,
			!(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))));
}

/* When the interpreter core is deselected, disable the debugger checkbox */
static void OnCpu_CpuClicked (GtkWidget *widget, gpointer user_data) {
	GtkWidget *check;
	GladeXML *xml = user_data;
	check = glade_xml_get_widget(xml, "GtkCheckButton_Dbg");

	// Debugger is only working with interpreter not recompiler, so let's set it
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), FALSE);

	gtk_widget_set_sensitive (check,
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

void OnCpu_Clicked (GtkDialog *dialog, gint arg1, gpointer user_data) {
	if (arg1 == GTK_RESPONSE_CLOSE) {
		GtkWidget *widget;
		GladeXML *xml = user_data;
		int tmp;
		long t;

		widget = glade_xml_get_widget(xml, "GtkCombo_PsxType");

		/* If nothing chosen, default to NTSC */
		tmp = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
		if (tmp == -1)	
			tmp = PSX_TYPE_NTSC;
		
		if (!strcmp("NTSC",psxtypes[tmp]))
			Config.PsxType = PSX_TYPE_NTSC;
		else
			Config.PsxType = PSX_TYPE_PAL;

		Config.Xa = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_Xa")));

		Config.Sio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_Sio")));

		Config.Mdec = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_Mdec")));

		Config.Cdda = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_CDDA")));

		Config.PsxAuto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_PsxAuto")));

		Config.Dbg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_Dbg")));

		t = Config.Cpu;
		Config.Cpu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_Cpu")));
		if (t != Config.Cpu) {
			psxCpu->Shutdown();
#ifdef PSXREC
			if (Config.Cpu) {
				if (Config.Dbg) psxCpu = &psxIntDbg;
				else psxCpu = &psxInt;
			}
			else psxCpu = &psxRec;
#else
			if (Config.Dbg) psxCpu = &psxIntDbg;
			else psxCpu = &psxInt;
#endif
			if (psxCpu->Init() == -1) {
				SysClose();
				exit(1);
			}
			psxCpu->Reset();
		}

		Config.PsxOut = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_PsxOut")));

		Config.SpuIrq = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_SpuIrq")));

		Config.RCntFix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_RCntFix")));

		Config.VSyncWA = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml, "GtkCheckButton_VSyncWA")));

		SaveConfig();
	} 

	gtk_widget_destroy(CpuDlg);
	CpuDlg = NULL;
}

void OnConf_Cpu() {
	GladeXML *xml;

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "CpuDlg", NULL);

	if (!xml) {
		g_warning("We could not load the interface!");
		return;
	}

	CpuDlg = glade_xml_get_widget(xml, "CpuDlg");

	PsxCombo = glade_xml_get_widget(xml, "GtkCombo_PsxType");
	gtk_combo_box_set_active (GTK_COMBO_BOX (PsxCombo), Config.PsxType);
	gtk_widget_set_sensitive (GTK_WIDGET (PsxCombo), !Config.PsxAuto);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_Xa")), Config.Xa);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_Sio")), Config.Sio);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_Mdec")), Config.Mdec);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_CDDA")), Config.Cdda);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_PsxAuto")), Config.PsxAuto);

	g_signal_connect_data(GTK_OBJECT(glade_xml_get_widget(xml, "GtkCheckButton_PsxAuto")), "toggled",
			GTK_SIGNAL_FUNC(OnCpu_PsxAutoClicked), xml, NULL, G_CONNECT_AFTER);

#ifdef PSXREC
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_Cpu")), Config.Cpu);

	g_signal_connect_data(GTK_OBJECT(glade_xml_get_widget(xml, "GtkCheckButton_Cpu")), "toggled",
			GTK_SIGNAL_FUNC(OnCpu_CpuClicked), xml, NULL, G_CONNECT_AFTER);
#else
	Config.Cpu = 1;

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_Cpu")), TRUE);
	gtk_widget_set_sensitive (GTK_WIDGET (glade_xml_get_widget(xml, "GtkCheckButton_Cpu")), FALSE);
#endif

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_Dbg")), Config.Cpu && Config.Dbg);
	gtk_widget_set_sensitive (GTK_WIDGET (glade_xml_get_widget(xml, "GtkCheckButton_Dbg")), Config.Cpu);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_PsxOut")), Config.PsxOut);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_SpuIrq")), Config.SpuIrq);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_RCntFix")), Config.RCntFix);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "GtkCheckButton_VSyncWA")), Config.VSyncWA);

	/* Setup a handler for when Close or Cancel is clicked */
	g_signal_connect_data(GTK_OBJECT(CpuDlg), "response",
			GTK_SIGNAL_FUNC(OnCpu_Clicked), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);
}

/* TODO Check whether configuration is required when we choose the plugin, and set the state of the
    button appropriately. New gtk tooltip API should allow us to put a tooltip explanation for
    disabled widgets */
/* TODO If combo screen hasn't been opened and the user chooses the menu config option, confs.Combo will be null and cause a segfault */
#define ConfPlugin(src, confs, plugin, name, parent)  { \
	void *drv; \
	src conf; \
	gchar *filename; \
 \
	GetComboText(confs.Combo, confs.plist, plugin); \
	filename = g_build_filename (getenv("HOME"), PLUGINS_DIR, plugin, NULL); \
	/*printf("Configuring plugin %s\n", filename);*/ \
	drv = SysLoadLibrary(filename); \
	if (drv == NULL) {printf("Error with file %s\n", filename);return; } \
\
	while (gtk_events_pending()) gtk_main_iteration(); \
	conf = (src) SysLoadSym(drv, name); \
	if (conf) { \
		conf(); \
	} else \
		SysInfoMessage (_("No configuration required"), _("This plugin doesn't need to be configured.")); \
	SysCloseLibrary(drv); \
	g_free (filename); \
}

void on_configure_plugin (GtkWidget *widget, gpointer user_data) {
	gint plugin_type = (int) user_data;
//	printf("Configuring plugin - %d\n", plugin_type);

	while (gtk_events_pending())
		gtk_main_iteration();
	if (all_config_set() == TRUE) {
		switch (plugin_type) {
			case PSE_LT_GPU:
				ConfPlugin(GPUconfigure, GpuConfS, Config.Gpu, "GPUconfigure", ConfDlg);
				break;
			case PSE_LT_SPU:
				ConfPlugin(SPUconfigure, SpuConfS, Config.Spu, "SPUconfigure", ConfDlg);
				break;
			case PSE_LT_CDR:
				ConfPlugin(CDRconfigure, CdrConfS, Config.Cdr, "CDRconfigure", ConfDlg);
				break;
		}
	} else
		ConfigurePlugins();
}

void on_about_plugin (GtkWidget *widget, gpointer user_data) {
	gint plugin_type = (int) user_data;
//	printf("About plugin - %d\n", plugin_type);
	while (gtk_events_pending())
		gtk_main_iteration();
	if (all_config_set() == TRUE) {
		switch (plugin_type) {
			case PSE_LT_GPU:
				ConfPlugin(GPUconfigure, GpuConfS, Config.Gpu, "GPUabout", ConfDlg);
				break;
			case PSE_LT_SPU:
				ConfPlugin(SPUconfigure, SpuConfS, Config.Spu, "SPUabout", ConfDlg);
				break;
			case PSE_LT_CDR:
				ConfPlugin(CDRconfigure, CdrConfS, Config.Cdr, "CDRabout", ConfDlg);
				break;
		}
	} else
		ConfigurePlugins();
}

void OnConfConf_Pad1About(GtkWidget *widget, gpointer user_data) {
	ConfPlugin(PADabout, Pad1ConfS, Config.Pad1, "PADabout", ConfDlg);
}

void OnConfConf_Pad2About(GtkWidget *widget, gpointer user_data) {
	ConfPlugin(PADabout, Pad2ConfS, Config.Pad2, "PADabout", ConfDlg);
}

void OnConfConf_Pad1Conf(GtkWidget *widget, gpointer user_data) {
	ConfPlugin(PADabout, Pad1ConfS, Config.Pad1, "PADconfigure", ConfDlg);
}

void OnConfConf_Pad2Conf(GtkWidget *widget, gpointer user_data) {
	ConfPlugin(PADabout, Pad2ConfS, Config.Pad2, "PADconfigure", ConfDlg);
}

void OnNet_Conf(GtkWidget *widget, gpointer user_data) {
	ConfPlugin(NETconfigure, NetConfS, Config.Net, "NETconfigure", NetDlg);
}

void OnNet_About(GtkWidget *widget, gpointer user_data) {
	ConfPlugin(NETabout, NetConfS, Config.Net, "NETabout", NetDlg);
}

void OnPluginPath_Changed(GtkWidget *wdg, gpointer data) {
	gchar *path;

	path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (wdg));
	strcpy(Config.PluginsDir, path);
	UpdatePluginsBIOS();
	UpdatePluginsBIOS_UpdateGUI(data);

	g_free (path);
}

void OnBiosPath_Changed(GtkWidget *wdg, gpointer data) {
	gchar *foldername;

	foldername = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (wdg));
	strcpy(Config.BiosDir, foldername);

//	printf("BIOS directory is now %s\n", foldername);

	UpdatePluginsBIOS();
	UpdatePluginsBIOS_UpdateGUI(data);

	g_free (foldername);
}

void OnConf_Clicked (GtkDialog *dialog, gint arg1, gpointer user_data) {
	if (arg1 == GTK_RESPONSE_OK) {
		GetComboText(GpuConfS.Combo, GpuConfS.plist, Config.Gpu);
		GetComboText(SpuConfS.Combo, SpuConfS.plist, Config.Spu);
		GetComboText(CdrConfS.Combo, CdrConfS.plist, Config.Cdr);
		GetComboText(Pad1ConfS.Combo, Pad1ConfS.plist, Config.Pad1);
		GetComboText(Pad2ConfS.Combo, Pad2ConfS.plist, Config.Pad2);
		GetComboText(BiosConfS.Combo, BiosConfS.plist, Config.Bios);
		/* TODO Validation */

		SaveConfig();
	} 

	gtk_widget_destroy (ConfDlg);
	ConfDlg = NULL;
}

void OnHelp_About(GtkWidget *widget, gpointer user_data) {
	GladeXML *xml;
	GtkWidget *about_dialog;
	
	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "AboutDlg", NULL);

	if (!xml) {
		g_warning("We could not load the interface!");
		return;
	}

	about_dialog = glade_xml_get_widget(xml, "AboutDlg");
	
	gtk_dialog_run (GTK_DIALOG (about_dialog));
	gtk_widget_destroy (about_dialog);
}

#define ComboAddPlugin(type) { \
	type##ConfS.plugins += 2; \
	strcpy(type##ConfS.plist[type##ConfS.plugins - 1], name); \
	strcpy(type##ConfS.plist[type##ConfS.plugins - 2], ent->d_name); \
	type##ConfS.glist = g_list_append(type##ConfS.glist, type##ConfS.plist[type##ConfS.plugins-1]); \
}

void populate_combo_box (GtkWidget *widget, GList *list) {
	GtkListStore *store;
	GtkCellRenderer *renderer;
	store = gtk_list_store_new (1, G_TYPE_STRING);

	/* Clear existing data from combo box */
	gtk_cell_layout_clear (GTK_CELL_LAYOUT (widget));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, "text", 0);

	while (list != NULL) {
		GtkTreeIter iter;
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, (char *)list->data, -1);
		list = list->next;
	}

	gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));
}

#define ConfCreatePConf(name, type) \
	/* Populate the relevant combo widget with the list of plugins. \
	   If no plugins available, disable the combo and its controls. \
	   Note that the Bios plugin has no About/Conf control. */ \
	type##ConfS.Combo = glade_xml_get_widget(xml, "GtkCombo_" name); \
	if (type##ConfS.glist != NULL) { \
		populate_combo_box (type##ConfS.Combo, type##ConfS.glist); \
		FindComboText(type##ConfS.Combo, type##ConfS.plist, Config.type); \
		gtk_widget_set_sensitive (type##ConfS.Combo, TRUE); \
		if (g_ascii_strcasecmp (name, "Bios") != 0) { \
			controlwidget = glade_xml_get_widget(xml, "btn_Conf" name); \
			gtk_widget_set_sensitive (controlwidget, TRUE); \
			controlwidget = glade_xml_get_widget(xml, "btn_About" name); \
			gtk_widget_set_sensitive (controlwidget, TRUE); \
		} \
	} else { \
		if (g_ascii_strcasecmp (name, "Bios") != 0) { \
			gtk_cell_layout_clear (GTK_CELL_LAYOUT (type##ConfS.Combo)); \
			gtk_widget_set_sensitive (type##ConfS.Combo, FALSE); \
			controlwidget = glade_xml_get_widget(xml, "btn_Conf" name); \
			gtk_widget_set_sensitive (controlwidget, FALSE); \
			controlwidget = glade_xml_get_widget(xml, "btn_About" name); \
			gtk_widget_set_sensitive (controlwidget, FALSE); \
		} \
	}

int plugin_is_available (gchar *plugin) {
	int retval;
	gchar *pluginfile;
	struct stat stbuf;

//	printf("Checking plugin_is_available - %s\n", plugin);
	pluginfile = g_strconcat (getenv("HOME"), PLUGINS_DIR, plugin, NULL);

	if (stat(pluginfile, &stbuf) == -1)
		retval = FALSE;
	else
		retval = TRUE;

	g_free (pluginfile);

	return retval;
}

/* TODO Combine this with all_config_set() */
int plugins_configured() {
	// make sure there are choices for all of the plugins!!
//	if ((strlen(Config.Gpu) == 0) || (strlen(Config.Spu) == 0) || (strlen(Config.Cdr) == 0) || (strlen(Config.Pad1) == 0) || (strlen(Config.Pad2) == 0)) {
	if (all_config_set() == FALSE)
		return FALSE;
//	}
	// and make sure they can all be accessed
	// if they can't be, wipe the variable and return FALSE
	if (plugin_is_available (Config.Gpu) == FALSE) { Config.Gpu[0] = '\0'; return FALSE; }
	if (plugin_is_available (Config.Spu) == FALSE) { Config.Spu[0] = '\0'; return FALSE; }
	if (plugin_is_available (Config.Cdr) == FALSE) { Config.Cdr[0] = '\0'; return FALSE; }
	if (plugin_is_available (Config.Pad1) == FALSE) { Config.Pad1[0] = '\0'; return FALSE; }
	if (plugin_is_available (Config.Pad2) == FALSE) { Config.Pad2[0] = '\0'; return FALSE; }

	// if everything is happy, return TRUE
	return TRUE;
}

int is_valid_bios_file (gchar *filename) {
	int valid;
	struct stat buf;

//	printf("  Checking is_valid_bios_file - %s\n", filename);
	if ((stat(filename, &buf) == -1) || (buf.st_size != (1024*512)))
		valid = FALSE;
	else {
		valid = TRUE;
//		printf("    %s is a valid BIOS file\n", filename);
	}

	return valid;
}

/* Add the name of the BIOS file to the drop-down list. This will
   be the filename, not the full path to the file */
void add_bios_to_list (gchar *bios_name, gchar *internal_name) {
	BiosConfS.plugins+=2;
	strcpy(BiosConfS.plist[BiosConfS.plugins-1], bios_name);
	strcpy(BiosConfS.plist[BiosConfS.plugins-2], internal_name);
	/* Error handling - BIOS
	strcpy(BiosConfS.plist[BiosConfS.plugins-1], internal_name);
	strcpy(BiosConfS.plist[BiosConfS.plugins-2], bios_name);*/
	BiosConfS.glist = g_list_append(BiosConfS.glist, BiosConfS.plist[BiosConfS.plugins-1]);
}

void scan_bios_dir (gchar *dirname) {
	DIR *dir;
	struct dirent *ent;
	gchar *filename;

//	printf("Scanning bios dir %s\n", dirname);

	dir = opendir(dirname);
	if (dir == NULL) {
		SysMessage(_("Could not open BIOS directory: '%s'\n"), dirname);
		return;
	}

	while ((ent = readdir(dir)) != NULL) {
		filename = g_build_filename(dirname, ent->d_name, NULL);
		if (is_valid_bios_file(filename))
			add_bios_to_list(g_path_get_basename(filename), g_path_get_basename (filename));
		g_free(filename);
	}
	closedir(dir);

//	printf("Finished scanning bios dir %s\n", dirname);
}

void UpdatePluginsBIOS() {
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	char name[256];
	gchar *linkname;

	GpuConfS.plugins  = 0; SpuConfS.plugins  = 0; CdrConfS.plugins  = 0;
	Pad1ConfS.plugins = 0; Pad2ConfS.plugins = 0; BiosConfS.plugins = 0;
	GpuConfS.glist  = NULL; SpuConfS.glist  = NULL; CdrConfS.glist  = NULL;
	Pad1ConfS.glist = NULL; Pad2ConfS.glist = NULL; BiosConfS.glist = NULL;
	GpuConfS.plist[0][0]  = '\0'; SpuConfS.plist[0][0]  = '\0'; CdrConfS.plist[0][0]  = '\0';
	Pad1ConfS.plist[0][0] = '\0'; Pad2ConfS.plist[0][0] = '\0'; BiosConfS.plist[0][0] = '\0';

	/* Load and get plugin info */
	dir = opendir(Config.PluginsDir);
	if (dir == NULL) {
		printf(_("Could not open directory: '%s'\n"), Config.PluginsDir);
		return;
	}
	while ((ent = readdir(dir)) != NULL) {
		long type, v;
		linkname = g_build_filename(Config.PluginsDir, ent->d_name, NULL);

		// only libraries past this point, not config tools
		if (strstr(linkname, ".so") == NULL && strstr(linkname, ".dylib") == NULL)
			continue;

		Handle = dlopen(linkname, RTLD_NOW);
		if (Handle == NULL) {
			printf("%s\n", dlerror());
			g_free(linkname);
			continue;
		}

		PSE_getLibType = (PSEgetLibType)dlsym(Handle, "PSEgetLibType");
		if (dlerror() != NULL) {
			if (strstr(linkname, "gpu") != NULL) type = PSE_LT_GPU;
			else if (strstr(linkname, "cdr") != NULL) type = PSE_LT_CDR;
			else if (strstr(linkname, "spu") != NULL) type = PSE_LT_SPU;
			else if (strstr(linkname, "pad") != NULL) type = PSE_LT_PAD;
			else { g_free(linkname); continue; }
		}
		else type = PSE_getLibType();

		PSE_getLibName = (PSEgetLibName) dlsym(Handle, "PSEgetLibName");
		if (dlerror() == NULL) {
			sprintf(name, "%s", PSE_getLibName());
			PSE_getLibVersion = (PSEgetLibVersion) dlsym(Handle, "PSEgetLibVersion");
			if (dlerror() == NULL) {
				char ver[32];

				v = PSE_getLibVersion();
				sprintf(ver, " %ld.%ld.%ld", v >> 16, (v >> 8) & 0xff, v & 0xff);
				strcat(name, ver);
			}
		}
		else strcpy(name, ent->d_name);

		if (type & PSE_LT_CDR)
			ComboAddPlugin(Cdr);
		if (type & PSE_LT_GPU)
			ComboAddPlugin(Gpu);
		if (type & PSE_LT_SPU)
			ComboAddPlugin(Spu);
		if (type & PSE_LT_PAD) {
			PADquery query = (PADquery)dlsym(Handle, "PADquery");
			if (query() & 0x1) {
				ComboAddPlugin(Pad1);
			}
			if (query() & 0x2) {
				ComboAddPlugin(Pad2);
			}
		}
		g_free(linkname);
	}
	closedir(dir);

	/* The BIOS list always contains the PCSX internal BIOS */
	add_bios_to_list(_("Internal HLE Bios"), "HLE");

	scan_bios_dir(Config.BiosDir);
}

void UpdatePluginsBIOS_UpdateGUI(GladeXML *xml) {
	/* Populate the plugin combo boxes */
	ConfCreatePConf("Gpu", Gpu);
	ConfCreatePConf("Spu", Spu);
	ConfCreatePConf("Pad1", Pad1);
	ConfCreatePConf("Pad2", Pad2);
	ConfCreatePConf("Cdr", Cdr);
	ConfCreatePConf("Bios", Bios);
}

void FindNetPlugin(GladeXML *xml) {
	DIR *dir;
	struct dirent *ent;
	void *Handle;
	char plugin[MAXPATHLEN],name[MAXPATHLEN];

	NetConfS.plugins  = 0;
	NetConfS.glist = NULL; 

	NetConfS.plugins += 2;
	strcpy(NetConfS.plist[NetConfS.plugins - 1], "Disabled");
	strcpy(NetConfS.plist[NetConfS.plugins - 2], "Disabled");
	NetConfS.glist = g_list_append(NetConfS.glist, NetConfS.plist[NetConfS.plugins - 1]);

	dir = opendir(Config.PluginsDir);
	if (dir == NULL)
		SysMessage(_("Could not open directory: '%s'\n"), Config.PluginsDir);
	else {
		/* ADB TODO Replace the following with a function */
		while ((ent = readdir(dir)) != NULL) {
			long type, v;

			sprintf(plugin, "%s/%s", Config.PluginsDir, ent->d_name);

			if (strstr(plugin, ".so") == NULL && strstr(plugin, ".dylib") == NULL)
				continue;
			Handle = dlopen(plugin, RTLD_NOW);
			if (Handle == NULL) continue;

			PSE_getLibType = (PSEgetLibType) dlsym(Handle, "PSEgetLibType");
			if (dlerror() != NULL) {
				if (strstr(plugin, "net") != NULL) type = PSE_LT_NET;
				else continue;
			}
			else type = PSE_getLibType();

			PSE_getLibName = (PSEgetLibName) dlsym(Handle, "PSEgetLibName");
			if (dlerror() == NULL) {
				sprintf(name, "%s", PSE_getLibName());
				PSE_getLibVersion = (PSEgetLibVersion) dlsym(Handle, "PSEgetLibVersion");
				if (dlerror() == NULL) {
					char ver[32];

					v = PSE_getLibVersion();
					sprintf(ver, " %ld.%ld.%ld",v>>16,(v>>8)&0xff,v&0xff);
					strcat(name, ver);
				}
			}
			else strcpy(name, ent->d_name);

			if (type & PSE_LT_NET) {
				ComboAddPlugin(Net);
			}
		}
		closedir(dir);

		ConfCreatePConf("Net", Net);
	}
}

void SysMessage(char *fmt, ...) {
	GtkWidget *Txt, *MsgDlg;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg) - 1] == '\n')
		msg[strlen(msg) - 1] = 0;

	if (!UseGui) {
		printf ("%s\n", msg);
		return;
	}

	MsgDlg =  gtk_dialog_new_with_buttons (_("Notice"), NULL,
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);

	gtk_window_set_position (GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);

	Txt = gtk_label_new (msg);
	gtk_label_set_line_wrap(GTK_LABEL(Txt), TRUE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(MsgDlg)->vbox), Txt);

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
		message_dialog =  gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				primary,
				NULL);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message_dialog),
				secondary);

		gtk_dialog_run (GTK_DIALOG (message_dialog));
		gtk_widget_destroy (message_dialog);
	}
}

void SysInfoMessage(gchar *primary, gchar *secondary) {
	GtkWidget *message_dialog;	
	if (!UseGui)
		printf ("%s - %s\n", primary, secondary);
	else {
		message_dialog =  gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_CLOSE,
				primary,
				NULL);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message_dialog),
				secondary);

		gtk_dialog_run (GTK_DIALOG (message_dialog));
		gtk_widget_destroy (message_dialog);
	}
}
