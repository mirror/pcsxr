#include <gtk/gtk.h>
#include <glade/glade.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void SaveConfig (GtkWidget *widget, gpointer user_datal);

#define READBINARY "rb"
#define WRITEBINARY "wb"
#define CONFIG_FILENAME "dfxvideo.cfg"

/* ADB */
enum {
	VIDMODE_320x200,
	VIDMODE_640x480,
	VIDMODE_800x600,
	VIDMODE_1024x768
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

void set_widget_sensitive (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_set_sensitive (widget, (int)user_data);
}

void on_use_fixes_toggled (GtkWidget *widget, gpointer user_data)
{
	GtkWidget *check, *table_fixes;
	GladeXML *xml;
	xml = (GladeXML*) user_data;
	check = glade_xml_get_widget (xml, "checkUseFixes");

	table_fixes = glade_xml_get_widget (xml, "table_fixes");

	/* Set the state of each of the fixes to the value of the use fixes toggle */
	gtk_container_foreach (GTK_CONTAINER (table_fixes), (GtkCallback) set_widget_sensitive,
		(void *)gtk_toggle_button_get_active (check));
}

int
main (int argc, char *argv[])
{
  GtkWidget *CfgWnd, *widget;
  GladeXML *xml;
  FILE *in;char t[256];int len,val; 
  float valf;
  char * pB, * p;
  char cfg[255];
  int i;
  char tempstr[50];

  if (argc!=2) {
    printf("Usage: cfgDFXVideo {ABOUT | CFG}\n");
    return 0;
  }
  if(strcmp(argv[1],"CFG")!=0 && strcmp(argv[1],"ABOUT")!=0) {
    printf("Usage: cfgDFXVideo {ABOUT | CFG}\n");
    return 0;
  }

  gtk_set_locale ();
  gtk_init (&argc, &argv);


       if (strcmp(argv[1], "ABOUT") == 0) {
		const char *authors[]= {"Pete Bernert and the P.E.Op.S. team", "Ryan Schultz", "Andrew Burton", NULL};
		widget = gtk_about_dialog_new ();
		gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (widget), "P.E.Op.S PCSX Video Plugin");
		gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (widget), "1.15");
		gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (widget), authors);
		gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (widget), "http://home.t-online.de/home/PeteBernert/");

	g_signal_connect_data(GTK_OBJECT(widget), "response",
			GTK_SIGNAL_FUNC(on_about_clicked), NULL, NULL, G_CONNECT_AFTER);

		gtk_widget_show (widget);
		gtk_main();

		return 0;
    }

    xml = glade_xml_new(DATADIR "dfxvideo.glade2", "CfgWnd", NULL);
    if (!xml) {
		g_warning("We could not load the interface!");
		return -1;
    }

    /*ADB wndMain = glade_xml_get_widget(xml, "CfgWnd");*/

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
/* ADB TODO Parse this like we parse the config file in PCSX - use common functions! */
  val=1;
  if(pB)
   {
    strcpy(t,"\nResX");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

       val = set_limit (p, len, 0, 1024);
   }

   /* ADB TODO Replace this with calls to the enum video_modes above */
 if (val == 1024) val = 3;
  if (val == 800) val = 2;
   if (val == 640) val = 1;
    if (val == 320) val = 0;


  gtk_combo_box_set_active(GTK_COMBO_BOX (glade_xml_get_widget(xml, "resCombo2")), val);

  val=0;
  if(pB)
   {
    strcpy(t,"\nNoStretch");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

       val = set_limit (p, len, 0, 9);
   }

  gtk_combo_box_set_active(GTK_COMBO_BOX (glade_xml_get_widget(xml, "stretchCombo2")), val);

  val=0;
  if(pB)
   {
    strcpy(t,"\nUseDither");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

           val = set_limit (p, len, 0, 2);
   }

   gtk_combo_box_set_active(GTK_COMBO_BOX (glade_xml_get_widget(xml, "ditherCombo2")), val);

 val=0;
  if(pB)
   {
    strcpy(t,"\nMaintain43");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

    val = set_limit (p, len, 0, 1);
   }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "maintain43")), val);
  
  val=0; //ADB Leave - these are default values
  if(pB)
   {
    strcpy(t,"\nFullScreen");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

        val = set_limit (p, len, 0, 1);
   }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "checkFullscreen")), val);

  val=0;
  if(pB)
   {
    strcpy(t,"\nShowFPS");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

        val = set_limit (p, len, 0, 1);
   }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "checkShowFPS")), val);

  val=1;
  if(pB)
   {
    strcpy(t,"\nUseFrameLimit");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

        val = set_limit (p, len, 0, 1);
   }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "checkAutoFPSLimit")), val);

  val=0;
  if(pB)
   {
    strcpy(t,"\nFPSDetection");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

        val = set_limit (p, len, 1, 2);
   }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "checkSetFPS")), !(val-1));

 val=0;
  if(pB)
   {
    strcpy(t,"\nUseFrameSkip");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
        val = set_limit (p, len, 0, 1);
   }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "checkFrameSkip")), val);

 valf=200;
  if(pB)
   {
    strcpy(t,"\nFrameRate");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
    if(p) valf=(float)atoi(p+len) / 10;
    if(valf<1) valf=1;
    if(valf>500) valf=500;
   }
  sprintf(tempstr,"%.1f",valf);
  gtk_entry_set_text(glade_xml_get_widget(xml, "entryFPS"),tempstr);

  val=0;
  if(pB)
   {
    strcpy(t,"\nUseFixes");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}

        val = set_limit (p, len, 0, 1);
   }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (glade_xml_get_widget(xml, "checkUseFixes")), val);

   
	if(pB)
	{
		strcpy(t,"\nCfgFixes");p=strstr(pB,t);if(p) {p=strstr(p,"=");len=1;}
		if (p)
			val = atoi(p + len);
	}

	for (i=0; i<10; i++)
	{
		sprintf(tempstr, "checkFix%d", i+1);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, tempstr)), (val>>i)&1 );
	}


  if(pB) free(pB);

	widget = glade_xml_get_widget(xml, "CfgWnd");
	g_signal_connect_data(GTK_OBJECT(widget), "delete_event",
			GTK_SIGNAL_FUNC(on_config_clicked), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "btn_close");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(SaveConfig), xml, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget(xml, "checkUseFixes");
	g_signal_connect_data(GTK_OBJECT(widget), "clicked",
			GTK_SIGNAL_FUNC(on_use_fixes_toggled), xml, NULL, G_CONNECT_AFTER);

	on_use_fixes_toggled (widget, (gpointer) xml);

  gtk_main ();
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
	GladeXML *xml;
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
   xml = (GladeXML*) user_data;

  val = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (xml, "resCombo2")));

