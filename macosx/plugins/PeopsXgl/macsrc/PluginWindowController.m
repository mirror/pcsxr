/***************************************************************************
    PluginWindowController.m
    The big bad boy that controls/creates the game window, the openGLView, and
    communicates with PCSXR itself
    
    PeopsOpenGPU
  
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
#import "Carbon/Carbon.h"
#include <OpenGL/gl.h> // OpenGL needed for "externals.h"
#include "externals.h"
#include "draw.h" // for CreateScanLines()
#undef BOOL

// not sure why these aren't class or instance variables...
NSWindow *gameWindow;
PluginWindowController *gameController;
NSRect windowFrame;
NSRect windowDefaultRect; // default window size (needed to go back into window mode)

NSRect FitRectInRect(NSRect source, NSRect destination)
{
    NSRect newRect;
    
    if (NSContainsRect(destination,source))
        return source;
    
    if (source.size.width > destination.size.width || source.size.height > destination.size.height){
        // have to rescale
        float ratio = source.size.width/source.size.height;
        if (ratio > destination.size.width/destination.size.height){
            source.size.width = destination.size.width;
            source.size.height = source.size.width / ratio ;
        }
        else{
            source.size.height = destination.size.height;
            source.size.width = source.size.height * ratio;
        }
    }
    // center horizontally and take top vertical
    newRect.origin.x = destination.origin.x + (destination.size.width - source.size.width)/2;
    newRect.origin.y = destination.origin.y + destination.size.height - source.size.height;
    newRect.size = source.size;
    
    return newRect;
}

@implementation PluginWindowController

+ (id)openGameView
{
    // create a window for the GPU and return
    // the controller that controls it

	if (gameWindow == nil) {
		if (gameController == nil) {
			gameController = [[PluginWindowController alloc] initWithWindowNibName:@"NetSfPeopsOpenGLGPUInterface"];
		}
		gameWindow = [gameController window];
	}
    else {
        NSLog(@"Well, we have a game window open already, which is kinda bad.");
        abort(); 
        return nil;
    }
    
    [gameWindow setBackgroundColor: [NSColor blackColor]];

    windowFrame.size.width=iResX;
    windowFrame.size.height=iResY;
    
    
	if (windowFrame.size.width != 0)
		[gameWindow setFrame:windowFrame display:NO];
        
	[gameWindow center];
    windowDefaultRect = [gameWindow frame];

	[gameWindow makeKeyAndOrderFront:nil];
	[gameController showWindow:nil];
    NSOpenGLView*  glInstance = [gameController openGLView];
    [glInstance setFrameSize: windowDefaultRect.size];
    [glInstance reshape];
//    [glView update];
   
	CGDirectDisplayID display = (CGDirectDisplayID)[[[gameWindow screen] deviceDescription][@"NSScreenNumber"] unsignedIntValue];
	if (CGDisplayIsCaptured(display)) {
		[gameController setFullscreen:YES];
	}
	
	return gameController;
}

- (void)subscribeToEvents
{
 NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self selector:@selector(applicationDidChangeScreenParameters:) 
    name:NSApplicationDidChangeScreenParametersNotification object:NSApp];

/* not used ATM:  
  [nc addObserver:self selector:@selector(applicationWillResignActive:) name:NSApplicationWillResignActiveNotification object:NSApp];
  [nc addObserver:self selector:@selector(applicationWillBecomeActive:) name:NSApplicationWillBecomeActiveNotification object:NSApp];
  [nc addObserver:self selector:@selector(applicationWillTerminate:) name:NSApplicationWillTerminateNotification object:NSApp];
*/
}

- (id)initWithCoder:(NSCoder *)aDecoder {

  self = [super initWithCoder:aDecoder];


  [self subscribeToEvents];
  return self;
}

- (id)initWithWindow:(NSWindow*)theWindow {
  self = [super initWithWindow:theWindow];

  [self subscribeToEvents];
  return self;
}

- (NSRect) screenFrame
{
    NSWindow* wind = [self window];
	CGDirectDisplayID display = (CGDirectDisplayID)[[[wind screen] deviceDescription][@"NSScreenNumber"] unsignedIntValue];

    return NSMakeRect (0,0,CGDisplayPixelsWide(display), CGDisplayPixelsHigh(display));
}

- (void) applicationDidChangeScreenParameters:(NSNotification*)aNotice
{
    // TODO: There could be issues with more drastic things like 
    // openGL pixel format, etc. when screen changes...

    // if fullscreen, conform to new size.
    if ([self fullscreen]){
        if (NSEqualRects([[self window] frame], [self screenFrame])){
            return;
        }

        [self adaptToFrame: [self screenFrame]];
        
    }
    else {
    // if windowed, recenter.
    // TODO: scale window if screen size is too small
        [[self window] center];

    }
    
}

- (PluginGLView *)openGLView
{
	return (PluginGLView *)glView;
}

- (void) cureAllIlls
{
    // try to reset the GPU without discarding textures, etc.
    // when a resize takes place, all hell breaks loose, so
    // this is necessarily ugly.

    // all this should be in draw.c, actually

// needed, but I don't know what it's for...
 rRatioRect.left   = rRatioRect.top=0;
 rRatioRect.right  = iResX;
 rRatioRect.bottom = iResY;

 [[glView openGLContext] makeCurrentContext];

 glFlush();
 glFinish();
 
 glViewport(rRatioRect.left,                           // init viewport by ratio rect
            iResY-(rRatioRect.top+rRatioRect.bottom),
            rRatioRect.right, 
            rRatioRect.bottom);  
 
 
 glScissor(0, 0, iResX, iResY);                        // init clipping (fullscreen)
 glEnable(GL_SCISSOR_TEST);     
 glMatrixMode(GL_PROJECTION);                          // init projection with psx resolution
 glLoadIdentity();
 glOrtho(0,PSXDisplay.DisplayMode.x,
         PSXDisplay.DisplayMode.y, 0, -1, 1);

 CreateScanLines();
// if(bKeepRatio) SetAspectRatio();                      // set ratio
 glFlush();
 glFinish();

 [NSOpenGLContext clearCurrentContext];

 [glView reshape]; // to get rid of fuglies on screen
// GLinitialize(); // blunt instrument method of setting a proper state.

}

