//
//  CheatController.m
//  Pcsxr
//

#import <Cocoa/Cocoa.h>
#include "psxcommon.h"
#include "cheat.h"
#import "CheatController.h"
#import "ARCBridge.h"

@implementation CheatController

- (id)init
{
	return self = [self initWithWindowNibName:@"CheatWindow"];
}

- (void)refresh
{
	[cheatView reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)view
{
    return NumCheats;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)col row:(NSInteger)idx
{
    if (idx >= NumCheats)
        return nil;
    NSString *ident = [col identifier];
    if ([ident isEqualToString:@"COL_NAME"]) {
        return @(Cheats[idx].Descr);
    } else if ([ident isEqualToString:@"COL_ENABLE"]) {
        return @(Cheats[idx].Enabled ? NSOnState : NSOffState);
    }
    SysPrintf("Unknown column identifier: %s\n", [[ident description] UTF8String]);
    return nil;
}

- (void)tableView:(NSTableView *)tableView setObjectValue:(id)object forTableColumn:(NSTableColumn *)col row:(NSInteger)row
{
    if (row >= NumCheats)
        return;
    NSString *ident = [col identifier];
    if ([ident isEqualToString:@"COL_ENABLE"]) {
        Cheats[row].Enabled = [object integerValue] == NSOnState;
    } else if ([ident isEqualToString:@"COL_NAME"]) {
		free(Cheats[row].Descr);
		Cheats[row].Descr = strdup([object UTF8String]);
	}
}

- (IBAction)LoadCheats:(id)sender
{
    NSOpenPanel *openDlg = RETAINOBJ([NSOpenPanel openPanel]);
    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    
    if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
        NSArray *files = [openDlg URLs];
		for (NSURL *theURL in files) {
			LoadCheats([[theURL path] fileSystemRepresentation]);
		}
        [self refresh];
    }
    RELEASEOBJ(openDlg);
}

- (IBAction)SaveCheats:(id)sender
{
    NSSavePanel *saveDlg = RETAINOBJ([NSSavePanel savePanel]);
    [saveDlg setPrompt:NSLocalizedString(@"Save Cheats", nil)];
    if ([saveDlg runModal] == NSFileHandlingPanelOKButton) {
        NSURL *url = [saveDlg URL];
        SaveCheats((const char *)[[url path] fileSystemRepresentation]);
    }
    RELEASEOBJ(saveDlg);
}

- (IBAction)clear:(id)sender
{
    ClearAllCheats();
    [self refresh];
}

- (IBAction)close:(id)sender
{
    [[self window] close];
}

@end
