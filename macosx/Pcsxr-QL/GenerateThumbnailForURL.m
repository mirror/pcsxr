#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#include "MyQuickLook.h"
#include <zlib.h>
#import <Cocoa/Cocoa.h>
#include "nopic.h"

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
	NSData *data;
	gzFile f;
	const char* state_filename;
	if ([url respondsToSelector:@selector(fileSystemRepresentation)]) {
		state_filename = [url fileSystemRepresentation];
	} else {
		state_filename = [[url path] fileSystemRepresentation];
	}
	if (!state_filename) {
		return fnfErr;
	}
	
	unsigned char *pMem = (unsigned char *) malloc(128*96*3);
	if (pMem == NULL)
		return mFulErr;
	
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
	
	NSBitmapImageRep *imRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:&pMem pixelsWide:NoPic_Image.width pixelsHigh:NoPic_Image.height bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bitmapFormat:0 bytesPerRow:NoPic_Image.width * NoPic_Image.bytes_per_pixel bitsPerPixel:24];
	if (imRep) {
		data = [imRep TIFFRepresentation];
		QLThumbnailRequestSetImageWithData(thumbnail, (__bridge CFDataRef)(data), NULL);
	}
	free(pMem);
	return noErr;
}

OSStatus GenerateThumbnailForMemCard(void *thisInterface, QLThumbnailRequestRef thumbnail, NSURL *url, NSDictionary *options, CGSize maxSize)
{
	return unimpErr;
}
