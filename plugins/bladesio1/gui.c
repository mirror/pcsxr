#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "settings.h"
#include "sio1.h"

/******************************************************************************/

Settings settings;

/******************************************************************************/

long CFGopen()
{
	GtkBuilder *builder;
	GtkWidget *widget, *MainWindow;
	
	//settingsRead();

	builder = gtk_builder_new();

	if (!gtk_builder_add_from_file(builder, DATADIR "sio1.ui", NULL)) {
		g_warning("We could not load the interface!");
		return 0;
	}

	MainWindow = gtk_builder_get_object(builder, "dlgStart");
	gtk_window_set_title(GTK_WINDOW(MainWindow), _("Link cable"));

	if (settings.player == 1) {
		widget = gtk_builder_get_object(builder, "rbServer");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	} else {
		widget = gtk_builder_get_object(builder, "rbClient");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	}
	
	if (gtk_dialog_run(GTK_DIALOG(MainWindow)) == GTK_RESPONSE_OK) {
		widget = gtk_builder_get_object(builder, "tbServerIP");
		strncpy(settings.ip, gtk_entry_get_text(GTK_ENTRY(widget)), sizeof(settings.ip) - 1);

		widget = gtk_builder_get_object(builder, "tbPort");
		settings.port = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

		widget = gtk_builder_get_object(builder, "rbServer");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
			settings.player = 1;
		} else {
			settings.player = 2;
		}

		//settingsWrite();
		
		gtk_widget_destroy(MainWindow);
		return 1;
	}

	gtk_widget_destroy(MainWindow);

	return 0;
}

int main( int argc, char *argv[] )
{
#ifdef ENABLE_NLS
	setlocale( LC_ALL, "" );
	bindtextdomain( GETTEXT_PACKAGE, LOCALE_DIR );
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
	textdomain( GETTEXT_PACKAGE );
#endif

	gtk_set_locale();
	gtk_init( &argc, &argv );

	if( argc > 1 )
	{
		if( !strcmp(argv[1], "open") )
		{
			return CFGopen();
		}
	}
	
	return 0;
}

/******************************************************************************/
