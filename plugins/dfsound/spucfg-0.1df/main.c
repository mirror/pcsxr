#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#endif

#define READBINARY "rb"
#define WRITEBINARY "wb"
#define CONFIG_FILENAME "dfsound.cfg"

GtkBuilder *builder;
GtkWidget *widget, *MainWindow;

void SaveConfig(GtkWidget *widget, gpointer user_datal);

/*	This function checks for the value being outside the accepted range,
	and returns the appropriate boundary value */
int set_limit (char *p, int len, int lower, int upper)
{
	int val = 0;

	if (p)
		val = atoi(p + len);

	if (val < lower)
		val = lower;
	if (val > upper)
		val = upper;

	return val;
}

void on_about_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy (widget);
	exit (0);
}

void OnConfigClose(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "CfgWnd")));
	exit(0);
}

int main(int argc, char *argv[])
{
	FILE *in;
	char t[256];
	int len, val = 0;
	char *pB, *p;
	char cfg[255];

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	if (argc < 2) {
		printf ("Usage: cfgDFSound {about | configure}\n");
		return 0;
	}

	if (strcmp(argv[1], "configure") != 0 && 
		strcmp(argv[1], "about") != 0) {
		printf ("Usage: cfgDFSound {about | configure}\n");
		return 0;
	}

	gtk_init(&argc, &argv);

	if (strcmp(argv[1], "about") == 0) {
		const char *authors[]= {"Pete Bernert and the P.E.Op.S. team", "Ryan Schultz", "Andrew Burton", NULL};
		widget = gtk_about_dialog_new ();
		gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (widget), "dfsound PCSXR Sound Plugin");
		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (widget), "1.6");
		gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (widget), authors);
		gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (widget), "http://pcsx-df.sourceforge.net/");

		g_signal_connect_data(G_OBJECT(widget), "response",
			G_CALLBACK(on_about_clicked), NULL, NULL, G_CONNECT_AFTER);

		gtk_widget_show (widget);
		gtk_main();

		return 0;
	}
	else if (strcmp(argv[1], "configure") == 0) {
		builder = gtk_builder_new();

		if (!gtk_builder_add_from_file(builder, DATADIR "dfsound.ui", NULL)) {
			g_warning("We could not load the interface!");
			return 0;
		}
		
		MainWindow = GTK_WIDGET(gtk_builder_get_object(builder, "CfgWnd"));

		strcpy(cfg, CONFIG_FILENAME);

		in = fopen(cfg, READBINARY);
		if (in) {
			pB = (char *)malloc(32767);
			memset(pB, 0, 32767);
			len = fread(pB, 1, 32767, in);
			fclose(in);
		} else {
			pB = 0;
			printf ("Error - no configuration file\n");
			/* TODO Raise error - no configuration file */
		}

		/* ADB TODO Replace a lot of the following with common functions */
		if (pB) {
			strcpy(t, "\nVolume");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}
			val = set_limit (p, len, -1, 4) + 1;
		} else val = 2;

		gtk_combo_box_set_active(GTK_COMBO_BOX (gtk_builder_get_object(builder, "cbVolume2")), val);

		if (pB) {
		strcpy(t, "\nUseInterpolation");
		p = strstr(pB, t);
		if (p) {
			p = strstr(p, "=");
			len = 1;
		}
			val = set_limit (p, len, 0, 3);
		} else val = 2;

		gtk_combo_box_set_active(GTK_COMBO_BOX (gtk_builder_get_object(builder, "cbInterpolation2")), val);

		if (pB) {
			strcpy(t, "\nXAPitch");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}
			val = set_limit (p, len, 0, 1);
		} else val = 0;

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "chkXASpeed")), val);

		if (pB) {
			strcpy(t, "\nHighCompMode");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}
			val = set_limit (p, len, 0, 1);
		} else val = 1;

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "chkHiCompat")), val);

		if (pB) {
			strcpy(t, "\nSPUIRQWait");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}

			val = set_limit (p, len, 0, 1);
		} else val = 1;

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "chkIRQWait")), val);

		if (pB) {
			strcpy(t, "\nDisStereo");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}

			val = set_limit (p, len, 0, 1);
		} else val = 0;

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "chkDisStereo")), val);

		if (pB) {
			strcpy(t, "\nFreqResponse");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}

			val = set_limit (p, len, 0, 1);
		} else val = 0;

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "chkFreqResponse")), val);

		if (pB) {
			strcpy(t, "\nUseReverb");
			p = strstr(pB, t);
			if (p) {
				p = strstr(p, "=");
				len = 1;
			}
			val = set_limit (p, len, 0, 2);
		} else val = 2;

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbReverb2")), val);

		if (pB)
			free(pB);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "CfgWnd"));
		g_signal_connect_data(G_OBJECT(widget), "destroy",
			G_CALLBACK(SaveConfig), builder, NULL, 0);

		widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
		g_signal_connect_data(G_OBJECT(widget), "clicked",
			G_CALLBACK(OnConfigClose), builder, NULL, G_CONNECT_AFTER);

		gtk_widget_show(MainWindow);
		gtk_main();
	}
	
	return 0;
}

void SaveConfig(GtkWidget *widget, gpointer user_data)
{
	FILE *fp;
	int val;

	fp = fopen(CONFIG_FILENAME, WRITEBINARY);
	if (fp == NULL) {
		fprintf(stderr, "Unable to write to configuration file %s!\n", CONFIG_FILENAME);
		exit(0);
	}

	val = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbVolume2")));
	fprintf(fp, "\nVolume = %d\n", val - 1);

	val = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbInterpolation2")));
	fprintf(fp, "\nUseInterpolation = %d\n", val);

	val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkXASpeed")));
	fprintf(fp, "\nXAPitch = %d\n", val);

	val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkHiCompat")));
	fprintf(fp, "\nHighCompMode = %d\n", val);

	val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkIRQWait")));
	fprintf(fp, "\nSPUIRQWait = %d\n", val);

	val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkDisStereo")));
	fprintf(fp, "\nDisStereo = %d\n", val);

	val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "chkFreqResponse")));
	fprintf(fp, "\nFreqResponse = %d\n", val);

	val = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "cbReverb2")));
	fprintf(fp, "\nUseReverb = %d\n", val);

	fclose(fp);
	exit(0);
}
