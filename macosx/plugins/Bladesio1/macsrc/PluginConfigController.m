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
#include "typedefs.h"
#include "sio1.h"

#define APP_ID @"net.pcsxr.Bladesio1"
#define PrefsKey APP_ID @" Settings"

static PluginConfigController *windowController = nil;

#define kSioEnabled @"SIO Enabled"
#define kSioPort @"Port"
#define kSioPlayer @"Player"
#define kSioIPAddress @"IP address"

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

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

void ConfDlgProc()
{
	//We need this block due to the xib's use of auto layout
	RunOnMainThreadSync(^{
		NSWindow *window;
		if (windowController == nil) {
			windowController = [[PluginConfigController alloc] initWithWindowNibName:@"Bladesio1PluginConfig"];
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
	[defaults registerDefaults:@{PrefsKey: @{kSioEnabled: @NO,
								 kSioPort: @33307,
								 kSioIPAddress: @"127.0.0.1",
								 kSioPlayer: @(PLAYER_DISABLED)}}];

	keyValues = [defaults dictionaryForKey:PrefsKey];

	settings.enabled = [keyValues[kSioEnabled] boolValue];
	settings.port = [keyValues[kSioPort] unsignedShortValue];
	settings.player = [keyValues[kSioPlayer] intValue];
	strlcpy(settings.ip, [keyValues[kSioIPAddress] cStringUsingEncoding:NSASCIIStringEncoding], sizeof(settings.ip));
}

@implementation Bladesio1PluginConfigController

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];

	NSString *theAddress = [ipAddressField stringValue];
	NSInteger asciiLen = [theAddress lengthOfBytesUsingEncoding:NSASCIIStringEncoding];
	if (asciiLen > (sizeof(settings.ip) - 1)) {
		NSBeginAlertSheet(@"Address too long", nil, nil, nil, [self window], nil, NULL, NULL, NULL, @"The address is too long.\n\nTry to use only the IP address and not a host name.");
		return;
	} else if (asciiLen == 0) {
		NSBeginAlertSheet(@"Blank address", nil, nil, nil, [self window], nil, NULL, NULL, NULL, @"The address specified is either blank, or can't be converted to ASCII.\n\nTry connecting directly using the IP address using latin numerals.");
		return;
	}
	
	writeDic[kSioEnabled] = (([enabledButton state]  == NSOnState) ? @YES : @NO);
	writeDic[kSioIPAddress] = theAddress;
	writeDic[kSioPort] = @((u16)[portField intValue]);
	
	{
		int player;
		switch ([playerMenu indexOfSelectedItem]) {
			default:
			case 0: player = PLAYER_DISABLED; break;
			case 1: player = PLAYER_MASTER; break;
			case 2: player = PLAYER_SLAVE; break;
		}
		writeDic[kSioPlayer] = @(player);
	}

	// write to defaults
	[defaults setObject:writeDic forKey:PrefsKey];
	[defaults synchronize];

	// and set global values accordingly
	ReadConfig();

	[self close];
}

- (IBAction)toggleEnabled:(id)sender
{
	BOOL isEnabled = [enabledButton state] == NSOnState ? YES : NO;
	
	for (NSView *subView in [[configBox subviews][0] subviews]) {
		if ([subView isKindOfClass:[NSTextField class]] && ![(NSTextField*)subView isEditable]) {
				[(NSTextField*)subView setTextColor:isEnabled ? [NSColor controlTextColor] : [NSColor disabledControlTextColor]];
		} else {
			if ([subView respondsToSelector:@selector(setEnabled:)]) {
				[(NSControl*)subView setEnabled:isEnabled];
			}
		}
	}
}

- (IBAction)resetPreferences:(id)sender
{
	[[NSUserDefaults standardUserDefaults] removeObjectForKey:PrefsKey];
	[self loadValues];
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	ReadConfig();

	// load from preferences
	keyValues = [[defaults dictionaryForKey:PrefsKey] mutableCopy];

	[enabledButton setState: [keyValues[kSioEnabled] boolValue] ? NSOnState : NSOffState];
	[ipAddressField setTitleWithMnemonic:keyValues[kSioIPAddress]];
	[portField setIntValue:[keyValues[kSioPort] intValue]];
	
	switch ([keyValues[kSioPlayer] integerValue]) {
		default:
		case PLAYER_DISABLED: [playerMenu selectItemAtIndex:0]; break;
		case PLAYER_MASTER: [playerMenu selectItemAtIndex:1]; break;
		case PLAYER_SLAVE: [playerMenu selectItemAtIndex:2]; break;
	}
	
	[self toggleEnabled:nil];
}

- (void)awakeFromNib
{
}

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginConfigController class]);
