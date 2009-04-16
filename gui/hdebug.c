/***************************************************************************
 *   Debugger-Interface for PCSX-DF                                        *
 *                                                                         *
 *   Copyright (C) 2008 Stefan Sikora                                      *
 *   hoshy['at']schrauberstube['dot']de                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

// TODO:
// - setting register values
// - step over instruction
// - Dumping/Loading of memory
// - Better gui-integration 

#include <gtk/gtk.h>
#include <string.h>
#include <pthread.h>
#include "r3000a.h"
#include "hdebug.h"

// Global variables
char buffer[1024*2];
GtkTextBuffer *hdb_listing = 0;
GtkTextBuffer *hdb_memdump = 0;
GtkTextBuffer *hdb_logging = 0;
GtkTextBuffer *hdb_registers = 0;
GtkWidget *hdb_command = 0;
GtkWidget *hdb_pausebutton = 0;

u32 hdb_memptr = 0x80000000;

int ready = 0;

// little helper for syncing with emulator
void waitforpause() {
	int actualPC;
	
	actualPC = psxRegs.pc;
	while(actualPC != psxRegs.pc) actualPC = psxRegs.pc;

	gtk_button_set_label(GTK_BUTTON(hdb_pausebutton), "RESUME");
}


// core-debugger-functions
void hdb_update_registers() {
	snprintf(buffer, 1024,
					 "v0 0x%08x   s0 0x%08x   t0 0x%08x\n" \
					 "v1 0x%08x   s1 0x%08x   t1 0x%08x\n" \
					 "                s2 0x%08x   t2 0x%08x\n" \
					 "a0 0x%08x   s3 0x%08x   t3 0x%08x\n" \
					 "a1 0x%08x   s4 0x%08x   t4 0x%08x\n" \
					 "a2 0x%08x   s5 0x%08x   t5 0x%08x\n" \
					 "a3 0x%08x   s6 0x%08x   t6 0x%08x\n" \
					 "                s7 0x%08x   t7 0x%08x\n" \
					 "k0 0x%08x   s8 0x%08x   t8 0x%08x\n" \
					 "k1 0x%08x                   t9 0x%08x\n\n" \
					 "gp 0x%08x   at 0x%08x   ra 0x%08x\n" \
					 "sp 0x%08x                   pc 0x%08x\n",
					 psxRegs.GPR.r[2], psxRegs.GPR.r[16], psxRegs.GPR.r[8],
					 psxRegs.GPR.r[3], psxRegs.GPR.r[17], psxRegs.GPR.r[9],
					 psxRegs.GPR.r[18], psxRegs.GPR.r[10],
					 psxRegs.GPR.r[4], psxRegs.GPR.r[19], psxRegs.GPR.r[11],
					 psxRegs.GPR.r[5], psxRegs.GPR.r[20], psxRegs.GPR.r[12],
					 psxRegs.GPR.r[6], psxRegs.GPR.r[21], psxRegs.GPR.r[13],
					 psxRegs.GPR.r[7], psxRegs.GPR.r[22], psxRegs.GPR.r[14],
					 psxRegs.GPR.r[23], psxRegs.GPR.r[15],
					 psxRegs.GPR.r[26], psxRegs.GPR.r[30], psxRegs.GPR.r[24],
					 psxRegs.GPR.r[27], psxRegs.GPR.r[25],
					 psxRegs.GPR.r[28], psxRegs.GPR.r[1], psxRegs.GPR.r[31],
					 psxRegs.GPR.r[29], psxRegs.pc);

	gtk_text_buffer_set_text(hdb_registers, buffer, strlen(buffer));
}


void hdb_update_listing(u32 opc) {
	int t;
	u32 *cptr;
	u32 ocod;
	char tbuf[100];
	char *bptr;
	
	buffer[0] = '\0';
	bptr = (char *)&buffer;	
	opc -= 15*4;

	for(t=0; t<=30; t++) {
		cptr = (u32 *)PSXM(opc);
		if (t == 15) strcat(bptr, ">"); else strcat(bptr, " ");
		ocod = cptr == NULL ? 0 : SWAP32(*cptr);
		sprintf(tbuf, "%s\n", disR3000AF(ocod, opc));
		strcat(bptr, tbuf);
		opc+=4;
	}
	strcat(bptr,"\0");
	gtk_text_buffer_set_text(hdb_listing, buffer, strlen(buffer));
}


void hdb_update_memdump() {
	int r, c;
	char tbuf[100];
	char *bptr;
	
	buffer[0] = '\0';
	bptr = (char*)&buffer;

	for(r=0; r<8; r++) {
		sprintf(tbuf, "0x%08x", hdb_memptr+r*16);
		strcat(bptr, tbuf);
		for(c=0; c<16; c++) {
			if(c==8) sprintf(tbuf, "-%02x", PSXMu8(hdb_memptr+r*16+c));
			else sprintf(tbuf, " %02x", PSXMu8(hdb_memptr+r*16+c));
			strcat(bptr, tbuf);
		}
		strcat(bptr, "\n");
	}
	strcat(bptr, "\0");
	gtk_text_buffer_set_text(hdb_memdump, buffer, strlen(buffer));
}


// Callback-functions
void on_hdb_mainwindow_destroy(GtkWidget *widget, gpointer data) {
	hdb_pause = 0;		// Run CPU
	gtk_widget_destroy((GtkWidget*)widget);
    gtk_main_quit();
    while (gtk_events_pending()) gtk_main_iteration();
	pthread_exit(NULL);
    ready = 0;
}


void on_hdb_pausebutton_clicked(GtkWidget *widget, gpointer data) {
	if(hdb_pause == 0) {
		hdb_pause = 1;
		waitforpause();

		hdb_update_registers();
		hdb_update_listing(psxRegs.pc);
		hdb_update_memdump();
	}
	else {
		hdb_pause = 0;
		gtk_button_set_label(GTK_BUTTON(widget), "PAUSE");
	}
}

void hdb_auto_pause () {
	if(hdb_pause == 0) {
		hdb_pause = 1;
		//waitforpause();

		hdb_update_registers();
		hdb_update_listing(psxRegs.pc);
		hdb_update_memdump();
	}
	else {
		hdb_pause = 0;
		//gtk_button_set_label(GTK_BUTTON(widget), "PAUSE");
	}
}

void on_hdb_tracebutton_clicked(GtkWidget *widget, gpointer data) {
	// Let emulator do one step and refresh debugger when in pause-mode!
	if(hdb_pause == 1) {
		hdb_pause = 2;

		waitforpause();
		hdb_update_registers();
		hdb_update_listing(psxRegs.pc);
	}
}


void on_hdb_dumpbutton_clicked(GtkWidget *widget, gpointer data) {
	// Save psx-memory for external analysis/modification
	// TODO
}


void on_hdb_loadbutton_clicked(GtkWidget *widget, gpointer data) {
	// Load psx-memory
	// TODO
}


void on_hdb_cmdbutton_clicked(GtkWidget *widget, gpointer data) {
	const gchar *cmdentry;
	char *actcmd;
	char *cmdsub;
	u32 tval;
	u8	tval8;
	int t;

	if ((cmdentry = gtk_entry_get_text(GTK_ENTRY(hdb_command))) != NULL) {
		actcmd = strdup(cmdentry);
		/* split and interpret debugger's commandline
			t - trace one instruction
			c - continue execution
			d addr - disassemble at addr
			m addr - show memory at addr (hex)
			s addr op1 op2 op3 ... - set memory
			b addr - break on addr
			bc - clear breakpoint
			r reg val - set register to a specific value (TODO)
		*/

		// get the command
		if ((cmdsub = strtok(actcmd, " ")) != NULL) {
			if(!strcmp(cmdsub, "t")) {
				hdb_pause = 2;
				usleep(100);			// wait 1/10th of a second
				hdb_update_registers();
				hdb_update_listing(psxRegs.pc);
			}

			if(!strcmp(cmdsub, "c")) hdb_pause = 0;	

			if(!strcmp(cmdsub, "bc")) {
				hdb_break = 0;
			}

			if(!strcmp(cmdsub, "b")) {
				cmdsub = strtok(NULL, " ");
				sscanf(cmdsub, "%x", (u32*)&tval);
				hdb_break = tval;
				hdb_pause = 3;

				waitforpause();
				hdb_update_memdump();
				hdb_update_registers();
				hdb_update_listing(psxRegs.pc);
			}

			if(!strcmp(cmdsub, "d")) {
				cmdsub = strtok(NULL, " ");
				sscanf(cmdsub, "%x", (u32*)&tval);
				hdb_update_listing(tval);
			}

			if(!strcmp(cmdsub, "m")) {
				cmdsub = strtok(NULL, " ");
				sscanf(cmdsub, "%x", (u32*)&tval);
				hdb_memptr = tval;
				hdb_update_memdump();
			}

			if(!strcmp(cmdsub, "s")) {
				cmdsub = strtok(NULL, " ");
				sscanf(cmdsub, "%x", (u32 *)&tval);
				t = 0;
				while((cmdsub = strtok(NULL, " "))) {
					u8 v;
					sscanf(cmdsub, "%x", (u8 *)&tval8);
					PSXMu8(tval+t) = tval8;
					t++;
				}
				hdb_update_memdump();
				hdb_update_listing(psxRegs.pc);
			}

			free(actcmd);
		}
		else {
			hdb_update_memdump();
			hdb_update_registers();
			hdb_update_listing(psxRegs.pc);
		}
	}
}


