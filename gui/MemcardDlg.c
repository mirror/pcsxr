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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <sys/time.h>

#include "Linux.h"
#include "../libpcsxcore/sio.h"

#if GTK_CHECK_VERSION(2, 20, 0) && !GTK_CHECK_VERSION(3, 0, 0)
#include "gdk-compat.h"
#endif

#define MAX_MEMCARD_BLOCKS 15

static gboolean quit;
static unsigned int currentIcon;

McdBlock Blocks[2][MAX_MEMCARD_BLOCKS];	// Assuming 2 cards, 15 blocks?
int IconC[2][MAX_MEMCARD_BLOCKS];
enum {
    CL_ICON,
    CL_TITLE,
    CL_STAT,
    CL_ID,
    CL_NAME,
    NUM_CL
};

static GtkBuilder *builder;
GtkWidget *GtkCList_McdList1, *GtkCList_McdList2;

static void AddColumns(GtkTreeView *treeview) {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	// column for icon
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes(_("Icon"),
			renderer, "pixbuf", CL_ICON, NULL);
	gtk_tree_view_append_column(treeview, column);

	// column for title
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Title"),
			renderer, "text", CL_TITLE, NULL);
	gtk_tree_view_append_column(treeview, column);

	// column for status
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Status"),
			renderer, "text", CL_STAT, NULL);
	gtk_tree_view_append_column(treeview, column);

	// column for id
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("ID"),
			renderer, "text", CL_ID, NULL);
	gtk_tree_view_append_column(treeview, column);

	// column for Name
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"),
			renderer, "text", CL_NAME, NULL);
	gtk_tree_view_append_column(treeview, column);
}

#if GTK_CHECK_VERSION(2, 20, 0)
static GdkPixbuf *SetIcon(GtkWidget *dialog, short *icon, int scale) {
	GdkPixbuf *pixbuf;
	cairo_surface_t *s;
	cairo_t *cr;
	int x, y, c, i, r, g, b;
	int size = 16 * scale;
	s = cairo_image_surface_create(CAIRO_FORMAT_RGB24, size, size);
	cr = cairo_create(s);
	cairo_scale(cr, scale, scale);
	for (i=0; i< 256; i++) {
		x = (i % 16);
		y = (i / 16);
		c = icon[i];
		r = (c & 0x001f) << 3;
		g = ((c & 0x03e0) >> 5) << 3;
		b = ((c & 0x7c00) >> 10) << 3;
		cairo_rectangle(cr, x, y, 1, 1);
		cairo_set_source_rgb(cr, r/255.0, g/255.0, b/255.0);
		cairo_fill(cr);
	}
	cairo_destroy(cr);
#if GTK_CHECK_VERSION(3, 0, 0)
	pixbuf = gdk_pixbuf_get_from_surface(s, 0, 0, size, size);
#else
	pixbuf = compat_gdk_pixbuf_get_from_surface(s, 0, 0, size, size);
#endif
	cairo_surface_destroy(s);
	return pixbuf;
}

#else
static GdkPixbuf *SetIcon(GtkWidget *dialog, short *icon, int i) {
	GdkPixmap *pixmap;
	GdkImage  *image;
	GdkVisual *visual;
	GdkPixbuf *pixbuf;
	GdkGC     *gc;
	int x, y, c;

	visual = gdk_window_get_visual(gtk_widget_get_window(dialog));

	if (visual->depth == 8) return NULL;

	image = gdk_image_new(GDK_IMAGE_NORMAL, visual, 32, 32);

	for (y = 0; y < 32; y++) {
		for (x = 0; x < 32; x++) {
			c = icon[(y>>1) * 16 + (x>>1)];
			c = ((c & 0x001f) << 10) | ((c & 0x7c00) >> 10) | (c & 0x03e0);
			if (visual->depth == 16)
				c = (c & 0x001f) | ((c & 0x7c00) << 1) | ((c & 0x03e0) << 1);
			else if (visual->depth == 24 || visual->depth == 32)
				c = ((c & 0x001f) << 3) | ((c & 0x03e0) << 6) | ((c & 0x7c00) << 9);

			gdk_image_put_pixel(image, x, y, c);
		}
	}

	pixmap = gdk_pixmap_new(gtk_widget_get_window(dialog), 32, 32, visual->depth);

	gc = gdk_gc_new(pixmap);
	gdk_draw_image(pixmap, gc, image, 0, 0, 0, 0, 32, 32);
	gdk_gc_destroy(gc);
	gdk_image_destroy(image);

	pixbuf = gdk_pixbuf_get_from_drawable(NULL, GDK_PIXMAP (pixmap), NULL,
										0, 0, 0, 0, -1, -1);
	g_object_unref(pixmap);
	
	return pixbuf;
}
#endif

