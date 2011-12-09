//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/NSString.h>
#import <AppKit/NSImage.h>
#include "sio.h"

@interface PcsxrMemoryObject : NSObject
{
	NSString *englishName;
	NSString *sjisName;
	NSString *memName;
	NSString *memID;
	NSImage *memImage;
	int memIconCount;
	BOOL notDeleted;
	unsigned char memFlags;
}
+ (NSImage *)imageFromMcd:(short *)icon;

- (id)initWithMcdBlock:(McdBlock *)infoBlock;

@property(copy, readwrite) NSString *englishName;
@property(copy, readwrite) NSString *sjisName;
@property(copy, readwrite) NSString *memName;
@property(copy, readwrite) NSString *memID;
@property(retain, readwrite) NSImage *memImage;
@property(readwrite) int memIconCount;
@property(readwrite, getter = isNotDeleted) BOOL notDeleted;
@property(readwrite) unsigned char memFlags;


@end
