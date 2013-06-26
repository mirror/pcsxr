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
@property (retain) NSURL *discURL;
@end

@implementation PcsxrDiscHandler

@synthesize discURL;

- (NSURL*)discURLFromFilePath:(NSString *)filePath
{
	if (!discURL) {
		self.discURL = [NSURL fileURLWithPath:filePath isDirectory:NO];
	}
	return self.discURL;
}

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = [[NSArray alloc] initWithObjects:@"com.alcohol-soft.mdfdisc", @"com.goldenhawk.cdrwin-cuesheet", @"com.apple.disk-image-ndif", @"public.iso-image", @"com.sony.psp.firmware", @"com.codeplex.pcsxr.compressed-bin-image", nil];
	}
	return utisupport;
}

- (BOOL)handleFile:(NSString *)theFile
{
	PcsxrController *appDelegate = [NSApp delegate];
	if ([EmuThread active] == YES) {
		if (UsingIso()) {
			SetIsoFile([theFile fileSystemRepresentation]);
			SetCdOpenCaseTime(time(NULL) + 2);
			LidInterrupt();
			//[EmuThread reset];
		} else {
			return NO;
		}
	} else {
		[appDelegate runURL:[self discURLFromFilePath:theFile]];
	}
	[[appDelegate recentItems] addRecentItem:[self discURLFromFilePath:theFile]];
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
