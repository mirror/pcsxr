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
#import "ARCBridge.h"

NSString *const memoryAnimateTimerKey = @"PCSXR Memory Card Image Animate";

@interface PcsxrMemoryObject ()
//Mangle the setters' names so that if someone tries to use them, they won't work
@property (readwrite, retain, setter = setEngName:) NSString *englishName;
@property (readwrite, retain, setter = setJapaneseName:) NSString *sjisName;
@property (readwrite, retain, setter = setTheMemName:) NSString *memName;
@property (readwrite, retain, setter = setTheMemId:) NSString *memID;
@property (readwrite, retain, setter = setTheMemImage:) NSImage *memImage;
@property (readwrite, getter = isNotDeleted, setter = setIsNotDeleted:) BOOL notDeleted;
@property (readwrite, setter = setTheMemFlags:) unsigned char memFlags;
@property (retain) NSArray *memImages;
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

- (id)initWithMcdBlock:(McdBlock *)infoBlock
{
	if (self = [super init]) {
		self.englishName = [NSString stringWithCString:infoBlock->Title encoding:NSASCIIStringEncoding];
		self.sjisName = [NSString stringWithCString:infoBlock->sTitle encoding:NSShiftJISStringEncoding];
		self.memImages = [PcsxrMemoryObject imagesFromMcd:infoBlock];
		if ([memImages count] == 0) {
			self.memImage = [PcsxrMemoryObject blankImage];
		} else if ([memImages count] == 1) {
			self.memImage = [memImages objectAtIndex:0];
		} else {
			self.memImage = [self.memImages objectAtIndex:0];
			[[NSNotificationCenter defaultCenter] addObserverForName:memoryAnimateTimerKey object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
				NSInteger index = [memImages indexOfObject:memImage];
				if (++index >= [memImages count]) {
					index = 0;
				}
				self.memImage = [memImages objectAtIndex:index];
			}];
		}
		self.memName = [NSString stringWithCString:infoBlock->Name encoding:NSASCIIStringEncoding];
		self.memID = [NSString stringWithCString:infoBlock->ID encoding:NSASCIIStringEncoding];
		self.memFlags = infoBlock->Flags;
		if ((infoBlock->Flags & 0xF0) == 0xA0) {
			if ((infoBlock->Flags & 0xF) >= 1 &&
				(infoBlock->Flags & 0xF) <= 3) {
				self.notDeleted = NO;
			} else
				self.notDeleted = NO;
		} else if ((infoBlock->Flags & 0xF0) == 0x50)
			self.notDeleted = YES;
		else
			self.notDeleted = NO;
	}
	return self;
}

@synthesize englishName;
@synthesize sjisName;
@synthesize memImage;
- (void)setTheMemImage:(NSImage *)theMemImage
{
	if (memImage == theMemImage) {
		return;
	}
	//This is the only setter that is being watched AND changed.
	[self willChangeValueForKey:@"memImage"];
#if __has_feature(objc_arc)
	memImage = theMemImage;
#else
	NSImage *tmpImage = memImage;
	memImage = [theMemImage retain];
	[tmpImage release];
#endif
	[self didChangeValueForKey:@"memImage"];
}
- (NSImage*)memImage
{
	@synchronized(memImage)
	{
		return memImage;
	}
}

@synthesize notDeleted;
@synthesize memFlags;
@synthesize memName;
@synthesize memID;
- (int)memIconCount
{
	return [memImages count];
}
@synthesize memImages;

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
#if !__has_feature(objc_arc)
	self.englishName = nil;
	self.sjisName = nil;
	self.memName = nil;
	self.memID = nil;
	self.memImage = nil;
	self.memImages = nil;
	
	[super dealloc];
#endif
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"%@ (%@): Name: %@ ID: %@ Flags: %d", englishName, sjisName, memName, memID, memFlags];
}

@end
