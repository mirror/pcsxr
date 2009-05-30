#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <glade/glade.h>
#include <gtk/gtk.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#define READBINARY "rb"
#define WRITEBINARY "wb"
#define CONFIG_FILENAME "dfsound.cfg"

void SaveConfig (GtkWidget *widget, gpointer user_datal);

/* ADB DONE
GtkWidget *wndMain = 0;

GtkWidget *create_CfgWnd(GladeXML * xml)
{
    wndMain = glade_xml_get_widget(xml, "CfgWnd");
    return wndMain;
}

GtkWidget *create_AboutWnd(GladeXML * xml)
{
    wndMain = glade_xml_get_widget(xml, "AboutWnd");
    return wndMain;
}*/

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

void on_config_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy (widget);
	exit (0);
}

int main(int argc, char *argv[])
{
    GtkWidget *widget;
	GtkWidget *wndMain;
    GladeXML *xml;
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

    if (argc != 2) {
    	printf ("Usage: cfgDFSound {ABOUT | CFG}\n");
		return 0;
	}

    if (strcmp(argv[1], "CFG") != 0 && strcmp(argv[1], "ABOUT") != 0) {
		printf ("Usage: cfgDFSound {ABOUT | CFG}\n");
		return 0;
    }

    gtk_set_locale();
    gtk_init(&argc, &argv);

    if (strcmp(argv[1], "ABOUT") == 0) {
		const char *authors[]= {"Pete Bernert and the P.E.Op.S. team", "Ryan Schultz", "Andrew Burton", NULL};
		widget = gtk_about_dialog_new ();
		gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (widget), "dfsound PCSX Sound Plugin");
		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (widget), "1.6");
		gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (widget), authors);
		gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (widget), "http://home.t-online.de/home/PeteBernert/");

		g_signal_connect_data(GTK_OBJECT(widget), "response",
			GTK_SIGNAL_FUNC(on_about_clicked), NULL, NULL, G_CONNECT_AFTER);

		gtk_widget_show (widget);
		gtk_main();

		return 0;
    }

    xml = glade_xml_new(DATADIR "dfsound.glade2", "CfgWnd", NULL);
    if (!xml) {
		g_warning("We could not load the interface!");
		return 255;
    }

    wndMain = glade_xml_get_widget(xml, "CfgWnd");

    strcpy(cfg, CONFIG_FILENAME);

    in = fopen(cfg, READBINARY);
    if (in) {
		pB = (char *) malloc(32767);
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
	    val = set_limit (p, len, 0, 4);
    } else val = 1;

    gtk_combo_box_set_active(GTK_COMBO_BOX (glade_xml_get_widget(xml, "cbVolume2")), val);

    if (pB) {
	strcpy(t, "\nUseInterpolation");
	p = strstr(pB, t);
	if (p) {
	    p = strstr(p, "=");
	    len = 1;
	}
	    val = set_limit (p, len, 0, 3);
    } else val = 2;

    gtk_combo_box_set_active(GTK_COMBO_BOX (glade_xml_get_widget(xml, "cbInterpolation2")), val);

    if (pB) {
	strcpy(t, "\nUseXA");
	p = strstr(pB, t);
	if (p) {
	    p = strstr(p, "=");
	    len = 1;
	}
		val = set_limit (p, len, 0, 1);
    } else val = 1;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chkEnableXA")), val);

    if (pB) {
		strcpy(t, "\nXAPitch");
		p = strstr(pB, t);
		if (p) {
		    p = strstr(p, "=");
	    	len = 1;
		}
		val = set_limit (p, len, 0, 1);
    } else val = 0;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chkXASpeed")), val);

    if (pB) {
		strcpy(t, "\nHighCompMode");
		p = strstr(pB, t);
		if (p) {
		    p = strstr(p, "=");
	    	len = 1;
		}
		val = set_limit (p, len, 0, 1);
    } else val = 0;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chkHiCompat")), val);

    if (pB) {
		strcpy(t, "\nSPUIRQWait");
		p = strstr(pB, t);
		if (p) {
		    p = strstr(p, "=");
		    len = 1;
		}

		val = set_limit (p, len, 0, 1);
    } else val = 1;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chkIRQWait")), val);

    if (pB) {
		strcpy(t, "\nDisStereo");
		p = strstr(pB, t);
		if (p) {
		    p = strstr(p, "=");
		    len = 1;
		}

		val = set_limit (p, len, 0, 1);
    } else val = 0;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chkDisStereo")), val);

    if (pB) {
		strcpy(t, "\nUseReverb");
		p = strstr(pB, t);
		if (p) {
		    p = strstr(p, "=");
		    len = 1;
		}
		val = set_limit (p, len, 0, 2);
    } else val = 2;

    gtk_combo_box_set_active(GTK_COMBO_BOX (glade_xml_get_widget(xml, "cbReverb2")), val);

    if (pB)
		free(pB);

	widget = glade_xml_get_widget(xml, "CfgWnd");
	g_signal_connect_data(GTK_OBJECT(widget), "delete_event",
			GTK_SIGNAL_FUNC(on_config_clicked), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_close");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(SaveConfig), xml, NULL, G_CONNECT_AFTER);

    gtk_main();
    return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////

void SetCfgVal(char *pB, char *pE, int val)
{
    char *p, *ps, *pC;
    char t[32];

    sprintf(t,"%d",val);
//    printf("%s - %s - %d\n", pB, pE, val);

    p = strstr(pB, pE);
    if (p) {
		p = strstr(p, "=");
		if (!p)
		    return;
		p++;
		while (*p && *p != '\n' && (*p < '0' || *p > '9'))
		    p++;
		if (*p == 0 || *p == '\n')
		    return;
		ps = p;
		while (*p >= '0' && *p <= '9')
		    p++;
		pC = (char *) malloc(32767);
		strcpy(pC, p);
		strcpy(ps, t);
		strcat(pB, pC);
		free(pC);
    } else {
		strcat(pB, pE);
		strcat(pB, " = ");
		strcat(pB, t);
		strcat(pB, "\n");
    }

}

void SaveConfig(GtkWidget *widget, gpointer user_data)
{
    FILE *in;
    GladeXML *xml;
    int len, val;
    char *pB;
    char cfg[255];

    pB = (char *) malloc(32767);
    memset(pB, 0, 32767);

    strcpy(cfg, CONFIG_FILENAME);

	/* ADB TODO Why do we read this in to just replace it again? */
    in = fopen(cfg, READBINARY);
    if (in) {
		len = fread(pB, 1, 32767, in);
		fclose(in);
    }

	xml = (GladeXML*) user_data;

    val = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (xml, "cbVolume2")));
    SetCfgVal(pB, "\nVolume", val);

    val = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (xml, "cbInterpolation2")));
	SetCfgVal(pB, "\nUseInterpolation", val);

    val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chkEnableXA")));
	SetCfgVal(pB, "\nUseXA", val);

    val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chkXASpeed")));
    SetCfgVal(pB, "\nXAPitch", val);

    val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chkHiCompat")));
    SetCfgVal(pB, "\nHighCompMode", val);

    val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chkIRQWait")));
    SetCfgVal(pB, "\nSPUIRQWait", val);

    val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chkDisStereo")));
    SetCfgVal(pB, "\nDisStereo", val);

    val = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (xml, "cbReverb2")));
    SetCfgVal(pB, "\nUseReverb", val);

    if ((in = fopen(cfg, WRITEBINARY)) != NULL) {
	fwrite(pB, strlen(pB), 1, in);
	fclose(in);
    }	/* ADB TODO Error checking? */

    free(pB);

    gtk_widget_destroy (glade_xml_get_widget (xml, "CfgWnd"));
//    g_free (xml);

	/* Close the window and exit control from the plugin */
	exit (0);
}
