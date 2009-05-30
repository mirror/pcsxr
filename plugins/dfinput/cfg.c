/*
 * Pad for Psemu Pro like Emulators
 * This is the config program, taken out from the pad
 * It's also responsible for the about-dialog box
 *
 * Written by Erich Kitzmuller <ammoq@ammoq.com>
 * Based on padXwin by linuzappz <linuzappz@hotmail.com>
 *
 * Copyright 2002,2003 by Erich Kitzmuller
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/joystick.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <errno.h>
#include "padjoy.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#ifdef __linux__
char *LibName = "Gamepad/Keyboard Input";
#else
char *LibName = "Keyboard Input";
#endif

#define CONFIG_FILENAME	"dfinput.cfg"

// Prototypes
static void loadConfig();
static void saveConfig();
static char *eventDescription(EventCode);
static void initPadtime();

// Filenames for device files, e.g. "/dev/input/js0"
static char devicefilename[MAXDEVICES][FILENAME_MAX+1] = {"/dev/input/js0", "/dev/input/js1"};

// File desciptors for device files
static int devicefile[MAXDEVICES] = { -1, -1 };

// Use Threading for joy device input?
static int use_threads = 1;

// Emulate Dualshock(TM) analog pad?
static int use_analog = 0;

// calibration data
int minzero[MAXAXES];
int maxzero[MAXAXES];

// axes status - so only changing status are reported
int axestatus[MAXDEVICES][MAXAXES];

// Assignment of PSX buttons to Events
static EventCode PadButtons[MAXDEVICES][MAXPSXBUTTONS] =
{
	{
		KEY_EVENT(XK_e),	// L2
		KEY_EVENT(XK_t),	// R2
		KEY_EVENT(XK_w),	// L1
		KEY_EVENT(XK_r),	// R1
		KEY_EVENT(XK_d),	// Triangle
		KEY_EVENT(XK_x),	// Circle
		KEY_EVENT(XK_z),	// Cross
		KEY_EVENT(XK_s),	// Square
		KEY_EVENT(XK_c),	// Select
		NO_EVENT,		// Left Analog
		NO_EVENT,		// Right Analog
		KEY_EVENT(XK_v),	// Start
		KEY_EVENT(XK_Up),	// Up
		KEY_EVENT(XK_Right),	// Right
		KEY_EVENT(XK_Down),	// Down
		KEY_EVENT(XK_Left),	// Left
		NO_EVENT,		// Left Anlaog X
		NO_EVENT,		// Left Analog Y
		NO_EVENT,		// Right Analog X
		NO_EVENT		// Right Analog Y
	},
	{
		NO_EVENT,		// L2
		NO_EVENT,		// R2
		NO_EVENT,		// L1
		NO_EVENT,		// R1
		NO_EVENT,		// Triangle
		NO_EVENT,		// Circle
		NO_EVENT,		// Cross
		NO_EVENT,		// Square
		NO_EVENT,		// Select
		NO_EVENT,		// Left Analog
		NO_EVENT,		// Right Analog
		NO_EVENT,		// Start
		NO_EVENT,		// Up
		NO_EVENT,		// Right
		NO_EVENT,		// Down
		NO_EVENT,		// Left
		NO_EVENT,		// Left Anlaog X
		NO_EVENT,		// Left Analog Y
		NO_EVENT,		// Right Analog X
		NO_EVENT		// Right Analog Y
	}
};

static Display *Dsp;

static EventCode macroLaunch[MAXDEVICES][MAXMACROS];
static EventCode macroEvents[MAXDEVICES][MAXMACROS][MAXMACROLENGTH];
static long macroInterval[MAXDEVICES][MAXMACROS][MAXMACROLENGTH];
static int macroActive[MAXDEVICES];
static int macroIndex[MAXDEVICES];
static long macroNext[MAXDEVICES];

void init_macros() {
    int i,j;

    for (i=0; i<MAXDEVICES; i++) {
        for (j=0; j<MAXMACROS; j++) {
            macroLaunch[i][j]=NO_EVENT;
            macroEvents[i][j][0]=NO_EVENT;
            macroInterval[i][j][0]=0;
        }
        macroActive[i]=-1;
        macroIndex[i]=0;
        macroNext[i]=0;
    }
}

long PADinit(long flags) {
    int i,j;

    init_macros();
    initPadtime();
    for (i=0; i<MAXDEVICES; i++) {
       maxzero[i] = 250;
       minzero[i] = -250;

       for (j=0; j<MAXAXES; j++) {
         axestatus[i][j] = AXESTS_UNKNOWN;
       }
    }
    loadConfig();

    return 0;
}

static long firstsecond = 0;

static void initPadtime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    firstsecond = tv.tv_sec;
}

// construct a time on our own
long getPadtime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec-firstsecond)*10000+tv.tv_usec/100;
}

unsigned short PadStat[2] = {0xffff, 0xffff};

// get pending events
static int getPendingEvents(int millisecondstowait, EventCode *events, int maxevents, int useGDK, int checkJoydevice, int checkXKeyboard, long *timing) {
    fd_set rfds;
    int retval;
    int i;
    int md;
    int eventsread=0;
    XEvent xe;
    GdkEvent *ge;
    int cntopen;
    struct timeval tv;
    int oldstatus;

#ifdef __linux__
    struct js_event je;

    if (checkJoydevice) {
        FD_ZERO(&rfds);
        md = -1;
        cntopen=0;
        for (i=0; i<MAXDEVICES; i++) {
            if (devicefile[i] > -1) {
                FD_SET(devicefile[i], &rfds);
                cntopen++;
            }
            if (devicefile[i] > md) md = devicefile[i];
        }
        tv.tv_sec = millisecondstowait / 1000;
        tv.tv_usec = 1000 * (millisecondstowait % 1000);

        retval = select(md+1, &rfds, NULL, NULL, &tv);

        while (retval && eventsread<maxevents-2*checkXKeyboard) {
            for (i=0; i<MAXDEVICES; i++) {
                if (devicefile[i]>-1 && FD_ISSET(devicefile[i], &rfds)) {
                    read (devicefile[i], &je, 8);

                    if (je.type == JS_EVENT_AXIS && je.number<MAXAXES) {
                        if (axestatus[i][je.number] == AXESTS_ANALOG) {
                            /* this axe should be reported analog */
                            events[eventsread++] = ANALOGAXIS_EVENT(i,je.number, (je.value+32768)>>8);
                            if (timing) {
                                (*timing)=getPadtime();
                                timing++;
                            }
                            if (eventsread == maxevents) return eventsread;
                        }
                        else if (je.value > maxzero[i]) {
                            if (axestatus[i][je.number] != AXESTS_PLUS &&
                                axestatus[i][je.number] != AXESTS_UNUSED) {

                                oldstatus = axestatus[i][je.number];

                                axestatus[i][je.number] = AXESTS_PLUS;

                                events[eventsread++] = AXISPLUS_EVENT(i,je.number);
                                if (timing) {
                                    (*timing)=getPadtime();
                                    timing++;
                                }
                                if (eventsread==maxevents) return eventsread;

                                if (oldstatus == AXESTS_MINUS) {
                                    events[eventsread++] = RELEASE_EVENT+AXISMINUS_EVENT(i,je.number);
                                    if (timing) {
                                        (*timing)=getPadtime();
                                        timing++;
                                    }
                                    if (eventsread==maxevents) return eventsread;
                                }

                            }
                        }
                        else if (je.value < minzero[i]) {
                            if (axestatus[i][je.number] != AXESTS_MINUS &&
                                axestatus[i][je.number] != AXESTS_UNUSED) {

                                oldstatus = axestatus[i][je.number];

                                axestatus[i][je.number] = AXESTS_MINUS;

                                events[eventsread++] = AXISMINUS_EVENT(i,je.number);
                                if (timing) {
                                    (*timing)=getPadtime();
                                    timing++;
                                }
                                if (eventsread==maxevents) return eventsread;

                                if (oldstatus == AXESTS_PLUS) {
                                    events[eventsread++] = RELEASE_EVENT+AXISPLUS_EVENT(i,je.number);
                                    if (timing) {
                                        (*timing)=getPadtime();
                                        timing++;
                                    }
                                    if (eventsread==maxevents) return eventsread;
                                }
                            }
                        }
                        else {
                            if (axestatus[i][je.number] != AXESTS_CENTER &&
                                axestatus[i][je.number] != AXESTS_UNUSED) {

                                oldstatus = axestatus[i][je.number];

                                axestatus[i][je.number] = AXESTS_CENTER;

                                if (oldstatus == AXESTS_PLUS) {
                                    events[eventsread++] = RELEASE_EVENT+AXISPLUS_EVENT(i,je.number);
                                    if (timing) {
                                        (*timing)=getPadtime();
                                        timing++;
                                    }
                                    if (eventsread==maxevents) return eventsread;
                                }
                                else if (oldstatus == AXESTS_MINUS) {
                                    events[eventsread++] = RELEASE_EVENT+AXISMINUS_EVENT(i,je.number);
                                    if (timing) {
                                        (*timing)=getPadtime();
                                        timing++;
                                    }
                                    if (eventsread==maxevents) return eventsread;
                                }
                            }
                        }
                    }
                    else if (je.type == JS_EVENT_BUTTON && je.number<MAXBUTTONS) {
                        events[eventsread++] = (je.value?0:RELEASE_EVENT) + BUTTON_EVENT(i,je.number);
                        if (timing) {
                            (*timing)=getPadtime();
                            timing++;
                        }
                        if (eventsread==maxevents) return eventsread;
                    }
                }
            }
            tv.tv_sec = 0;
            tv.tv_usec = 0;

            retval = select(md+1, &rfds, NULL, NULL, &tv);
        }

    }
