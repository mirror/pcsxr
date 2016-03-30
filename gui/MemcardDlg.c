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

#define MAX_MEMCARD_BLOCKS 15
#define ICON_SIZE 16 * 16
#define ISLINKMIDBLOCK(Info) (((Info)->Flags & 0xF) == 0x2)
#define ISLINKENDBLOCK(Info) (((Info)->Flags & 0xF) == 0x3)
#define ISLINKBLOCK(Info) (ISLINKENDBLOCK((Info)) || ISLINKMIDBLOCK((Info)))
#define ISDELETED(Info) (((Info)->Flags & 0xF) >= 1 && ((Info)->Flags & 0xF) <= 3)
#define ISBLOCKDELETED(Info) (((Info)->Flags & 0xF0) == 0xA0)
#define ISSTATUSDELETED(Info) (ISBLOCKDELETED(Info) && ISDELETED(Info))
#define ISLINKED(Data) ( ((Data) != 0xFFFFU) && ((Data) <= MAX_MEMCARD_BLOCKS) )
#define GETLINKFORBLOCK(Data, block) (*((Data)+(((block)*128)+0x08)))

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

short IconDeleted[ICON_SIZE];
short IconLinked[ICON_SIZE];

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

static GdkPixbuf *SetIcon(GtkWidget *dialog, short *icon, int scale) {
	GdkPixbuf *gdkpixbuf;
	GdkPixbuf *gdkpixbuf2;
	guchar *dest_pixels;
	u32 x, y;
	u16 c;

	gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, 16, 16);
	dest_pixels = gdk_pixbuf_get_pixels(gdkpixbuf);

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			c = icon[y * 16 + x];
			dest_pixels[0] = (c & 0x001f) << 3;
			dest_pixels[1] = (c & 0x03e0) >> 2;
			dest_pixels[2] = (c & 0x7c00) >> 7;
			dest_pixels += 3;
		}
	}

	if(scale != 1) {
		gdkpixbuf2 = gdk_pixbuf_scale_simple(gdkpixbuf, 16 * scale, 16 * scale, GDK_INTERP_NEAREST);
		g_object_unref(gdkpixbuf);
		return gdkpixbuf2;
	}

	return gdkpixbuf;
}

static gchar* MCDStatusToChar(McdBlock *Info) {
	gchar *state;
	if (ISBLOCKDELETED(Info)) {
		if (ISDELETED(Info))
			state = _("Deleted");
		else
			state = _("Free");
	} else if ((Info->Flags & 0xF0) == 0x50) {
		if ((Info->Flags & 0xF) == 0x1)
			state = _("Used");
		else if (ISLINKMIDBLOCK(Info))
			state = _("Link");
		else if (ISLINKENDBLOCK(Info))
			state = _("End link");
	} else
		state = _("Free");
	return state;
}

u8 GetLinkReference(char* mcddata, const u8 block) {
	u8 i;
	for (i=0; i < MAX_MEMCARD_BLOCKS; i++) {
		u16 link = GETLINKFORBLOCK(mcddata, i)+1; // 0...15 index
		if (link == block) {
			return i;
		}
	}
	return 0;
}

static void OnTreeSelectionChanged(GtkTreeSelection *selection, gpointer user_data);
static void LoadListItems(int mcd, boolean newstore) {
	int i;
	GtkListStore *store;
	GtkWidget *List;
	GtkWidget *dialog;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	gchar *title;
	char *mcddata;

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "McdsDlg"));

	if (mcd == 1) {
		mcddata = Mcd1Data;
		List = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList1"));
	} else {
		mcddata = Mcd2Data;
		List = GTK_WIDGET(gtk_builder_get_object(builder, "GtkCList_McdList2"));
	}

	if (newstore) {
		store = gtk_list_store_new(NUM_CL, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	} else {
		store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(List)));
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	}

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		McdBlock *Info;
		const gchar *state;
		short* iconlinkptr, iconcount;

		Info = &Blocks[mcd - 1][i];
		IconC[mcd - 1][i] = 0;
		state = MCDStatusToChar(Info);

		if (ISSTATUSDELETED(Info)) {
			iconlinkptr = IconDeleted;
		} else if (ISLINKBLOCK(Info)) { // // TODO link icons exists or not?
			iconlinkptr = IconLinked;
		} else {
			iconcount = Info->IconCount>0?Info->IconCount:1;
			iconlinkptr = &Info->Icon[(currentIcon % iconcount) * ICON_SIZE];
		}
		pixbuf = SetIcon(dialog, iconlinkptr, 2);

		if (newstore) gtk_list_store_append(store, &iter);

		GError *error=NULL;
		title = g_convert(Info->sTitle, strlen(Info->sTitle), "UTF-8",
			"Shift-JIS", NULL, NULL, &error);

		if (error)
		{
			// Some characters caused problems because of custom encoding.
			// Let's use the ASCII title as fallback.
			// Otherwise custom decoding from that region
			// of BIOS needed which is way overkill here.
			title = g_convert(Info->Title, strlen(Info->Title), "UTF-8",
				"Shift-JIS", NULL, NULL, NULL);
			g_clear_error(&error);
		}

		gtk_list_store_set(store, &iter,
				CL_ICON, pixbuf,
				CL_TITLE, title,
				CL_STAT, state,
				CL_NAME, Info->Name,
				CL_ID, Info->ID,
				-1);
		

		g_free(title);
		
		g_object_unref(pixbuf);
		if (!newstore) gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}

	if (newstore) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(List), GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(List), TRUE);
	} else {
		OnTreeSelectionChanged(gtk_tree_view_get_selection(GTK_TREE_VIEW(List)), GINT_TO_POINTER(mcd));
	}
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

	LoadListItems(1, TRUE);
	LoadListItems(2, TRUE);

	UpdateFilenameButtons(widget);
}