static gchar* MCDStatusToChar(McdBlock *Info) {
	gchar *state;
	if ((Info->Flags & 0xF0) == 0xA0) {
		if ((Info->Flags & 0xF) >= 1 && (Info->Flags & 0xF) <= 3)
			state = _("Deleted");
		else
			state = _("Free");
	} else if ((Info->Flags & 0xF0) == 0x50) {
		if ((Info->Flags & 0xF) == 0x1)
			state = _("Used");
		else if ((Info->Flags & 0xF) == 0x2)
			state = _("Link");
		else if ((Info->Flags & 0xF) == 0x3)
			state = _("End link");
        } else
		state = _("Free");
	return state;
}

static void LoadListItems(int mcd, GtkWidget *widget) {
	int i;
	GtkWidget *List;
	GtkWidget *dialog;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	gchar *title;

	store = gtk_list_store_new(NUM_CL, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "McdsDlg"));

	if (mcd == 1) List = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList1"));
	else List = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList2"));

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		McdBlock *Info;
		gchar *state;

		Info = &Blocks[mcd - 1][i];
		IconC[mcd - 1][i] = 0;
		state = MCDStatusToChar(Info);

		pixbuf = SetIcon(dialog, Info->Icon, 2);

		gtk_list_store_append(store, &iter);

		title = g_convert(Info->sTitle, strlen(Info->sTitle), "UTF-8",
			"Shift-JIS", NULL, NULL, NULL);

		gtk_list_store_set(store, &iter,
				CL_ICON, pixbuf,
				CL_TITLE, title,
				CL_STAT, state,
				CL_NAME, Info->Name,
				CL_ID, Info->ID,
				-1);

		g_free(title);
		
		g_object_unref(pixbuf);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(List), GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(List), TRUE);
	gtk_widget_show(List);
}

static void UpdateFilenameButtons(GtkWidget *widget) {
	int i;
	GtkWidget *dialog;
	const char *filename;
	gchar *p;

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "McdsDlg"));

	for (i = 0; i < 2; i++) {
		if (i == 0) {
			widget = GTK_WIDGET(gtk_builder_get_object(builder, "Mcd1Label"));
			filename = Config.Mcd1;
		} else {
			widget = GTK_WIDGET(gtk_builder_get_object(builder, "Mcd2Label"));
			filename = Config.Mcd2;
		}

		p = g_path_get_basename(filename);
		gtk_label_set_text(GTK_LABEL(widget), p);
		g_free(p);
	}
}

static void LoadMcdDlg(GtkWidget *widget) {
	int i;

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		GetMcdBlockInfo(1, i + 1, &Blocks[0][i]);
		GetMcdBlockInfo(2, i + 1, &Blocks[1][i]);
	}

	LoadListItems(1, widget);
	LoadListItems(2, widget);

	UpdateFilenameButtons(widget);
}

static void OnTreeSelectionChanged(GtkTreeSelection *selection, gpointer user_data);

