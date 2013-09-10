//
//  PcsxrFreezeStateHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrFreezeStateHandler.h"
#import "EmuThread.h"
#import "PluginList.h"
#include "misc.h"

@implementation PcsxrFreezeStateHandler

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = @[@"com.codeplex.pcsxr.freeze"];
	}
	return utisupport;
}

- (BOOL)handleFile:(NSString *)theFile
{
	if (CheckState([theFile fileSystemRepresentation]) != 0) {
		return NO;
	}
	if (![EmuThread active]) {
		PluginList *pluginList = [PluginList list];
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"NetPlay"]) {
			[pluginList enableNetPlug];
		} else {
			[pluginList disableNetPlug];
		}

		[EmuThread run];
	}
	return [EmuThread defrostAt:theFile];
}

@end