- (void)dealloc
{
	NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
	
	[nc removeObserver:self];
	windowFrame = [[self window] frame]; // huh?
}

// forget keyDownEvents
- (void)keyDown:(NSEvent *)theEvent
{
	// Not required any more
}

- (void)mouseDown:(NSEvent *)theEvent
{
	if ([self fullscreen]) {
		[self setFullscreen:NO];
	}
}

- (BOOL)fullscreen
{
	return inFullscreen;
}

- (void)setFullscreen:(BOOL)flag
{
// this is called by cocoa, not the main PSX thread.
// Messing with the opengl context is a Bad Thing.
// Therefore, just set a global flag, and
// wait around for a frame until
// gpu.c calls fullscreenswap() from
// the right thread

    if ([self fullscreen] == flag)
        return;
        
    if (flag)
        bChangeWinMode = 2;
    else
        bChangeWinMode = 1;

}

- (void)performFullscreenSwap
{
    // ah, that's better. We are called from the main PSX thread
    // after a screen update, so we're clean.
    // bChangeWinMode is a global set from PSX
    
    int flag = bChangeWinMode - 1; // 1 = go to window, 2 = go to fullscreen
    bChangeWinMode = 0; // this is our flag that launched us, so 0 now
    
	NSWindow *window = [self window];
	NSScreen *screen = [window screen];

    CGDirectDisplayID display = (CGDirectDisplayID)[[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];

    NSRect newPlace;
    
    if (flag){
        [window setLevel: NSScreenSaverWindowLevel];
        newPlace = [self screenFrame] ; 
		CGDisplayHideCursor(display);
		CGAssociateMouseAndMouseCursorPosition(NO); // this could be bad since it disables mouse somewhat
    }
    else{
        [window setLevel: NSNormalWindowLevel];
        newPlace = windowDefaultRect;
		CGDisplayShowCursor(display);
		CGAssociateMouseAndMouseCursorPosition(YES);
    }

    if (flag) inFullscreen = TRUE;
    else inFullscreen = FALSE;
    
    if (!inFullscreen)
        newPlace = FitRectInRect(newPlace, NSMakeRect(0,0,CGDisplayPixelsWide(display),CGDisplayPixelsHigh(display)-24)); // with menu bar room
    
    [self adaptToFrame: newPlace];

}

- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame
{
	[self setFullscreen:YES];
	
	return NO;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize
{
    // we don't bother

    return proposedFrameSize;
        
	if (!(([sender resizeFlags] & NSShiftKeyMask) == NSShiftKeyMask)) {
		NSRect oldSize = [sender frame];
		NSRect viewSize = [glView frame];
		
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
	[[NSNotificationCenter defaultCenter] postNotificationName:@"emuWindowDidClose" object:self];
	gameController = nil;
	gameWindow = nil;
    CGReleaseAllDisplays();
	return YES;
}

// these two funcs should be handled by the window class but
// since we do fullscreen tweaking (hiding mouse, etc), 
// the controller must do it ATM...
- (void)windowDidBecomeKey:(NSNotification*)aNotice
{
    // if in fullscreen, we must restore level and mouse hiding.
    // it might be cooler if window goes to "window" size or hides
    // instead of taking up full screen in background.
    
	NSWindow *window = [self window];
  	NSScreen *screen = [window screen];

    CGDirectDisplayID display = (CGDirectDisplayID)[[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];

    if ([self fullscreen]){
        [window setLevel: NSScreenSaverWindowLevel];
		CGDisplayHideCursor(display);
    }
}

- (void)windowDidResignKey:(NSNotification*)aNotice
{
    // if in fullscreen, we must abdicate mouse hiding and level.
	NSWindow *window = [self window];
	NSScreen *screen = [window screen];
    CGDirectDisplayID display = (CGDirectDisplayID)[[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];

    if ([self fullscreen]){
        [window setLevel: NSNormalWindowLevel];
		CGDisplayShowCursor(display);
    }
}

- (void) adaptToFrame:(NSRect)aFrame
{
    // do magic so everything goes as planned
    // when the window area changes

    int proportionalWidth, proportionalHeight;
    
    NSWindow* window = [self window];
    
    [window setFrame:aFrame display:NO];
    
    // assume square pixel ratio on the monitor
    if ((aFrame.size.width*3)/4 <= aFrame.size.height) { // is window skinnier than it needs to be?
        proportionalHeight = (aFrame.size.width*3)/4; // then shrink the content height (letterbox)
        proportionalWidth = aFrame.size.width;        // and conform to width
    } else {
        proportionalWidth = (aFrame.size.height*4)/3;
        proportionalHeight = aFrame.size.height;
    }
    
    NSRect fitToWindow = NSMakeRect(
        roundf((aFrame.size.width - proportionalWidth)/2.0), 
        roundf((aFrame.size.height - proportionalHeight)/2.0), 
        roundf(proportionalWidth), roundf(proportionalHeight));
        
    [glView setFrame:fitToWindow];
    [glView reshape];
    iResX = roundf(proportionalWidth);
    iResY = roundf(proportionalHeight);
        
    [self cureAllIlls]; // do some fixin'
    return;
    
}

@end

void ChangeWindowMode(void)
{
    // glue from PSX thread. Globals are already set
    [ gameController performFullscreenSwap];
}