static void UpdateMcdDlg(GtkWidget *widget) {
	int i;

	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		GetMcdBlockInfo(1, i + 1, &Blocks[0][i]);
		GetMcdBlockInfo(2, i + 1, &Blocks[1][i]);
	}

	LoadListItems(1, FALSE);
	LoadListItems(2, FALSE);

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

		gint mcd = GPOINTER_TO_INT(user_data);
		if (mcd == 1) strncpy(Config.Mcd1, name, MAXPATHLEN);
		else strncpy(Config.Mcd2, name, MAXPATHLEN);

		LoadMcd(mcd, name);
		LoadMcdDlg(widget);

		g_free(name);
	}

	gtk_widget_destroy(chooser);
	g_free(path);
}

static int GetFreeMemcardSlot(gint target_card, gint count, u8* blocks) {
	McdBlock *Info;
	gint foundcount=0, i=-1;

	// search for empty (formatted) blocks first
	while (i < MAX_MEMCARD_BLOCKS && foundcount < count) {
		Info = &Blocks[target_card][++i];
		if ((Info->Flags & 0xFF) == 0xA0) { // if A0 but not A1, etc..
			blocks[foundcount++] = i+1;
		}
	}

	//printf("formatstatus1=%i %i\n", foundcount, count);
	if (foundcount == count)
		return foundcount;

	// not enough free formatted slots, include deleted ones
	i = -1;
	foundcount=0;
	memset(blocks, 0x0, MAX_MEMCARD_BLOCKS*sizeof(u8));
	while (i < MAX_MEMCARD_BLOCKS && foundcount < count) {
		Info = &Blocks[target_card][++i];
		if ((Info->Flags & 0xFF) >= 0xA0) { // A2 or A6 f.e.
			blocks[foundcount++] = i+1;
		} //printf("delstatus=%x\n", Info->Flags);
	}

	//printf("formatstatus2=%i %i\n", foundcount, count);
	if (foundcount == count)
		return foundcount;

	return -1;
}

void CopyMemcardData(char *from, char *to, gint srci, gint dsti,
						gchar *str, const u16 linkindex) {
	u16* linkptr;
	u8* checksumptr;

	// header	
	memcpy(to + dsti * 128, from + srci * 128, 128);

	// Link field to next block (multi block saves)
	linkptr = (u16*)&to[(dsti*128)+0x08];
	if (ISLINKED(*linkptr)) {
		// TODO: link index is 2 bytes, but how can link it be
		// greater than num blocks (> 14), 41161 f.e..?
		checksumptr = &to[(dsti*128)+0x7F]; // update checksum
		*checksumptr ^= ((u8*)(linkptr))[0]; // checksum minus old index (lower byte)
		*checksumptr ^= ((u8*)(linkptr))[1]; // checksum minus old index (upper byte)
		*linkptr = linkindex; // next block in 0...14 index
		*checksumptr ^= linkindex>>8; // checksum plus new index (upper byte)
		*checksumptr ^= linkindex&0xFF; // checksum plus new index (lower byte)
		//printf("link = %i %i\n", dsti, linkindex);
	}

	SaveMcd((char *)str, to, dsti * 128, 128);

	// data	
	memcpy(to + dsti * 1024 * 8, from + srci * 1024 * 8, 1024 * 8);
	SaveMcd((char *)str, to, dsti * 1024 * 8, 1024 * 8);

	//printf("data = %s\n", from + (srci+1) * 128);
}

