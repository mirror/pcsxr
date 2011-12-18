//
//  PcsxrMemCardHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemCardHandler.h"
#import "ConfigurationController.h"

@implementation PcsxrMemCardHandler

+ (NSArray *)supportedUTIs
{
	return [NSArray arrayWithObject:@"com.codeplex.pcsxr.memcard"];
}

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
		memChosen = 0;
	}
    
    return self;
}

- (id)init
{
	self = [self initWithWindowNibName:@"PcsxrMemCardDocument"];
	return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

- (IBAction)setMemCard:(id)sender
{
	memChosen = [sender tag];
	
	[NSApp stopModal];
}


- (BOOL)handleFile:(NSString *)theFile
{
	if (![self window]) {
		[NSBundle loadNibNamed:@"PcsxrMemCardDocument" owner:self];
	}
	[cardPath setObjectValue:[[NSURL fileURLWithPath:theFile] lastPathComponent]];
	
	[NSApp runModalForWindow:[self window]];
	
	[[self window] orderOut:self];
	
	if (memChosen != 0) {
		[ConfigurationController setMemoryCard:memChosen toPath:theFile];
	}
	return YES;
}


@end
