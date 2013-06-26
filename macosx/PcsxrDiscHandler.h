//
//  PcsxrDiscHandler.h
//  Pcsxr
//
//  Created by Charles Betts on 12/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PcsxrFileHandle.h"

@interface PcsxrDiscHandler : NSObject <PcsxrFileHandle>
{
	NSURL *discURL;
}

@end
