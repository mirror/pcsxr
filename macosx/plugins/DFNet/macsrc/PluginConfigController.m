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

#define kIPADDRKEY @"IP Address"
#define kIPPORT @"IP Port"
#define kPLAYERNUM @"Player Number"

#define APP_ID @"net.codeplex.pcsxr.DFNet"
#define PrefsKey APP_ID @" Settings"
#define NSLocalizedStringInBundle(key, bundle, comment) \
	[bundle localizedStringForKey:(key) value:@"" table:nil]

static PluginConfigController *windowController = nil;

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

void AboutDlgProc()
{
	// Get parent application instance
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];
	
	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (!path) {
		path = [bundle pathForResource:@"Credits" ofType:@"rtfd"];
	}
	if (path) {
		credits = [[NSAttributedString alloc] initWithPath:path documentAttributes:NULL];
	} else {
		credits = [[NSAttributedString alloc] initWithString:@""];
	}
	
	// Get Application Icon
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[bundle bundlePath]];
	[icon setSize:NSMakeSize(64, 64)];
	
	NSDictionary *infoPaneDict =
	@{@"ApplicationName": [bundle objectForInfoDictionaryKey:@"CFBundleName"],
	  @"ApplicationIcon": icon,
	  @"ApplicationVersion": [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
	  @"Version": [bundle objectForInfoDictionaryKey:@"CFBundleVersion"],
	  @"Copyright": [bundle objectForInfoDictionaryKey:@"NSHumanReadableCopyright"],
	  @"Credits": credits};
	dispatch_async(dispatch_get_main_queue(), ^{
		[NSApp orderFrontStandardAboutPanelWithOptions:infoPaneDict];
	});
}

void ConfDlgProc()
{
	RunOnMainThreadSync(^{
		NSWindow *window;
		
		if (windowController == nil) {
			windowController = [[PluginConfigController alloc] initWithWindowNibName:@"DFNet"];
		}
		window = [windowController window];
		
		[windowController loadValues];
		
		[window center];
		[window makeKeyAndOrderFront:nil];
	});
}

void ReadConfig()
{
	NSDictionary *keyValues;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	[defaults registerDefaults:@{PrefsKey: @{kIPADDRKEY: @"127.0.0.1",
								 kIPPORT: @33306,
								 kPLAYERNUM: @1}}];
	
	keyValues = [defaults dictionaryForKey:PrefsKey];

	conf.PortNum = [keyValues[kIPPORT] unsignedShortValue];
	conf.PlayerNum = [keyValues[kPLAYERNUM] intValue];
	strlcpy(conf.ipAddress, [keyValues[kIPADDRKEY] cStringUsingEncoding:NSASCIIStringEncoding], sizeof(conf.ipAddress));
}

@implementation PluginConfigController
@synthesize ipAddress;
@synthesize portNum;
@synthesize playerNum;

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSBundle *curBundle = [NSBundle bundleForClass:[PluginConfigController class]];
	
	NSString *theAddress = [ipAddress  stringValue];
	NSInteger asciiLen = [theAddress lengthOfBytesUsingEncoding:NSASCIIStringEncoding];
	if (asciiLen > (sizeof(conf.ipAddress) - 1)) {
		NSBeginAlertSheet(NSLocalizedStringInBundle(@"Address Too Long", curBundle, nil), nil, nil, nil, [self window], nil, NULL, NULL, NULL, @"%@", NSLocalizedStringInBundle(@"The address is too long.\n\nTry to use only the IP address and not a host name.", curBundle, nil));
		return;
	} else if (asciiLen == 0) {
		NSBeginAlertSheet(NSLocalizedStringInBundle(@"Blank Address", curBundle, nil), nil, nil, nil, [self window], nil, NULL, NULL, NULL, @"%@", NSLocalizedStringInBundle(@"The address specified is either blank, or can't be converted to ASCII.\n\nTry connecting directly using the IP address using latin numerals.", curBundle, nil));
		return;
	}

	
	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:[defaults dictionaryForKey:PrefsKey]];
	writeDic[kIPPORT] = @((unsigned short)[portNum intValue]);
	writeDic[kPLAYERNUM] = @([playerNum intValue]);
	writeDic[kIPADDRKEY] = theAddress;

	// write to defaults
	[defaults setObject:writeDic forKey:PrefsKey];
	[defaults synchronize];

	// and set global values accordingly
	ReadConfig();

	[self close];
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	ReadConfig();
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];

	[ipAddress setStringValue:keyValues[kIPADDRKEY]];
	[portNum setIntValue:[keyValues[kIPPORT] unsignedShortValue]];
	[playerNum setIntValue:[keyValues[kPLAYERNUM] intValue]];
}

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginConfigController class])
