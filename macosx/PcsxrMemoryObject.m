//
//  PcsxrMemoryObject.m
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemoryObject.h"

@implementation PcsxrMemoryObject

@synthesize englishName;
@synthesize sjisName;
@synthesize memImage;
@synthesize deleted;
@synthesize memNumber;
@synthesize memFlags;

- (void)dealloc
{
	[englishName release];
	[sjisName release];
	[memImage release];
	
	[super dealloc];
}

@end
