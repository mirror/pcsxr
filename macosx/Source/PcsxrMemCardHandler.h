//
//  PcsxrMemCardHandler.h
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PcsxrFileHandle.h"

@interface PcsxrMemCardHandler : NSWindowController <PcsxrFileHandle>
@property (weak) IBOutlet NSTextField *cardPath;
- (IBAction)setMemCard:(id)sender;
@end
