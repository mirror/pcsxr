
#include <IOKit/hid/IOHIDUsageTables.h>
#include "HID_Utilities_External.h"
#include "PlugPAD.h"

/////////////////////////////////////////////////////////
typedef void* HWND;
#include "psemu_plugin_defs.h"

long DoConfiguration();
void DoAbout();

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

static unsigned char padid[2] = {0x41, 0x41};

int LoadConfig();

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
    padid[0] = 0x41;
    padid[1] = 0x41;

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

static unsigned char stdpar[2][8] = {
	{0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80},
	{0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80}
};

static unsigned char unk46[2][8] = {
	{0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A},
	{0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A}
};

static unsigned char unk47[2][8] = {
	{0xFF, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00}
};

static unsigned char unk4c[2][8] = {
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static unsigned char unk4d[2][8] = { 
	{0xFF, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0xFF, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

static unsigned char stdcfg[2][8]   = { 
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static unsigned char stdmode[2][8]  = { 
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static unsigned char stdmodel[2][8] = { 
	{0xFF,
	 0x5A,
	 0x01, // 03 - dualshock2, 01 - dualshock
	 0x02, // number of modes
	 0x01, // current mode: 01 - analog, 00 - digital
	 0x02,
	 0x01,
	 0x00},
	{0xFF, 
	 0x5A,
	 0x01, // 03 - dualshock2, 01 - dualshock
	 0x02, // number of modes
	 0x01, // current mode: 01 - analog, 00 - digital
	 0x02,
	 0x01,
	 0x00}
};

static unsigned char CurPad = 0, CurByte = 0, CurCmd = 0, CmdLen = 0;

enum {
	CMD_READ_DATA_AND_VIBRATE = 0x42,
	CMD_CONFIG_MODE = 0x43,
	CMD_SET_MODE_AND_LOCK = 0x44,
	CMD_QUERY_MODEL_AND_MODE = 0x45,
	CMD_QUERY_ACT = 0x46, // ??
	CMD_QUERY_COMB = 0x47, // ??
	CMD_QUERY_MODE = 0x4C, // QUERY_MODE ??
	CMD_VIBRATION_TOGGLE = 0x4D,
};

unsigned char PADstartPoll(int pad) {
	CurPad = pad - 1;
	CurByte = 0;

	return 0xFF;
}

unsigned char PADpoll(unsigned char value) {
	static unsigned char	*buf = NULL;
	PadDataS				data;

	if (CurByte == 0) {
		CurByte++;

		// Don't enable Analog/Vibration for a standard pad
		if (gControllerType[CurPad] != PSE_PAD_TYPE_ANALOGPAD) {
			CurCmd = CMD_READ_DATA_AND_VIBRATE;
		} else {
			CurCmd = value;
		}

		switch (CurCmd) {
			case CMD_CONFIG_MODE:
				CmdLen = 8;
				buf = stdcfg[CurPad];
				if (stdcfg[CurPad][3] == 0xFF) return 0xF3;
				else return padid[CurPad];

			case CMD_SET_MODE_AND_LOCK:
				CmdLen = 8;
				buf = stdmode[CurPad];
				return 0xF3;

			case CMD_QUERY_MODEL_AND_MODE:
				CmdLen = 8;
				buf = stdmodel[CurPad];
				buf[4] = (padid[CurPad] == 0x41 ? 0 : 1);
				return 0xF3;

			case CMD_QUERY_ACT:
				CmdLen = 8;
				buf = unk46[CurPad];
				return 0xF3;

			case CMD_QUERY_COMB:
				CmdLen = 8;
				buf = unk47[CurPad];
				return 0xF3;

			case CMD_QUERY_MODE:
				CmdLen = 8;
				buf = unk4c[CurPad];
				return 0xF3;

			case CMD_VIBRATION_TOGGLE:
				CmdLen = 8;
				buf = unk4d[CurPad];
				return 0xF3;

			case CMD_READ_DATA_AND_VIBRATE:
			default:
				if (CurPad == 0) {
					PADreadPort1(&data);
				} else {
					PADreadPort2(&data);
				}

				stdpar[CurPad][2] = data.buttonStatus & 0xFF;
				stdpar[CurPad][3] = data.buttonStatus >> 8;

				if (padid[CurPad] != 0x41) {
					CmdLen = 8;

					stdpar[CurPad][4] = data.rightJoyX;
					stdpar[CurPad][5] = data.rightJoyY;
					stdpar[CurPad][6] = data.leftJoyX;
					stdpar[CurPad][7] = data.leftJoyY;
				} else {
					CmdLen = 4;
				}

				buf = stdpar[CurPad];
				return padid[CurPad];
		}
	}

	switch (CurCmd) {
		case CMD_CONFIG_MODE:
			if (CurByte == 2) {
				switch (value) {
					case 0:
						buf[2] = 0;
						buf[3] = 0;
						break;

					case 1:
						buf[2] = 0xFF;
						buf[3] = 0xFF;
						break;
				}
			}
			break;

		case CMD_SET_MODE_AND_LOCK:
			if (CurByte == 2) {
				padid[CurPad] = value ? 0x73 : 0x41;
			}
			break;

		case CMD_QUERY_ACT:
			if (CurByte == 2) {
				switch (value) {
					case 0: // default
						buf[5] = 0x02;
						buf[6] = 0x00;
						buf[7] = 0x0A;
						break;

					case 1: // Param std conf change
						buf[5] = 0x01;
						buf[6] = 0x01;
						buf[7] = 0x14;
						break;
				}
			}
			break;

		case CMD_QUERY_MODE:
			if (CurByte == 2) {
				switch (value) {
					case 0: // mode 0 - digital mode
						buf[5] = PSE_PAD_TYPE_STANDARD;
						break;

					case 1: // mode 1 - analog mode
						buf[5] = PSE_PAD_TYPE_ANALOGPAD;
						break;
				}
			}
			break;
	}

	if (CurByte >= CmdLen) return 0;
	return buf[CurByte++];
}

long PADkeypressed() {
	return 0;
}
