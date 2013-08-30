/*
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * All Rights Reserved.
 *
 * Based on: HIDInput by Gil Pedersen.
 * Copyright (c) 2004, Gil Pedersen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#import <Cocoa/Cocoa.h>
#import "PadController.h"
#import "ARCBridge.h"
#include "pad.h"

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

static NSWindow *padWindow = nil;
static PadController *padController = nil;

#define APP_ID @"net.pcsxr.DFInputPlugin"
#define PrefsKey APP_ID @" Settings"

#define kDFThreading @"Threading"
#define kDFPad1 @"Pad 1"
#define kDFPad2 @"Pad 2"

static void SetDefaultConfig() {
	memset(&g.cfg, 0, sizeof(g.cfg));
	
	g.cfg.Threaded = 1;
	
	g.cfg.PadDef[0].DevNum = 0;
	g.cfg.PadDef[1].DevNum = 1;
	
	g.cfg.PadDef[0].Type = PSE_PAD_TYPE_STANDARD;
	g.cfg.PadDef[1].Type = PSE_PAD_TYPE_STANDARD;
	
	// Pad1 keyboard
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].Key = 9;
	g.cfg.PadDef[0].KeyDef[DKEY_START].Key = 10;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].Key = 127;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].Key = 125;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].Key = 126;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].Key = 124;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].Key = 16;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].Key = 18;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].Key = 14;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].Key = 15;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].Key = 3;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].Key = 8;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].Key = 7;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].Key = 2;
	g.cfg.PadDef[0].KeyDef[DKEY_ANALOG].Key = 12;
	
	// Pad1 joystick
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_SELECT].J.Button = 8;
	g.cfg.PadDef[0].KeyDef[DKEY_START].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_START].J.Button = 9;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_UP].J.Axis = -2;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_RIGHT].J.Axis = 1;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_DOWN].J.Axis = 2;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].JoyEvType = AXIS;
	g.cfg.PadDef[0].KeyDef[DKEY_LEFT].J.Axis = -1;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_L2].J.Button = 4;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_L1].J.Button = 6;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_R2].J.Button = 5;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_R1].J.Button = 7;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_TRIANGLE].J.Button = 0;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_CIRCLE].J.Button = 1;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_CROSS].J.Button = 2;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].JoyEvType = BUTTON;
	g.cfg.PadDef[0].KeyDef[DKEY_SQUARE].J.Button = 3;
	
	// Pad2 joystick
	g.cfg.PadDef[1].KeyDef[DKEY_SELECT].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_SELECT].J.Button = 8;
	g.cfg.PadDef[1].KeyDef[DKEY_START].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_START].J.Button = 9;
	g.cfg.PadDef[1].KeyDef[DKEY_UP].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_UP].J.Axis = -2;
	g.cfg.PadDef[1].KeyDef[DKEY_RIGHT].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_RIGHT].J.Axis = 1;
	g.cfg.PadDef[1].KeyDef[DKEY_DOWN].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_DOWN].J.Axis = 2;
	g.cfg.PadDef[1].KeyDef[DKEY_LEFT].JoyEvType = AXIS;
	g.cfg.PadDef[1].KeyDef[DKEY_LEFT].J.Axis = -1;
	g.cfg.PadDef[1].KeyDef[DKEY_L2].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_L2].J.Button = 4;
	g.cfg.PadDef[1].KeyDef[DKEY_L1].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_L1].J.Button = 6;
	g.cfg.PadDef[1].KeyDef[DKEY_R2].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_R2].J.Button = 5;
	g.cfg.PadDef[1].KeyDef[DKEY_R1].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_R1].J.Button = 7;
	g.cfg.PadDef[1].KeyDef[DKEY_TRIANGLE].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_TRIANGLE].J.Button = 0;
	g.cfg.PadDef[1].KeyDef[DKEY_CIRCLE].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_CIRCLE].J.Button = 1;
	g.cfg.PadDef[1].KeyDef[DKEY_CROSS].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_CROSS].J.Button = 2;
	g.cfg.PadDef[1].KeyDef[DKEY_SQUARE].JoyEvType = BUTTON;
	g.cfg.PadDef[1].KeyDef[DKEY_SQUARE].J.Button = 3;
}



void LoadPADConfig()
{
	SetDefaultConfig();
	[[NSUserDefaults standardUserDefaults] registerDefaults:
	 [NSDictionary dictionaryWithObject:[NSDictionary dictionaryWithObjectsAndKeys:
										 DefaultPadArray(0), kDFPad1,
										 DefaultPadArray(1), kDFPad2,
										 @YES, kDFThreading,
										 nil]
								 forKey:PrefsKey]];
	
	//Load the old preferences if present.
	NSFileManager *fm = [NSFileManager defaultManager];
	NSString *oldPrefPath = [NSString pathWithComponents:@[NSHomeDirectory(), @"Library", @"Preferences", @"net.pcsxr.DFInput.plist"]];
	if ([fm fileExistsAtPath:oldPrefPath]) {
		char buf[256] = {0};
		int current = 0, a = 0, b = 0, c = 0;

		FILE *fp = fopen([oldPrefPath fileSystemRepresentation], "r");
		if (fp == NULL) {
			return;
		}
		
		while (fgets(buf, 256, fp) != NULL) {
			if (strncmp(buf, "Threaded=", 9) == 0) {
				g.cfg.Threaded = atoi(&buf[9]);
			} else if (strncmp(buf, "[PAD", 4) == 0) {
				current = atoi(&buf[4]) - 1;
				if (current < 0) {
					current = 0;
				} else if (current > 1) {
					current = 1;
				}
			} else if (strncmp(buf, "DevNum=", 7) == 0) {
				g.cfg.PadDef[current].DevNum = atoi(&buf[7]);
			} else if (strncmp(buf, "Type=", 5) == 0) {
				g.cfg.PadDef[current].Type = atoi(&buf[5]);
			} else if (strncmp(buf, "Select=", 7) == 0) {
				sscanf(buf, "Select=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_SELECT].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_SELECT].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_SELECT].J.d = c;
			} else if (strncmp(buf, "L3=", 3) == 0) {
				sscanf(buf, "L3=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_L3].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_L3].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_L3].J.d = c;
			} else if (strncmp(buf, "R3=", 3) == 0) {
				sscanf(buf, "R3=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_R3].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_R3].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_R3].J.d = c;
			} else if (strncmp(buf, "Analog=", 7) == 0) {
				sscanf(buf, "Analog=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_ANALOG].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_ANALOG].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_ANALOG].J.d = c;
			} else if (strncmp(buf, "Start=", 6) == 0) {
				sscanf(buf, "Start=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_START].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_START].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_START].J.d = c;
			} else if (strncmp(buf, "Up=", 3) == 0) {
				sscanf(buf, "Up=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_UP].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_UP].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_UP].J.d = c;
			} else if (strncmp(buf, "Right=", 6) == 0) {
				sscanf(buf, "Right=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_RIGHT].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_RIGHT].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_RIGHT].J.d = c;
			} else if (strncmp(buf, "Down=", 5) == 0) {
				sscanf(buf, "Down=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_DOWN].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_DOWN].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_DOWN].J.d = c;
			} else if (strncmp(buf, "Left=", 5) == 0) {
				sscanf(buf, "Left=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_LEFT].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_LEFT].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_LEFT].J.d = c;
			} else if (strncmp(buf, "L2=", 3) == 0) {
				sscanf(buf, "L2=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_L2].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_L2].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_L2].J.d = c;
			} else if (strncmp(buf, "R2=", 3) == 0) {
				sscanf(buf, "R2=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_R2].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_R2].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_R2].J.d = c;
			} else if (strncmp(buf, "L1=", 3) == 0) {
				sscanf(buf, "L1=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_L1].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_L1].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_L1].J.d = c;
			} else if (strncmp(buf, "R1=", 3) == 0) {
				sscanf(buf, "R1=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_R1].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_R1].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_R1].J.d = c;
			} else if (strncmp(buf, "Triangle=", 9) == 0) {
				sscanf(buf, "Triangle=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_TRIANGLE].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_TRIANGLE].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_TRIANGLE].J.d = c;
			} else if (strncmp(buf, "Circle=", 7) == 0) {
				sscanf(buf, "Circle=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_CIRCLE].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_CIRCLE].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_CIRCLE].J.d = c;
			} else if (strncmp(buf, "Cross=", 6) == 0) {
				sscanf(buf, "Cross=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_CROSS].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_CROSS].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_CROSS].J.d = c;
			} else if (strncmp(buf, "Square=", 7) == 0) {
				sscanf(buf, "Square=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].KeyDef[DKEY_SQUARE].Key = a;
				g.cfg.PadDef[current].KeyDef[DKEY_SQUARE].JoyEvType = b;
				g.cfg.PadDef[current].KeyDef[DKEY_SQUARE].J.d = c;
			} else if (strncmp(buf, "LeftAnalogXP=", 13) == 0) {
				sscanf(buf, "LeftAnalogXP=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XP].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XP].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XP].J.d = c;
			} else if (strncmp(buf, "LeftAnalogXM=", 13) == 0) {
				sscanf(buf, "LeftAnalogXM=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XM].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XM].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_XM].J.d = c;
			} else if (strncmp(buf, "LeftAnalogYP=", 13) == 0) {
				sscanf(buf, "LeftAnalogYP=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YP].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YP].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YP].J.d = c;
			} else if (strncmp(buf, "LeftAnalogYM=", 13) == 0) {
				sscanf(buf, "LeftAnalogYM=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YM].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YM].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_LEFT][ANALOG_YM].J.d = c;
			} else if (strncmp(buf, "RightAnalogXP=", 14) == 0) {
				sscanf(buf, "RightAnalogXP=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XP].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XP].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XP].J.d = c;
			} else if (strncmp(buf, "RightAnalogXM=", 14) == 0) {
				sscanf(buf, "RightAnalogXM=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XM].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XM].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_XM].J.d = c;
			} else if (strncmp(buf, "RightAnalogYP=", 14) == 0) {
				sscanf(buf, "RightAnalogYP=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YP].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YP].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YP].J.d = c;
			} else if (strncmp(buf, "RightAnalogYM=", 14) == 0) {
				sscanf(buf, "RightAnalogYM=%d,%d,%d", &a, &b, &c);
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YM].Key = a;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YM].JoyEvType = b;
				g.cfg.PadDef[current].AnalogDef[ANALOG_RIGHT][ANALOG_YM].J.d = c;
			}
		}
		
		fclose(fp);
		//Save to new preferences
		SavePADConfig();
		//Delete the old preferences
		[fm removeItemAtPath:oldPrefPath error:NULL];
	} else {
		NSDictionary *dfPrefs = [[NSUserDefaults standardUserDefaults] dictionaryForKey:PrefsKey];
		g.cfg.Threaded = [[dfPrefs objectForKey:kDFThreading] boolValue];
		LoadPadArray(0, [dfPrefs objectForKey:kDFPad1]);
		LoadPadArray(1, [dfPrefs objectForKey:kDFPad2]);
	}
}

void SavePADConfig()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSMutableDictionary *pad1Dict = nil, *pad2Dict = nil;
	NSDictionary *prefDict = [defaults dictionaryForKey:PrefsKey];
	pad1Dict = [[NSMutableDictionary alloc] initWithDictionary:[prefDict objectForKey:kDFPad1]];
	pad2Dict = [[NSMutableDictionary alloc] initWithDictionary:[prefDict objectForKey:kDFPad2]];
	prefDict = nil;
	
	[pad1Dict addEntriesFromDictionary:SavePadArray(0)];
	[pad2Dict addEntriesFromDictionary:SavePadArray(1)];
	
	[defaults setObject:[NSDictionary dictionaryWithObjectsAndKeys:
						 g.cfg.Threaded ? @YES : @NO, kDFThreading,
						 pad1Dict, kDFPad1,
						 pad2Dict, kDFPad2,
						 nil] forKey:PrefsKey];
	[defaults synchronize];
	RELEASEOBJ(pad1Dict);
	RELEASEOBJ(pad2Dict);
}

void DoAbout()
{
	// Get parent application instance
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];

	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (path) {
		credits = [[NSAttributedString alloc] initWithPath: path
				documentAttributes:NULL];
		AUTORELEASEOBJNORETURN(credits);
	} else {
		credits = AUTORELEASEOBJ([[NSAttributedString alloc] initWithString:@""]);
	}

	// Get Application Icon
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[bundle bundlePath]];
	NSSize size = NSMakeSize(64, 64);
	[icon setSize:size];

	NSDictionary *infoPaneDict =
	[[NSDictionary alloc] initWithObjectsAndKeys:
	 [bundle objectForInfoDictionaryKey:@"CFBundleName"], @"ApplicationName",
	 icon, @"ApplicationIcon",
	 [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"], @"ApplicationVersion",
	 [bundle objectForInfoDictionaryKey:@"CFBundleVersion"], @"Version",
	 [bundle objectForInfoDictionaryKey:@"NSHumanReadableCopyright"], @"Copyright",
	 credits, @"Credits",
	 nil];
	dispatch_async(dispatch_get_main_queue(), ^{
		[NSApp orderFrontStandardAboutPanelWithOptions:infoPaneDict];
	});
	RELEASEOBJ(infoPaneDict);
}

long DoConfiguration()
{
	RunOnMainThreadSync(^{
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
#else
		SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);
#endif
		LoadPADConfig();
		
		if (padWindow == nil) {
			if (padController == nil) {
				padController = [[PadController alloc] initWithWindowNibName:@"NetPcsxrHIDInputPluginMain"];
			}
			padWindow = [padController window];
		}
		
		[padWindow center];
		[padWindow makeKeyAndOrderFront:nil];
	});

	return 0;
}

@implementation PadController

- (IBAction)cancel:(id)sender
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
#else
	SDL_Quit();
#endif
	[self close];
}

- (IBAction)ok:(id)sender
{
	SavePADConfig();
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
#else
	SDL_Quit();
#endif
	[self close];
}

- (void)awakeFromNib
{
	[[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(windowWillClose:)
    name:NSWindowWillCloseNotification object:[self window]];

	[controllerView1 addSubview: controllerView];
	[controllerView setController:0];
}

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
	PadView *newView = nil;
	if ([[tabViewItem identifier] isEqualToString:@"pad1"])
		newView = controllerView1;
	else if ([[tabViewItem identifier] isEqualToString:@"pad2"])
		newView = controllerView2;

	if (nil != newView) {
		[controllerView removeFromSuperviewWithoutNeedingDisplay];
		[newView addSubview: controllerView];
		[controllerView setFrame:[newView frame]];
		[controllerView setController:[newView isEqual:controllerView1] ? 0 : 1];
	}
}

- (void)windowBecameKey:(NSNotification *)notification
{
	if ([[controllerView1 subviews] count] > 0)
		[controllerView setController:0];
	else if ([[controllerView2 subviews] count] > 0)
		[controllerView setController:1];

	[[NSNotificationCenter defaultCenter] removeObserver:self
    name:NSWindowDidBecomeKeyNotification object:[self window]];
}

- (void)windowWillClose:(NSNotification *)aNotification
{
	if ([aNotification object] == [self window]) {
		[[NSNotificationCenter defaultCenter] addObserver:self
		 selector:@selector(windowBecameKey:)
		 name:NSWindowDidBecomeKeyNotification object:[self window]];
	}
}

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([padController class]);
