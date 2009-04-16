#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include <gtk/gtk.h>

#include "cdr.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"

/* list of drives */
GList * driveslist = NULL;

/* private functions prototypes */
void fill_drives_list(void);
int is_cdrom(char *device);


/***************************************************************************
 * Config Dialog.
 ***************************************************************************/

void on_cfg_dialog_show (GtkWidget *widget, gpointer user_data)
{
	GtkWidget *devcombo;
	GtkWidget *rmmenu;
	GtkWidget *subQbtn;
	GtkWidget *spinC;
	GtkWidget *spinS;

	LoadConf();
	fill_drives_list();

	devcombo = lookup_widget (GTK_WIDGET (widget), "cddev_combo");
	rmmenu   = lookup_widget (GTK_WIDGET (widget), "readmode_optionmenu");
	subQbtn  = lookup_widget (GTK_WIDGET (widget), "subQ_button");
	spinC    = lookup_widget (GTK_WIDGET (widget), "spinCacheSize");
	spinS    = lookup_widget (GTK_WIDGET (widget), "spinCdrSpeed");

	/* show values */
	gtk_combo_set_popdown_strings (GTK_COMBO (devcombo), driveslist); 
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (devcombo)->entry), CdromDev);
	gtk_option_menu_set_history (GTK_OPTION_MENU (rmmenu), ReadMode);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (subQbtn), UseSubQ);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinC), (float)CacheSize);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinS), (float)CdrSpeed);

	/* ??? is correct to free? it's a global... */
	g_list_free(driveslist);
}


void on_cfg_cancelbutton_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	gtk_main_quit();
}


void on_cfg_okbutton_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *deventry;
	GtkWidget *rmmenu;
	GtkWidget *subQbtn;
	GtkWidget *spinC;
	GtkWidget *spinS;
	char *tmp;

	deventry = lookup_widget (GTK_WIDGET (button), "cddev_entry");
	rmmenu   = lookup_widget (GTK_WIDGET (button), "readmode_optionmenu");
	subQbtn  = lookup_widget (GTK_WIDGET (button), "subQ_button");
	spinC    = lookup_widget (GTK_WIDGET (button), "spinCacheSize");
	spinS    = lookup_widget (GTK_WIDGET (button), "spinCdrSpeed");

	tmp = gtk_entry_get_text (GTK_ENTRY (deventry));
	strcpy(CdromDev, tmp);
	ReadMode = gtk_option_menu_get_history (GTK_OPTION_MENU(rmmenu));
	UseSubQ  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(subQbtn));
	CacheSize= gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinC));
	CdrSpeed = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinS));

	SaveConf();

	gtk_widget_destroy(gtk_widget_get_toplevel (GTK_WIDGET (button)));
	gtk_main_quit();
}


/***************************************************************************
 * About Dialog.
 ***************************************************************************/

void on_abt_okbutton_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
	gtk_main_quit();
}


/***************************************************************************
 * Private functions.
 ***************************************************************************/

/*
 * get_drives_list: retrieves available cd drives. At the moment it use a quite
 * ugly "brute force" method: we check for the most common location for cdrom
 * in /dev and chech if they are cdrom devices.
 * If your cdrom path is not listed here you'll have to type it in the dialog
 * entry yourself (or add it here and recompile).
 * Are there any other common entry to add to the list? (especially scsi, I
 * deliberately ignored old non standard cdroms... )
 * If you come up with a better method let me know!!
 */

void fill_drives_list(void)
{
	int i = 0;
	static char *cdrom_devices[]={
		"/dev/cdrom",
		"/dev/cdroms/cdrom0",
		"/dev/cdroms/cdrom1",
		"/dev/cdroms/cdrom2",
		"/dev/cdroms/cdrom3",				
		"/dev/hda",
		"/dev/hdb",
		"/dev/hdc",
		"/dev/hdd",
		"/dev/scd0",
		"/dev/scd1",
		"/dev/scd2",
		"/dev/scd3",		
		"/dev/optcd",
		NULL};

	/* fisrt we put our current drive */
	driveslist = g_list_append(driveslist, CdromDev);

	/* scan cdrom_devices for real cdrom and add them to driveslist */ 
	while(cdrom_devices[i] != NULL){
	
		/* check that is not our current dev (already in list) */
		if (strcmp(cdrom_devices[i], CdromDev) != 0){
		
			/* check that is a cdrom device */
			if (is_cdrom(cdrom_devices[i])){
				driveslist = g_list_append(driveslist, cdrom_devices[i]);
			}
		}
		++i;
	}

	return;
}


/* function to check if the device is a cdrom */
int is_cdrom(char *device){
	struct stat st;
	int fd = -1;

	/* check if the file exist */
	if (stat(device, &st) <0) return 0;

	/* check if is a block or char device */
	if (!S_ISCHR(st.st_mode) && !S_ISBLK(st.st_mode)) return 0;

	/* try to open the device file descriptor */
	if ((fd = open(device, O_RDONLY | O_NONBLOCK)) < 0) return 0;

	/* I need a method to check is a device is really a cdrom.
	   some problems/ideas are:
	   - different protocls (ide, scsi, old proprietary...)
 	   - maybe we can use major number (see linux/major.h) to do some check.
	     major number can be retrieved with (st.st_rdev>>8)
	     scsi has SCSI_CDROM_MAJOR but does this cover all scsi drives?
	     beside IDE major is the same for hard disks and cdroms...
	     and DVDs?
	   - another idea is to parse /proc, but again IDE, scsi etc have 
	     different files... I've not found a way to query "which cd drives
	     are available?"

	   Now I use this ioctl which works also if the drive is empty,
	   I hope that is implemented for all the drives... here works
	   fine: at least doesn't let me to select my HD as cds ;)
	*/ 
	/* try a ioctl to see if it's a cdrom device */
	if (ioctl(fd, CDROM_GET_CAPABILITY, NULL) < 0){
		close(fd);
		return 0;
	}

	close(fd);

	/* yes, it seems a cd drive! */
	return 1;
}

#endif
