
#ifndef __PLUG_PAD_H__
#define __PLUG_PAD_H__

#include <IOKit/hid/IOHIDUsageTables.h>

#define MAX_NUM_PADS		2
#define MAX_NUM_KEYS		64
#define MAX_NUM_AXES		32

typedef struct {
    unsigned long button;
    unsigned char reverse;
    
    pRecDevice device;
    pRecElement element;
} keyEntry;

typedef struct {
    unsigned long axis;
    unsigned char reverse;
    unsigned char positive;
    unsigned char lastValue;
    
    pRecDevice device;
    pRecElement element;
} axisEntry;

extern int gControllerType[MAX_NUM_PADS];
extern int gNumKeys[MAX_NUM_PADS];
extern keyEntry gKeys[MAX_NUM_PADS][MAX_NUM_KEYS];
extern int gNumAxes[MAX_NUM_PADS];
extern axisEntry gAxes[MAX_NUM_PADS][MAX_NUM_AXES];

#endif //__PLUG_PAD_H__