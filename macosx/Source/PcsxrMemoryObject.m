//
//  PcsxrMemoryObject.m
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PcsxrMemoryObject.h"

@interface PcsxrMemoryObject ()
@property (readwrite, copy) NSString *name;
@property (readwrite, copy) NSString *memName;
@property (readwrite, copy) NSString *memID;
@property (readwrite) uint8_t startingIndex;
@property (readwrite) uint8_t blockSize;

@property (readwrite, strong) NSArray *memoryCardImages;
@property (readwrite) PCSXRMemFlags flagNameIndex;
@property (readwrite, nonatomic, strong) NSImage *memImage;
@property (readwrite) BOOL hasImages;
@end

@implementation PcsxrMemoryObject

+ (NSArray *)imagesFromMcd:(McdBlock *)block
{
	NSMutableArray *imagesArray = [[NSMutableArray alloc] initWithCapacity:block->IconCount];
	for (int i = 0; i < block->IconCount; i++) {
		NSImage *memImage;
		@autoreleasepool {
			NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:16 pixelsHigh:16 bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
			
			short *icon = block->Icon;
			
			int x, y, c, v, r, g, b;
			for (v = 0; v < 256; v++) {
				x = (v % 16);
				y = (v / 16);
				c = icon[(i * 256) + v];
				r = (c & 0x001f) << 3;
				g = ((c & 0x03e0) >> 5) << 3;
				b = ((c & 0x7c00) >> 10) << 3;
				[imageRep setColor:[NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0] atX:x y:y];
			}
			memImage = [[NSImage alloc] init];
			[memImage addRepresentation:imageRep];
			[memImage setSize:NSMakeSize(32, 32)];
		}
		[imagesArray addObject:memImage];
	}
	return [NSArray arrayWithArray:imagesArray];
}

static NSString *MemLabelDeleted;
static NSString *MemLabelFree;
static NSString *MemLabelUsed;
static NSString *MemLabelLink;
static NSString *MemLabelEndLink;

+ (void)initialize
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSBundle *mainBundle = [NSBundle mainBundle];
		MemLabelDeleted = [[mainBundle localizedStringForKey:@"MemCard_Deleted" value:@"" table:nil] copy];
		MemLabelFree = [[mainBundle localizedStringForKey:@"MemCard_Free" value:@"" table:nil] copy];
		MemLabelUsed = [[mainBundle localizedStringForKey:@"MemCard_Used" value:@"" table:nil] copy];
		MemLabelLink = [[mainBundle localizedStringForKey:@"MemCard_Link" value:@"" table:nil] copy];
		MemLabelEndLink = [[mainBundle localizedStringForKey:@"MemCard_EndLink" value:@"" table:nil] copy];
	});
}

- (NSImage*)memoryImageAtIndex:(NSInteger)idx
{
	if (!self.hasImages || idx > self.memIconCount) {
		return [PcsxrMemoryObject blankImage];
	}
	return memImages[idx];
}

+ (NSString*)memoryLabelFromFlag:(PCSXRMemFlags)flagNameIndex
{
	switch (flagNameIndex) {
		default:
		case memFlagFree:
			return MemLabelFree;
			break;
			
		case memFlagEndLink:
			return MemLabelEndLink;
			break;
			
		case memFlagLink:
			return MemLabelLink;
			break;
			
		case memFlagUsed:
			return MemLabelUsed;
			break;
			
		case memFlagDeleted:
			return MemLabelDeleted;
			break;
	}
}

+ (NSImage *)blankImage
{
	static NSImage *imageBlank = nil;
	if (imageBlank == nil) {
		NSRect imageRect = NSMakeRect(0, 0, 16, 16);
		imageBlank = [[NSImage alloc] initWithSize:imageRect.size];
		[imageBlank lockFocus];
		[[NSColor blackColor] set];
		[NSBezierPath fillRect:imageRect];
		[imageBlank unlockFocus];
	}
	return [imageBlank copy];
}

+ (PCSXRMemFlags)memFlagsFromBlockFlags:(unsigned char)blockFlags
{
	if ((blockFlags & 0xF0) == 0xA0) {
		if ((blockFlags & 0xF) >= 1 && (blockFlags & 0xF) <= 3)
			return memFlagDeleted;
		else
			return memFlagFree;
	} else if ((blockFlags & 0xF0) == 0x50) {
		if ((blockFlags & 0xF) == 0x1)
			return memFlagUsed;
		else if ((blockFlags & 0xF) == 0x2)
			return memFlagLink;
		else if ((blockFlags & 0xF) == 0x3)
			return memFlagEndLink;
	} else
		return memFlagFree;
	
	//Xcode complains unless we do this...
	NSLog(@"Unknown flag %x", blockFlags);
	return memFlagFree;
}

- (instancetype)initWithMcdBlock:(McdBlock *)infoBlock startingIndex:(uint8_t)startIdx size:(uint8_t)memSize
{
	if (self = [super init]) {
		self.startingIndex = startIdx;
		self.blockSize = memSize;
		self.flagNameIndex = [PcsxrMemoryObject memFlagsFromBlockFlags:infoBlock->Flags];
		if (self.flagNameIndex == memFlagFree) {
			self.memoryCardImages = @[];
			self.hasImages = NO;
			self.name = @"Free block";
			self.memID = self.memName = @"";
		} else {
			self.name = [NSString stringWithCString:infoBlock->sTitle encoding:NSShiftJISStringEncoding];
			self.memoryCardImages = [PcsxrMemoryObject imagesFromMcd:infoBlock];
			
			if ([memImages count] == 0) {
				self.hasImages = NO;
			} else {
				self.hasImages = YES;
			}
			self.memName = @(infoBlock->Name);
			self.memID = @(infoBlock->ID);
		}
	}
	return self;
}