#endif

    if (checkXKeyboard) {
        if (useGDK) {
            while ((ge = gdk_event_get() ) != NULL ) {
                if  (ge->type == GDK_KEY_PRESS) {
                    events[eventsread++] = KEY_EVENT(ge->key.keyval);
                    if (timing) {
                        (*timing)=((GdkEventKey *)ge)->time*10;
                        timing++;
                    }
                    if (eventsread==maxevents) return eventsread;
                }
                else if (ge->type == GDK_KEY_RELEASE) {
                    events[eventsread++] = RELEASE_EVENT+KEY_EVENT(ge->key.keyval);
                    if (timing) {
                        (*timing)=((GdkEventKey *)ge)->time*10;
                        timing++;
                    }
                    if (eventsread==maxevents) return eventsread;
                }
                gdk_event_free(ge);
            }
        }
        else {
            while ((i=XPending(Dsp))) {
                while (i--) {
                    XNextEvent(Dsp, &xe);
                    switch (xe.type) {
                        case KeyPress:
                            events[eventsread++] = KEY_EVENT(XLookupKeysym((XKeyEvent *)&xe, 0));
                            if (timing) {
                                (*timing)=getPadtime();
                                timing++;
                            }
                            if (eventsread==maxevents) return eventsread;
                            break;
                        case KeyRelease:
                            events[eventsread++] = RELEASE_EVENT+KEY_EVENT(XLookupKeysym((XKeyEvent *)&xe, 0));
                            if (timing) {
                                (*timing)=getPadtime();
                                timing++;
                            }
                            if (eventsread==maxevents) return eventsread;
                            break;
/*
                        case FocusIn:
                            XAutoRepeatOff(Dsp);
                            break;
                        case FocusOut:
                            XAutoRepeatOn(Dsp);
                            break;
*/
                    }
                }
            }
        }
    }

    return eventsread;
}


