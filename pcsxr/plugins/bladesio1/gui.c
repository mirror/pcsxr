#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

long CFGopen()
{
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

	if( !strcmp(argv[1], "open") )
    {
		return CFGopen();
	}
	
	return 0;
}
