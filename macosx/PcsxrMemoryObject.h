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

@property(readonly) NSString *englishName;
@property(readonly) NSString *sjisName;
@property(readonly) NSString *memName;
@property(readonly) NSString *memID;
@property(readonly) NSImage *memImage;
@property(readonly) int memIconCount;
@property(readonly, getter = isNotDeleted) BOOL notDeleted;
@property(readonly) unsigned char memFlags;


@end
