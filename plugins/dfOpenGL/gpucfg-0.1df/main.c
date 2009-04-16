#include <gtk/gtk.h>
#include <glade/glade.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define CONFIG_FILENAME "dfopengl.cfg"
#define READBINARY "rb"
#define WRITEBINARY "wb"

void on_config_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy (widget);
	exit (0);
}

void on_about_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy (widget);
	exit (0);
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

void save_config (GtkWidget *widget, gpointer user_data)
{
	GladeXML *xml;
	char cfg[255];
	FILE *in;
	int len, val;
	char *pB;
	
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
	
	xml = (GladeXML*) user_data;
	
	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chk_fullscreen")));
	SetCfgVal(pB,"\nFullscreen",val);
	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chk_bilinear")));
	SetCfgVal(pB,"\nBilinear",val);
	val = atoi (gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (xml, "te_maxtextures"))));
	SetCfgVal(pB,"\nMaxTextures",val);
	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chk_wireframe")));
	SetCfgVal(pB,"\nWireframe",val);
	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chk_antialias")));
	SetCfgVal(pB,"\nAntialias",val);
	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chk_clearscreen")));
	SetCfgVal(pB,"\nClearscreen",val);
	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "chk_framelimit")));
	SetCfgVal(pB,"\nFrameLimit",val);
	val = atoi (gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (xml, "te_windowX"))));
	SetCfgVal(pB,"\nwindowX",val);
	val = atoi (gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (xml, "te_windowY"))));
	SetCfgVal(pB,"\nwindowY",val);
	
	/* Write to the config file */
	if((in=fopen(cfg, WRITEBINARY))!=NULL) {
		fwrite(pB,strlen(pB),1,in);
		fclose(in);
	}
	free(pB);
	
	gtk_widget_destroy (glade_xml_get_widget (xml, "CfgWnd"));

	/* Close the window and exit control from the plugin */
	exit (0); 
}

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

int
main (int argc, char *argv[])
{
	GtkWidget *widget;
	GladeXML *xml;
	FILE *in;
	char t[256];
	int len;
	char * pB, * p;
	char cfg[255];
	char tempstr[128];

	if (argc!=2) {
		printf("Usage: cfgOpenGL {ABOUT | CFG}\n");
		return 0;
	}
	if(strcmp(argv[1],"CFG")!=0 && strcmp(argv[1],"ABOUT")!=0) {
		printf("Usage: cfgOpenGL {ABOUT | CFG}\n");
		return 0;
	}

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	if (strcmp(argv[1], "ABOUT") == 0) {
		widget = gtk_about_dialog_new ();
		gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (widget), "PCSX OpenGL Video Plugin");
		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (widget), "1.0");

		g_signal_connect_data(GTK_OBJECT(widget), "response",
							  GTK_SIGNAL_FUNC(on_about_clicked), NULL, NULL, G_CONNECT_AFTER);

		gtk_widget_show (widget);
		gtk_main();

		return 0;
	}

	xml = glade_xml_new(DATADIR "dfopengl.glade2", "CfgWnd", NULL);
	if (!xml) {
		g_warning("We could not load the interface!");
		return -1;
	}

	strcpy(cfg, CONFIG_FILENAME);
	
	in = fopen(cfg, READBINARY);
	if(in) {
		pB = (char *)malloc(32767);
		memset(pB, 0, 32767);
		len = fread(pB, 1, 32767, in);
		fclose(in);
	} else {
		pB = 0;
		printf("Couldn't find config file %s - creating new file\n", cfg);
	}

	/* Set default values */
	int bFullscreen = FALSE;
	int bBilinear = FALSE;
	int nMaxTextures = 64;
	int bWireFrame = FALSE;
	int bAntialias = FALSE;
	int bClearScreen = FALSE;
	int bFrameLimit = TRUE;
	short nwindowX = 1024;
	short nwindowY = 768;

	if (pB) {
		strcpy(t,"\nFullscreen");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		bFullscreen = set_limit (p, len, 0, 1);
		
		strcpy(t,"\nBilinear");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		bBilinear = set_limit (p, len, 0, 1);
		
		strcpy(t,"\nMaxTextures");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		nMaxTextures = set_limit (p, len, 0, 128);	/* TODO - What should the limit be? */
printf("Max Textures is %d\n", nMaxTextures);

		strcpy(t,"\nWireframe");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		bWireFrame = set_limit (p, len, 0, 1);

		strcpy(t,"\nAntialias");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		bAntialias = set_limit (p, len, 0, 1);

		strcpy(t,"\nClearscreen");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		bClearScreen = set_limit (p, len, 0, 1);

		strcpy(t,"\nFrameLimit");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		bFrameLimit = set_limit (p, len, 0, 1);

		strcpy(t,"\nwindowX");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		nwindowX = set_limit (p, len, 0, 1600);	/* TODO - limit ? */

		strcpy(t,"\nwindowY");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		nwindowY = set_limit (p, len, 0, 1200);	/* TODO - limit ? */

		free(pB);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chk_fullscreen")), bFullscreen);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chk_bilinear")), bBilinear);
	
	snprintf (tempstr, 10, "%d", nMaxTextures);
	gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget(xml, "te_maxtextures")), tempstr);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chk_wireframe")), bWireFrame);
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chk_antialias")), bAntialias);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chk_clearscreen")), bClearScreen);
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "chk_framelimit")), bFrameLimit);
	
	snprintf (tempstr, 10, "%d", nwindowX);
	gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget(xml, "te_windowX")), tempstr);
	
	snprintf (tempstr, 10, "%d", nwindowY);
	gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget(xml, "te_windowY")), tempstr);
	
	widget = glade_xml_get_widget(xml, "CfgWnd");
	g_signal_connect_data(GTK_OBJECT(widget), "delete_event",
						  GTK_SIGNAL_FUNC(on_config_clicked), NULL, NULL, G_CONNECT_AFTER);
	gtk_widget_show_all (widget);
	
	widget = glade_xml_get_widget(xml, "btn_close");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
						  GTK_SIGNAL_FUNC(save_config), xml, NULL, G_CONNECT_AFTER);
	
	gtk_main ();
	return 0;
}
