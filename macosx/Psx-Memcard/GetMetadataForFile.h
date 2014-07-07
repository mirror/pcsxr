//
//  GetMetadataForFile.h
//  Pcsxr
//
//  Created by C.W. Betts on 6/6/14.
//
//

#ifndef Pcsxr_GetMetadataForFile_h
#define Pcsxr_GetMetadataForFile_h

#include <CoreFoundation/CoreFoundation.h>

__private_extern Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI, CFStringRef pathToFile);

#endif
