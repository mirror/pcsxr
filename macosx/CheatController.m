//
//  CheatController.m
//  Pcsxr
//

#import <Cocoa/Cocoa.h>
#include "psxcommon.h"
#include "cheat.h"
#import "CheatController.h"
#import "ARCBridge.h"
#import "PcsxrCheatHandler.h"

#define kTempCheatCodesName @"tempCheatCodes"
@interface PcsxrCheatTempObject : NSObject
{
	uint32_t address;
	uint16_t value;
}
@property (readwrite) uint32_t address;
@property (readwrite) uint16_t value;

- (id)initWithAddress:(uint32_t)add value:(uint16_t)val;
@end

@implementation PcsxrCheatTempObject
@synthesize address, value;

- (id)initWithAddress:(uint32_t)add value:(uint16_t)val
{
	if (self = [super init]) {
		self.address = add;
		self.value = val;
	}
	return self;
}

- (id)initWithCheatCode:(CheatCode *)theCheat
{
	return [self initWithAddress:theCheat->Addr value:theCheat->Val];
}

- (NSString*)description
{
	return [NSString stringWithFormat:@"%u %u", address, value];
}

@end

@implementation CheatController
@synthesize tempCheatCodes;

- (id)init
{
	return self = [self initWithWindowNibName:@"CheatWindow"];
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
	if (self = [super initWithCoder:aDecoder]) {
		self.tempCheatCodes = AUTORELEASEOBJ([[NSMutableArray alloc] init]);
	}
	return self;
}

- (id)initWithWindow:(NSWindow *)window
{
	if (self = [super initWithWindow:window]) {
		self.tempCheatCodes = AUTORELEASEOBJ([[NSMutableArray alloc] init]);
	}
	return self;
}

- (void)refresh
{
	[cheatView reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)view
{
	if (view == cheatView) {
		return NumCheats;
	} else
		return 0;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)col row:(NSInteger)idx
{
	if (tableView == cheatView) {
		if (idx >= NumCheats)
			return nil;
		NSString *ident = [col identifier];
		if ([ident isEqualToString:@"COL_NAME"]) {
			return @(Cheats[idx].Descr);
		} else if ([ident isEqualToString:@"COL_ENABLE"]) {
			return @( Cheats[idx].Enabled ? NSOnState : NSOffState);
		}
		NSLog(@"Unknown column identifier: %@", ident);
		return nil;
	} else
		return nil;
}

#if 0
- (void)awakeFromNib
{
	[addressFormatter setPositivePrefix:@"0x"];
	[valueFormatter setPositivePrefix:@"0x"];
}
#endif

- (void)tableView:(NSTableView *)tableView setObjectValue:(id)object forTableColumn:(NSTableColumn *)col row:(NSInteger)row
{
	if (tableView == cheatView) {
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
}

- (IBAction)loadCheats:(id)sender
{
	NSOpenPanel *openDlg = RETAINOBJ([NSOpenPanel openPanel]);
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowsMultipleSelection:NO];
	[openDlg setAllowedFileTypes:[PcsxrCheatHandler supportedUTIs]];
	
	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
		NSURL *file = [openDlg URL];
		LoadCheats([[file path] fileSystemRepresentation]);
		[self refresh];
	}
	RELEASEOBJ(openDlg);
}

- (IBAction)saveCheats:(id)sender
{
	NSSavePanel *saveDlg = RETAINOBJ([NSSavePanel savePanel]);
	[saveDlg setAllowedFileTypes:[PcsxrCheatHandler supportedUTIs]];
	[saveDlg setCanSelectHiddenExtension:YES];
	[saveDlg setCanCreateDirectories:YES];
	[saveDlg setPrompt:NSLocalizedString(@"Save Cheats", nil)];
	if ([saveDlg runModal] == NSFileHandlingPanelOKButton) {
		NSURL *url = [saveDlg URL];
		SaveCheats([[url path] fileSystemRepresentation]);
	}
	RELEASEOBJ(saveDlg);
}

- (IBAction)clear:(id)sender
{
	ClearAllCheats();
	[self refresh];
}

- (IBAction)closeCheatEdit:(id)sender
{
	[NSApp endSheet:editCheatWindow returnCode:[sender tag] == 1 ? NSCancelButton : NSOKButton];
}

- (IBAction)removeCheatValue:(id)sender
{
	NSIndexSet *toRemoveIndex = [editCheatView selectedRowIndexes];
	[self willChange:NSKeyValueChangeRemoval valuesAtIndexes:toRemoveIndex forKey:kTempCheatCodesName];
	[tempCheatCodes removeObjectsAtIndexes:toRemoveIndex];
	[self didChange:NSKeyValueChangeRemoval valuesAtIndexes:toRemoveIndex forKey:kTempCheatCodesName];	
}

- (IBAction)addCheatValue:(id)sender
{
	[self willChange:NSKeyValueChangeInsertion valuesAtIndexes:[NSIndexSet indexSetWithIndex:[tempCheatCodes count]] forKey:kTempCheatCodesName];
	[tempCheatCodes addObject:AUTORELEASEOBJ([[PcsxrCheatTempObject alloc] init])];
	[self didChange:NSKeyValueChangeInsertion valuesAtIndexes:[NSIndexSet indexSetWithIndex:[tempCheatCodes count]] forKey:kTempCheatCodesName];
}

- (void)editCheatCodeSheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton) {
		//FIXME: Expand the current cheat code list as needed
		//FIXME: Contract the current cheat code list as needed
		
		const char *tmpCheat = [[tempCheatCodes componentsJoinedByString:@"\n"] cStringUsingEncoding:NSASCIIStringEncoding];
		char *cheatCpy = strdup(tmpCheat);
		EditCheat((int)[cheatView selectedRow], Cheats[[cheatView selectedRow]].Descr, cheatCpy);
		free(cheatCpy);
	}
	
	[sheet orderOut:nil];
}

- (IBAction)editCheat:(id)sender
{
	[self willChangeValueForKey:kTempCheatCodesName];
	[tempCheatCodes removeAllObjects];
	
	Cheat *currentCheat = &Cheats[[cheatView selectedRow]];
	
	for (NSInteger i = 0; i < currentCheat->n; i++) {
		CheatCode *curCode = &CheatCodes[currentCheat->First + i];
		PcsxrCheatTempObject *tmpobj = [[PcsxrCheatTempObject alloc] initWithCheatCode:curCode];
		[tempCheatCodes addObject:tmpobj];
		RELEASEOBJ(tmpobj);
	}
	[self didChangeValueForKey:kTempCheatCodesName];
	[NSApp beginSheet:editCheatWindow modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(editCheatCodeSheetDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (IBAction)addCheat:(id)sender
{
	AddCheat(NULL, "0 0");
}

@end
