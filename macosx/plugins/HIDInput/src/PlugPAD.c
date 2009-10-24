
#include <IOKit/hid/IOHIDUsageTables.h>
#include "HID_Utilities.h"
#include "PlugPAD.h"


/////////////////////////////////////////////////////////
typedef void* HWND;
#include "PSEmu_Plugin_Defs.h"

const char *LibName = "HIDInput";
const int version = 0;
const int revision = 1;
const int build = 0;

const char *PSEgetLibName(void) {
	return LibName;
}

unsigned long PSEgetLibType(void) {
	return PSE_LT_PAD;
}

unsigned long PSEgetLibVersion(void) {
	return version << 16 | revision << 8 | build;
}
/////////////////////////////////////////////////////////


// FIXME: dynamically allocate this
int gControllerType[MAX_NUM_PADS];
int gNumKeys[MAX_NUM_PADS];
keyEntry gKeys[MAX_NUM_PADS][MAX_NUM_KEYS];
int gNumAxes[MAX_NUM_PADS];
axisEntry gAxes[MAX_NUM_PADS][MAX_NUM_AXES];

static long sPadFlags = 0;

long PADinit(long flags) {
    sPadFlags |= flags;

    // kHIDPage_GenericDesktop,kHIDUsage_GD_GamePad
    if (!HIDHaveDeviceList()) {
        // List all HID devices
        HIDBuildDeviceList(kHIDPage_GenericDesktop, 0);
        
        if (HIDCountDevices() == 0) {
            // No devices found!
            HIDReleaseDeviceList();
            return PSE_PAD_ERR_INIT;
        }
        
        //HIDCloseReleaseInterface(
    }
    
    LoadConfig();
    
	 return 0;
}

long PADshutdown(void) {
    sPadFlags = 0;

    //HIDReleaseAllDeviceQueues();
    HIDReleaseDeviceList();
    
    return 0;
}

long PADopen(unsigned long *Disp) {
    //printf("start PADopen()\n");
    
    /*HIDReleaseAllDeviceQueues();
    
    if (sPadFlags & PSE_PAD_USE_PORT1) {
        for (i=0; i<numKeys; i++) {
            HIDQueueElement();
        }
    }
    if (sPadFlags & PSE_PAD_USE_PORT2) {
        for (i=0; i<numKeys; i++) {
            HIDQueueElement();
        }
    }*/
    
    return 0;
}

long PADclose(void) {
    //HIDReleaseAllDeviceQueues();
    
    return 0;
}

long PADconfigure(void) {
    // make sure our previous configuration was loaded
    if (sPadFlags == 0) {
        fprintf(stderr, "PADconfigure() called before PADinit()\n");
        if (!HIDHaveDeviceList()) {
            HIDBuildDeviceList(kHIDPage_GenericDesktop, 0);
        }
    }
    
    return DoConfiguration();
}

void PADabout(void) {
    DoAbout();
}

long _readPortX(PadDataS *data, int port)
{
    unsigned short buttonState = 0xffff;
    keyEntry *keys = gKeys[port];
    axisEntry *axes = gAxes[port];
    int i;
    
    //pRecDevice device;
    //IOHIDEventStruct event;
    
    /*device = HIDGetFirstDevice();
    do {
        while (HIDGetEvent(device, &event)) {
            switch (event->type) {
                case kIOHIDElementTypeInput_Button:
                    if (event->value) {
                    
                    }
                    break;
                default:
                    break;
            }
        }
    while (HIDGetNextDevice(device));*/
    
    for (i=0; i<gNumKeys[port]; i++) {
        long value = HIDGetElementValue(keys[i].device, keys[i].element);
        
        if (keys[i].element->usagePage == kHIDPage_GenericDesktop && 
            keys[i].element->usage >= kHIDUsage_GD_X && keys[i].element->usage <= kHIDUsage_GD_Rz) {
            /* axis input device */
            value = HIDCalibrateValue(value, keys[i].element);
            value = HIDScaleValue(value, keys[i].element);
            if (keys[i].reverse) {
                if (value < 64)
                    buttonState &= ~(1 << keys[i].button);
            } else {
                if (value > 191)
                    buttonState &= ~(1 << keys[i].button);
            }
        } else {
            if (value)
                buttonState &= ~(1 << keys[i].button);
        }
    }
    
    for (i=0; i<gNumAxes[port]; i++) {
        long value = HIDGetElementValue(axes[i].device, axes[i].element);
        
        if (value != axes[i].lastValue) {
            axes[i].lastValue = value;
            
            if (axes[i].element->usagePage == kHIDPage_GenericDesktop && 
                axes[i].element->usage >= kHIDUsage_GD_X && axes[i].element->usage <= kHIDUsage_GD_Rz) {
                /* axis input device */
                value = HIDCalibrateValue(value, axes[i].element);
                value = HIDScaleValue(value, axes[i].element);
                if (!axes[i].positive) value = 255-value;
                
                if (value >= 127) {
                    if (axes[i].reverse) value = 255-value;
                    
                    switch (axes[i].axis) {
                        case 0: data->rightJoyX = value; break;
                        case 1: data->rightJoyY = value; break;
                        case 2: data->leftJoyX  = value; break;
                        case 3: data->leftJoyY  = value; break;
                    }
                }
            }
        }
    }
    
    data->controllerType = gControllerType[port];
    data->buttonStatus = buttonState;
    return 0;
}

long PADreadPort1(PadDataS *data) {
    static unsigned char lastRightJoyX = 128;
    static unsigned char lastRightJoyY = 128;
    static unsigned char lastLeftJoyX = 128;
    static unsigned char lastLeftJoyY = 128;
    
    data->rightJoyX = lastRightJoyX;
    data->rightJoyY = lastRightJoyY;
    data->leftJoyX = lastLeftJoyX;
    data->leftJoyY = lastLeftJoyY;
    
    _readPortX(data, 0);
     
    lastRightJoyX = data->rightJoyX;
    lastRightJoyY = data->rightJoyY;
    lastLeftJoyX = data->leftJoyX;
    lastLeftJoyY = data->leftJoyY;
    
    return 0;
}

long PADreadPort2(PadDataS *data) {
    static unsigned char lastRightJoyX = 128;
    static unsigned char lastRightJoyY = 128;
    static unsigned char lastLeftJoyX = 128;
    static unsigned char lastLeftJoyY = 128;
    
    data->rightJoyX = lastRightJoyX;
    data->rightJoyY = lastRightJoyY;
    data->leftJoyX = lastLeftJoyX;
    data->leftJoyY = lastLeftJoyY;
    
    _readPortX(data, 1);
     
    lastRightJoyX = data->rightJoyX;
    lastRightJoyY = data->rightJoyY;
    lastLeftJoyX = data->leftJoyX;
    lastLeftJoyY = data->leftJoyY;
    
    return 0;
}

long PADkeypressed()
{
	return 0;
}
