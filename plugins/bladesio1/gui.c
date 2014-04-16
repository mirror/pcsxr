/***************************************************************************
 *   Copyright (C) 2013 by Blade_Arma <edgbla@yandex.ru>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "psxcommon.h"
#include "psemu_plugin_defs.h"

#include "cfg.c"
#include "sio1.h"

/***************************************************************************/

#define MAXINTERFACES 16

void sockGetIP(char *IPAddress) {
	int fd, intrface;
	struct ifreq buf[MAXINTERFACES];
	struct ifconf ifc;
	struct sockaddr_in addr;

	strcpy(IPAddress, "127.0.0.1");

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
			intrface = ifc.ifc_len / sizeof(struct ifreq);
			while (intrface-- > 0) {
				if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface]))) {
					memcpy(&addr, &(buf[intrface].ifr_addr), sizeof(addr));
					strcpy(IPAddress, inet_ntoa(addr.sin_addr));
					break;
				}
			}
		}
		close(fd);
	}
}

void cfgSysMessage(const char *fmt, ...) {
	GtkWidget *MsgDlg;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg) - 1] == '\n') msg[strlen(msg) - 1] = 0;

	MsgDlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("NetPlay"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(MsgDlg), "%s", msg);

	gtk_dialog_run(GTK_DIALOG(MsgDlg));
	gtk_widget_destroy(MsgDlg);
}

void OnCopyIP(GtkWidget *widget, gpointer user_data) {
	char str[256];

	sockGetIP(str);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), str, strlen(str));
	cfgSysMessage(_("IP %s"), str);
}

/***************************************************************************/

static
void configure() {
	GtkBuilder *builder;
	GtkWidget *widget, *MainWindow;

	builder = gtk_builder_new();

	if(!gtk_builder_add_from_file(builder, DATADIR "sio1.ui", NULL))
		g_warning("We could not load the interface!");

	settingsRead();

	MainWindow = GTK_WIDGET(gtk_builder_get_object(builder, "dlgStart"));
	gtk_window_set_title(GTK_WINDOW(MainWindow), _("Link Cable Configuration"));

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "btnCopyIP"));
	g_signal_connect_data(G_OBJECT(widget), "clicked",
		G_CALLBACK(OnCopyIP), NULL, NULL, G_CONNECT_AFTER);

	switch(settings.player) {
		case PLAYER_DISABLED:
			widget = GTK_WIDGET(gtk_builder_get_object(builder, "rbDisabled"));
			break;
		case PLAYER_MASTER:
			widget = GTK_WIDGET(gtk_builder_get_object(builder, "rbServer"));
			break;
		case PLAYER_SLAVE:
			widget = GTK_WIDGET(gtk_builder_get_object(builder, "rbClient"));
			break;
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "tbServerIP"));
	gtk_entry_set_text(GTK_ENTRY(widget), settings.ip);

	widget = GTK_WIDGET(gtk_builder_get_object(builder, "tbPort"));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), settings.port);

	if(gtk_dialog_run(GTK_DIALOG(MainWindow)) == GTK_RESPONSE_OK) {
		widget = GTK_WIDGET(gtk_builder_get_object(builder, "tbServerIP"));
		strncpy(settings.ip, gtk_entry_get_text(GTK_ENTRY(widget)), sizeof(settings.ip) - 1);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "tbPort"));
		settings.port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "rbDisabled"));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
			settings.player = PLAYER_DISABLED;
		else {
			widget = GTK_WIDGET(gtk_builder_get_object(builder, "rbServer"));
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
				settings.player = PLAYER_MASTER;
			else
				settings.player = PLAYER_SLAVE;
		}

		settingsWrite();

		gtk_widget_destroy(MainWindow);
	}

	gtk_widget_destroy(MainWindow);
}

static
void about() {
	const char *authors[]= {"edgbla <edgbla@yandex.ru>", NULL};
	GtkWidget *widget;

	widget = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(widget), "Link Cable");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(widget), "1.0");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(widget), authors);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(widget), "http://www.codeplex.com/pcsxr/");

	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
}

/***************************************************************************/

int main(int argc, char *argv[]) {
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gtk_init(&argc, &argv);

	if (argc < 2) {
    	printf ("Usage: cfgBladeSio1 {about | configure}\n");
		return 0;
	}

    if (strcmp(argv[1], "configure") != 0 && 
		strcmp(argv[1], "about") != 0) {
		printf ("Usage: cfgBladeSio1 {about | configure}\n");
		return 0;
    }

	if(!strcmp(argv[1], "configure"))
		configure();
	else if(!strcmp(argv[1], "about"))
		about();

	return 0;
}

/***************************************************************************/
