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

+ (NSArray *)imagesFromMcd:(McdBlock *)block;
+ (NSString*)memoryLabelFromFlag:(PCSXRMemFlags)flagNameIndex;
+ (NSImage *)blankImage;
+ (PCSXRMemFlags)memFlagsFromBlockFlags:(unsigned char)blockFlags;

- (id)initWithMcdBlock:(McdBlock *)infoBlockc startingIndex:(uint8_t)startIdx size:(uint8_t)memSize;

@property (readonly, strong) NSString *englishName;
@property (readonly, strong) NSString *sjisName;
@property (readonly, strong) NSString *memName;
@property (readonly, strong) NSString *memID;
@property (readonly) PCSXRMemFlags flagNameIndex;
@property (readonly) uint8_t startingIndex;
@property (readonly) uint8_t blockSize;

@property (readonly, unsafe_unretained, nonatomic) NSImage *memImage;
@property (readonly, nonatomic) unsigned memIconCount;
@property (readonly, unsafe_unretained, nonatomic) NSString *flagName;
@property (readonly, unsafe_unretained, nonatomic) NSAttributedString *attributedFlagName;
@property (readonly, nonatomic) BOOL isBiggerThanOne;

@end
