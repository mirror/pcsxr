//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "sio.h"

typedef NS_ENUM(char, PCSXRMemFlag) {
	memFlagDeleted,
	memFlagFree,
	memFlagUsed,
	memFlagLink,
	memFlagEndLink
};

@interface PcsxrMemoryObject : NSObject

+ (NSArray *)imagesFromMcd:(McdBlock *)block;
+ (NSString*)memoryLabelFromFlag:(PCSXRMemFlag)flagNameIndex;
+ (NSImage *)blankImage;
+ (PCSXRMemFlag)memFlagsFromBlockFlags:(unsigned char)blockFlags;

- (instancetype)initWithMcdBlock:(McdBlock *)infoBlockc startingIndex:(uint8_t)startIdx size:(uint8_t)memSize NS_DESIGNATED_INITIALIZER;

- (NSImage*)memoryImageAtIndex:(NSInteger)idx;

@property (readonly, copy) NSString *title;
@property (readonly, copy) NSString *name;
@property (readonly, copy) NSString *identifier;
@property (readonly, strong) NSArray *imageArray;
@property (readonly, strong, nonatomic) NSImage *image;
@property (readonly) PCSXRMemFlag flag;
@property (readonly) uint8_t startingIndex;
@property (readonly) uint8_t blockSize;
@property (readonly) BOOL hasImages;

@property (readonly, copy) NSImage *firstImage;
@property (readonly, nonatomic) NSUInteger iconCount;
@property (readonly, unsafe_unretained, nonatomic) NSString *flagName;
@property (readonly, unsafe_unretained, nonatomic) NSAttributedString *attributedFlagName;
@property (readonly, nonatomic) BOOL showCount;

@end
