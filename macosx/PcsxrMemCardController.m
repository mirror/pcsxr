//
//  PcsxrMemCardManager.m
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemCardController.h"
#import "PcsxrMemoryObject.h"
#include "sio.h"

#define MAX_MEMCARD_BLOCKS 15

static inline NSImage *imageFromMcd(short * icon)
{
	NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:16 pixelsHigh:16 bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:0 bitsPerPixel:0];
	
#if 0
	int x, y, c;
	for (y = 0; y < 32; y++) {
		for (x = 0; x < 32; x++) {
			c = icon[(y>>1) * 16 + (x>>1)];
			c = ((c & 0x001f) << 10) | ((c & 0x7c00) >> 10) | (c & 0x03e0);
			c = ((c & 0x001f) << 3) | ((c & 0x03e0) << 6) | ((c & 0x7c00) << 9);
			
			NSUInteger NSc = c;
			
			[imageRep setPixel:&NSc atX:x y:y];
		}
	}
#else
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
#endif
	NSImage *theImage = [[NSImage alloc] init];
	[theImage addRepresentation:imageRep];
	[theImage setScalesWhenResized:YES];
	[theImage setSize:NSMakeSize(32, 32)];
	[imageRep release];
	return [theImage autorelease];
}

@implementation PcsxrMemCardController

//memCard1Array KVO functions

-(void)insertObject:(PcsxrMemoryObject *)p inMemCard1ArrayAtIndex:(NSUInteger)index {
    [memCard1Array insertObject:p atIndex:index];
}

-(void)removeObjectFromMemCard1ArrayAtIndex:(NSUInteger)index {
    [memCard1Array removeObjectAtIndex:index];
}

- (void)setMemCard1Array:(NSMutableArray *)a
{
	if (memCard1Array != a) {
		[memCard1Array release];
		memCard1Array = [[NSMutableArray alloc] initWithArray:a];
	}
}

- (NSArray *)memCard1Array
{
	return memCard1Array;
}

//memCard2Array KVO functions

-(void)insertObject:(PcsxrMemoryObject *)p inMemCard2ArrayAtIndex:(NSUInteger)index {
    [memCard2Array insertObject:p atIndex:index];
}

-(void)removeObjectFromMemCard2ArrayAtIndex:(NSUInteger)index {
    [memCard2Array removeObjectAtIndex:index];
}

- (void)setMemCard2Array:(NSMutableArray *)a
{
	if (memCard2Array != a) {
		[memCard2Array release];
		memCard2Array = [[NSMutableArray alloc] initWithArray:a];
	}
}

- (NSArray *)memCard2Array
{
	return memCard2Array;
}


- (id)init
{
	self = [self initWithWindowNibName:@"MemCardManager"];
	return self;
}

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        LoadMcds(Config.Mcd1, Config.Mcd2);
		[self setMemCard1Array:[[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS]];
		[self setMemCard2Array:[[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS]];
    }
    
    return self;
}

- (void)loadMemoryCardInfoForCard:(int)theCard
{
	NSInteger i;
	McdBlock info;
	NSMutableArray *newArray = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
	
	for (i = 0; i < MAX_MEMCARD_BLOCKS; i++) {
		GetMcdBlockInfo(theCard, i, &info);
		PcsxrMemoryObject *ob = [[PcsxrMemoryObject alloc] init];
		NSString *engDes = nil, *japDes = nil;
		ob.englishName = [NSString stringWithCString:info.Title encoding:NSASCIIStringEncoding];
		ob.sjisName = [NSString stringWithCString:info.sTitle encoding:NSShiftJISStringEncoding];
		ob.memImage = imageFromMcd(info.Icon);
		ob.memNumber = i;
		[newArray insertObject:ob atIndex:i];
		[ob release];
	}
	if (theCard == 1) {
		[self setMemCard1Array:newArray];
	} else {
		[self setMemCard2Array:newArray];
	}
	[newArray release];
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
	[self loadMemoryCardInfoForCard:1];
	[self loadMemoryCardInfoForCard:2];
}

- (IBAction)moveToLeft:(id)sender
{
	
}

- (IBAction)moveToRight:(id)sender
{
	
}

- (IBAction)formatCard:(id)sender
{
	NSInteger formatOkay = NSRunAlertPanel(NSLocalizedString(@"Format Card", nil), NSLocalizedString(@"Formatting a memory card will remove all data on it.\n\nThis cannot be undone.", nil), NSLocalizedString(@"Cancel", nil), NSLocalizedString(@"Format", nil), nil);
	if (formatOkay == NSAlertAlternateReturn) {
		NSInteger memCardSelect = [sender tag];
		if (memCardSelect == 1) {
			CreateMcd(Config.Mcd1);
			[self loadMemoryCardInfoForCard:1];
		} else {
			CreateMcd(Config.Mcd2);
			[self loadMemoryCardInfoForCard:2];
		}
	}
}

- (IBAction)deleteMemoryObject:(id)sender {
	NSInteger deleteOkay = NSRunAlertPanel(NSLocalizedString(@"Delete Block", nil), NSLocalizedString(@"Deleting a block will remove all saved data on that block.\n\nThis cannot be undone.", nil), NSLocalizedString(@"Cancel", nil), NSLocalizedString(@"Delete", nil), nil);
	if (deleteOkay == NSAlertAlternateReturn) {
		NSInteger memCardSelect = [sender tag];
		if (memCardSelect == 1) {
		
			[self loadMemoryCardInfoForCard:1];
		} else {
		
			[self loadMemoryCardInfoForCard:2];
		}
	}
}
@end