static void UpdateListItems(int mcd, GtkWidget *widget) {
	GtkWidget *List;
	GtkWidget *dialog;
	GtkListStore *store;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	short *pIcon;
	int i;
	gchar *title;

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "McdsDlg"));

	if (mcd == 1) List = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList1"));
	else List = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList2"));

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(List)));
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		McdBlock *Info;
		gchar *state;

		Info = &Blocks[mcd - 1][i];
		IconC[mcd - 1][i] = 0;

		state = MCDStatusToChar(Info);

		if (Info->IconCount > 0) {
			pIcon = &Info->Icon[(currentIcon % Info->IconCount) * 16 * 16];
		} else {
			pIcon = Info->Icon;
		}

		pixbuf = SetIcon(dialog, pIcon, 2);
		title = g_convert(Info->sTitle, strlen(Info->sTitle), "UTF-8",
			"Shift-JIS", NULL, NULL, NULL);

		gtk_list_store_set(store, &iter,
				CL_ICON, pixbuf,
				CL_TITLE, title,
				CL_STAT, state,
				CL_NAME, Info->Name,
				CL_ID, Info->ID,
				-1);

		g_free(title);

		g_object_unref(pixbuf);
		gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}

	gtk_widget_show(List);

	OnTreeSelectionChanged(gtk_tree_view_get_selection(GTK_TREE_VIEW(List)), GINT_TO_POINTER(mcd));
}

static void UpdateMcdDlg(GtkWidget *widget) {
	int i;

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		GetMcdBlockInfo(1, i + 1, &Blocks[0][i]);
		GetMcdBlockInfo(2, i + 1, &Blocks[1][i]);
	}

	UpdateListItems(1, widget);
	UpdateListItems(2, widget);

	UpdateFilenameButtons(widget);
}

static void OnMcd_Close(GtkDialog *dialog, gint arg1, gpointer user_data) {
	quit = TRUE;
	SaveConfig();
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void OnMcd_FileChange(GtkWidget *widget, gpointer user_data) {
	gint memcard = GPOINTER_TO_INT(user_data);
	gchar *filename;
	GtkWidget *chooser;

	// Ask for name of memory card
	chooser = gtk_file_chooser_dialog_new(_("Select A File"),
	    NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	    NULL);

	if (memcard == 1)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(chooser), Config.Mcd1);
	else
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(chooser), Config.Mcd2);

	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK) {
		gtk_widget_hide(chooser);

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

		if (filename != NULL) {
			if (memcard == 1) strncpy(Config.Mcd1, filename, MAXPATHLEN);
			else strncpy(Config.Mcd2, filename, MAXPATHLEN);

			LoadMcd(memcard, filename);
			LoadMcdDlg(widget);

			g_free(filename);
		}
	}

	gtk_widget_destroy(chooser);
}

// format a memory card
static void OnMcd_Format(GtkWidget *widget, gpointer user_data) {
	GtkWidget *message_dialog;
	gint result;
	char *str;

	gint memcard = GPOINTER_TO_INT(user_data);

	message_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		_("Format this Memory Card?"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(message_dialog),
		_("If you format the memory card, the card will be empty, and any existing data overwritten."));
	gtk_dialog_add_buttons(GTK_DIALOG(message_dialog),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		_("Format card"), GTK_RESPONSE_YES, NULL);

	result = gtk_dialog_run(GTK_DIALOG(message_dialog));
	gtk_widget_destroy(message_dialog);

	if (result == GTK_RESPONSE_YES) {
		if (memcard == 1) str = Config.Mcd1;
		else str = Config.Mcd2;

		CreateMcd(str);
		LoadMcd(memcard, str);

		UpdateMcdDlg(widget);
	}
}

