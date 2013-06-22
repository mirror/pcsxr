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

@property(readonly, retain) NSString *englishName;
@property(readonly, retain) NSString *sjisName;
@property(readonly, retain) NSString *memName;
@property(readonly, retain) NSString *memID;
@property(readonly, retain) NSImage *memImage;
@property(readonly) int memIconCount;
@property(readonly, getter = isNotDeleted) BOOL notDeleted;
@property(readonly) unsigned char memFlags;


@end
