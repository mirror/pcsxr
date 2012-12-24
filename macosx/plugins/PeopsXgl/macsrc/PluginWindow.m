/***************************************************************************
    PluginWindow.m
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

#import "PluginWindow.h"
@implementation NetSfPeopsOpenGLGPUPluginWindow
/*
- (BOOL)windowShouldClose:(id)sender
{
	[[NSNotificationCenter defaultCenter] postNotificationName:@"emuWindowDidClose" object:self];
	
	return YES;
}*/

- (void)sendEvent:(NSEvent *)theEvent
{
	NSEventType type = [theEvent type];
	if (type == NSKeyDown || type == NSKeyUp) {
		if (type == NSKeyDown && [theEvent keyCode] == 53 /* escape */) {
			// reroute to menu event
			[[NSApp mainMenu] performKeyEquivalent:theEvent];
		}
		
		// ignore all key Events
		return;
	}

	[super sendEvent:theEvent];
}

- (id) initWithContentRect: (NSRect) contentRect
                 styleMask: (NSUInteger) aStyle
                   backing: (NSBackingStoreType) bufferingType
                     defer: (BOOL) flag
{
    if (self = [super initWithContentRect: contentRect
                                styleMask: NSBorderlessWindowMask
                                  backing: bufferingType
                                    defer: flag])
    {
    return self;
    }

    return nil;
}



- (void) windowDidResize:(NSNotification*)notice
{
}

- (void) windowDidUpdate: (NSNotification*)notice
{
}

- (void)windowDidMove:(NSNotification *)notification
{

}



- (BOOL) canBecomeKeyWindow
{
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
}

- (void)keyUp:(NSEvent *)theEvent
{
}


- (BOOL)validateMenuItem:(NSMenuItem*) menuItem
{
//    SEL bleh = [menuItem action];
//    NSLog(@"Validate: %@, action: %@", menuItem, NSStringFromSelector(bleh));
    if ([menuItem action] == @selector(performClose:))
        return YES;
    
    return NO;
    
}

- (void) performClose: (id)sender
{
    if ([self delegate]){
  //      NSLog(@"We have a delegate %@", [self delegate]);

        if ([[self delegate] windowShouldClose:sender])
            [super close];
    }
    else {
        // hmm, just explode I guess
        [super close];
    }
}

- (void)mouseDown:(NSEvent *)theEvent {    

    // Get the mouse location in window coordinates.
    initialLocation = [theEvent locationInWindow];

}

 

/*

 Once the user starts dragging the mouse, move the window with it. The window has no title bar for the user to drag (so we have to implement dragging ourselves)

 */

- (void)mouseDragged:(NSEvent *)theEvent {

    if ([self level] == NSScreenSaverWindowLevel)
    // we don't drag when in fullscreen
    // note that it is only because a quirk in the ordering of events
    // and the time that NSScreenSaverWindowLevel is set
    // that we'll never be dragged while in the background
        return; 
        
    NSRect screenVisibleFrame = [[NSScreen mainScreen] visibleFrame];
    NSRect windowFrame = [self frame];
    NSPoint newOrigin = windowFrame.origin;

    // Get the mouse location in window coordinates.
    NSPoint currentLocation = [theEvent locationInWindow];

    // Update the origin with the difference between the new mouse location and the old mouse location.
    newOrigin.x += (currentLocation.x - initialLocation.x);
    newOrigin.y += (currentLocation.y - initialLocation.y);

    // Don't let window get dragged up under the menu bar
    if ((newOrigin.y + windowFrame.size.height) > (screenVisibleFrame.origin.y + screenVisibleFrame.size.height)) {
        newOrigin.y = screenVisibleFrame.origin.y + (screenVisibleFrame.size.height - windowFrame.size.height);
    }

    // Move the window to the new location
    [self setFrameOrigin:newOrigin];

}

@end
