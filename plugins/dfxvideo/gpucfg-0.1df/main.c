#include <gtk/gtk.h>

#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

GtkBuilder *builder;
GtkWidget *widget, *MainWindow;

void SaveConfig(GtkWidget *widget, gpointer user_datal);

#define READBINARY "rb"
#define WRITEBINARY "wb"
#define CONFIG_FILENAME "dfxvideo.cfg"

enum {
	VIDMODE_320x200 = 0,
	VIDMODE_640x480,
	VIDMODE_800x600,
	VIDMODE_1024x768,
	VIDMODE_1152x864,
	VIDMODE_1280x1024,
	VIDMODE_1600x1200
}; /* Video_modes */

/*ADB static GtkWidget * wndMain=0;*/

/*	This function checks for the value being outside the accepted range,
	and returns the appropriate boundary value */
int set_limit (char *p, int len, int lower, int upper)
{
	int val = 0;

	if (p)
		val = atoi(p + len);
	/* printf("Checking for val %d greater than %d and lower than %d, ", val, lower, upper);*/
	if (val < lower)
		val = lower;
	if (val > upper)
		val = upper;
	/* printf ("val is now %d\n", val);*/
	return val;
}

void on_about_clicked(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy (widget);
	exit (0);
}

void on_fullscreen_toggled(GtkWidget *widget, gpointer user_data)
{
	GtkToggleButton *check;
	GtkComboBox *resCombo2;

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "checkFullscreen"));
	resCombo2 = GTK_COMBO_BOX(gtk_builder_get_object(builder, "resCombo2"));

	gtk_widget_set_sensitive(GTK_WIDGET(resCombo2), !gtk_toggle_button_get_active(check));
}

void on_use_fixes_toggled(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *table_fixes;
	GtkToggleButton* check;
	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder,"checkUseFixes"));

	table_fixes = GTK_WIDGET(gtk_builder_get_object(builder,"table_fixes"));

	/* Set the state of each of the fixes to the value of the use fixes toggle */
	gtk_container_foreach (GTK_CONTAINER (table_fixes), (GtkCallback) gtk_widget_set_sensitive,
		GINT_TO_POINTER(gtk_toggle_button_get_active (check)));
}

void on_fps_toggled(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *checkSetFPS, *checkAutoFPSLimit, *entryFPS;

	checkSetFPS = GTK_WIDGET(gtk_builder_get_object(builder, "checkSetFPS"));
	checkAutoFPSLimit = GTK_WIDGET(gtk_builder_get_object(builder, "checkAutoFPSLimit"));
	entryFPS = GTK_WIDGET(gtk_builder_get_object(builder, "entryFPS"));

	gtk_widget_set_sensitive(entryFPS, 
							 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkSetFPS)) &&
							 !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkAutoFPSLimit)));
	gtk_widget_set_sensitive(checkAutoFPSLimit, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkSetFPS)));
}

void OnConfigClose(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(builder, "CfgWnd")));
	exit(0);
}

