#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#include "MyQuickLook.h"
//#include <zlib.h>
#import <Cocoa/Cocoa.h>
//#include "nopic.h"
#import "PSXMemEnumerator.h"

/* -----------------------------------------------------------------------------
    Generate a thumbnail for file

   This function's job is to create thumbnail for designated file as fast as possible
   ----------------------------------------------------------------------------- */

#define ImageDivider 32

static OSStatus GenerateThumbnailForFreeze(void *thisInterface, QLThumbnailRequestRef preview, NSURL *url, NSDictionary *options, CGSize maxSize);
static OSStatus GenerateThumbnailForMemCard(void *thisInterface, QLThumbnailRequestRef preview, NSURL *url, NSDictionary *options, CGSize maxSize);

OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
	OSStatus theErr = noErr;
	@autoreleasepool {
		NSURL *urlNS = (__bridge NSURL*)url;
		NSString *UTI = (__bridge NSString*)contentTypeUTI;
		NSDictionary *optionsNS = (__bridge NSDictionary*)options;
		NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
		if ([workspace type:UTI conformsToType:@"com.codeplex.pcsxr.freeze"]) {
			theErr = GenerateThumbnailForFreeze(thisInterface, thumbnail, urlNS, optionsNS, maxSize);
		} else if ([workspace type:UTI conformsToType:@"com.codeplex.pcsxr.memcard"]) {
			theErr = GenerateThumbnailForMemCard(thisInterface, thumbnail, urlNS, optionsNS, maxSize);
		}
		
	}
	return theErr;
}

void CancelThumbnailGeneration(void *thisInterface, QLThumbnailRequestRef thumbnail)
{
	// Implement only if supported
}

OSStatus GenerateThumbnailForFreeze(void *thisInterface, QLThumbnailRequestRef thumbnail, NSURL *url, NSDictionary *options, CGSize maxSize)
{
#if 0
	gzFile f;
	const char* state_filename = NULL;
	NSBitmapImageRep *imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:96 pixelsHigh:128 bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:0 bitsPerPixel:0];

	if ([url respondsToSelector:@selector(fileSystemRepresentation)]) {
		state_filename = [url fileSystemRepresentation];
	} else {
		state_filename = [[url path] fileSystemRepresentation];
	}
	if (!state_filename) {
		return fnfErr;
	}
	
	unsigned char pMem[128*96*3] = {0};
	f = gzopen(state_filename, "rb");
	if (f != NULL) {
		gzseek(f, 32, SEEK_SET); // skip header
		gzseek(f, sizeof(uint32_t), SEEK_CUR);
		gzseek(f, sizeof(uint8_t), SEEK_CUR);
		gzread(f, pMem, 128*96*3);
		gzclose(f);
	} else {
		memcpy(pMem, NoPic_Image.pixel_data, 128*96*3);
	}
	
	@autoreleasepool {
		unsigned char *ppMem = pMem;
		int x, y;
		for (y = 0; y < 96; y++) {
			for (x = 0; x < 128; x++) {
				[imageRep setColor:[NSColor colorWithCalibratedRed:*(ppMem+2)/255.0 green:*(ppMem+1)/255.0 blue:*(ppMem+0)/255.0 alpha:1.0] atX:x y:y];
				ppMem+=3;
			}
		}
	}
	
	NSImage *theImage = [[NSImage alloc] init];
	[theImage addRepresentation:imageRep];
	[theImage setSize:NSMakeSize(NoPic_Image.width, NoPic_Image.height)];
	
	if (theImage) {
		NSData *data = [theImage TIFFRepresentation];
		QLThumbnailRequestSetImageWithData(thumbnail, (__bridge CFDataRef)(data), NULL);
	}
	return noErr;
#else
	return noErr;
#endif
}

static NSImage *MemoryImageAtIndex(NSArray *memArray, NSInteger my)
{
	NSInteger i = 0;
	for (PcsxrMemoryObject *obj in memArray) {
		NSIndexSet *idxSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(i, obj.blockSize)];
		if ([idxSet containsIndex:my]) {
			return obj.firstImage;
		}
		i += obj.blockSize;
	}
	
	return nil;
}

OSStatus GenerateThumbnailForMemCard(void *thisInterface, QLThumbnailRequestRef thumbnail, NSURL *url, NSDictionary *options, CGSize maxSize)
{
	NSArray *memCards = CreateArrayByEnumeratingMemoryCardAtURL(url);
	if (!memCards) {
		return noErr;
	}
	
	NSBundle *Bundle;
	{
		CFBundleRef cfbundle = QLThumbnailRequestGetGeneratorBundle(thumbnail);
		NSURL *bundURL = CFBridgingRelease(CFBundleCopyBundleURL(cfbundle));
		Bundle = [[NSBundle alloc] initWithURL:bundURL];
	}

	NSRect imageRect = NSMakeRect(0, 0, ImageDivider, ImageDivider);
	NSImage *blankImage = [[NSImage alloc] initWithSize:imageRect.size];
	[blankImage lockFocus];
	[[NSColor blackColor] set];
	[NSBezierPath fillRect:imageRect];
	[blankImage unlockFocus];

	NSImage *memImages = [[NSImage alloc] initWithSize:NSMakeSize((4 * ImageDivider), (4 * ImageDivider))];

	NSInteger allMems = 0;
	for (PcsxrMemoryObject *obj in memCards) {
		allMems += obj.blockSize;
	}
	[memImages lockFocus];
	[[NSColor clearColor] set];
	[NSBezierPath fillRect:NSMakeRect(0, 0, (4 * ImageDivider), (4 * ImageDivider))];
	for (int i = 1; i < 16; i++) {
		NSInteger x = (i % 4) * ImageDivider, y = (3 * ImageDivider) - ((i / 4) * ImageDivider);
		NSImage *curImage;
		if (i < allMems) {
			curImage = MemoryImageAtIndex(memCards, i - 1);
		} else {
			curImage = blankImage;
		}
		[curImage drawInRect:NSMakeRect(x, y, ImageDivider, ImageDivider) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
	}
	NSURL *psxMemURL = [Bundle URLForResource:@"pcsxrmemcard" withExtension:@"icns"];
	NSImage *psxMemIcon = [[NSImage alloc] initByReferencingURL:psxMemURL];
	psxMemIcon.size = NSMakeSize(ImageDivider, ImageDivider);
	[psxMemIcon drawInRect:NSMakeRect(0, 3 * ImageDivider, ImageDivider, ImageDivider) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
	
	[memImages unlockFocus];
	
	NSData *data = [memImages TIFFRepresentation];
	QLThumbnailRequestSetImageWithData(thumbnail, (__bridge CFDataRef)(data), NULL);
	
	return noErr;
}
