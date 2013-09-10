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
		
	} else {
		credits = [[NSAttributedString alloc] initWithString:@""];
	}

	// Get Application Icon
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[bundle bundlePath]];
	NSSize size = NSMakeSize(64, 64);
	[icon setSize:size];

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
	[defaults registerDefaults:@{PrefsKey: @{@"Threaded": @YES,
			@"Cache Size": @64,
			@"Speed": @0}}];

	keyValues = [defaults dictionaryForKey:PrefsKey];

	ReadMode = ([keyValues[@"Threaded"] boolValue] ? THREADED : NORMAL);
	CacheSize = [keyValues[@"Cache Size"] intValue];
	CdrSpeed = [keyValues[@"Speed"] integerValue];
}

@implementation PluginConfigController
@synthesize keyValues;

- (IBAction)cancel:(id)sender
{
	self.keyValues = nil;
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	NSMutableDictionary *writeDic = [keyValues mutableCopy];

	writeDic[@"Threaded"] = ([Cached intValue] ? @YES : @NO);
	writeDic[@"Cache Size"] = @([CacheSize integerValue]);

	switch ([CdSpeed indexOfSelectedItem]) {
		case 1: writeDic[@"Speed"] = @1; break;
		case 2: writeDic[@"Speed"] = @2; break;
		case 3: writeDic[@"Speed"] = @4; break;
		case 4: writeDic[@"Speed"] = @8; break;
		case 5: writeDic[@"Speed"] = @16; break;
		case 6: writeDic[@"Speed"] = @32; break;
		default: writeDic[@"Speed"] = @0; break;
	}

	// write to defaults
	[defaults setObject:writeDic forKey:PrefsKey];
	[defaults synchronize];

	// and set global values accordingly
	ReadConfig();
	
	self.keyValues = nil;
	[self close];
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	ReadConfig();

	// load from preferences
	self.keyValues = [NSMutableDictionary dictionaryWithDictionary:[defaults dictionaryForKey:PrefsKey]];

	[Cached setIntValue:[keyValues[@"Threaded"] intValue]];
	[CacheSize setIntegerValue:[keyValues[@"Cache Size"] integerValue]];

	switch ([keyValues[@"Speed"] intValue]) {
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