/*---------------------------------------------------------------------*/
/*                          Config Dialogue stuff                      */
/*---------------------------------------------------------------------*/

// analyse Eventcode
static PadJoyEvent *EventCode2PadJoyEvent(EventCode p_e) {
    static PadJoyEvent event;
    EventCode e;
    int i,p;

    event.event_type = EVENTTYPE_NONE;
    event.pad = 0;
    event.no = 0;
    event.value = 0;

    if (!p_e) {
      return &event;
    }

    e = p_e;

    if (e>RELEASE_EVENT) {
      event.value = 0;
      e -= RELEASE_EVENT;
    }
    else {
      event.value = 1;
    }

    if (e && e<FIRST_JOY_EVENT) {
         event.event_type = EVENTTYPE_KEY;
         event.no = e;
         return &event;
    }

    if (e >= FIRST_ANALOG_EVENT) {
      event.event_type = EVENTTYPE_ANALOG;
      event.pad = (e-FIRST_ANALOG_EVENT)/(256*MAXAXES);
      event.no = (e-ANALOGAXIS_EVENT(event.pad,0,0))/256;
      event.value = e & 0xff;
      return &event;
    }


    for (p=0; p<MAXDEVICES; p++) {
        for (i=0; i<MAXAXES; i++) {
            if (e == AXISPLUS_EVENT(p,i)) {
                event.event_type = EVENTTYPE_AXISPLUS;
                event.pad = p;
                event.no = i;
                return &event;
            }
            if (e == AXISMINUS_EVENT(p,i)) {
                event.event_type = EVENTTYPE_AXISMINUS;
                event.pad = p;
                event.no = i;
                return &event;
            }
        }

        for (i=0; i<MAXBUTTONS; i++) {
            if (e == BUTTON_EVENT(p,i)) {
				event.event_type = EVENTTYPE_BUTTON;
                event.pad = p;
                event.no = i;
                return &event;
            }
        }
    }

    return &event;
}



// get the description of an Eventcode
// this implementation is not optimized,
// but I only need it for the configuration part so speed doesn't matter
// should be changed to use EventCode2PadJoyEvent
static char *eventDescription(EventCode e) {
    static char buffer[256];
    int i,p;

    if (e && e<FIRST_JOY_EVENT) {
         sprintf(buffer, "\"%s\"", XKeysymToString(e-FIRST_KEY_EVENT));
         return buffer;
    }

    if (e >= FIRST_ANALOG_EVENT) {
      p = (e-FIRST_ANALOG_EVENT)/(256*MAXAXES);
      i = (e-ANALOGAXIS_EVENT(p,0,0))/256;
      sprintf(buffer, "X%d", i);
      return buffer;
    }

    for (p=0; p<MAXDEVICES; p++) {
        for (i=0; i<MAXAXES; i++) {
            if (e == AXISPLUS_EVENT(p,i)) {
                sprintf(buffer, "A%d+", i);
                return buffer;
            }
            if (e == AXISMINUS_EVENT(p,i)) {
                sprintf(buffer, "A%d-", i);
                return buffer;
            }
        }

        for (i=0; i<MAXBUTTONS; i++) {
            if (e == BUTTON_EVENT(p,i)) {
                sprintf(buffer, "B%d", i);
                return buffer;
            }
        }
    }

    sprintf(buffer, "???");
    return buffer;
}


// get a String for each EventCode
// more accurate than eventDescription()
// should be changed to use EventCode2PadJoyEvent
static char *EventCode2String(EventCode p_e) {
    static char buffer[256];
    int i,p,v;
    char push_release;
    EventCode e;

    if (!p_e) {
      sprintf(buffer, "???");
      return buffer;
    }

    e = p_e;

    if (e>RELEASE_EVENT) {
      push_release = 'R';
      e -= RELEASE_EVENT;
    }
    else {
      push_release = 'P';
    }

    if (e && e<FIRST_JOY_EVENT) {
         sprintf(buffer, "K%c\"%s\"", push_release, XKeysymToString(e-FIRST_KEY_EVENT));
         return buffer;
    }

    if (e >= FIRST_ANALOG_EVENT) {
      p = (e-FIRST_ANALOG_EVENT)/(256*MAXAXES);
      i = (e-ANALOGAXIS_EVENT(p,0,0))/256;
      v = e & 0xff;;
      sprintf(buffer, "X%dP%dv%d",p, i, v);
      return buffer;
    }


    for (p=0; p<MAXDEVICES; p++) {
        for (i=0; i<MAXAXES; i++) {
            if (e == AXISPLUS_EVENT(p,i)) {
                sprintf(buffer, "A%d%c%d+", p, push_release, i);
                return buffer;
            }
            if (e == AXISMINUS_EVENT(p,i)) {
                sprintf(buffer, "A%d%c%d-", p, push_release, i);
                return buffer;
            }
        }

        for (i=0; i<MAXBUTTONS; i++) {
            if (e == BUTTON_EVENT(p,i)) {
                sprintf(buffer, "B%d%c%d", p, push_release, i);
                return buffer;
            }
        }
    }

    sprintf(buffer, "???");
    return buffer;
}