// SetCfgVal(pB,"\nRes",val);

/* ADB TODO Replace with constants and enums */
 if (val == 0) { SetCfgVal(pB,"\nResX",320); SetCfgVal(pB,"\nResY",240); }
 if (val == 1) { SetCfgVal(pB,"\nResX",640); SetCfgVal(pB,"\nResY",480); }
 if (val == 2) { SetCfgVal(pB,"\nResX",800); SetCfgVal(pB,"\nResY",600); }
 if (val == 3) { SetCfgVal(pB,"\nResX",1024); SetCfgVal(pB,"\nResY",768); }

 val = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (xml, "stretchCombo2")));
 SetCfgVal(pB,"\nNoStretch",val);

 val = gtk_combo_box_get_active (GTK_COMBO_BOX (glade_xml_get_widget (xml, "ditherCombo2")));
 SetCfgVal(pB,"\nUseDither",val);

    val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "maintain43")));
 SetCfgVal(pB,"\nMaintain43",val);

     val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "checkFullscreen")));
 SetCfgVal(pB,"\nFullScreen",val);

     val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "checkShowFPS")));
 SetCfgVal(pB,"\nShowFPS",val);

     val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "checkAutoFPSLimit")));
 SetCfgVal(pB,"\nUseFrameLimit",val);

 val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "checkSetFPS")));
 SetCfgVal(pB,"\nFPSDetection",(!val)+1);

  val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "checkFrameSkip")));
 SetCfgVal(pB,"\nUseFrameSkip",val);

 //Framerate stored *10
 val = atof(gtk_entry_get_text(glade_xml_get_widget(xml, "entryFPS"))) * 10;
 SetCfgVal(pB,"\nFrameRate",val);

 val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, "checkUseFixes")));
 SetCfgVal(pB,"\nUseFixes",val);


	val = 0;
	for (i=0; i<10; i++)
	{
		sprintf(tempstr, "checkFix%d", i+1);
		if( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (xml, tempstr))) )
			val |= 1 << i;
	}

	SetCfgVal(pB,"\nCfgFixes",val);



 if((in=fopen(cfg, WRITEBINARY))!=NULL)
  {
   fwrite(pB,strlen(pB),1,in);
   fclose(in);
  }

 free(pB);
 
    gtk_widget_destroy (glade_xml_get_widget (xml, "CfgWnd"));
//    g_free (xml);

	/* Close the window and exit control from the plugin */
	exit (0); 
}
