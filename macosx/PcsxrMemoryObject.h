//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/NSObject.h>
#include "sio.h"
#import "ARCBridge.h"

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
	NSInteger memImageIndex;
	NSArray *memImages;
	BOOL notDeleted;
	unsigned char memFlags;
}
+ (NSArray *)imagesFromMcd:(McdBlock *)block;

- (id)initWithMcdBlock:(McdBlock *)infoBlock;

@property (readonly, arcstrong) NSString *englishName;
@property (readonly, arcstrong) NSString *sjisName;
@property (readonly, arcstrong) NSString *memName;
@property (readonly, arcstrong) NSString *memID;
@property (readonly, unsafe_unretained, nonatomic) NSImage *memImage;
@property (readonly) int memIconCount;
@property (readonly, getter = isNotDeleted) BOOL notDeleted;
@property (readonly) unsigned char memFlags;

@end
