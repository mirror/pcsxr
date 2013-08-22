//
//  SockDialog.h
//  DFNet
//
//  Created by C.W. Betts on 2/18/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface SockDialog : NSWindowController
{
	IBOutlet NSProgressIndicator *spinningBar;
}
- (IBAction)cancel:(id)sender;
@end