int
main (int argc, char *argv[])
{
	GtkWidget *CfgWnd, *widget;
	FILE *in;char t[256];int len,val;
	float valf;
	char * pB, * p;
	char cfg[255];
	int i;
	char tempstr[50];

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	if (argc < 2) {
		printf ("Usage: cfgDFXVideo {about | configure}\n");
		return 0;
	}

	if (strcmp(argv[1], "configure") != 0 && 
		strcmp(argv[1], "about") != 0) {
		printf ("Usage: cfgDFXVideo {about | configure}\n");
		return 0;
	}

	gtk_init (&argc, &argv);

	if (strcmp(argv[1], "about") == 0) {
		const char *authors[]= {"Pete Bernert and the P.E.Op.S. team", "Ryan Schultz", "Andrew Burton", NULL};
		widget = gtk_about_dialog_new ();
		gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (widget), "P.E.Op.S PCSXR Video Plugin");
		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (widget), "1.17");
		gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (widget), authors);
		gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (widget), "http://pcsx-df.sourceforge.net/");

		g_signal_connect_data(G_OBJECT(widget), "response",
			G_CALLBACK(on_about_clicked), NULL, NULL, G_CONNECT_AFTER);

		gtk_widget_show (widget);
		gtk_main();

		return 0;
	}
	else {
		builder = gtk_builder_new();
		
		if (!gtk_builder_add_from_file(builder, DATADIR "dfxvideo.ui", NULL)) {
			g_warning("We could not load the interface!");
			return -1;
		}

		/*ADB wndMain = gtk_builder_get_object(builder, "CfgWnd");*/

		strcpy(cfg, CONFIG_FILENAME);

		in = fopen(cfg,READBINARY);
		/* ADB TODO This is bad - asking for problems; need to read in line by line */
		if(in)
		{
			pB=(char *)malloc(32767);
			memset(pB,0,32767);
			len = fread(pB, 1, 32767, in);
			fclose(in);
		}
		else{ pB=0;printf("Couldn't find config file %s\n", cfg);}
		/* ADB TODO Parse this like we parse the config file in PCSXR - use common functions! */
		val=1;
		if(pB)
		{
			strcpy(t,"\nResX");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
			val = set_limit (p, len, 0, 1600);
		}

		if (val == 1600) val = VIDMODE_1600x1200;
		else if (val == 1280) val = VIDMODE_1280x1024;
		else if (val == 1152) val = VIDMODE_1152x864;
		else if (val == 1024) val = VIDMODE_1024x768;
		else if (val == 800) val = VIDMODE_800x600;
		else if (val == 640) val = VIDMODE_640x480;
		else if (val == 320) val = VIDMODE_320x200;

		gtk_combo_box_set_active(GTK_COMBO_BOX (gtk_builder_get_object(builder, "resCombo2")), val);

		val=0;
		if(pB)
		{
			strcpy(t,"\nNoStretch");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

			val = set_limit (p, len, 0, 9);
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX (gtk_builder_get_object(builder, "stretchCombo2")), val);

		val=0;
		if(pB)
		{
			strcpy(t,"\nDithering");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

				val = set_limit (p, len, 0, 2);
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX (gtk_builder_get_object(builder, "ditherCombo2")), val);

		val=0;
		if(pB)
		{
			strcpy(t,"\nMaintain43");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

			val = set_limit (p, len, 0, 1);
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "maintain43")), val);
		
		val=0; //ADB Leave - these are default values
		if(pB)
		{
			strcpy(t,"\nFullScreen");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

				val = set_limit (p, len, 0, 1);
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "checkFullscreen")), val);

		val=0;
		if(pB)
		{
			strcpy(t,"\nShowFPS");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

				val = set_limit (p, len, 0, 1);
		}

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "checkShowFPS")), val);

		val=1;
		if(pB)
		{
			strcpy(t,"\nUseFrameLimit");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

				val = set_limit (p, len, 0, 1);
		}
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "checkSetFPS")), val);

		val=0;
		if(pB)
		{
			strcpy(t,"\nFPSDetection");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

				val = set_limit (p, len, 1, 2);
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "checkAutoFPSLimit")), (val-1));

		val=0;
		if(pB)
		{
			strcpy(t,"\nUseFrameSkip");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
				val = set_limit (p, len, 0, 1);
		}

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "checkFrameSkip")), val);

		valf=200;
		if(pB)
		{
			strcpy(t,"\nFrameRate");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
			if(p) valf=(float)atoi(p+len) / 10;
			if(valf<1) valf=1;
			if(valf>500) valf=500;
		}
		sprintf(tempstr,"%.1f",valf);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "entryFPS")), tempstr);

		val=0;
		if(pB)
		{
			strcpy(t,"\nUseFixes");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

				val = set_limit (p, len, 0, 1);
		}
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder, "checkUseFixes")), val);

		
			if(pB)
			{
				strcpy(t,"\nCfgFixes");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
				if (p)
					val = atoi(p + len);
			}

			for (i=0; i<12; i++)
			{
				sprintf(tempstr, "checkFix%d", i+1);
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,tempstr)), (val>>i)&1 );
			}


		if(pB) free(pB);

			widget = GTK_WIDGET(gtk_builder_get_object(builder, "CfgWnd"));
			g_signal_connect_data(G_OBJECT(widget), "destroy",
					G_CALLBACK(SaveConfig), NULL, NULL, 0);

			widget = GTK_WIDGET(gtk_builder_get_object(builder, "btn_close"));
			g_signal_connect_data(G_OBJECT(widget), "clicked",
					G_CALLBACK(OnConfigClose), NULL, NULL, G_CONNECT_AFTER);

			widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkFullscreen"));
			g_signal_connect_data(G_OBJECT(widget), "clicked",
					G_CALLBACK(on_fullscreen_toggled), NULL, NULL, G_CONNECT_AFTER);

			widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkUseFixes"));
			g_signal_connect_data(G_OBJECT(widget), "clicked",
					G_CALLBACK(on_use_fixes_toggled), NULL, NULL, G_CONNECT_AFTER);

			widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkSetFPS"));
			g_signal_connect_data(G_OBJECT(widget), "clicked",
					G_CALLBACK(on_fps_toggled), NULL, NULL, G_CONNECT_AFTER);

			widget = GTK_WIDGET(gtk_builder_get_object(builder, "checkAutoFPSLimit"));
			g_signal_connect_data(G_OBJECT(widget), "clicked",
					G_CALLBACK(on_fps_toggled), NULL, NULL, G_CONNECT_AFTER);

			on_fullscreen_toggled(widget, NULL);
			on_fps_toggled(widget, NULL);
			on_use_fixes_toggled(widget, NULL);

		gtk_main ();
	}
	
	return 0;
}