// reversal of EventCode2String
static EventCode String2EventCode(char *s) {
    static char buffer[256];
    int i,p;
    char *q;
    char push_release;
    EventCode e;

    if (s[0]>='0' && s[0]<='9') return atoi(s);  // allow numeric input

    e=0;
    push_release = 'P';

    switch(s[0]) {
      case 'K':
        push_release = s[1];
        strncpy(buffer, s+3, 255);
        q=buffer;
        i=1;
        while (*q) {
          if (*q=='"') i=!i;
          if (*q==' ' && !i)
            *q='\0';
          else
            q++;
        }
        if (s[2]=='"' && buffer[0] && buffer[strlen(buffer)-1]=='"') {
          buffer[strlen(buffer)-1] = '\0';
          e = KEY_EVENT(XStringToKeysym(buffer));
        }
        break;
      case 'A':
        if (s[1]>='0' && s[1]<='1' && strlen(s)>=5) {
          p = s[1]-'0';
          push_release = s[2];
          i = atoi(s+3);
          q=s+3;
          while (*q && *q!='+' && *q!='-') q++;
          if (*q=='+')
            e = AXISPLUS_EVENT(p,i);
          else if (*q=='-')
            e = AXISMINUS_EVENT(p,i);
        }
        break;
      case 'B':
        if (s[1]>='0' && s[1]<='1' && strlen(s)>=4) {
          p = s[1]-'0';
          push_release = s[2];
          i = atoi(s+3);
          e = BUTTON_EVENT(p,i);
        }
        break;
      case 'X':
        if (s[1]>='0' && s[1]<='1' && strlen(s)>=5) {
          p = s[1]-'0';
          i = atoi(s+3);
          q=s+3;
          while (*q && *q!='v') q++;
          if (*q=='v')
            e = ANALOGAXIS_EVENT(p,i,atoi(q+1));
        }
        break;
    }

    if (push_release=='R')
      return e+RELEASE_EVENT;
    else
      return e;
}

static void saveConfig() {
    FILE *f;
    int i,j,k;

    f = fopen(CONFIG_FILENAME, "w");
    if (!f) {
        fprintf(stderr, "DFInput error: couldn't write config file!\n");
        return;
    }

    fprintf(f,"[general]\n");
    fprintf(f,"use_threads     = %d\n", use_threads);
    fprintf(f,"use_analog      = %d\n", use_analog);
    for (i=0;i<MAXDEVICES;i++) {
        fprintf(f,"[pad %d]\n", i+1);
        fprintf(f,"devicefilename  = %s\n", devicefilename[i]);
        fprintf(f,"minzero = %d\n", minzero[i]);
        fprintf(f,"maxzero = %d\n", maxzero[i]);
        fprintf(f,"event_l2       = %s\n", EventCode2String(PadButtons[i][0]));
        fprintf(f,"event_r2       = %s\n", EventCode2String(PadButtons[i][1]));
        fprintf(f,"event_l1       = %s\n", EventCode2String(PadButtons[i][2]));
        fprintf(f,"event_r1       = %s\n", EventCode2String(PadButtons[i][3]));
        fprintf(f,"event_triangle = %s\n", EventCode2String(PadButtons[i][4]));
        fprintf(f,"event_circle   = %s\n", EventCode2String(PadButtons[i][5]));
        fprintf(f,"event_cross    = %s\n", EventCode2String(PadButtons[i][6]));
        fprintf(f,"event_square   = %s\n", EventCode2String(PadButtons[i][7]));
        fprintf(f,"event_select   = %s\n", EventCode2String(PadButtons[i][8]));
        fprintf(f,"event_lanalog  = %s\n", EventCode2String(PadButtons[i][9]));
        fprintf(f,"event_ranalog  = %s\n", EventCode2String(PadButtons[i][10]));
        fprintf(f,"event_start    = %s\n", EventCode2String(PadButtons[i][11]));
        fprintf(f,"event_up       = %s\n", EventCode2String(PadButtons[i][12]));
        fprintf(f,"event_right    = %s\n", EventCode2String(PadButtons[i][13]));
        fprintf(f,"event_down     = %s\n", EventCode2String(PadButtons[i][14]));
        fprintf(f,"event_left     = %s\n", EventCode2String(PadButtons[i][15]));
        fprintf(f,"event_lanax    = %s\n", EventCode2String(PadButtons[i][16]));
        fprintf(f,"event_lanay    = %s\n", EventCode2String(PadButtons[i][17]));
        fprintf(f,"event_ranax    = %s\n", EventCode2String(PadButtons[i][18]));
        fprintf(f,"event_ranay    = %s\n", EventCode2String(PadButtons[i][19]));
        for (j=0; j<MAXMACROS; j++) {
            fprintf(f, "[macro %d]\n", j+1);
            fprintf(f, "event_launch  = %s\n", EventCode2String(macroLaunch[i][j]));
            fprintf(f, "events        =");
            for (k=0; k<MAXMACROLENGTH && macroEvents[i][j][k]; k++) {
                fprintf(f, " %s", EventCode2String(macroEvents[i][j][k]));
            }
            fprintf(f,"\n");
            fprintf(f, "interval      =");
            for (k=0; k<MAXMACROLENGTH && macroEvents[i][j][k]; k++) {
                fprintf(f, " %ld", macroInterval[i][j][k]);
            }
            fprintf(f,"\n");
        }
    }

    fclose(f);
}

