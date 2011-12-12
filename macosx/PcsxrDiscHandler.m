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

@implementation PcsxrDiscHandler

- (BOOL)handleFile:(NSString *)theFile
{
	SetIsoFile([theFile fileSystemRepresentation]);
	[EmuThread run];
	return YES;
}

@end