void SetCfgVal(char * pB,char * pE,int val)
{
	char * p, *ps, *pC;char t[32];

	sprintf(t,"%d",val);

	p=strstr(pB,pE);
	if(p)
	{
		p=strstr(p,"=");
		if(!p) return;
		p++;
		while(*p && *p!='\n' && (*p<'0' || *p>'9')) p++;
		if(*p==0 || *p=='\n') return;
		ps=p;
		while(*p>='0' && *p<='9') p++;
		pC=(char *)malloc(32767);
		strcpy(pC,p);
		strcpy(ps,t);
		strcat(pB,pC);
		free(pC);
	}
	else
	{
		strcat(pB,pE);
		strcat(pB," = ");
		strcat(pB,t);
		strcat(pB,"\n");
	}
}

void SaveConfig(GtkWidget *widget, gpointer user_data)
{
	FILE *in;int len,val;char * pB;
	char cfg[255];
	char tempstr[50];
	int i;
	struct stat buf;

	pB=(char *)malloc(32767);
	memset(pB,0,32767);

	strcpy(cfg, CONFIG_FILENAME);

	/* ADB TODO Why do we read this in just to replace it again? */
	in = fopen(cfg,READBINARY);
	if(in)
	{
		len = fread(pB, 1, 32767, in);
		fclose(in);
	}

	val = gtk_combo_box_get_active (GTK_COMBO_BOX (gtk_builder_get_object(builder,"resCombo2")));

	if (val == VIDMODE_320x200) { SetCfgVal(pB,"\nResX",320); SetCfgVal(pB,"\nResY",240); }
	else if (val == VIDMODE_640x480) { SetCfgVal(pB,"\nResX",640); SetCfgVal(pB,"\nResY",480); }
	else if (val == VIDMODE_800x600) { SetCfgVal(pB,"\nResX",800); SetCfgVal(pB,"\nResY",600); }
	else if (val == VIDMODE_1024x768) { SetCfgVal(pB,"\nResX",1024); SetCfgVal(pB,"\nResY",768); }
	else if (val == VIDMODE_1152x864) { SetCfgVal(pB,"\nResX",1152); SetCfgVal(pB,"\nResY",864); }
	else if (val == VIDMODE_1280x1024) { SetCfgVal(pB,"\nResX",1280); SetCfgVal(pB,"\nResY",1024); }
	else if (val == VIDMODE_1600x1200) { SetCfgVal(pB,"\nResX",1600); SetCfgVal(pB,"\nResY",1200); }

	val = gtk_combo_box_get_active (GTK_COMBO_BOX (gtk_builder_get_object(builder,"stretchCombo2")));
	SetCfgVal(pB,"\nNoStretch",val);

	val = gtk_combo_box_get_active (GTK_COMBO_BOX (gtk_builder_get_object(builder,"ditherCombo2")));
	SetCfgVal(pB,"\nDithering",val);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"maintain43")));
	SetCfgVal(pB,"\nMaintain43",val);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"checkFullscreen")));
	SetCfgVal(pB,"\nFullScreen",val);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"checkShowFPS")));
	SetCfgVal(pB,"\nShowFPS",val);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"checkSetFPS")));
	SetCfgVal(pB,"\nUseFrameLimit",val);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"checkAutoFPSLimit")));
	SetCfgVal(pB,"\nFPSDetection",val+1);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"checkFrameSkip")));
	SetCfgVal(pB,"\nUseFrameSkip",val);

	//Framerate stored *10
	val = atof(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(builder, "entryFPS")))) * 10;
	SetCfgVal(pB,"\nFrameRate",val);

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,"checkUseFixes")));
	SetCfgVal(pB,"\nUseFixes",val);


	val = 0;
	for (i=0; i<12; i++)
	{
		sprintf(tempstr, "checkFix%d", i+1);
		if( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(builder,tempstr))) )
			val |= 1 << i;
	}

	SetCfgVal(pB,"\nCfgFixes",val);



	if((in=fopen(cfg, WRITEBINARY))!=NULL)
	{
		fwrite(pB,strlen(pB),1,in);
		fclose(in);
	}

	free(pB);

	// Close the window and exit control from the plugin
	exit (0); 
}