// create a new, formatted memory card
static void OnMcd_New(GtkWidget *widget, gpointer user_data) {
	GtkWidget *chooser;
	gchar *path;
	gint memcard = GPOINTER_TO_INT(user_data);

	// Ask for name of new memory card
	chooser = gtk_file_chooser_dialog_new(_("Create a new Memory Card"),
	    NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
	    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
	    NULL);

	// Card should be put into $HOME/.pcsxr/memcards
	path = g_build_filename(g_get_home_dir(), ".pcsxr", "memcards", NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), path);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), _("New Memory Card.mcd"));
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK) {
		gchar *name;

		gtk_widget_hide(chooser);
		name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

		CreateMcd(name);

		if (memcard == 1) strncpy(Config.Mcd1, name, MAXPATHLEN);
		else strncpy(Config.Mcd2, name, MAXPATHLEN);

		LoadMcd(memcard, name);
		LoadMcdDlg(widget);

		g_free(name);
	}

	gtk_widget_destroy(chooser);
	g_free(path);
}

static int GetFreeMemcardSlot(gint target_card, gint count) {
	McdBlock *Info;
	gint foundcount=0, i=-1;

	// search for empty (formatted) blocks first
	while (i < MAX_MEMCARD_BLOCKS && foundcount < count) {
		Info = &Blocks[target_card][++i];
		if ((Info->Flags & 0xFF) == 0xA0) { // if A0 but not A1
			foundcount++;
		} else if (foundcount >= 1) { // need to find n count consecutive blocks
			foundcount=0;
		} else {
		}
		//printf("formatstatus=%x\n", Info->Flags);
	}

	if (foundcount == count)
		return (i-foundcount+1);

	// no free formatted slots, try to find a deleted one
	foundcount=0;
	i = -1;
	while (i < MAX_MEMCARD_BLOCKS && foundcount < count) {
		Info = &Blocks[target_card][++i];
		if ((Info->Flags & 0xF0) == 0xA0) { // A2 or A6 f.e.
			foundcount++;
		} else if (foundcount >= 1) { // need to find n count consecutive blocks
			foundcount=0;
		} else {
		}
		//printf("delstatus=%x\n", Info->Flags);
	}

	if (foundcount == count)
		return (i-foundcount+1);

	return -1;
}

static void CopyMemcardData(char *from, char *to, gint srci, gint dsti, gchar *str) {
	// header	
	memcpy(to + (dsti + 1) * 128, from + (srci + 1) * 128, 128);
	SaveMcd((char *)str, to, (dsti + 1) * 128, 128);

	// data	
	memcpy(to + (dsti + 1) * 1024 * 8, from + (srci+1) * 1024 * 8, 1024 * 8);
	SaveMcd((char *)str, to, (dsti + 1) * 1024 * 8, 1024 * 8);

	//printf("data = %s\n", from + (srci+1) * 128);
}

gint GetMcdBlockCount(gint mcd, gint startblock) {
	McdBlock b;
	gint i;

	// check status on startblock+1...n
	for (i=1; i <= (MAX_MEMCARD_BLOCKS-startblock); i++) {
		GetMcdBlockInfo(mcd, startblock+i, &b);
		//printf("i=%i, mcd=%i, startblock=%i, diff=%i, flags=%x\n", i, mcd, startblock, (MAX_MEMCARD_BLOCKS-startblock), b.Flags);
		if ((b.Flags & 0x3) == 0x3) {
			return i+1;
		} else if ((b.Flags & 0x2) == 0x2) {
			//i++
		} else {
			return i;
		}
	}
	return i; // startblock was the last block so count = 1
}

