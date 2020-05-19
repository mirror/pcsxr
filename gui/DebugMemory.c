/*  Memory Viewer/Dumper for PCSX-Reloaded
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Linux.h"
#include "../libpcsxcore/psxmem.h"
#include "../libpcsxcore/r3000a.h"
#include <gtk/gtk.h>

#define MEMVIEW_MAX_LINES 256

static GtkBuilder *builder;
static GtkWidget *MemViewDlg = NULL;
static u32 MemViewAddress = 0;
static u8 MemViewValue = 0;

/* Copy value of MemViewAddress as text of entry_address */
static void sync_update_address_text() {
	char buftext[20];
	GtkWidget *widget;

	sprintf(buftext, "%.8X", MemViewAddress | 0x80000000);
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_address"));
	gtk_entry_set_text(GTK_ENTRY(widget), buftext);
}

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

	MemViewAddress &= 0x1fffff;

	sync_update_address_text();

	start = MemViewAddress & 0x1ffff0;
	end = start + MEMVIEW_MAX_LINES * 16;

	if (end > 0x1fffff) end = 0x1fffff;

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_MemView"));

	buftext[16] = '\0';

	while (start < end) {
		sprintf(bufaddr, "%.8X", start | 0x80000000);

		for (i = 0; i < 16; i++) {
			buftext[i] = psxMs8(start + i);
			sprintf(bufdata[i], "%.2X", (u8)buftext[i]);
			if ((u8)buftext[i] < 32 || (u8)buftext[i] >= 127)
				buftext[i] = '.';
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
	gtk_widget_show(widget);
}

static void MemView_Prev() {
	MemViewAddress -= MEMVIEW_MAX_LINES;
	sync_update_address_text();
	UpdateMemViewDlg();
}

static void MemView_Next() {
	MemViewAddress += MEMVIEW_MAX_LINES;
	sync_update_address_text();
	UpdateMemViewDlg();
}

static void MemView_Go() {
	GtkWidget *widget;

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_address"));

	sscanf(gtk_entry_get_text(GTK_ENTRY(widget)), "%x", &MemViewAddress);

	UpdateMemViewDlg();
}

static void MemView_RowActivated(
	GtkTreeView			*tree_view,
	GtkTreePath			*path,
	GtkTreeViewColumn	*column,
	gpointer			user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *mdl = gtk_tree_view_get_model(tree_view);

	if (gtk_tree_model_get_iter(mdl, &iter, path)) {
		const char *title = gtk_tree_view_column_get_title(column);
		char *value, *address;
		s8 index;
		if (title[1] != '\0') {
			index=1;
		} else if (title[0] > 0x40) {
			index=title[0]-0x36;
		} else {
			index=title[0]-0x2F;
		}
		gtk_tree_model_get(mdl, &iter, 0, &address,  index, &value, -1);
		sscanf(value, "%x", &MemViewValue);
		sscanf(address, "%x", &MemViewAddress);
		MemViewAddress &= ~0xF;
		MemViewAddress |= (index-1u);
		sync_update_address_text();

		g_free(address);
		g_free(value);
	}
}

static void MemView_Dump() {
	GtkWidget *dlg;
	GtkWidget *box, *table, *label, *start_edit, *length_edit;
	char buf[10];

	dlg = gtk_dialog_new_with_buttons(_("Memory Dump"), GTK_WINDOW(MemViewDlg),
		GTK_DIALOG_MODAL, "_Cancel", GTK_RESPONSE_CANCEL,
		"_OK", GTK_RESPONSE_ACCEPT, NULL);

	box = GTK_WIDGET(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));

	table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 5);
	gtk_grid_set_column_spacing(GTK_GRID(table), 5);

	label = gtk_label_new(_("Start Address (Hexadecimal):"));
	gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);
	gtk_widget_show(label);

	start_edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(start_edit), 8);
	sprintf(buf, "%.8X", MemViewAddress | 0x80000000);
	gtk_entry_set_text(GTK_ENTRY(start_edit), buf);
	gtk_grid_attach(GTK_GRID(table), start_edit, 1, 0, 1, 1);
	gtk_widget_show(start_edit);

	label = gtk_label_new(_("Length (Decimal):"));
	gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);
	gtk_widget_show(label);

	length_edit = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), length_edit, 1, 1, 1, 1);
	gtk_widget_show(length_edit);

	gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 5);

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
	gtk_widget_show_all(dlg);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		s32 start = 0, length = 0;

		sscanf(gtk_entry_get_text(GTK_ENTRY(start_edit)), "%x", &start);
		sscanf(gtk_entry_get_text(GTK_ENTRY(length_edit)), "%d", &length);

		start &= 0x1fffff;

		if (start + length > 0x1fffff) {
			length = 0x1fffff - start;
		}

		if (length > 0) {
			GtkWidget *file_chooser = gtk_file_chooser_dialog_new(_("Dump to File"),
				NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
				"_Cancel", GTK_RESPONSE_CANCEL,
				"_Save", GTK_RESPONSE_ACCEPT, NULL);

			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), getenv("HOME"));

			if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
				gchar *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
				FILE *fp = fopen(file, "wb");

				if (fp != NULL) {
					fwrite(&psxM[start], 1, length, fp);
					fclose(fp);
				} else {
					SysMessage(_("Error writing to %s!"), file);
				}

				g_free(file);
			}

			gtk_widget_destroy(file_chooser);
		}
	}

	gtk_widget_destroy(dlg);
}