#pragma mark - Property Synthesizers
@synthesize name;
@synthesize memName;
@synthesize memID;
@synthesize memoryCardImages = memImages;
@synthesize flagNameIndex;
@synthesize blockSize;
@synthesize startingIndex;
@synthesize memImage = _memImage;

#pragma mark Non-synthesized Properties
- (NSUInteger)memIconCount
{
	return [memImages count];
}

- (NSImage*)firstMemImage
{
	if (self.hasImages == NO) {
		return [PcsxrMemoryObject blankImage];
	}
	return memImages[0];
}

- (NSImage*)memImage
{
	if (self.hasImages == NO) {
		NSImage *tmpBlank = [PcsxrMemoryObject blankImage];
		tmpBlank.size = NSMakeSize(32, 32);
		return tmpBlank;
	}
	
	if (!_memImage) {
		NSMutableData *gifData = [NSMutableData new];
		
		CGImageDestinationRef dst = CGImageDestinationCreateWithData((__bridge CFMutableDataRef)gifData, kUTTypeGIF, self.memIconCount, NULL);
		NSDictionary *gifPrep = @{(NSString *) kCGImagePropertyGIFDictionary: @{(NSString *) kCGImagePropertyGIFDelayTime: @0.30f}};
		for (NSImage *theImage in memImages) {
			CGImageRef imageRef = [theImage CGImageForProposedRect:NULL context:nil hints:nil];
			CGImageDestinationAddImage(dst, imageRef,(__bridge CFDictionaryRef)(gifPrep));
		}
		CGImageDestinationFinalize(dst);
		CFRelease(dst);
		
		_memImage = [[NSImage alloc] initWithData:gifData];
		_memImage.size = NSMakeSize(32, 32);
	}
	return _memImage;
}

- (NSString*)flagName
{
	return [PcsxrMemoryObject memoryLabelFromFlag:flagNameIndex];
}

static inline void SetupAttrStr(NSMutableAttributedString *mutStr, NSColor *txtclr)
{
	NSRange wholeStrRange = NSMakeRange(0, mutStr.string.length);
	[mutStr addAttribute:NSFontAttributeName value:[NSFont userFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]] range:wholeStrRange];
	[mutStr addAttribute:NSForegroundColorAttributeName value:txtclr range:wholeStrRange];
	[mutStr setAlignment:NSCenterTextAlignment range:wholeStrRange];
}

- (NSAttributedString*)attributedFlagName
{
	static NSAttributedString *attribMemLabelDeleted;
	static NSAttributedString *attribMemLabelFree;
	static NSAttributedString *attribMemLabelUsed;
	static NSAttributedString *attribMemLabelLink;
	static NSAttributedString *attribMemLabelEndLink;
	
	static dispatch_once_t attrStrSetOnceToken;
	dispatch_once(&attrStrSetOnceToken, ^{
		NSMutableAttributedString *tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelFree];
		SetupAttrStr(tmpStr, [NSColor greenColor]);
		attribMemLabelFree = [tmpStr copy];
		
#ifdef DEBUG
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelEndLink];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelEndLink = [tmpStr copy];
		
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelLink];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelLink = [tmpStr copy];
		
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelUsed];
		SetupAttrStr(tmpStr, [NSColor controlTextColor]);
		attribMemLabelUsed = [tmpStr copy];
#else
		tmpStr = [[NSMutableAttributedString alloc] initWithString:@"Multi-save"];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelEndLink = [tmpStr copy];
		
		//tmpStr = [[NSMutableAttributedString alloc] initWithString:@"Multi-save"];
		//SetupAttrStr(tmpStr, [NSColor blueColor]);
		//attribMemLabelLink = [tmpStr copy];
		//RELEASEOBJ(tmpStr);
		attribMemLabelLink = attribMemLabelEndLink;
		
		//display nothing
		attribMemLabelUsed = [[NSAttributedString alloc] initWithString:@""];
#endif
		
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelDeleted];
		SetupAttrStr(tmpStr, [NSColor redColor]);
		attribMemLabelDeleted = [tmpStr copy];
	});
	
	switch (flagNameIndex) {
		default:
		case memFlagFree:
			return attribMemLabelFree;
			break;
			
		case memFlagEndLink:
			return attribMemLabelEndLink;
			break;
			
		case memFlagLink:
			return attribMemLabelLink;
			break;
			
		case memFlagUsed:
			return attribMemLabelUsed;
			break;
			
		case memFlagDeleted:
			return attribMemLabelDeleted;
			break;
	}
}

- (BOOL)isBiggerThanOne
{
	if (flagNameIndex == memFlagFree) {
		//Always show the size of the free blocks
		return YES;
	} else {
		return blockSize != 1;
	}
}

#pragma mark -

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"%@: Name: %@ ID: %@, type: %@ start: %i size: %i", name, memName, memID, self.flagName, startingIndex, blockSize];
}

@end