static void OnMcd_CopyTo(GtkWidget *widget, gpointer user_data) {
	gint dstmcd = GPOINTER_TO_INT(user_data);
	gint srcmcd;

	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	gint *i, j, count, srci, first_free_slot;
	GtkTreeSelection *treesel;
	gchar *str;
	char *source, *destination;

	if (dstmcd == 1) {
		treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(GtkCList_McdList2));
		srcmcd = 2;
	} else {
		treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(GtkCList_McdList1));
		srcmcd = 1;
	}
	//printf("src=%i and dst=%i\n", srcmcd, dstmcd);

	// If the item selected is not reported as a 'Free' slot
	if (gtk_tree_selection_get_selected(treesel, &model, &iter)) {
		path = gtk_tree_model_get_path(model, &iter);
		i = gtk_tree_path_get_indices(path);
		srci    = *i;
		gtk_tree_path_free(path);
	}

	// get how many blocks there are including linked blocks
	count = GetMcdBlockCount(srcmcd, (srci+1));

	// Determine the first free slot in the target memory card
	first_free_slot = GetFreeMemcardSlot((dstmcd - 1), count);
	if (first_free_slot == -1) {
		// No free slots available on the destination card
		SysErrorMessage(_("No free space on memory card"),
						_("There are no free slots available on the target memory card. Please delete a slot first."));
		return;
	}

	if (dstmcd == 1) {
		str = Config.Mcd1;
		source = Mcd2Data;
		destination = Mcd1Data;
	} else {
		str = Config.Mcd2;
		source = Mcd1Data;
		destination = Mcd2Data;
	}

	for (j=0; j < count; j++) {
		CopyMemcardData(source, destination, (srci+j), (first_free_slot+j), str);
		//printf("count = %i, firstfree=%i, i=%i\n", count, first_free_slot, j);
	}

	UpdateMcdDlg(widget);
}

static void OnMemcardDelete(GtkWidget *widget, gpointer user_data) {
	McdBlock *Info;
	int i, xor = 0, j;
	char *data, *ptr;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	gchar *filename;
	gboolean selected;
	GtkWidget *tree;
	GtkTreeSelection *sel;

	gint memcard = GPOINTER_TO_INT(user_data);

	if (memcard == 1) {
		tree = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList1"));
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (tree));
		selected = gtk_tree_selection_get_selected (sel, &model, &iter);
		data = Mcd1Data;
		filename = Config.Mcd1;
	} else {
		tree = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList2"));
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (tree));
		selected = gtk_tree_selection_get_selected(sel, &model, &iter);
		data = Mcd2Data;
		filename = Config.Mcd2;
	}

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);

		i++;
		ptr = data + i * 128;
		Info = &Blocks[memcard - 1][i - 1];

		if ((Info->Flags & 0xF0) == 0xA0) {
			if ((Info->Flags & 0xF) >= 1 &&
				(Info->Flags & 0xF) <= 3) { // deleted
				*ptr = 0x50 | (Info->Flags & 0xF);
			} else return;
		} else if ((Info->Flags & 0xF0) == 0x50) { // used
				*ptr = 0xA0 | (Info->Flags & 0xF);
		} else { return; }

		for (j = 0; j < 127; j++) xor ^= *ptr++;
		*ptr = xor;

		SaveMcd((char *)filename, data, i * 128, 128);
		UpdateMcdDlg(widget);
	}
}

static void OnTreeSelectionChanged(GtkTreeSelection *selection, gpointer user_data) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	gboolean selected;
	int i;
	McdBlock b;

	gint memcard = GPOINTER_TO_INT(user_data);

	selected = gtk_tree_selection_get_selected(selection, &model, &iter);

	if (selected) {
		path = gtk_tree_model_get_path(model, &iter);
		i = *gtk_tree_path_get_indices(path);
		gtk_tree_path_free(path);

		// If a row was selected, and the row is not blank, we can now enable
		// some of the disabled widgets
		if (memcard == 1) {
			GetMcdBlockInfo(1, i + 1, &b);

			if ((b.Flags >= 0xA1 && b.Flags <= 0xA3) || ((b.Flags & 0xF0) == 0x50)) {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete1")), TRUE);
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete1")), FALSE);
			}

			if ((b.Flags & 0xF3) == 0x51) {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo2")), TRUE);
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo2")), FALSE);
			}
		} else {
			GetMcdBlockInfo(2, i + 1, &b);

			if ((b.Flags >= 0xA1 && b.Flags <= 0xA3) || ((b.Flags & 0xF0) == 0x50)) {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete2")), TRUE);
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete2")), FALSE);
			}

			if ((b.Flags & 0xF3) == 0x51) {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo1")), TRUE);
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo1")), FALSE);
			}
		}
	} else {
		if (memcard == 1) {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo2")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete1")), FALSE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo1")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete2")), FALSE);
		}
	}
}

