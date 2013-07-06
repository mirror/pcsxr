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
@class NSAttributedString;

extern NSString *const memoryAnimateTimerKey;

typedef enum _PCSXRMemFlags {
	memFlagDeleted,
	memFlagFree,
	memFlagUsed,
	memFlagLink,
	memFlagEndLink
} PCSXRMemFlags;

@interface PcsxrMemoryObject : NSObject
{
	NSString *englishName;
	NSString *sjisName;
	NSString *memName;
	NSString *memID;
	
	NSArray *memImages;
	NSInteger memImageIndex;
	
	PCSXRMemFlags flagNameIndex;
	unsigned char memFlags;
}
+ (NSArray *)imagesFromMcd:(McdBlock *)block;
+ (NSString*)memoryLabelFromFlag:(PCSXRMemFlags)flagNameIndex;

- (id)initWithMcdBlock:(McdBlock *)infoBlock;

@property (readonly, arcstrong) NSString *englishName;
@property (readonly, arcstrong) NSString *sjisName;
@property (readonly, arcstrong) NSString *memName;
@property (readonly, arcstrong) NSString *memID;
@property (readonly) PCSXRMemFlags flagNameIndex;
@property (readonly) unsigned char memFlags;
@property (readonly, unsafe_unretained, nonatomic) NSString *flagName;
@property (readonly, unsafe_unretained, nonatomic) NSAttributedString *attributedFlagName;
@property (readonly, unsafe_unretained, nonatomic) NSImage *memImage;
@property (readonly, nonatomic) int memIconCount;
@property (readonly, getter = isNotDeleted, nonatomic) BOOL notDeleted DEPRECATED_ATTRIBUTE;

@end
