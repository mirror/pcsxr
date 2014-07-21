//
//  PcsxrPluginHandler.m
//  Pcsxr
//
//  Created by Charles Betts on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrPluginHandler.h"
#import "EmuThread.h"

@interface PcsxrPluginHandler ()
@property BOOL moveOK;
@end

@implementation PcsxrPluginHandler
@synthesize pluginName;
@synthesize moveOK;

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport = nil;
	if (utisupport == nil) {
		utisupport = @[@"com.codeplex.pcsxr.plugin"];
	}
	return utisupport;
}

- (instancetype)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        moveOK = NO;
    }
    
    return self;
}

- (NSString*)windowNibName
{
	return @"AddPluginSheet";
}

- (instancetype)init
{
	return self = [self initWithWindowNibName:@"AddPluginSheet"];
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
	if ([EmuThread active]) {
		return NO;
	}
	
	[pluginName setObjectValue:[theFile lastPathComponent]];
	
	[NSApp runModalForWindow:[self window]];
	
	[[self window] orderOut:nil];
	if (moveOK == YES) {
		NSURL *supportURL = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
		NSURL *url = [[supportURL URLByAppendingPathComponent:@"Pcsxr" isDirectory:YES] URLByAppendingPathComponent:@"PlugIns" isDirectory:YES];

		NSFileWrapper *wrapper = [[NSFileWrapper alloc] initWithPath:theFile];
		NSString *dst = [[url URLByAppendingPathComponent:[wrapper filename]] path];
		if ([wrapper writeToFile:dst atomically:NO updateFilenames:NO]) {
			[[NSWorkspace sharedWorkspace] noteFileSystemChanged:[url path]];
			NSRunInformationalAlertPanel(NSLocalizedString(@"Installation Succesfull", nil),
										 NSLocalizedString(@"The installation of the specified plugin was succesfull. In order to use it, please restart the application.", nil), 
										 nil, nil, nil);
		} else {
			NSRunAlertPanel(NSLocalizedString(@"Installation Failed!", nil),
							NSLocalizedString(@"The installation of the specified plugin failed. Please try again, or make a manual install.", nil), 
							nil, nil, nil);
		}
	}
	return YES;
}

@end