// init debugger and gui
void hdb_init() {
        if (ready) return;
	GtkWidget *hdb_mainwindow;
	GtkWidget *hdb_tracebutton;
	GtkWidget *hdb_dumpbutton;
	GtkWidget *hdb_loadbutton;
	GtkWidget *hdb_cmdbutton;
	GtkWidget *hdb_listbox, *hdb_membox, *hdb_logbox, *hdb_regbox;
	GtkWidget *hbox, *vbox, *hbox2, *hbox3;
	PangoFontDescription *font_desc;
		
	hdb_pause = 0;		// Run CPU
	
	gtk_init(NULL, NULL);
	font_desc = pango_font_description_from_string ("Fixed 8");
	
    // create and show the widgets
    hdb_mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title((GtkWindow*)hdb_mainwindow, "PCSX-DF Debugger");
    gtk_window_set_position((GtkWindow*)hdb_mainwindow, GTK_WIN_POS_CENTER);
    
	hbox = gtk_hbox_new(FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 2);
	hbox2 = gtk_hbox_new(FALSE, 2);
	hbox3 = gtk_hbox_new(FALSE, 2);
	
	hdb_registers = gtk_text_buffer_new(NULL);
	hdb_regbox = gtk_text_view_new_with_buffer(hdb_registers);
	gtk_widget_modify_font(hdb_regbox, font_desc);

	hdb_listing = gtk_text_buffer_new(NULL);
	hdb_listbox = gtk_text_view_new_with_buffer(hdb_listing);
	gtk_widget_modify_font(hdb_listbox, font_desc);
	
	hdb_memdump = gtk_text_buffer_new(NULL);
	hdb_membox = gtk_text_view_new_with_buffer(hdb_memdump);
	gtk_widget_modify_font(hdb_membox, font_desc);

	hdb_logging = gtk_text_buffer_new(NULL);
	hdb_logbox = gtk_text_view_new_with_buffer(hdb_logging);
	gtk_widget_modify_font(hdb_logbox, font_desc);

	hdb_command = gtk_entry_new();
	hdb_cmdbutton = gtk_button_new_with_label(">");

	hdb_pausebutton = gtk_button_new_with_label("PAUSE");
	hdb_tracebutton = gtk_button_new_with_label("TRACE");
	hdb_dumpbutton = gtk_button_new_with_label("DUMP");
	hdb_loadbutton = gtk_button_new_with_label("LOAD");

	gtk_container_add(GTK_CONTAINER(hdb_mainwindow), hbox);
	gtk_container_add(GTK_CONTAINER(hbox), vbox);
	gtk_container_add(GTK_CONTAINER(hbox), hdb_listbox);

	gtk_container_add(GTK_CONTAINER(vbox), hdb_regbox);
	gtk_container_add(GTK_CONTAINER(vbox), hdb_membox);
	gtk_container_add(GTK_CONTAINER(vbox), hdb_logbox);

	gtk_container_add(GTK_CONTAINER(hbox3), hdb_command);
	gtk_container_add(GTK_CONTAINER(hbox3), hdb_cmdbutton);

	gtk_container_add(GTK_CONTAINER(vbox), hbox3);
	gtk_container_add(GTK_CONTAINER(vbox), hbox2);
	gtk_container_add(GTK_CONTAINER(hbox2), hdb_pausebutton);
	gtk_container_add(GTK_CONTAINER(hbox2), hdb_tracebutton);
	gtk_container_add(GTK_CONTAINER(hbox2), hdb_dumpbutton);
	gtk_container_add(GTK_CONTAINER(hbox2), hdb_loadbutton);

	pango_font_description_free (font_desc);
  	
    gtk_widget_show_all(hdb_mainwindow);

    // connect signals to callback functions
    g_signal_connect(G_OBJECT(hdb_mainwindow), "destroy", G_CALLBACK(on_hdb_mainwindow_destroy), NULL);
    g_signal_connect(G_OBJECT(hdb_pausebutton), "clicked", G_CALLBACK(on_hdb_pausebutton_clicked), NULL);
    g_signal_connect(G_OBJECT(hdb_tracebutton), "clicked", G_CALLBACK(on_hdb_tracebutton_clicked), NULL);
    g_signal_connect(G_OBJECT(hdb_dumpbutton), "clicked", G_CALLBACK(on_hdb_dumpbutton_clicked), NULL);
    g_signal_connect(G_OBJECT(hdb_loadbutton), "clicked", G_CALLBACK(on_hdb_loadbutton_clicked), NULL);
    g_signal_connect(G_OBJECT(hdb_cmdbutton), "clicked", G_CALLBACK(on_hdb_cmdbutton_clicked), NULL);

	hdb_update_registers();
	hdb_update_listing(psxRegs.pc);
	hdb_update_memdump();

	ready = 1;
	gtk_main();
	pthread_exit(NULL);
}


// Start debugger in own thread
void hdb_start() {
	pthread_t tid;

	pthread_create(&tid, NULL, (void *)hdb_init, NULL);
//	hdb_init();
}