gint GetMcdBlockCount(gint mcd, u8 startblock, u8* blocks) {
	gint i=0;
	u8 *data, *dataT, curblock=startblock;
	u16 linkblock;
	
	if (mcd == 1) data = Mcd1Data;
	if (mcd == 2) data = Mcd2Data;

	blocks[i++] = startblock;
	do {
		dataT = data+((curblock*128)+0x08); 
		linkblock = ((u16*)dataT)[0];

		// TODO check if target block has link flag (2 or 3)
		linkblock = ( ISLINKED(linkblock) ? linkblock : 0xFFFFU );
		blocks[i++] = curblock = linkblock + 1;
		//printf("LINKS %x %x %x %x %x\n", blocks[0], blocks[i-2], blocks[i-1], blocks[i], blocks[i+1]);
	} while (ISLINKED(linkblock));
	return i-1;
}

static void OnMcd_CopyTo(GtkWidget *widget, gpointer user_data) {
	gint dstmcd = GPOINTER_TO_INT(user_data);
	gint srcmcd;

	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	gint *i, j, count, srci, free_slots;
	GtkTreeSelection *treesel;
	gchar *str;
	char *source, *destination;
	u8* srctbl = calloc(MAX_MEMCARD_BLOCKS, sizeof(u8));
	u8* dsttbl = calloc(MAX_MEMCARD_BLOCKS, sizeof(u8));

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
		srci = *i;
		gtk_tree_path_free(path);
	}

	// get how many blocks source is (including linked blocks)
	count = GetMcdBlockCount(srcmcd, (srci+1), srctbl);

	// Determine the first free slot in the target memory card
	free_slots = GetFreeMemcardSlot((dstmcd - 1), count, dsttbl);
	if (free_slots == -1) {
		// No free slots available on the destination card
		SysErrorMessage(_("No free space on memory card"),
						_("There are no free slots available on the target memory card. Please delete a slot first."));
		goto ret;
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

	for (j=0; srctbl[j] > 0; j++) {
		// last parameter specifies link index (next block)
		CopyMemcardData(source, destination, 
					srctbl[j], dsttbl[j], str, dsttbl[j+1]-1);
		//printf("count = %i, indices=(%x,%x) jindex=%i\n", count, srctbl[j], dsttbl[j], j);
	}

	UpdateMcdDlg(widget);

ret:
	free(srctbl);
	free(dsttbl);
}

static void OnMemcardDelete(GtkWidget *widget, gpointer user_data) {
	McdBlock *Info;
	int xorsum, j;
	u16 i, starti;
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
		i = starti = *gtk_tree_path_get_indices(path);

		// Delete selected file and all linked blocks also (flag value 2 or 3)
		do {
			ptr = data + (++i) * 128;
			Info = &Blocks[memcard - 1][i - 1];
			//printf("Deleting %s %x (%i)\n", Info->ID, Info->Flags, i);

			// N iter: check if link was valid but pointed to normal block
			if (i!=(starti+1) && !ISLINKBLOCK(Info)) {
				SysErrorMessage(_("Memory card is corrupted"),
								_("Link block pointed to normal block which is not allowed."));
				break;
			}
			
			if ((Info->Flags & 0xF0) == 0xA0) {
				if ((Info->Flags & 0xF) >= 1 &&
						(Info->Flags & 0xF) <= 3) { // deleted
					*ptr = 0x50 | (Info->Flags & 0xF);
				} else return;
			} else if ((Info->Flags & 0xF0) == 0x50) { // used
				*ptr = 0xA0 | (Info->Flags & 0xF);
			} else { return; }

			for (j = 0, xorsum = 0; j < 127; j++) {
				xorsum ^= *ptr++;
			}
			*ptr = xorsum;

			SaveMcd((char *)filename, data, i * 128, 128);

			// Check links
			i = GETLINKFORBLOCK(data, i); //0...15 index when ++i at top of loop
		} while (i <= MAX_MEMCARD_BLOCKS);

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
				//gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(builder, "GtkButton_Delete1")), _("Delete"));
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "GtkButton_CopyTo2")), FALSE);
				//gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(builder, "GtkButton_Delete1")), _("Undelete"));
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
	LoadListItems(1, FALSE);
	LoadListItems(2, FALSE);
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

	memset(IconDeleted, 0x18, sizeof(IconDeleted));
	memset(IconLinked, 0x05, sizeof(IconLinked));

	while (gtk_events_pending()) {  gtk_main_iteration(); }
}