static void loadConfig() {
    FILE *f;
    int i;
    char line[FILENAME_MAX+30];
    int pad=0;
    int macronr=0;
    char *val;

    f = fopen(CONFIG_FILENAME, "r");
    if (!f) {
//        fprintf(stderr, "DFInput warning: config file not found.");
        return;
    }

    while(!feof(f)) {
        fgets(line, FILENAME_MAX+29, f);
        i=strlen(line)-1;
        while (i>0 && line[i]<32) line[i--]='\0';

        val=NULL;
        while(i>0) {
           if (line[i]=='=') val = line+(i+1);
           i--;
        }
        if (val) {
            while (*val==' ') val++;
        }

        if (!strcmp(line, "[general]")) {
            // nothing to do
        }
        else if (!strncmp(line, "use_threads", 11)) {
           use_threads = atoi(val);
        }
        else if (!strncmp(line, "use_analog", 10)) {
           use_analog = atoi(val);
        }
        else if (!strcmp(line, "[pad 1]")) {
            pad = 0;
        }
        else if (!strcmp(line, "[pad 2]")) {
            pad = 1;
        }
        else if (!strncmp(line, "[macro ", 7)) {
            macronr = atoi(line+7)-1;
            if (macronr<0 || macronr>=MAXMACROS) macronr=0;
        }
        else if (!strncmp(line, "devicefilename", 14)) {
           strcpy(devicefilename[pad], val);
        }
        else if (!strncmp(line, "minzero", 7)) {
           minzero[pad] = atoi(val);
        }
        else if (!strncmp(line, "maxzero", 7)) {
           maxzero[pad] = atoi(val);
        }
        else if (!strncmp(line, "event_l2", 8)) PadButtons[pad][0] = String2EventCode(val);
        else if (!strncmp(line, "event_r2", 8)) PadButtons[pad][1] = String2EventCode(val);
        else if (!strncmp(line, "event_l1", 8)) PadButtons[pad][2] = String2EventCode(val);
        else if (!strncmp(line, "event_r1", 8)) PadButtons[pad][3] = String2EventCode(val);
        else if (!strncmp(line, "event_triangle", 14)) PadButtons[pad][4] = String2EventCode(val);
        else if (!strncmp(line, "event_circle", 12)) PadButtons[pad][5] = String2EventCode(val);
        else if (!strncmp(line, "event_cross", 11)) PadButtons[pad][6] = String2EventCode(val);
        else if (!strncmp(line, "event_square", 12)) PadButtons[pad][7] = String2EventCode(val);
        else if (!strncmp(line, "event_select", 12)) PadButtons[pad][8] = String2EventCode(val);
        else if (!strncmp(line, "event_lanalog", 13)) PadButtons[pad][9] = String2EventCode(val);
        else if (!strncmp(line, "event_ranalog", 13)) PadButtons[pad][10] = String2EventCode(val);
        else if (!strncmp(line, "event_start", 11)) PadButtons[pad][11] = String2EventCode(val);
        else if (!strncmp(line, "event_up", 8)) PadButtons[pad][12] = String2EventCode(val);
        else if (!strncmp(line, "event_right", 11)) PadButtons[pad][13] = String2EventCode(val);
        else if (!strncmp(line, "event_down", 10)) PadButtons[pad][14] = String2EventCode(val);
        else if (!strncmp(line, "event_left", 10)) PadButtons[pad][15] = String2EventCode(val);
        else if (!strncmp(line, "event_lanax", 11)) PadButtons[pad][16] = String2EventCode(val);
        else if (!strncmp(line, "event_lanay", 11)) PadButtons[pad][17] = String2EventCode(val);
        else if (!strncmp(line, "event_ranax", 11)) PadButtons[pad][18] = String2EventCode(val);
        else if (!strncmp(line, "event_ranay", 11)) PadButtons[pad][19] = String2EventCode(val);
        else if (!strncmp(line, "event_launch", 12)) macroLaunch[pad][macronr] = String2EventCode(val);
        else if (!strncmp(line, "events", 6)) {
            i=0;
            while (*val) {
                macroEvents[pad][macronr][i++]=String2EventCode(val);
                while (*val && *val!=' ') val++;
                if (*val==' ') val++;
            }
            macroEvents[pad][macronr][i]=NO_EVENT;
        }
        else if (!strncmp(line, "interval", 8)) {
            i=0;
            while (*val) {
                macroInterval[pad][macronr][i++]=atol(val);
                while (*val && *val!=' ') val++;
                if (*val==' ') val++;
            }
        }
//        else fprintf(stderr, "DFInput error:  can't interpret %s\n", line);
   }
}

static int currentPad=0;

static struct {
  GtkWidget *config_window;

/* ADB
  GSList *padnogroup;
  GtkWidget *padno_radio[2];*/
  GtkWidget *filename_entry;	// TODO
  GtkWidget *button[CONFIGBUTTONCOUNT];
  GtkWidget *label[CONFIGBUTTONCOUNT];
/* ADB
  GSList *pcsxgroup;
  GtkWidget *epsxe_radio;
  GtkWidget *pcsx_radio;
  GtkWidget *thread_check;
  GtkWidget *analog_check;*/
  GtkWidget *macro_button[MAXMACROS];
  GtkWidget *macro_label[MAXMACROS];
  GtkWidget *macro_def_button[MAXMACROS];

  GtkWidget *ok_button;
  GtkWidget *cancel_button;
} ConfWidgets;

static struct { int nr;
         char *label;
         int x;
         int y; }
  buttonInfo[CONFIGBUTTONCOUNT] =
{
{0, "  L2  ", 20, 30},
{2, "  L1  ", 20, 70},
{1, "  R2  ", 350, 30},
{3, "  R1  ", 350, 70},
{12, "  ^  ", 70, 110},
{15, "  <  ", 20, 140},
{13, "  >  ", 120, 140},
{14, "  v  ", 70, 170},
{4, "  /\\  ", 300, 110},
{7, "  [_]  ", 250, 140},
{5, "  (_)  ", 350, 140},
{6, "  ><  ", 300, 170},
{8, "  Select  ", 100, 200},
{11, "  Start  ", 250, 200},
{9, "  +  ",  90, 280},
{10,"  +  ", 320, 280},
{16,"  --  ", 20, 280},
{17,"  |  ", 70, 250},
{18,"  --  ",250, 280},
{19,"  |  ",300, 250}
};

static void showPadConfiguration() {
    int i;

    // gtk_toggle_button_set_active(ConfWidgets.padno_radio[currentPad], TRUE);
    // gtk_toggle_button_set_active(ConfWidgets.padno_radio[1-currentPad], FALSE);

    gtk_entry_set_text ( GTK_ENTRY(ConfWidgets.filename_entry), devicefilename[currentPad] );

    for (i=0; i<CONFIGBUTTONCOUNT; i++) {
        gtk_label_set(GTK_LABEL(ConfWidgets.label[i]), eventDescription(PadButtons[currentPad][buttonInfo[i].nr]));
    }

    for (i=0; i<MAXMACROS; i++) {
        gtk_label_set(GTK_LABEL(ConfWidgets.macro_label[i]), eventDescription(macroLaunch[currentPad][i]));
    }
}

