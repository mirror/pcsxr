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
#include "cdr.h"
#import "ARCBridge.h"

#define APP_ID @"net.pcsxr.DFCdrom"
#define PrefsKey APP_ID @" Settings"

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

static PluginConfigController *windowController = nil;

void AboutDlgProc()
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

void ConfDlgProc()
{
	RunOnMainThreadSync(^{
		NSWindow *window;
		
		if (windowController == nil) {
			windowController = [[PluginConfigController alloc] initWithWindowNibName:@"DFCdromPluginConfig"];
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
	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
		[NSDictionary dictionaryWithObjectsAndKeys:
			@YES, @"Threaded",
			@64, @"Cache Size",
			@0, @"Speed",
			nil], PrefsKey, nil]];

	keyValues = [defaults dictionaryForKey:PrefsKey];

	ReadMode = ([[keyValues objectForKey:@"Threaded"] boolValue] ? THREADED : NORMAL);
	CacheSize = [[keyValues objectForKey:@"Cache Size"] intValue];
	CdrSpeed = [[keyValues objectForKey:@"Speed"] intValue];
}

@implementation PluginConfigController

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];

	[writeDic setObject:@((BOOL)[Cached intValue]) forKey:@"Threaded"];
	[writeDic setObject:@([CacheSize intValue]) forKey:@"Cache Size"];

	switch ([CdSpeed indexOfSelectedItem]) {
		case 1: [writeDic setObject:@1 forKey:@"Speed"]; break;
		case 2: [writeDic setObject:@2 forKey:@"Speed"]; break;
		case 3: [writeDic setObject:@4 forKey:@"Speed"]; break;
		case 4: [writeDic setObject:@8 forKey:@"Speed"]; break;
		case 5: [writeDic setObject:@16 forKey:@"Speed"]; break;
		case 6: [writeDic setObject:@32 forKey:@"Speed"]; break;
		default: [writeDic setObject:@0 forKey:@"Speed"]; break;
	}

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

	// load from preferences
	RELEASEOBJ(keyValues);
	keyValues = [[defaults dictionaryForKey:PrefsKey] mutableCopy];

	[Cached setIntValue:[[keyValues objectForKey:@"Threaded"] intValue]];
	[CacheSize setIntValue:[[keyValues objectForKey:@"Cache Size"] intValue]];

	switch ([[keyValues objectForKey:@"Speed"] intValue]) {
		case 1: [CdSpeed selectItemAtIndex:1]; break;
		case 2: [CdSpeed selectItemAtIndex:2]; break;
		case 4: [CdSpeed selectItemAtIndex:3]; break;
		case 8: [CdSpeed selectItemAtIndex:4]; break;
		case 16: [CdSpeed selectItemAtIndex:5]; break;
		case 32: [CdSpeed selectItemAtIndex:6]; break;
		default: [CdSpeed selectItemAtIndex:0]; break;
	}
}

- (void)awakeFromNib
{
}

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginConfigController class]);