static void MemView_Patch() {
	GtkWidget *dlg;
	GtkWidget *box, *table, *label, *addr_edit, *val_edit;
	char buf[12];

	dlg = gtk_dialog_new_with_buttons(_("Memory Patch"), GTK_WINDOW(MemViewDlg),
		GTK_DIALOG_MODAL, "_Cancel", GTK_RESPONSE_CANCEL,
		"_OK", GTK_RESPONSE_ACCEPT, NULL);

	box = GTK_WIDGET(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));

	table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 5);
	gtk_grid_set_column_spacing(GTK_GRID(table), 5);

	label = gtk_label_new(_("Address (Hexadecimal):"));
	gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);
	gtk_widget_show(label);

	addr_edit = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(addr_edit), 8);
	sprintf(buf, "%.8X", MemViewAddress | 0x80000000);
	gtk_entry_set_text(GTK_ENTRY(addr_edit), buf);
	gtk_grid_attach(GTK_GRID(table), addr_edit, 1, 0, 1, 1);
	gtk_widget_show(addr_edit);

	label = gtk_label_new(_("Value (Hexa string):"));
	gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);
	gtk_widget_show(label);

	val_edit = gtk_entry_new();
	sprintf(buf, "%.2X", MemViewValue);
	gtk_entry_set_text(GTK_ENTRY(val_edit), buf);
	gtk_grid_attach(GTK_GRID(table), val_edit, 1, 1, 1, 1);
	gtk_widget_show(val_edit);

	gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 5);

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
	gtk_widget_show_all(dlg);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
		u32 addr = 0xffffffff, val = 0;
		const char *p = gtk_entry_get_text(GTK_ENTRY(val_edit));
		int r = strlen(p);

		sscanf(gtk_entry_get_text(GTK_ENTRY(addr_edit)), "%x", &addr);

		if (r > 0 && addr != 0xffffffff) {
			addr &= 0x1fffff;
			MemViewAddress = addr;

			while (r > 0 && addr <= 0x1fffff) {
				sscanf(p, "%2x", &val);
				p += 2;
				r -= 2;

				while (r > 0 && (*p == '\t' || *p == ' ')) {
					p++;
					r--;
				}

				psxMemWrite8(addr, (u8)val);
#ifdef PSXREC
				psxCpu->Clear(addr, 1);
#endif
				addr++;
			}

			UpdateMemViewDlg();
		}
	}

	gtk_widget_destroy(dlg);
}

// close the memory viewer window
static void MemView_Close(GtkWidget *widget, gpointer user_data) {
	gtk_widget_destroy(MemViewDlg);
	MemViewDlg = NULL;
}

void RunDebugMemoryDialog() {
	GtkWidget *widget;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	int i;
	
	builder = gtk_builder_new();
	
	if (!gtk_builder_add_from_resource(builder, "/org/pcsxr/gui/pcsxr.ui", NULL)) {
		g_warning("Error: interface could not be loaded!");
		return;
	}
	
	MemViewDlg = GTK_WIDGET(gtk_builder_get_object(builder, "MemViewDlg"));
	
	gtk_window_set_title(GTK_WINDOW(MemViewDlg), _("Memory Viewer"));
	gtk_widget_show (MemViewDlg);
	
	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_MemView"));

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


	UpdateMemViewDlg();

	g_signal_connect_data(G_OBJECT(widget), "row-activated",
		G_CALLBACK(MemView_RowActivated), builder, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_dump"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(MemView_Dump), builder, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_patch"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(MemView_Patch), builder, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_go"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(MemView_Go), builder, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_prev"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(MemView_Prev), builder, NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_next"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(MemView_Next), builder, NULL, G_CONNECT_AFTER);

	g_signal_connect_data(G_OBJECT(MemViewDlg), "response",
		G_CALLBACK(MemView_Close), builder, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);
}
