//
//  PcsxrPluginHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrPluginHandler.h"

@implementation PcsxrPluginHandler

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        moveOK = NO;
    }
    
    return self;
}

- (id)init
{
	self = [super initWithWindowNibName:@"AddPluginSheet"];
	return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

- (IBAction)closeAddPluginSheet:(id)sender
{
	if ([[sender keyEquivalent] isEqualToString:@"\r"]) {
		moveOK = YES;
	} else {
		moveOK = NO;
	}
	[NSApp stopModal];
}

- (BOOL)handleFile:(NSString *)theFile
{
	if (![self window])
		[NSBundle loadNibNamed:@"AddPluginSheet" owner:self];
	
	[pluginName setObjectValue:[[NSURL fileURLWithPath:theFile] lastPathComponent]];
	
	[NSApp runModalForWindow:[self window]];

	[[self window] orderOut:self];
	if (moveOK == YES) {
		NSFileWrapper *wrapper = [[NSFileWrapper alloc] initWithPath:theFile];
		NSString *dst = [NSString stringWithFormat:@"%@/%@", 
						 [[NSBundle mainBundle] builtInPlugInsPath],
						 [wrapper filename]];
		if ([wrapper writeToFile:dst atomically:NO updateFilenames:NO]) {
			[[NSWorkspace sharedWorkspace] noteFileSystemChanged:[[NSBundle mainBundle] builtInPlugInsPath]];
			NSRunInformationalAlertPanel(NSLocalizedString(@"Installation Succesfull", nil),
										 NSLocalizedString(@"The installation of the specified plugin was succesfull. In order to use it, please restart the application.", nil), 
										 nil, nil, nil);
		} else {
			NSRunAlertPanel(NSLocalizedString(@"Installation Failed!", nil),
							NSLocalizedString(@"The installation of the specified plugin failed. Please try again, or make a manual install.", nil), 
							nil, nil, nil);
		}
		[wrapper release];
	}
	return YES;
}


@end
