/***************************************************************************
    MappingCell.h
    HIDInput
  
    Created by Gil Pedersen on Mon Jun 07 2004.
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

#import "MappingCell.h"
#import "Keyconfig.h"

@implementation MappingCell

- (id)initTextCell:(NSString *)aString {
    self = [super initTextCell:aString];
    
	 [self setEditable:NO];
	 
    return self;
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(int)selStart length:(int)selLength
{
	[super selectWithFrame:aRect inView:controlView editor:textObj delegate:anObject start:selStart length:selLength];
	
	pRecDevice device;
	pRecElement element;
	NSEvent *endEvent;
	NSPoint where = {0.0, 0.0};
	ControllerList *controllerList = [[KeyConfig current] controllerList];
	int whichPad = [controllerList currentController];
	NSTableView *tableView = (NSTableView *)[self controlView];
	int i, direction;
	
	/* start a modal session */
	NSModalSession session = [NSApp beginModalSessionForWindow:[tableView window]];
	[NSApp runModalSession:session];

	/* delay for a little while to allow user to release the button pressed to activate the element */
	[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.15]];
	
	/* wait for 10 seconds for user to press a key */
	for (i=0;i<10;i++) {
		[NSApp runModalSession:session];
		direction = HIDConfigureAction(&device, &element, 1.0/* timeout */);
		if (0 != direction)
			break;
	}
	if (element) {
		if (element->usagePage == kHIDPage_KeyboardOrKeypad && element->usage == kHIDUsage_KeyboardEscape) {
			/* escape cancels */
			element = nil;
		} else {
			KeyConfig *config = [KeyConfig current];
			NSString *mappingId = [KeyConfig mappingIdForElement:element onDevice:device reverse:(direction < 0)];
			[config addMapping:mappingId
					forElement:[controllerList elementNameAtIndex:[tableView selectedRow] type:[config currentTypeForPlayer:whichPad]] 
					player:whichPad];
		}
		/* discard any events we have received while waiting for the button press */
		endEvent = [NSEvent otherEventWithType:NSApplicationDefined location:where 
									modifierFlags:0 timestamp:(NSTimeInterval)0
									windowNumber:0 context:[NSGraphicsContext currentContext] subtype:0 data1:0 data2:0];
		[NSApp postEvent:endEvent atStart:NO];
		[NSApp discardEventsMatchingMask:NSAnyEventMask beforeEvent:endEvent];
	}
	[NSApp endModalSession:session];
	
	/* move selection to the next list element */
	[self endEditing:textObj];
	if (element) {
		int nextRow = [tableView selectedRow]+1;
		if (nextRow >= [tableView numberOfRows]) {
			[tableView deselectAll:self];
			return;
		}
		[tableView selectRow:nextRow byExtendingSelection:NO];
		
		// I wonder if it's a good idea to begin to edit the next element automatically - for now i think not
		//[tableView editColumn:[tableView columnWithIdentifier:@"button"] row:nextRow withEvent:nil select:YES];
	}
	[[tableView window] makeFirstResponder:tableView];
}

@end
