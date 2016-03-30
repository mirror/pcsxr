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
@property (readwrite, copy) NSString *title;
@property (readwrite, copy) NSString *name;
@property (readwrite, copy) NSString *identifier;
@property (readwrite) uint8_t startingIndex;
@property (readwrite) uint8_t blockSize;

@property (readwrite, strong) NSArray *imageArray;
@property (readwrite) PCSXRMemFlag flag;
@property (readwrite, nonatomic, strong) NSImage *image;
@property (readwrite) BOOL hasImages;
@end

#pragma pack(push,2)
struct PSXRGBColor {
	UInt8 r;
	UInt8 g;
	UInt8 b;
};
#pragma pack(pop)

@implementation PcsxrMemoryObject

+ (NSArray *)imagesFromMcd:(McdBlock *)block
{
	NSMutableArray *imagesArray = [[NSMutableArray alloc] initWithCapacity:block->IconCount];
	for (int i = 0; i < block->IconCount; i++) {
		NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:16 pixelsHigh:16 bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:16*3 bitsPerPixel:24];
		struct PSXRGBColor *cocoaImageData = (struct PSXRGBColor *)imageRep.bitmapData;
		short *icon = block->Icon;
		
		for (int v = 0; v < 256; v++) {
			int c = icon[(i * 256) + v];
			int r = (c & 0x001f) << 3;
			int g = ((c & 0x03e0) >> 5) << 3;
			int b = ((c & 0x7c00) >> 10) << 3;
			struct PSXRGBColor *colorItem = &cocoaImageData[v];
			colorItem->r = r;
			colorItem->g = g;
			colorItem->b = b;
		}
		NSImage *memImage = [[NSImage alloc] init];
		[memImage addRepresentation:imageRep];
		[memImage setSize:NSMakeSize(32, 32)];
		[imagesArray addObject:memImage];
	}
	return [NSArray arrayWithArray:imagesArray];
}

static NSString *MemLabelDeleted;
static NSString *MemLabelFree;
static NSString *MemLabelUsed;
static NSString *MemLabelLink;
static NSString *MemLabelEndLink;
static NSString *MemLabelMultiSave;

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
		MemLabelMultiSave = [[mainBundle localizedStringForKey:@"MemCard_MultiSave" value:@"" table:nil] copy];
	});
}

- (NSImage*)memoryImageAtIndex:(NSInteger)idx
{
	if (!self.hasImages || idx > self.iconCount) {
		return [PcsxrMemoryObject blankImage];
	}
	return memImages[idx];
}

+ (NSString*)memoryLabelFromFlag:(PCSXRMemFlag)flagNameIndex
{
	switch (flagNameIndex) {
		default:
		case PCSXRMemFlagFree:
			return MemLabelFree;
			break;
			
		case PCSXRMemFlagEndLink:
			return MemLabelEndLink;
			break;
			
		case PCSXRMemFlagLink:
			return MemLabelLink;
			break;
			
		case PCSXRMemFlagUsed:
			return MemLabelUsed;
			break;
			
		case PCSXRMemFlagDeleted:
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

+ (PCSXRMemFlag)memFlagsFromBlockFlags:(unsigned char)blockFlags
{
	if ((blockFlags & 0xF0) == 0xA0) {
		if ((blockFlags & 0xF) >= 1 && (blockFlags & 0xF) <= 3)
			return PCSXRMemFlagDeleted;
		else
			return PCSXRMemFlagFree;
	} else if ((blockFlags & 0xF0) == 0x50) {
		if ((blockFlags & 0xF) == 0x1)
			return PCSXRMemFlagUsed;
		else if ((blockFlags & 0xF) == 0x2)
			return PCSXRMemFlagLink;
		else if ((blockFlags & 0xF) == 0x3)
			return PCSXRMemFlagEndLink;
	} else
		return PCSXRMemFlagFree;
	
	//Xcode complains unless we do this...
	NSLog(@"Unknown flag %x", blockFlags);
	return PCSXRMemFlagFree;
}

- (instancetype)initWithMcdBlock:(McdBlock *)infoBlock startingIndex:(uint8_t)startIdx size:(uint8_t)memSize
{
	if (self = [super init]) {
		self.startingIndex = startIdx;
		self.blockSize = memSize;
		self.flag = [PcsxrMemoryObject memFlagsFromBlockFlags:infoBlock->Flags];
		if (self.flag == PCSXRMemFlagFree) {
			self.imageArray = @[];
			self.hasImages = NO;
			self.title = @"Free block";
			self.identifier = self.name = @"";
		} else {
			self.title = [NSString stringWithCString:infoBlock->sTitle encoding:NSShiftJISStringEncoding];
			self.imageArray = [PcsxrMemoryObject imagesFromMcd:infoBlock];
			
			if ([memImages count] == 0) {
				self.hasImages = NO;
			} else {
				self.hasImages = YES;
			}
			self.name = @(infoBlock->Name);
			self.identifier = @(infoBlock->ID);
		}
	}
	return self;
}

#pragma mark - Property Synthesizers
@synthesize title;
@synthesize name;
@synthesize identifier;
@synthesize imageArray = memImages;
@synthesize flag;
@synthesize blockSize;
@synthesize startingIndex;
@synthesize image = _memImage;

#pragma mark Non-synthesized Properties
- (NSUInteger)iconCount
{
	return [memImages count];
}

- (NSImage*)firstImage
{
	if (self.hasImages == NO) {
		return [PcsxrMemoryObject blankImage];
	}
	return memImages[0];
}

- (NSImage*)image
{
	if (self.hasImages == NO) {
		NSImage *tmpBlank = [PcsxrMemoryObject blankImage];
		tmpBlank.size = NSMakeSize(32, 32);
		return tmpBlank;
	}
	
	if (!_memImage) {
		NSMutableData *gifData = [NSMutableData new];
		
		CGImageDestinationRef dst = CGImageDestinationCreateWithData((__bridge CFMutableDataRef)gifData, kUTTypeGIF, self.iconCount, NULL);
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
	return [PcsxrMemoryObject memoryLabelFromFlag:flag];
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
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelMultiSave];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelEndLink = [tmpStr copy];
		
		// Same as attribMemLabelEndLink on release builds
		attribMemLabelLink = attribMemLabelEndLink;
		
		//display nothing
		attribMemLabelUsed = [[NSAttributedString alloc] initWithString:@""];
#endif
		
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelDeleted];
		SetupAttrStr(tmpStr, [NSColor redColor]);
		attribMemLabelDeleted = [tmpStr copy];
	});
	
	switch (flag) {
		default:
		case PCSXRMemFlagFree:
			return attribMemLabelFree;
			break;
			
		case PCSXRMemFlagEndLink:
			return attribMemLabelEndLink;
			break;
			
		case PCSXRMemFlagLink:
			return attribMemLabelLink;
			break;
			
		case PCSXRMemFlagUsed:
			return attribMemLabelUsed;
			break;
			
		case PCSXRMemFlagDeleted:
			return attribMemLabelDeleted;
			break;
	}
}

- (BOOL)showCount
{
	if (flag == PCSXRMemFlagFree) {
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
	return [NSString stringWithFormat:@"%@: Name: %@ ID: %@, type: %@ start: %i size: %i", title, name, identifier, self.flagName, startingIndex, blockSize];
}

@end
