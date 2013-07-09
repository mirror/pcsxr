//
//  PcsxrMemCardHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemCardHandler.h"
#import "ConfigurationController.h"
#import "EmuThread.h"
#import "ARCBridge.h"

@implementation PcsxrMemCardHandler

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = @[@"com.codeplex.pcsxr.memcard"];
		RETAINOBJNORETURN(utisupport);
	}
	return utisupport;
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
	return self = [self initWithWindowNibName:@"PcsxrMemCardDocument"];
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
	if ([EmuThread active]) {
		return NO;
	}
	if (![self window]) {
		[NSBundle loadNibNamed:@"PcsxrMemCardDocument" owner:self];
	}
	[cardPath setObjectValue:[[NSFileManager defaultManager] displayNameAtPath:theFile]];
	
	[NSApp runModalForWindow:[self window]];
	
	[[self window] orderOut:nil];
	
	if (memChosen != 0) {
		[ConfigurationController setMemoryCard:(int)memChosen toPath:theFile];
	}
	return YES;
}

@end