static void OnConfCancel(GtkWidget *widget, gpointer user_data) {
    loadConfig();
    gtk_widget_hide(ConfWidgets.config_window);
    gtk_widget_destroy(ConfWidgets.config_window);
    gtk_main_quit();
}

static void OnConfOk(GtkWidget *widget, gpointer user_data) {
    saveConfig();
    gtk_widget_hide(ConfWidgets.config_window);
    gtk_widget_destroy(ConfWidgets.config_window);
    gtk_main_quit();
}

static void OnConfBtn(GtkWidget *But, gpointer data) {
    EventCode events[MAXCNT];
    EventCode e=NO_EVENT;
    int i,j,cnt;
    KeySym ksym;
    PadJoyEvent *pje;
    int ok=0;
    int labelnr = (int) gtk_object_get_user_data(GTK_OBJECT(But));
    int btnnr = buttonInfo[labelnr].nr;
    int e_rem1 =NO_EVENT;
    int e_rem2 =NO_EVENT;

    devicefile[currentPad] = open(devicefilename[currentPad], O_RDONLY);

    if (devicefilename[currentPad][0] && devicefile[currentPad] == -1) {
      fprintf(stderr, "DFInput: could not open device %s, errno=%d\n", devicefilename[currentPad], errno);
    }

    for (i=0; i<MAXDEVICES; i++) {
      for (j=0; j<MAXAXES; j++) {
        axestatus[i][j] = AXESTS_UNKNOWN;
      }
    }

    for (i=0; i<100 && !ok; i++) {
        cnt = getPendingEvents(30, events, MAXCNT, 1, 1, 1, NULL);

        for (j=0; j<cnt && !ok; j++) {
            e = events[j];
            if (e<RELEASE_EVENT) {
                pje = EventCode2PadJoyEvent(e);
                if (btnnr>15) {
                    if (pje->event_type == EVENTTYPE_AXISPLUS || pje->event_type == EVENTTYPE_AXISMINUS) {
                        e = ANALOGAXIS_EVENT(pje->pad, pje->no, 0);
                        PadButtons[currentPad][btnnr] = e;
                        ok = 1;
                        e_rem1 = AXISPLUS_EVENT(pje->pad, pje->no);
                        e_rem2 = AXISMINUS_EVENT(pje->pad, pje->no);
                    }
                }
                else if (e<FIRST_JOY_EVENT) {
                    ksym = e-FIRST_KEY_EVENT;
                    if (ksym != XK_Escape) {
                        PadButtons[currentPad][btnnr] = e;
                        ok=1;
                    }
                }
                else {
                    PadButtons[currentPad][btnnr] = e;
                    if (pje->event_type == EVENTTYPE_AXISPLUS || pje->event_type == EVENTTYPE_AXISMINUS) {
                      e_rem1 = ANALOGAXIS_EVENT(pje->pad, pje->no, 0);
                    }
                    ok=1;
                }
            }

            if (!ok) {
                fprintf(stderr, "DFInput: event %ld (%s) is not usable.\n", (long) e, EventCode2String(e));
            }
        }
    }

    if (!ok) {
        fprintf(stderr, "DFInput: no usable input received.\n");
    }

    while (getPendingEvents(0, events, MAXCNT, 1, 1, 1, NULL)){} // read pending events to clear buffers

    close(devicefile[currentPad]);
    devicefile[currentPad] = -1;

    if (ok) {
        // If this event is assigned to another button, remove this assignment
        for (i=0; i<MAXDEVICES; i++) {
            for (j=0; j<MAXPSXBUTTONS; j++) {
                if ((PadButtons[i][j] == e || PadButtons[i][j] == e_rem1 || PadButtons[i][j] == e_rem2) &&
                    (i!=currentPad || j!=btnnr)) {
                    PadButtons[i][j] = NO_EVENT;
                }
            }
            for (j=0; j<MAXMACROS; j++) {
                if (macroLaunch[i][j] == e || macroLaunch[i][j] == e_rem1 || macroLaunch[i][j] == e_rem2) {
                    macroLaunch[i][j] = NO_EVENT;
                }
            }
        }

        showPadConfiguration();
    }
}

static void OnMacroBtn(GtkWidget *But, gpointer data) {
    EventCode events[MAXCNT];
    EventCode e=NO_EVENT;
    PadJoyEvent *pje;
    int i,j,cnt;
    KeySym ksym;
    int ok=0;
    int e_rem1 =NO_EVENT;

    int macronr = (int) gtk_object_get_user_data(GTK_OBJECT(But));

    devicefile[currentPad] = open(devicefilename[currentPad], O_RDONLY);

    for (i=0; i<MAXDEVICES; i++) {
      for (j=0; j<MAXAXES; j++) {
        axestatus[i][j] = AXESTS_UNKNOWN;
      }
    }

    for (i=0; i<100 && !ok; i++) {
        cnt = getPendingEvents(30, events, MAXCNT, 1, 1, 1, NULL);

        for (j=0; j<cnt && !ok; j++) {
            e = events[j];
            if (e<RELEASE_EVENT) {
                pje = EventCode2PadJoyEvent(e);
                if (e<FIRST_JOY_EVENT) {
                    ksym = e-FIRST_KEY_EVENT;
                    if (ksym != XK_Escape) {
                        macroLaunch[currentPad][macronr] = e;
                        ok=1;
                    }
                }
                else {
                    macroLaunch[currentPad][macronr] = e;
                    ok=1;
                    if (pje->event_type == EVENTTYPE_AXISPLUS || pje->event_type == EVENTTYPE_AXISMINUS) {
                      e_rem1 = ANALOGAXIS_EVENT(pje->pad, pje->no, 0);
                    }
                }
            }
        }
    }
    while (getPendingEvents(0, events, MAXCNT, 1, 1, 1, NULL)){} // read pending events to clear buffers

    close(devicefile[currentPad]);
    devicefile[currentPad] = -1;

    if (ok) {
        // If this event is assigned to another button, remove this assignment
        for (i=0; i<MAXDEVICES; i++) {
            for (j=0; j<MAXPSXBUTTONS; j++) {
                if (PadButtons[i][j] == e || PadButtons[i][j] == e_rem1) {
                    PadButtons[i][j] = NO_EVENT;
                }
            }
            for (j=0; j<MAXMACROS; j++) {
                if (macroLaunch[i][j] == e && (i!=currentPad || j!=macronr)) {
                    macroLaunch[i][j] = NO_EVENT;
                }
            }
        }

        showPadConfiguration();
    }
}

