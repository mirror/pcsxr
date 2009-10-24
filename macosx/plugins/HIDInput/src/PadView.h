/***************************************************************************
    PadView.h
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

#define PadView NetPcsxHIDInputPluginPadView

#import <Cocoa/Cocoa.h>
#import "ControllerList.h"

@class ControllerList;

@interface PadView : NSView
{
    IBOutlet NSTableView *tableView;
    IBOutlet NSPopUpButton *typeMenu;
	 
	 ControllerList *controller;
}
- (IBAction)setType:(id)sender;

- (void)setController:(int)which;

@end
