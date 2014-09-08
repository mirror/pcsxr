//
//  PcsxrDiscHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrDiscHandler.h"
#import "EmuThread.h"
#include "psxcommon.h"
#include "plugins.h"
#include "cdrom.h"
#import "RecentItemsMenu.h"
#import "PcsxrController.h"

@interface PcsxrDiscHandler ()
@property (nonatomic, strong) NSURL *discURL;
@property (weak) NSString *discPath;
@end

@implementation PcsxrDiscHandler
@synthesize discURL = _discURL;
- (NSURL*)discURL
{
	if (!_discURL) {
		self.discURL = [NSURL fileURLWithPath:discPath];
	}
	return _discURL;
}

@synthesize discPath;

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = @[@"com.alcohol-soft.mdfdisc", @"com.goldenhawk.cdrwin-cuesheet",
					   @"com.apple.disk-image-ndif", @"public.iso-image", @"com.sony.psp.firmware",
					   @"com.codeplex.pcsxr.compressed-bin-image", @"com.coppertino.vox.cue",
					   @"com.apple.macbinary-​archive"];
	}
	return utisupport;
}

- (BOOL)handleFile:(NSString *)theFile
{
	self.discPath = theFile;
	PcsxrController *appDelegate = [(NSApplication*)NSApp delegate];
	if ([EmuThread active] == YES && !UsingIso()) {
		return NO;
	}
	[appDelegate runURL:[self discURL]];
	[[appDelegate recentItems] addRecentItem:[self discURL]];
	return YES;
}

@end
