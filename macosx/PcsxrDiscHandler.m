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
#import "ARCBridge.h"

@interface PcsxrDiscHandler ()
@property (nonatomic, arcstrong) NSURL *discURL;
@property (arcweak) NSString *discPath;
@end

@implementation PcsxrDiscHandler
@synthesize discURL = _discURL;
- (NSURL*)discURL
{
	if (!_discURL) {
		self.discURL = [NSURL fileURLWithPath:discPath isDirectory:NO];
	}
	return _discURL;
}

@synthesize discPath;

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = @[@"com.alcohol-soft.mdfdisc", @"com.goldenhawk.cdrwin-cuesheet", @"com.apple.disk-image-ndif", @"public.iso-image", @"com.sony.psp.firmware", @"com.codeplex.pcsxr.compressed-bin-image"];
		RETAINOBJNORETURN(utisupport);
	}
	return utisupport;
}

- (BOOL)handleFile:(NSString *)theFile
{
	self.discPath = theFile;
	PcsxrController *appDelegate = [NSApp delegate];
	if ([EmuThread active] == YES && !UsingIso()) {
		return NO;
	}
	[appDelegate runURL:[self discURL]];
	[[appDelegate recentItems] addRecentItem:[self discURL]];
	return YES;
}

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.discURL = nil;
	
	[super dealloc];
}
#endif

@end
