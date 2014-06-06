//
//  GetMetadataForFile.m
//  Psx-Memcard
//
//  Created by C.W. Betts on 6/6/14.
//
//

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#include "GetMetadataForFile.h"

//==============================================================================
//
//	Get metadata attributes from document files
//
//	The purpose of this function is to extract useful information from the
//	file formats for your document, and set the values into the attribute
//  dictionary for Spotlight to include.
//
//==============================================================================

Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI, CFStringRef pathToFile)
{
    Boolean ok = FALSE;
    @autoreleasepool {
		NSMutableDictionary *attr = (__bridge NSMutableDictionary*)attributes;
		NSString *path = (__bridge NSString*)pathToFile;
		
	}
    
	// Return the status
    return ok;
}
