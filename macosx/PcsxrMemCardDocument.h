//
//  PcsxrMemCardDocument.h
//  Pcsxr
//
//  Created by Charles Betts on 12/3/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PcsxrMemCardDocument : NSDocument {
	IBOutlet NSWindow *changeMemCardSheet;
	IBOutlet NSTextField *cardPath;
	int memChosen;
}

- (IBAction)setMemCard:(id)sender;

@end
