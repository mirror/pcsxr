/***************************************************************************
    PadView.m
    HIDInput
  
    Created by Gil Pedersen on Thu May 27 2004.
    Copyright (c) 2004 Gil Pedersen.
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#import "PadView.h"

@implementation PadView

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil) {
		// Add initialization code here
		//NSLog(@"rect: %f,%f;%f,%f\n", frameRect.origin.x, frameRect.origin.y, frameRect.size.width, frameRect.size.height);
		//controller = [[ControllerList alloc] initWithConfig];
		//setCurrentController:0];
		controller = [[[KeyConfig current] controllerList] retain];
	}
	return self;
}

- (void)dealloc
{
	[controller release];
	
	[super dealloc];
}

- (void)drawRect:(NSRect)rect
{
		//NSLog(@"drawRect: %f,%f;%f,%f\n", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

- (IBAction)setType:(id)sender
{
	[[KeyConfig current] setCurrentType:[[controller controllerTypes] objectAtIndex:[sender indexOfSelectedItem]] 
			player:[controller currentController]];
		//[controller setType:[[controller controllerTypes] objectAtIndex:[sender indexOfSelectedItem]]];
	[tableView reloadData];
}

- (void)setController:(int)which
{
	int i;
	[controller setCurrentController:which];
	[tableView setDataSource:controller];
	
	/* create type popup menu */
	[typeMenu removeAllItems];
	NSArray *typeList = [controller controllerTypes];
	NSString *current = [[KeyConfig current] currentTypeForPlayer:which];
	
	for (i=0; i<[typeList count]; i++) {
		NSString *name = [typeList objectAtIndex:i];
		if ([name isEqualToString:@"-"]) {
			[[typeMenu menu] addItem:[NSMenuItem separatorItem]];
		} else {
			[typeMenu addItemWithTitle:NSLocalizedString(name, @"")];
			if ([name isEqualToString:current])
				[typeMenu selectItemAtIndex:i];
		}
		if (0==i)
			[typeMenu selectItemAtIndex:0];
	}
	[self setType:typeMenu];
	
	//[tableView reloadData];
}


- (BOOL)control:(NSControl *)control textShouldBeginEditing:(NSText *)fieldEditor
{
	return false;
}

/* handles key events on the pad list */
- (void)keyDown:(NSEvent *)theEvent
{
	int key = [theEvent keyCode];
	
	if ([[theEvent window] firstResponder] == tableView) {
		if (key == 51 || key == 117) {
			// delete keys - remove the mappings for the selected item
			KeyConfig *config = [KeyConfig current];
			int player = [[config controllerList] currentController];
			NSString *name = [[config controllerList] elementNameAtIndex:[tableView selectedRow] type:[config currentTypeForPlayer:player]];
			[config removeMappingsForElement:name player:player];
			[tableView reloadData];
			return;
		} else if (key == 36) {
			// return key - configure the selected item
			[tableView editColumn:[tableView columnWithIdentifier:@"button"] row:[tableView selectedRow] withEvent:nil select:YES];
			return;
		}
	}
	
	[super keyDown:theEvent];
}

@end
