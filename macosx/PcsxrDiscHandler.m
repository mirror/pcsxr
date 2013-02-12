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
#import "RecentItemsMenu.h"
#import "PcsxrController.h"

@implementation PcsxrDiscHandler

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
			SetCdOpenCaseTime(time(NULL) + 2);
			SetIsoFile([theFile fileSystemRepresentation]);
			//[EmuThread reset];
		} else {
			return NO;
		}
	} else {
		SetIsoFile([theFile fileSystemRepresentation]);
		[EmuThread run];
	}
	[[appDelegate recentItems] addRecentItem:[NSURL fileURLWithPath:theFile]];
	return YES;
}

@end
