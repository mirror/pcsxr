//
//  PcsxrMemCardDocument.m
//  Pcsxr
//
//  Created by Charles Betts on 12/3/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemCardDocument.h"
#import "ConfigurationController.h"

@implementation PcsxrMemCardDocument

- (int)showMemoryCardChooserForFile:(NSString *)theFile
{
	if (!changeMemCardSheet) {
		[NSBundle loadNibNamed:[self windowNibName] owner:self];
	}
	[cardPath setObjectValue:theFile];
	
	[NSApp runModalForWindow:changeMemCardSheet];
	
	[NSApp endSheet:changeMemCardSheet];
	[changeMemCardSheet orderOut:self];
	
	return memChosen;
}

- (id)init
{
    self = [super init];
    if (self) {
		memChosen = 0;
    }
    return self;
}

- (NSString *)windowNibName
{
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"PcsxrMemCardDocument";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
    // Add any code here that needs to be executed once the windowController has loaded the document's window.
}

#if 0
- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError
{
    /*
     Insert code here to write your document to data of the specified type. If outError != NULL, ensure that you create and set an appropriate error when returning nil.
    You can also choose to override -fileWrapperOfType:error:, -writeToURL:ofType:error:, or -writeToURL:ofType:forSaveOperation:originalContentsURL:error: instead.
    */
    if (outError) {
        *outError = [NSError errorWithDomain:NSOSStatusErrorDomain code:unimpErr userInfo:NULL];
    }
    return nil;
}

- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError
{
	/*
	Insert code here to read your document from the given data of the specified type. If outError != NULL, ensure that you create and set an appropriate error when returning NO.
    You can also choose to override -readFromFileWrapper:ofType:error: or -readFromURL:ofType:error: instead.
    If you override either of these, you should also override -isEntireFileLoaded to return NO if the contents are lazily loaded.
	 */
    if (outError) {
        *outError = [NSError errorWithDomain:NSOSStatusErrorDomain code:unimpErr userInfo:NULL];
    }
    return YES;
}
#endif

- (BOOL)readFromFileWrapper:(NSFileWrapper *)fileWrapper ofType:(NSString *)typeName error:(NSError **)outError
{
	int chosen = [self showMemoryCardChooserForFile:[fileWrapper filename]];
	if (chosen == 0) {
		if (outError) {
			*outError = [NSError errorWithDomain:NSOSStatusErrorDomain code:unimpErr userInfo:nil];
		}
		return NO;
	} else {
		[ConfigurationController setMemoryCard:chosen toPath:[[self fileURL] path]];
		//TODO: make this return YES so that "File Can't be Opened" dialog box doesn't pop up, but the window itself doesn't stay on the screen.
		return NO;
	}
}

- (IBAction)setMemCard:(id)sender
{
	memChosen = [sender tag];
	
	[NSApp stopModal];
}

@end
