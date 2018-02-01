/***************************************************************************
    PluginWindow.h - Specialization of the main game window (borderless)
    PeopsSoftGPU
  
    Created by Gil Pedersen on Wed April 21 2004.
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
 
 /* this is a borderless window that can be dragged about. Also, holds a GLView */
#import <Cocoa/Cocoa.h>

@interface NetSfPeopsOpenGLGPUPluginWindow : NSWindow

@property (readonly) BOOL canBecomeKeyWindow; // to stop the beeping

- (void) sendEvent:(NSEvent *)theEvent;
- (void) windowDidResize:(NSNotification*)notice;
- (void) windowDidUpdate: (NSNotification*)notice;
- (void) windowDidMove:(NSNotification *)notice;

- (void) performClose: (id)sender;
- (void) mouseDown:(NSEvent *)theEvent; 
- (void) mouseDragged:(NSEvent *)theEvent;

- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;

- (BOOL)validateMenuItem:(NSMenuItem*) item;
@end
