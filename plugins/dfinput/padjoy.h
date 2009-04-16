#ifndef _PADJOY_H_

#define _PADJOY_H_

#define DEBUG(x) fprintf(stderr, "DFInput: %s\n", x);

// Maximum number of joy devices
#define MAXDEVICES 2
// Maximum number of supported axis (per device, a normal joystick has 2)
#define MAXAXES 20
// Maximum number of supported buttons (per device)
#define MAXBUTTONS 32
// Number of PSX-Buttons  (including analog axis)
#define MAXPSXBUTTONS 20

// Status of digital Axes (avoids repeated events of the same type on an analog pad)
#define AXESTS_UNUSED -1
#define AXESTS_UNKNOWN 0
#define AXESTS_CENTER 1
#define AXESTS_PLUS 2
#define AXESTS_MINUS 3
#define AXESTS_ANALOG 4

// every kind of supported event is coded into a long int
typedef int32_t EventCode;

// Position of directional buttons in PadButtons
#define PSXBTN_UP 12
#define PSXBTN_RIGHT 13
#define PSXBTN_DOWN 14
#define PSXBTN_LEFT 15

// macros to define Eventcodes
// this may look like I make it too complicated, but it keeps things open for future improvements
#define NO_EVENT 0
#define FIRST_KEY_EVENT 0
#define FIRST_JOY_EVENT (1<<16)
#define RELEASE_EVENT (1<<30)
#define FIRST_ANALOG_EVENT (1<<20)

#define KEY_EVENT(n) (FIRST_KEY_EVENT+(n))
#define AXISPLUS_EVENT(p,n) (FIRST_JOY_EVENT+2*(n)+(p)*(2*MAXAXES+MAXBUTTONS))
#define AXISMINUS_EVENT(p,n) (FIRST_JOY_EVENT+2*(n)+1+(p)*(2*MAXAXES+MAXBUTTONS))
#define BUTTON_EVENT(p,n) (FIRST_JOY_EVENT+2*MAXAXES+(n)+(p)*(2*MAXAXES+MAXBUTTONS))
#define ANALOGAXIS_EVENT(p,n,v) (FIRST_ANALOG_EVENT+256*(n)+(v)+(p)*(256*MAXAXES))

// Makro Definitions etc.
#define MAXMACROS 3
#define MAXMACROLENGTH 100

typedef struct
{
    unsigned char controllerType;
    unsigned short buttonStatus;
    unsigned char rightJoyX, rightJoyY, leftJoyX, leftJoyY;
    unsigned char moveX, moveY;
    unsigned char reserved[91];
} PadDataS;

// Number of events to fetch at once...
#define MAXCNT 100

/* number of buttons required for configuration */
#define CONFIGBUTTONCOUNT 20

#define EVENTTYPE_NONE -1
#define EVENTTYPE_KEY 0
#define EVENTTYPE_BUTTON 1
#define EVENTTYPE_AXISPLUS 2
#define EVENTTYPE_AXISMINUS 3
#define EVENTTYPE_ANALOG 4

// The following struct is used in the config part only
typedef struct
{
   unsigned int event_type; // Button, Axe, AnalogAxe
   unsigned int pad;
   unsigned int no; // Keycode in case of key event
   int value;  // 0=release, 1=press, or analog value in case of EVENTTYPE_ANALOG
} PadJoyEvent;

#endif
