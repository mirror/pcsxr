//
//  PcsxrMemoryObject.m
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemoryObject.h"
#import <Foundation/NSString.h>
#import <Foundation/NSArray.h>
#import <AppKit/NSColor.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSAttributedString.h>
#import "ARCBridge.h"

NSString *const memoryAnimateTimerKey = @"PCSXR Memory Card Image Animate";

@interface PcsxrMemoryObject ()
@property (readwrite, arcstrong) NSString *englishName;
@property (readwrite, arcstrong) NSString *sjisName;
@property (readwrite, arcstrong) NSString *memName;
@property (readwrite, arcstrong) NSString *memID;
@property (readwrite) int startingIndex;
@property (readwrite) int blockSize;


@property (readwrite, nonatomic) NSInteger memImageIndex;
@property (arcstrong) NSArray *memImages;
@property (readwrite) PCSXRMemFlags flagNameIndex;
@end

@implementation PcsxrMemoryObject

+ (NSArray *)imagesFromMcd:(McdBlock *)block
{
	NSMutableArray *imagesArray = [[NSMutableArray alloc] initWithCapacity:block->IconCount];
	for (int i = 0; i < block->IconCount; i++) {
		NSImage *memImage = nil;
		{
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
			RELEASEOBJ(imageRep);
			[memImage setSize:NSMakeSize(32, 32)];
		}
		[imagesArray addObject:memImage];
		RELEASEOBJ(memImage);
	}
	NSArray *retArray = [NSArray arrayWithArray:imagesArray];
	RELEASEOBJ(imagesArray);
	return retArray;
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
		MemLabelDeleted = [[mainBundle localizedStringForKey:@"MemCard_Deleted" value:@"" table:((void *)0)] copy];
		MemLabelFree = [[mainBundle localizedStringForKey:@"MemCard_Free" value:@"" table:((void *)0)] copy];
		MemLabelUsed = [[mainBundle localizedStringForKey:@"MemCard_Used" value:@"" table:((void *)0)] copy];
		MemLabelLink = [[mainBundle localizedStringForKey:@"MemCard_Link" value:@"" table:((void *)0)] copy];
		MemLabelEndLink = [[mainBundle localizedStringForKey:@"MemCard_EndLink" value:@"" table:((void *)0)] copy];
	});
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
		NSRect imageRect = NSMakeRect(0, 0, 32, 32);
		imageBlank = [[NSImage alloc] initWithSize:imageRect.size];
		[imageBlank lockFocus];
		[[NSColor blackColor] set];
		[NSBezierPath fillRect:imageRect];
		[imageBlank unlockFocus];
	}
	return imageBlank;
}

- (id)initWithMcdBlock:(McdBlock *)infoBlock startingIndex:(int)startIdx
{
	return [self initWithMcdBlock:infoBlock startingIndex:startIdx size:1];
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
#ifdef DEBUG
	NSLog(@"Unknown flag ");
#endif
	return memFlagFree;
}

- (id)initWithMcdBlock:(McdBlock *)infoBlock startingIndex:(int)startIdx size:(int)memSize
{
	if (self = [super init]) {
		self.startingIndex = startIdx;
		self.blockSize = memSize;
		self.flagNameIndex = [PcsxrMemoryObject memFlagsFromBlockFlags:infoBlock->Flags];
		if (self.flagNameIndex == memFlagFree) {
			self.memImages = @[];
			self.memImageIndex = -1;
			self.englishName = self.sjisName = @"Free block";
			self.memID = self.memName = @"";
		} else {
			self.englishName = @(infoBlock->Title);
			self.sjisName = [NSString stringWithCString:infoBlock->sTitle encoding:NSShiftJISStringEncoding];
			
			if ([englishName isEqualToString:sjisName]) {
#ifdef DEBUG
				NSLog(@"English name and SJIS name are the same: %@. Replacing the sjis string with the English string.", englishName);
#endif
				self.sjisName = self.englishName;
			}
			@autoreleasepool {
				self.memImages = [PcsxrMemoryObject imagesFromMcd:infoBlock];
			}
			if ([memImages count] == 0) {
				self.memImageIndex = -1;
			} else if ([memImages count] == 1) {
				self.memImageIndex = 0;
			} else {
				self.memImageIndex = 0;
				[[NSNotificationCenter defaultCenter] addObserverForName:memoryAnimateTimerKey object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
					NSInteger index = memImageIndex;
					if (++index >= [memImages count]) {
						index = 0;
					}
					self.memImageIndex = index;
				}];
			}
			self.memName = @(infoBlock->Name);
			self.memID = @(infoBlock->ID);
		}
	}
	return self;
}

#pragma mark - Property Synthesizers
@synthesize englishName;
@synthesize sjisName;
@synthesize memImageIndex;
- (void)setMemImageIndex:(NSInteger)theMemImageIndex
{
	[self willChangeValueForKey:@"memImage"];
	memImageIndex = theMemImageIndex;
	[self didChangeValueForKey:@"memImage"];
}

@synthesize memName;
@synthesize memID;
@synthesize memImages;
@synthesize flagNameIndex;
@synthesize blockSize;
@synthesize startingIndex;

#pragma mark Non-synthesize Properties
- (unsigned)memIconCount
{
	return (unsigned)[memImages count];
}

- (NSImage*)memImage
{
	if (memImageIndex == -1) {
		return [PcsxrMemoryObject blankImage];
	}
	return [memImages objectAtIndex:memImageIndex];
}

- (NSString*)flagName
{
	return [PcsxrMemoryObject memoryLabelFromFlag:flagNameIndex];
}

NS_INLINE void SetupAttrStr(NSMutableAttributedString *mutStr, NSColor *txtclr)
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
		RELEASEOBJ(tmpStr);
		
#ifdef DEBUG
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelEndLink];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelEndLink = [tmpStr copy];
		RELEASEOBJ(tmpStr);
		
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelLink];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelLink = [tmpStr copy];
		RELEASEOBJ(tmpStr);
		
		tmpStr = [[NSMutableAttributedString alloc] initWithString:MemLabelUsed];
		SetupAttrStr(tmpStr, [NSColor controlTextColor]);
		attribMemLabelUsed = [tmpStr copy];
		RELEASEOBJ(tmpStr);
#else
		tmpStr = [[NSMutableAttributedString alloc] initWithString:@"Multi-save"];
		SetupAttrStr(tmpStr, [NSColor blueColor]);
		attribMemLabelEndLink = [tmpStr copy];
		RELEASEOBJ(tmpStr);
		
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
		RELEASEOBJ(tmpStr);
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
#if !__has_feature(objc_arc)
	self.englishName = nil;
	self.sjisName = nil;
	self.memName = nil;
	self.memID = nil;
	self.memImages = nil;
	
	[super dealloc];
#endif
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"%@ (%@): Name: %@ ID: %@, type: %@ start: %i size: %i", englishName, sjisName, memName, memID, self.flagName, startingIndex, blockSize];
}

@end
