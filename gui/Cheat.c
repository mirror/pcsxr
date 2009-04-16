/*  Cheat Support for PCSX-Reloaded
 *
 *  Copyright (C) 2009, Wei Mingzhi <whistler@openoffice.org>.
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
#include <stdint.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "Linux.h"

#include "../libpcsxcore/cheat.h"

GtkWidget *CheatListDlg = NULL;
GtkWidget *CheatSearchDlg = NULL;

static void LoadCheatListItems(int index) {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkTreeIter iter;
	GtkWidget *widget;
	GladeXML *xml;

	int i;

	xml = glade_get_widget_tree(CheatListDlg);
	widget = glade_xml_get_widget(xml, "GtkCList_Cheat");

	for (i = 0; i < NumCheats; i++) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, Cheats[i].Enabled ? _(" Yes") : "",
			1, Cheats[i].Descr, -1);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(widget), TRUE);
	gtk_widget_show(widget);

	if (index >= NumCheats) {
		index = NumCheats - 1;
	}

	if (index >= 0) {
		GtkTreePath *path;
		GtkTreeSelection *sel;

		path = gtk_tree_path_new_from_indices(index, -1);
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

		gtk_tree_selection_select_path(sel, path);
		gtk_tree_path_free(path);
	}
}

static void CheatList_TreeSelectionChanged(GtkTreeSelection *selection, gpointer user_data) {
	GladeXML *xml;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int i;

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);

		// If a row was selected, and the row is not blank, we can now enable
		// some of the disabled widgets
		xml = glade_get_widget_tree(CheatListDlg);

		gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "editbutton1")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "delbutton1")), TRUE);
		if (Cheats[i].Enabled) {
			gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "enablebutton1")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "disablebutton1")), TRUE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "enablebutton1")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "disablebutton1")), FALSE);
		}
	} else {
		xml = glade_get_widget_tree(CheatListDlg);

		gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "editbutton1")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "delbutton1")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "enablebutton1")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "disablebutton1")), FALSE);
	}

	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "savebutton1")), NumCheats);
}

static void OnCheatListDlg_AddClicked(GtkWidget *widget, gpointer user_data) {
	GtkWidget *dlg;
	GtkWidget *box, *scroll, *label, *descr_edit, *code_edit;

	dlg = gtk_dialog_new_with_buttons(_("Add New Cheat"), GTK_WINDOW(CheatListDlg),
		GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gtk_window_set_default_size(GTK_WINDOW(dlg), 350, 350);

	box = GTK_WIDGET(GTK_DIALOG(dlg)->vbox);

	label = gtk_label_new(_("Cheat Description:"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	descr_edit = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(box), descr_edit, FALSE, FALSE, 5);
	gtk_widget_show(descr_edit);

	label = gtk_label_new(_("Cheat Code:"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	code_edit = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(code_edit), GTK_WRAP_CHAR);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), code_edit);
	gtk_widget_show(code_edit);

	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 5);
	gtk_widget_show(scroll);

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);

	gtk_widget_show_all(dlg);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(code_edit));
		GtkTextIter s, e;
		char *codetext;

		gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(b), &s, &e);
		codetext = strdup(gtk_text_buffer_get_text(GTK_TEXT_BUFFER(b), &s, &e, FALSE));

		if (AddCheat(gtk_entry_get_text(GTK_ENTRY(descr_edit)), codetext) != 0) {
			SysErrorMessage(_("Error"), _("Invalid cheat code!"));
		}

		LoadCheatListItems(NumCheats - 1);

		free(codetext);
	}

	gtk_widget_destroy(dlg);
}

static void OnCheatListDlg_EditClicked(GtkWidget *widget, gpointer user_data) {
	GtkWidget *dlg;
	GtkWidget *box, *scroll, *label, *descr_edit, *code_edit;
	GladeXML *xml;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int index, i;
	char buf[8192];
	char *p = buf;

	xml = glade_get_widget_tree(CheatListDlg);
	widget = glade_xml_get_widget(xml, "GtkCList_Cheat");

	selected = gtk_tree_selection_get_selected(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)),
		&model, &iter);

	if (!selected) {
		return;
	}

	path = gtk_tree_model_get_path(model, &iter);
	index = *gtk_tree_path_get_indices(path);

	dlg = gtk_dialog_new_with_buttons(_("Edit Cheat"), GTK_WINDOW(CheatListDlg),
		GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gtk_window_set_default_size(GTK_WINDOW(dlg), 350, 350);

	box = GTK_WIDGET(GTK_DIALOG(dlg)->vbox);

	label = gtk_label_new(_("Cheat Description:"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	descr_edit = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(descr_edit), Cheats[index].Descr);
	gtk_box_pack_start(GTK_BOX(box), descr_edit, FALSE, FALSE, 5);
	gtk_widget_show(descr_edit);

	label = gtk_label_new(_("Cheat Code:"));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	code_edit = gtk_text_view_new();

	for (i = Cheats[index].First; i < Cheats[index].First + Cheats[index].n; i++) {
		sprintf(p, "%.8X %.4X\n", CheatCodes[i].Addr, CheatCodes[i].Val);
		p += 14;
		*p = '\0';
	}

	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(code_edit)),
		buf, -1);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(code_edit), GTK_WRAP_CHAR);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), code_edit);
	gtk_widget_show(code_edit);

	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 5);
	gtk_widget_show(scroll);

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);

	gtk_widget_show_all(dlg);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(code_edit));
		GtkTextIter s, e;
		char *codetext;

		gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(b), &s, &e);
		codetext = strdup(gtk_text_buffer_get_text(GTK_TEXT_BUFFER(b), &s, &e, FALSE));

		if (EditCheat(index, gtk_entry_get_text(GTK_ENTRY(descr_edit)), codetext) != 0) {
			SysErrorMessage(_("Error"), _("Invalid cheat code!"));
		}

		LoadCheatListItems(index);

		free(codetext);		
	}

	gtk_widget_destroy(dlg);
}

static void OnCheatListDlg_DelClicked(GtkWidget *widget, gpointer user_data) {
	GladeXML *xml;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int i = -1;

	xml = glade_get_widget_tree(CheatListDlg);
	widget = glade_xml_get_widget(xml, "GtkCList_Cheat");

	selected = gtk_tree_selection_get_selected(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)),
		&model, &iter);

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);

		RemoveCheat(i);
	}

	LoadCheatListItems(i); // FIXME: should remove it from the list directly
	                       // rather than regenerating the whole list
}

static void OnCheatListDlg_EnableClicked(GtkWidget *widget, gpointer user_data) {
	GladeXML *xml;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int i = -1;

	xml = glade_get_widget_tree(CheatListDlg);
	widget = glade_xml_get_widget(xml, "GtkCList_Cheat");

	selected = gtk_tree_selection_get_selected(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)),
		&model, &iter);

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);

		Cheats[i].Enabled = 1;
	}

	LoadCheatListItems(i); // FIXME: should modify it in the list directly
	                       // rather than regenerating the whole list
}

static void OnCheatListDlg_DisableClicked(GtkWidget *widget, gpointer user_data) {
	GladeXML *xml;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int i = -1;

	xml = glade_get_widget_tree(CheatListDlg);
	widget = glade_xml_get_widget(xml, "GtkCList_Cheat");

	selected = gtk_tree_selection_get_selected(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)),
		&model, &iter);

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);

		Cheats[i].Enabled = 0;
	}

	LoadCheatListItems(i); // FIXME: should modify it in the list directly
	                       // rather than regenerating the whole list
}

static void OnCheatListDlg_OpenClicked(GtkWidget *widget, gpointer user_data) {
	GtkWidget *chooser;
	gchar *filename;

	GtkFileFilter *filter;

	chooser = gtk_file_chooser_dialog_new (_("Open Cheat File"),
		NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	filename = g_build_filename(getenv("HOME"), CHEATS_DIR, NULL);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), filename);
	g_free(filename);

	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*.cht");
	gtk_file_filter_set_name (filter, _("PCSX Cheat Code Files (*.cht)"));
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

	if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		gtk_widget_destroy (GTK_WIDGET (chooser));
		while (gtk_events_pending()) gtk_main_iteration();
	} else {
		gtk_widget_destroy (GTK_WIDGET (chooser));
		while (gtk_events_pending()) gtk_main_iteration();
		return;
	}

	LoadCheats(filename);

	g_free(filename);

	LoadCheatListItems(-1);
}

static void OnCheatListDlg_SaveClicked(GtkWidget *widget, gpointer user_data) {
	GtkWidget *chooser;
	gchar *filename;
	GtkFileFilter *filter;

	chooser = gtk_file_chooser_dialog_new(_("Save Cheat File"),
		NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	filename = g_build_filename(getenv("HOME"), CHEATS_DIR, NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), filename);
	g_free(filename);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.cht");
	gtk_file_filter_set_name(filter, _("PCSX Cheat Code Files (*.cht)"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_filter_set_name(filter, _("All Files (*.*)"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		gtk_widget_destroy (GTK_WIDGET(chooser));
		while (gtk_events_pending()) gtk_main_iteration();
	} else {
		gtk_widget_destroy (GTK_WIDGET(chooser));
		while (gtk_events_pending()) gtk_main_iteration();
		return;
	}

	SaveCheats(filename);

	g_free(filename);
}

static void OnCheatListDlg_Clicked() {
	gtk_widget_destroy(CheatListDlg);
	CheatListDlg = NULL;
}

// run the cheat list dialog
void RunCheatListDialog() {
	GladeXML *xml;
	GtkWidget *widget;
	GtkTreeSelection *treesel;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "CheatListDlg", NULL);
	if (!xml) {
		g_warning(_("Error: Glade interface could not be loaded!"));
		return;
	}

	CheatListDlg = glade_xml_get_widget(xml, "CheatListDlg");
	gtk_window_set_title(GTK_WINDOW(CheatListDlg), _("Cheat Codes"));

	widget = glade_xml_get_widget(xml, "GtkCList_Cheat");

	// column for enable
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Enable"),
			renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(widget), column);

	// column for description
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
			renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(widget), column);

	LoadCheatListItems(-1);

	treesel = gtk_tree_view_get_selection (GTK_TREE_VIEW(widget));
	gtk_tree_selection_set_mode (treesel, GTK_SELECTION_SINGLE);
	g_signal_connect_data (G_OBJECT (treesel), "changed",
						   G_CALLBACK (CheatList_TreeSelectionChanged),
						   NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "addbutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_AddClicked), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "editbutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_EditClicked), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "delbutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_DelClicked), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "enablebutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_EnableClicked), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "disablebutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_DisableClicked), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "loadbutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_OpenClicked), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "savebutton1");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(OnCheatListDlg_SaveClicked), xml, NULL, G_CONNECT_AFTER);

	// Setup a handler for when Close or Cancel is clicked
	g_signal_connect_data(GTK_OBJECT(CheatListDlg), "response",
			GTK_SIGNAL_FUNC(OnCheatListDlg_Clicked), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);

	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "savebutton1")), NumCheats);
	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "editbutton1")), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "delbutton1")), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "enablebutton1")), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "disablebutton1")), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET(glade_xml_get_widget(xml, "editbutton1")), FALSE);
}

// run the cheat search dialog
void RunCheatSearchDialog() {
}
