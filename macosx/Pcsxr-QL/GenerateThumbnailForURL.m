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
	return unimpErr;
#endif
}

OSStatus GenerateThumbnailForMemCard(void *thisInterface, QLThumbnailRequestRef thumbnail, NSURL *url, NSDictionary *options, CGSize maxSize)
{
	//NSArray *memCards = CreateArrayByEnumeratingMemoryCardAtURL(url);
	return unimpErr;
}
