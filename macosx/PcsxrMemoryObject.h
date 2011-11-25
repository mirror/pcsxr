//
//  PcsxrMemoryObject.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/NSString.h>
#import <AppKit/NSImage.h>

@interface PcsxrMemoryObject : NSObject
{
	NSString *englishName;
	NSString *sjisName;
	NSImage *memImage;
	short memNumber;
	unsigned char memFlags;
}

@property(copy, readwrite) NSString * englishName;
@property(copy, readwrite) NSString * sjisName;
@property(retain, readwrite) NSImage * memImage;
@property(readwrite) short memNumber;
@property(readwrite) unsigned char memFlags;


@end
