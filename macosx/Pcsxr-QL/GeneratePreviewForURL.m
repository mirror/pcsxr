#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#include "MyQuickLook.h"
#include <zlib.h>
#import <Foundation/Foundation.h>
#include "nopic.h"

/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
	@autoreleasepool {
		NSData *data;
		NSURL *urlNS = (__bridge NSURL *)(url);
		gzFile f;
		const char* state_filename;
		if ([urlNS respondsToSelector:@selector(fileSystemRepresentation)]) {
			state_filename = [urlNS fileSystemRepresentation];
		} else {
			state_filename = [[urlNS path] fileSystemRepresentation];
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
			
			memcpy(pMem, NoPic_Image.pixel_data, 128*96*3);
		}
		NSBitmapImageRep *imRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:(unsigned char**)&NoPic_Image.pixel_data pixelsWide:NoPic_Image.width pixelsHigh:NoPic_Image.height bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bitmapFormat:0 bytesPerRow:NoPic_Image.width * NoPic_Image.bytes_per_pixel bitsPerPixel:24];
		if (imRep) {
			data = [imRep TIFFRepresentation];
			QLPreviewRequestSetDataRepresentation(preview, (__bridge CFDataRef)(data), kUTTypeImage, NULL);
		}
		
		return noErr;
	}
}

void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview)
{
	// Implement only if supported
}
