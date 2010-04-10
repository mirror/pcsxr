/*  Memory Viewer/Dumper for PCSX-Reloaded
 *
 *  Copyright (C) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#include "Linux.h"
#include "../libpcsxcore/psxmem.h"
#include <glade/glade.h>

#define MEMVIEW_MAX_LINES 256

static GtkWidget *MemViewDlg = NULL;
static u32 MemViewAddress = 0;

static void UpdateMemViewDlg() {
	s32 start, end;
	int i;
	char bufaddr[9], bufdata[16][3], buftext[17];

	GtkListStore *store = gtk_list_store_new(18, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING);

	GtkTreeIter iter;
	GtkWidget *widget;
	GladeXML *xml;

	xml = glade_get_widget_tree(MemViewDlg);

	MemViewAddress &= 0x1fffff;

	sprintf(buftext, "%.8X", MemViewAddress);
	widget = glade_xml_get_widget(xml, "entry_address");
	gtk_entry_set_text(GTK_ENTRY(widget), buftext);

	start = MemViewAddress & 0x1ffff0;
	end = start + MEMVIEW_MAX_LINES * 16;

	if (end > 0x1fffff) end = 0x1fffff;

	widget = glade_xml_get_widget(xml, "GtkCList_MemView");

	buftext[16] = '\0';

	while (start < end) {
		sprintf(bufaddr, "%.8X", start);

		for (i = 0; i < 16; i++) {
			buftext[i] = PSXMs8(start + i);
			sprintf(bufdata[i], "%.2X", (u8)buftext[i]);
			if (buftext[i] < 32) buftext[i] = '.';
		}

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, bufaddr, 1, bufdata[0],
			2, bufdata[1], 3, bufdata[2], 4, bufdata[3], 5, bufdata[4],
			6, bufdata[5], 7, bufdata[6], 8, bufdata[7], 9, bufdata[8],
			10, bufdata[9], 11, bufdata[10], 12, bufdata[11], 13, bufdata[12],
			14, bufdata[13], 15, bufdata[14], 16, bufdata[15], 17, buftext, -1);

		start += 16;
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(widget), GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(widget), TRUE);
	gtk_widget_show(widget);
}

static void MemView_Go() {
	GtkWidget *widget;
	GladeXML *xml;

	xml = glade_get_widget_tree(MemViewDlg);
	widget = glade_xml_get_widget(xml, "entry_address");

	sscanf(gtk_entry_get_text(GTK_ENTRY(widget)), "%x", &MemViewAddress);

	UpdateMemViewDlg();
}

static void MemView_Dump() {
	// TODO
}

static void MemView_Patch() {
	// TODO
	UpdateMemViewDlg();
}

// close the memory viewer window
static void MemView_Close(GtkWidget *widget, gpointer user_data) {
	gtk_widget_destroy(MemViewDlg);
	MemViewDlg = NULL;
}

void RunDebugMemoryDialog() {
	GladeXML *xml;
	GtkWidget *widget;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	PangoFontDescription *pfd;
	int i;

	xml = glade_xml_new(PACKAGE_DATA_DIR "pcsx.glade2", "MemViewDlg", NULL);
	if (!xml) {
		g_warning(_("Error: Glade interface could not be loaded!"));
		return;
	}

	MemViewDlg = glade_xml_get_widget(xml, "MemViewDlg");
	gtk_window_set_title(GTK_WINDOW(MemViewDlg), _("Memory Viewer"));

	widget = glade_xml_get_widget(xml, "GtkCList_MemView");

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Address"),
		renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	for (i = 0; i < 16; i++) {
		const char *p = "0123456789ABCDEF";
		char buf[2];

		buf[0] = p[i];
		buf[1] = '\0';

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(buf,
			renderer, "text", i + 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);
	}

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Text"),
		renderer, "text", 17, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(widget), column);

	pfd = pango_font_description_from_string("Bitstream Vera Sans Mono, "
		"DejaVu Sans Mono, Liberation Mono, FreeMono, Sans Mono 9");
	gtk_widget_modify_font(widget, pfd);
	pango_font_description_free(pfd);

	UpdateMemViewDlg();

	widget = glade_xml_get_widget(xml, "btn_dump");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
		GTK_SIGNAL_FUNC(MemView_Dump), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_patch");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
		GTK_SIGNAL_FUNC(MemView_Patch), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_go");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
		GTK_SIGNAL_FUNC(MemView_Go), xml, NULL, G_CONNECT_AFTER);

	g_signal_connect_data(GTK_OBJECT(MemViewDlg), "response",
		GTK_SIGNAL_FUNC(MemView_Close), xml, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);
}
