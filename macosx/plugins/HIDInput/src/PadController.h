/***************************************************************************
    PadController.h
    HIDInput
  
    Created by Gil Pedersen on Sun Mar 07 2004.
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

#define PadController NetPcsxHIDInputPluginPadController

#import <Cocoa/Cocoa.h>
#import "PadView.h"

@class PadView;

@interface PadController : NSWindowController
{
    IBOutlet PadView *controllerView;
    IBOutlet id controllerView1;
    IBOutlet id controllerView2;
}

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
@end
