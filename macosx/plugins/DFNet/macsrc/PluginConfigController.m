/*
 * Copyright (c) 2010, Wei Mingzhi <whistler@openoffice.org>.
 * All Rights Reserved.
 *
 * Based on: Cdrom for Psemu Pro like Emulators
 * By: linuzappz <linuzappz@hotmail.com>
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

#import "PluginConfigController.h"
#include "dfnet.h"

NSString * const kIPADDRKEY = @"IP Address";
NSString * const kIPPORT = @"IP Port";
NSString * const kPLAYERNUM = @"Player Number";

#define APP_ID @"net.codeplex.pcsxr.DFNet"
#define PrefsKey APP_ID @" Settings"

static PluginConfigController *windowController = nil;

void AboutDlgProc()
{
	// Get parent application instance
	NSApplication *app = [NSApplication sharedApplication];
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];

	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (path) {
		credits = [[[NSAttributedString alloc] initWithPath: path
				documentAttributes:NULL] autorelease];
	} else {
		credits = [[[NSAttributedString alloc] initWithString:@""] autorelease];
	}

	// Get Application Icon
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[bundle bundlePath]];
	[icon setSize:NSMakeSize(64, 64)];

	[app orderFrontStandardAboutPanelWithOptions:[NSDictionary dictionaryWithObjectsAndKeys:
			[bundle objectForInfoDictionaryKey:@"CFBundleName"], @"ApplicationName",
			icon, @"ApplicationIcon",
			[bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"], @"ApplicationVersion",
			[bundle objectForInfoDictionaryKey:@"CFBundleVersion"], @"Version",
			[bundle objectForInfoDictionaryKey:@"NSHumanReadableCopyright"], @"Copyright",
			credits, @"Credits",
			nil]];
}

void ConfDlgProc()
{
	NSWindow *window;

	if (windowController == nil) {
		windowController = [[PluginConfigController alloc] initWithWindowNibName:@"DFNet"];
	}
	window = [windowController window];

	[windowController loadValues];

	[window center];
	[window makeKeyAndOrderFront:nil];
}

void ReadConfig()
{
	NSDictionary *keyValues;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
								[NSDictionary dictionaryWithObjectsAndKeys:
								 @"127.0.0.1",kIPADDRKEY,
								 [NSNumber numberWithInt:33306], kIPPORT,
								 [NSNumber numberWithInt:1], kPLAYERNUM,
								 nil], PrefsKey, nil]];
	
	keyValues = [defaults dictionaryForKey:PrefsKey];

	conf.PortNum = [[keyValues objectForKey:kIPPORT] intValue];
	conf.PlayerNum = [[keyValues objectForKey:kPLAYERNUM] intValue];
	strlcpy(conf.ipAddress, [[keyValues objectForKey:kIPADDRKEY] cStringUsingEncoding:NSASCIIStringEncoding], sizeof(conf.ipAddress));
}

@implementation PluginConfigController

- (IBAction)cancel:(id)sender
{
	[self close];
	[windowController release];
	windowController = nil;
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:[defaults dictionaryForKey:PrefsKey]];
	[writeDic setObject:[NSNumber numberWithInt:[portNum intValue]] forKey:kIPPORT];
	[writeDic setObject:[NSNumber numberWithInt:[playerNum intValue]] forKey:kPLAYERNUM];
	[writeDic setObject:[ipAddress  stringValue] forKey:kIPADDRKEY];

	// write to defaults
	[defaults setObject:writeDic forKey:PrefsKey];
	[defaults synchronize];

	// and set global values accordingly
	ReadConfig();

	[self close];
	[windowController release];
	windowController = nil;
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	ReadConfig();
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];

	[ipAddress setStringValue:[keyValues objectForKey:kIPADDRKEY]];
	[portNum setStringValue:[[keyValues objectForKey:kIPPORT] stringValue]];
	[playerNum setStringValue:[[keyValues objectForKey:kPLAYERNUM] stringValue]];
}

@end

const char* PLUGLOC(char *toloc)
{
	NSBundle *mainBundle = [NSBundle bundleForClass:[PluginConfigController class]];
	NSString *origString = nil, *transString = nil;
	origString = [NSString stringWithCString:toloc encoding:NSUTF8StringEncoding];
	transString = [mainBundle localizedStringForKey:origString value:nil table:nil];
	return [transString cStringUsingEncoding:NSUTF8StringEncoding];
}