static void OnMacroDefineBtn(GtkWidget *But, gpointer data) {
    EventCode events[MAXCNT];
    EventCode e=NO_EVENT;
    PadJoyEvent *pje;
    int i,j,cnt;
    int ok=0;
    long now;
    long timing[MAXCNT];

    int macronr = (int) gtk_object_get_user_data(GTK_OBJECT(But));

    devicefile[currentPad] = open(devicefilename[currentPad], O_RDONLY);

    now = -1;

    for (i=0; i<MAXDEVICES; i++) {
      for (j=0; j<MAXAXES; j++) {
        axestatus[i][j] = AXESTS_UNUSED;
      }
    }

    for (i=0; i<MAXDEVICES; i++) {
        for (j=0; j<MAXPSXBUTTONS; j++) {
            pje = EventCode2PadJoyEvent(PadButtons[i][j]);

            if (pje->event_type == EVENTTYPE_AXISPLUS || pje->event_type == EVENTTYPE_AXISMINUS) {
                axestatus[pje->pad][pje->no] = AXESTS_UNKNOWN;
            }
            else if (pje->event_type == EVENTTYPE_ANALOG && use_analog) {
                axestatus[pje->pad][pje->no] = AXESTS_ANALOG;
            }
        }
    }

    i=0;

    gtk_widget_add_events(ConfWidgets.config_window,GDK_KEY_RELEASE_MASK);

	/* Listen for input events until finished (Escape is pressed) or timeout */
    while (!ok && i<MAXMACROLENGTH-1) {
        cnt = getPendingEvents(2000, events, MAXCNT, 1, 1, 1, timing);

        if (cnt && now<0) {
            now=timing[0];
        }
        else if (!cnt && i) {
          // inactivity for 2 seconds, finish definition
          ok=1;
        }

        for (j=0; j<cnt && !ok && i<MAXMACROLENGTH-1; j++) {
            e = events[j];
            if ((e-FIRST_KEY_EVENT) != XK_Escape && (e-FIRST_KEY_EVENT) != (XK_Escape+RELEASE_EVENT)) {
                macroEvents[currentPad][macronr][i] = e;
                macroInterval[currentPad][macronr][i] = timing[j]-now;
                now = timing[j];
                i++;
            }
            else {
                ok=1;
            }
        }
    }

    if (i>0) {
        macroEvents[currentPad][macronr][i] = NO_EVENT;
    }

    close(devicefile[currentPad]);
    devicefile[currentPad] = -1;

}

static void OnThreadCheck(GtkWidget *But, gpointer data) {
	use_threads = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(But));
}

static void OnAnalogCheck(GtkWidget *But, gpointer data) {
	use_analog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(But));
}

static void OnPadnoRadio(GtkWidget *But, gpointer data) {
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (But)))
		currentPad = (int) data;
	showPadConfiguration();
}

static void OnFilenameEntry(GtkWidget *Ent, gpointer data) {
    strcpy(devicefilename[currentPad], gtk_entry_get_text(GTK_ENTRY(Ent)));
}

/* Closed by clicking on the top-right hand corner */
void on_config_win_clicked (GtkWidget *widget, gpointer user_data) {
	gtk_widget_destroy (widget);
	gtk_main_quit();
	exit (0);
}

