//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/NSObject.h>
#include "sio.h"

@class NSImage;
@class NSString;
@class NSArray;

extern NSString *const memoryAnimateTimerKey;

@interface PcsxrMemoryObject : NSObject
{
	NSString *englishName;
	NSString *sjisName;
	NSString *memName;
	NSString *memID;
	NSImage *memImage;
	NSArray *memImages;
	int memIconCount;
	BOOL notDeleted;
	unsigned char memFlags;
}
+ (NSImage *)imageFromMcd:(McdBlock *)block index:(int)idx;
+ (NSArray *)imagesFromMcd:(McdBlock *)block;

- (id)initWithMcdBlock:(McdBlock *)infoBlock;

@property(readonly, retain) NSString *englishName;
@property(readonly, retain) NSString *sjisName;
@property(readonly, retain) NSString *memName;
@property(readonly, retain) NSString *memID;
@property(readwrite, retain) NSImage *memImage;
@property(readonly) int memIconCount;
@property(readonly, getter = isNotDeleted) BOOL notDeleted;
@property(readonly) unsigned char memFlags;


@end
