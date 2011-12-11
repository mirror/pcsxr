//
//  PcsxrFileHandle.h
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol PcsxrFileHandle <NSObject>

- (BOOL)handleFile:(NSString *)theFile;

@end
