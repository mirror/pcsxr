//
//  PcsxrMemoryObject.m
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemoryObject.h"
#import <Foundation/NSString.h>
#import <AppKit/NSColor.h>
#import <AppKit/NSImage.h>
#import "ARCBridge.h"

@interface PcsxrMemoryObject ()
//Mangle the setters' names so that if someone tries to use them, they won't work
@property(readwrite, retain, setter = setEngName:) NSString *englishName;
@property(readwrite, retain, setter = setJapaneseName:) NSString *sjisName;
@property(readwrite, retain, setter = setTheMemName:) NSString *memName;
@property(readwrite, retain, setter = setTheMemId:) NSString *memID;
@property(readwrite, retain, setter = setTheMemImage:) NSImage *memImage;
@property(readwrite, setter = setIconCount:) int memIconCount;
@property(readwrite, getter = isNotDeleted, setter = setIsNotDeleted:) BOOL notDeleted;
@property(readwrite, setter = setTheMemFlags:) unsigned char memFlags;
@end

@implementation PcsxrMemoryObject

+ (NSImage *)imageFromMcd:(short *)icon
{
	NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:16 pixelsHigh:16 bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
	
	int x, y, c, i, r, g, b;
	for (i = 0; i < 256; i++) {
		x = (i % 16);
		y = (i / 16);
		c = icon[i];
		r = (c & 0x001f) << 3;
		g = ((c & 0x03e0) >> 5) << 3;
		b = ((c & 0x7c00) >> 10) << 3;
		[imageRep setColor:[NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0] atX:x y:y];
	}
	NSImage *theImage = [[NSImage alloc] init];
	[theImage addRepresentation:imageRep];
	RELEASEOBJ(imageRep);
	[theImage setSize:NSMakeSize(32, 32)];
	return AUTORELEASEOBJ(theImage);
}

- (id)initWithMcdBlock:(McdBlock *)infoBlock
{
	if (self = [super init]) {
		self.englishName = [NSString stringWithCString:infoBlock->Title encoding:NSASCIIStringEncoding];
		self.sjisName = [NSString stringWithCString:infoBlock->sTitle encoding:NSShiftJISStringEncoding];
		@autoreleasepool {
			self.memImage = [PcsxrMemoryObject imageFromMcd:infoBlock->Icon];
		}
		self.memName = [NSString stringWithCString:infoBlock->Name encoding:NSASCIIStringEncoding];
		self.memID = [NSString stringWithCString:infoBlock->ID encoding:NSASCIIStringEncoding];
		self.memIconCount = infoBlock->IconCount;
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
@synthesize notDeleted;
@synthesize memFlags;
@synthesize memName;
@synthesize memID;
@synthesize memIconCount;

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.englishName = nil;
	self.sjisName = nil;
	self.memName = nil;
	self.memID = nil;
	self.memImage = nil;
	
	[super dealloc];
}
#endif

- (NSString *)description
{
	return [NSString stringWithFormat:@"%@ (%@): Name: %@ ID: %@ Flags: %d", englishName, sjisName, memName, memID, memFlags];
}

@end
