//
//  PcsxrFreezeStateHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrFreezeStateHandler.h"
#import "EmuThread.h"
#import "ARCBridge.h"
#include "misc.h"

@implementation PcsxrFreezeStateHandler

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = RETAINOBJ([NSArray arrayWithObject:@"com.codeplex.pcsxr.freeze"]);
	}
	return utisupport;
}

- (BOOL)handleFile:(NSString *)theFile
{
	if (CheckState([theFile fileSystemRepresentation]) != 0) {
		return NO;
	}
	if (![EmuThread active]) {
		[EmuThread run];
	}
	return [EmuThread defrostAt:theFile];
}

@end