gboolean updateFunc(gpointer data) {
	if (quit) return FALSE;
	currentIcon++;
	UpdateListItems(1, GtkCList_McdList1);
	UpdateListItems(2, GtkCList_McdList2);
	g_timeout_add(200, updateFunc, 0);
	return FALSE;
}

void OnConf_Mcds() {
	GtkWidget *dialog;
	GtkWidget *widget;
	GtkTreeSelection *treesel1, *treesel2;
	gchar *str;

	builder = gtk_builder_new();
	
	if (!gtk_builder_add_from_file(builder, PACKAGE_DATA_DIR "pcsxr.ui", NULL)) {
		g_warning("Error: interface could not be loaded!");
		return;
	}

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "McdsDlg"));

	gtk_window_set_title(GTK_WINDOW(dialog), _("Memory Card Manager"));
	gtk_widget_show (dialog);

	// Assign default memory cards
	if (!strlen(Config.Mcd1)) {
		str = g_strconcat(getenv("HOME"), DEFAULT_MEM_CARD_1, NULL);
		strcpy(Config.Mcd1, str);
		g_free(str);
	}

	if (!strlen(Config.Mcd2)) {
		str = g_strconcat(getenv("HOME"), DEFAULT_MEM_CARD_2, NULL);
		strcpy(Config.Mcd2, str);
		g_free(str);
	}

	GtkCList_McdList1 = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList1"));
	AddColumns(GTK_TREE_VIEW(GtkCList_McdList1));
	GtkCList_McdList2 = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList2"));
	AddColumns(GTK_TREE_VIEW(GtkCList_McdList2));

	treesel1 = gtk_tree_view_get_selection(GTK_TREE_VIEW (GtkCList_McdList1));
	gtk_tree_selection_set_mode(treesel1, GTK_SELECTION_SINGLE);
	g_signal_connect_data(G_OBJECT(treesel1), "changed",
						  G_CALLBACK(OnTreeSelectionChanged),
						  GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	treesel2 = gtk_tree_view_get_selection(GTK_TREE_VIEW (GtkCList_McdList2));
	gtk_tree_selection_set_mode(treesel2, GTK_SELECTION_SINGLE);
	g_signal_connect_data(G_OBJECT(treesel2), "changed",
						  G_CALLBACK(OnTreeSelectionChanged),
						  GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);

	LoadMcdDlg(dialog);

	// Setup a handler for when Close or Cancel is clicked
	g_signal_connect_data(G_OBJECT(dialog), "response",
			G_CALLBACK(OnMcd_Close), builder, (GClosureNotify)g_object_unref, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Format1"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_Format), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Format2"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_Format), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "Mcd1Button"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_FileChange), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "Mcd2Button"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_FileChange), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_New1"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_New), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_New2"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_New), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo1"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_CopyTo), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);
	gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo2"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnMcd_CopyTo), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);
	gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete1"));
	g_signal_connect_data (G_OBJECT (widget), "clicked",
			G_CALLBACK(OnMemcardDelete), GINT_TO_POINTER(1), NULL, G_CONNECT_AFTER);
	gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_Delete2"));
	g_signal_connect_data (G_OBJECT (widget), "clicked",
			G_CALLBACK(OnMemcardDelete), GINT_TO_POINTER(2), NULL, G_CONNECT_AFTER);
	gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);

	quit = FALSE;
	currentIcon = 0;

	g_timeout_add(1, updateFunc, 0);

	while (gtk_events_pending()) {  gtk_main_iteration(); }
}
