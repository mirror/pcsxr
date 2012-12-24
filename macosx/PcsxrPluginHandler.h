//
//  PcsxrPluginHandler.h
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PcsxrFileHandle.h"

@interface PcsxrPluginHandler : NSWindowController <PcsxrFileHandle> {
	IBOutlet NSTextField *pluginName;
	
	BOOL moveOK;
}
- (IBAction)closeAddPluginSheet:(id)sender;

@end
