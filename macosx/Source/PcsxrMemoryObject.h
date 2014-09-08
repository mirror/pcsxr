//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "sio.h"

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

@property (readonly, strong) NSString *name;
@property (readonly, strong) NSString *memName;
@property (readonly, strong) NSString *memID;
@property (readonly, strong) NSArray *memoryCardImages;
@property (readonly, strong, nonatomic) NSImage *memImage;
@property (readonly) PCSXRMemFlags flagNameIndex;
@property (readonly) uint8_t startingIndex;
@property (readonly) uint8_t blockSize;
@property (readonly) BOOL hasImages;

@property (readonly, copy) NSImage *firstMemImage;
@property (readonly, nonatomic) NSUInteger memIconCount;
@property (readonly, unsafe_unretained, nonatomic) NSString *flagName;
@property (readonly, unsafe_unretained, nonatomic) NSAttributedString *attributedFlagName;
@property (readonly, nonatomic) BOOL isBiggerThanOne;

@end
