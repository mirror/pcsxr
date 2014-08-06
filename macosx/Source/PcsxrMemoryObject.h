//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "sio.h"

extern NSString *const memoryAnimateTimerKey;

typedef NS_ENUM(char, PCSXRMemFlags) {
	memFlagDeleted,
	memFlagFree,
	memFlagUsed,
	memFlagLink,
	memFlagEndLink
};

@interface PcsxrMemoryObject : NSObject

+ (NSArray *)imagesFromMcd:(McdBlock *)block;
+ (NSString*)memoryLabelFromFlag:(PCSXRMemFlags)flagNameIndex;
+ (NSImage *)blankImage;
+ (PCSXRMemFlags)memFlagsFromBlockFlags:(unsigned char)blockFlags;

- (instancetype)initWithMcdBlock:(McdBlock *)infoBlockc startingIndex:(uint8_t)startIdx size:(uint8_t)memSize NS_DESIGNATED_INITIALIZER;

- (NSImage*)memoryImageAtIndex:(NSInteger)idx;

@property (readonly, strong) NSString *englishName;
@property (readonly, strong) NSString *sjisName;
@property (readonly, strong) NSString *memName;
@property (readonly, strong) NSString *memID;
@property (readonly) PCSXRMemFlags flagNameIndex;
@property (readonly) uint8_t startingIndex;
@property (readonly) uint8_t blockSize;

@property (readonly, copy) NSImage *firstMemImage;
@property (readonly, unsafe_unretained, nonatomic) NSImage *memImage;
@property (readonly, nonatomic) unsigned memIconCount;
@property (readonly, unsafe_unretained, nonatomic) NSString *flagName;
@property (readonly, unsafe_unretained, nonatomic) NSAttributedString *attributedFlagName;
@property (readonly, nonatomic) BOOL isBiggerThanOne;

@end
