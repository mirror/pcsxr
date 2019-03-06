/***************************************************************************
    PluginWindowController.m
    PeopsSoftGPU
  
    Created by Gil Pedersen on Tue April 12 2004.
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

#import "PluginWindowController.h"
#import "PluginWindow.h"
#include "externals.h"
#undef BOOL

NSWindow *gameWindow;
PluginWindowController *gameController;
NSRect windowFrame;

@implementation PluginWindowController
{
	NSWindow *fullWindow;
}
+ (id)openGameView
{
	if (gameWindow == nil) {
		if (gameController == nil) {
			gameController = [[PluginWindowController alloc] initWithWindowNibName:@"NetSfPeopsSoftGPUInterface"];
		}
		gameWindow = [gameController window];
	}
	
	windowFrame = NSMakeRect(0, 0, iResX + 8, iResY + 4);
	
	windowFrame = [NSWindow contentRectForFrameRect:windowFrame styleMask:NSTitledWindowMask];

	
	if (windowFrame.size.width != 0) {
		[gameWindow setFrame:windowFrame display:NO];
	}
	[gameWindow center];

	[gameWindow makeKeyAndOrderFront:nil];
	[gameController showWindow:nil];
	
	CGDirectDisplayID display = (CGDirectDisplayID)[[[gameWindow screen] deviceDescription][@"NSScreenNumber"] unsignedIntValue];
	if (CGDisplayIsCaptured(display)) {
		[gameController setFullscreen:YES];
	}
	
	return gameController;
}

- (PluginGLView *)openGLView
{
	return (PluginGLView *)self.glView;
}

- (void)dealloc
{
	if (fullWindow) {
		[fullWindow orderOut:self];
	}
	fullWindow = nil;
	
	windowFrame = [[self window] frame];
}

// forget keyDownEvents
- (void)keyDown:(NSEvent *)theEvent
{
	// Not required any more
}

- (void)mouseDown:(NSEvent *)theEvent
{
	if (self.fullscreen) {
		[self setFullscreen:NO];
	}
}

- (BOOL)isFullscreen
{
	return (fullWindow != nil);
}

- (void)setFullscreen:(BOOL)flag
{
	NSWindow *window = [self window];
	NSScreen *screen = [window screen];
	CGDirectDisplayID display = (CGDirectDisplayID)[[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
	
	NSDisableScreenUpdates();
	
	if (flag) {
		if (!CGDisplayIsCaptured(display)) {
			CGDisplayCapture(display);
			
			CGDisplayCount count = 10;
			CGDirectDisplayID displays[10];
			CGGetActiveDisplayList(10, displays, &count);
			if (count == 1) {
				CGDisplayHideCursor(display);
				CGAssociateMouseAndMouseCursorPosition(NO);
			}
			
			//[window orderOut:self];
		}
		
		size_t width = CGDisplayPixelsWide(display);
		size_t height = CGDisplayPixelsHigh(display);
		
		// assume square pixel ratio on the monitor
		if ((width*3)/4 < height) {
			height = (width*3)/4;
		} else {
			width = (height*4)/3;
		}

		fullWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect((CGDisplayPixelsWide(display)-width)/2, 
																						  (CGDisplayPixelsHigh(display)-height)/2, 
																						  width, height)
							styleMask:NSBorderlessWindowMask
							backing:NSBackingStoreRetained
							defer:NO
							screen:screen];
		
		//[[glView openGLContext] setFullScreen];
		[[self.glView openGLContext] setView:[fullWindow contentView]];
		[self.glView reshape];
		//[[glView openGLContext] update];
		//[fullWindow setContentView:glView];
		
		[fullWindow setBackgroundColor:[NSColor blackColor]];
		[fullWindow setHasShadow:NO];
		[fullWindow setDelegate:self];
		
		[fullWindow setLevel:CGShieldingWindowLevel()];
		[fullWindow makeKeyAndOrderFront:self];

		[[self window] makeKeyAndOrderFront:self];
	} else {
		CGDisplayRelease(display);
		//CGReleaseAllDisplays();

		CGAssociateMouseAndMouseCursorPosition(YES);
		CGDisplayShowCursor(display);

		if (fullWindow) {
			[fullWindow orderOut:self];
			fullWindow = nil;
			
			[[self.glView openGLContext] setView:_glView];
			[self.glView reshape];
			//[window setContentView:glView];
		}
		
		[[self window] makeKeyAndOrderFront:self];
	}
	
	NSEnableScreenUpdates();
}

- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame
{
	self.fullscreen = YES;
	
	return NO;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize
{
	if (!(([sender resizeFlags] & NSShiftKeyMask) == NSShiftKeyMask)) {
		NSRect oldSize = [sender frame];
		NSRect viewSize = [self.glView frame];
		
		float xDiff = NSWidth(oldSize) - NSWidth(viewSize);
		float yDiff = NSHeight(oldSize) - NSHeight(viewSize);
		
		//if ((proposedFrameSize.height / proposedFrameSize.width) < (3.0/4.0))
		//	proposedFrameSize.height = ((proposedFrameSize.width - xDiff) * 3.0) / 4.0 + yDiff;
		//else
			proposedFrameSize.width = ((proposedFrameSize.height - yDiff) * 4.0) / 3.0 + xDiff;
	}
	
	return proposedFrameSize;
}

- (void)windowWillMiniaturize:(NSNotification *)aNotification
{
	[[NSNotificationCenter defaultCenter] postNotificationName:@"emuWindowWantPause" object:self];
}

- (void)windowDidDeminiaturize:(NSNotification *)aNotification
{
	[[NSNotificationCenter defaultCenter] postNotificationName:@"emuWindowWantResume" object:self];
}

- (BOOL)windowShouldClose:(id)sender
{
	if (fullWindow) {
		return NO;
	}
	[[NSNotificationCenter defaultCenter] postNotificationName:@"emuWindowDidClose" object:self];
	gameController = nil;
	gameWindow = nil;
	
	return YES;
}

@end