static void CreateConfigWindow(void) {
    char buffer[100];
    int i;
    GtkWidget *hbox;

    GladeXML *xml;
    xml = glade_xml_new (DATADIR "dfinput.glade2", "CfgWin", NULL);
    /* ADB TODO Error checking */

    GtkWidget *widget = glade_xml_get_widget (xml, "CfgWin");
    ConfWidgets.config_window = widget;
    GtkWidget *fixedbox = glade_xml_get_widget (xml, "fixed1");

	g_signal_connect_data(GTK_OBJECT(widget), "delete_event",
		GTK_SIGNAL_FUNC(on_config_win_clicked), NULL, NULL, G_CONNECT_AFTER);

/* DELETE   hbox = gtk_hbox_new(FALSE, 0);
    gtk_fixed_put(GTK_FIXED(fixedbox), hbox, 10, 10);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Pad number:"), FALSE, TRUE, 2);*/

//    ConfWidgets.padnogroup = NULL;

	widget = glade_xml_get_widget (xml, "radiobutton1");
	g_signal_connect_data (GTK_OBJECT (widget), "toggled",
			GTK_SIGNAL_FUNC (OnPadnoRadio), (gpointer) 0, NULL, G_CONNECT_AFTER);
	widget = glade_xml_get_widget (xml, "radiobutton2");
	g_signal_connect_data (GTK_OBJECT (widget), "toggled",
			GTK_SIGNAL_FUNC (OnPadnoRadio), (gpointer) 1, NULL, G_CONNECT_AFTER);

//    hbox = gtk_hbox_new(FALSE, 0);
//    gtk_fixed_put (GTK_FIXED (fixedbox), hbox, 10, 40);

//    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Device file:"), FALSE, TRUE, 2);
    /*ConfWidgets.filename_entry = gtk_entry_new();*/

    widget = glade_xml_get_widget (xml, "filename_entry");
    gtk_entry_set_text ( GTK_ENTRY(widget), (gchar *) devicefilename[0] );
    gtk_entry_set_max_length ( GTK_ENTRY(widget), FILENAME_MAX);
    //gtk_box_pack_start(GTK_BOX(hbox), ConfWidgets.filename_entry, FALSE, TRUE, 2);
    gtk_signal_connect(GTK_OBJECT(widget), "changed", GTK_SIGNAL_FUNC(OnFilenameEntry), NULL);
    ConfWidgets.filename_entry = widget;

/*  TODO
    ConfWidgets.thread_check = gtk_check_button_new_with_label("multithreaded");*/
	widget = glade_xml_get_widget (xml, "chkMultithreaded");
	g_signal_connect_data (GTK_OBJECT (widget), "toggled",
			GTK_SIGNAL_FUNC (OnThreadCheck), NULL, NULL, G_CONNECT_AFTER);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), use_threads);

    /* TODO
    ConfWidgets.analog_check = gtk_check_button_new_with_label("analog");*/
	widget = glade_xml_get_widget (xml, "chkAnalog");
    g_signal_connect_data (GTK_OBJECT (widget), "toggled",
			GTK_SIGNAL_FUNC (OnAnalogCheck), NULL, NULL, G_CONNECT_AFTER);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), use_analog);

    // Pad Buttons
    for (i=0; i<CONFIGBUTTONCOUNT; i++) {
        hbox = gtk_hbox_new(FALSE, 0);
        ConfWidgets.button[i] = gtk_button_new_with_label(buttonInfo[i].label);
        gtk_box_pack_start(GTK_BOX(hbox), ConfWidgets.button[i], FALSE, TRUE, 2);
        gtk_fixed_put(GTK_FIXED(fixedbox), hbox, buttonInfo[i].x, buttonInfo[i].y);
        gtk_object_set_user_data(GTK_OBJECT(ConfWidgets.button[i]), (char *)i);
        gtk_signal_connect(GTK_OBJECT(ConfWidgets.button[i]), "clicked", GTK_SIGNAL_FUNC(OnConfBtn), NULL);
        ConfWidgets.label[i] = gtk_label_new(eventDescription(PadButtons[0][buttonInfo[i].nr]));
        gtk_box_pack_start(GTK_BOX(hbox), ConfWidgets.label[i], FALSE, TRUE, 2);
    }

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_fixed_put(GTK_FIXED(fixedbox), hbox, 20, 390);
    // Macro Buttons
    for (i=0; i<MAXMACROS; i++) {
        sprintf(buffer, "M%d", i+1);
        ConfWidgets.macro_button[i] = gtk_button_new_with_label(buffer);
        gtk_box_pack_start(GTK_BOX(hbox), ConfWidgets.macro_button[i], FALSE, TRUE, 2);
        gtk_object_set_user_data(GTK_OBJECT(ConfWidgets.macro_button[i]), (char *)i);
        gtk_signal_connect(GTK_OBJECT(ConfWidgets.macro_button[i]), "clicked", GTK_SIGNAL_FUNC(OnMacroBtn), NULL);
        ConfWidgets.macro_label[i] = gtk_label_new(eventDescription(macroLaunch[0][i]));
        gtk_box_pack_start(GTK_BOX(hbox), ConfWidgets.macro_label[i], FALSE, TRUE, 2);
        ConfWidgets.macro_def_button[i] = gtk_button_new_with_label("Def");
        gtk_box_pack_start(GTK_BOX(hbox), ConfWidgets.macro_def_button[i], FALSE, TRUE, 2);
        gtk_object_set_user_data(GTK_OBJECT(ConfWidgets.macro_def_button[i]), (char *)i);
        gtk_signal_connect(GTK_OBJECT(ConfWidgets.macro_def_button[i]), "clicked", GTK_SIGNAL_FUNC(OnMacroDefineBtn), NULL);
        if (i<MAXMACROS-1) {
            gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("   "), FALSE, TRUE, 2);
        }
    }

	gtk_widget_show_all (GTK_WIDGET (fixedbox));

	widget = glade_xml_get_widget (xml, "btnOK");
	g_signal_connect_data (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (OnConfOk), NULL, NULL, G_CONNECT_AFTER);

	widget = glade_xml_get_widget (xml, "btnCancel");
	g_signal_connect_data (GTK_OBJECT (widget), "clicked", GTK_SIGNAL_FUNC (OnConfCancel), NULL, NULL, G_CONNECT_AFTER);
}

long PADconfigure(void) {
    currentPad = 0;
    initPadtime();
    PADinit(0);
    CreateConfigWindow();
    gtk_widget_show_all(ConfWidgets.config_window);

    gtk_main();

    return 0;
}


/*---------------------------------------------------------------------*/
/*                          About dialogue stuff                       */
/*---------------------------------------------------------------------*/

void PADabout(void) {
	GladeXML *xml;
	xml = glade_xml_new (DATADIR "dfinput.glade2", "AboutWin", NULL);

	GtkWidget *widget = glade_xml_get_widget (xml, "AboutWin");

	/* TODO Authors, about, version */

	gtk_dialog_run (GTK_DIALOG (widget));
	gtk_widget_destroy (widget);
//	gtk_main_quit();
}

/*---------------------------------------------------------------------*/
/*                          Main program                               */
/*---------------------------------------------------------------------*/

int main(int argc, char **argv) {
#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale();
  gtk_init (&argc, &argv);

  if (argc>1 && !strcmp(argv[1], "-about")) {
    PADabout();
  }
  else {
    PADconfigure();
  }

  gtk_exit (0);

  return 0;
}
