/***************************************************************************
    PluginWindowController.h
    The big bad boy that controls/creates the game window, the openGLView, and
    communicates with PCSXR itself
    PeopsOpenGPU
  
    Created by Gil Pedersen on Mon April 11 2004.
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

/*
    In truth, this controller should be the ONLY place that glues between the 
    running PCSXR gpu and the mac interface. 
    ATM, some of the glue is here, some of it in drawgl.m, and a couple
    of bits are in the config controller...
*/

#define PluginWindowController NetSfPeopsOpenGLGPUPluginWindowController

#import <Cocoa/Cocoa.h>
#import "PluginGLView.h"

@class PluginWindowController;

extern NSWindow *gameWindow;
extern PluginWindowController *gameController;

@interface PluginWindowController : NSWindowController

@property (weak) IBOutlet NSOpenGLView *glView;

+ (id)openGameView;
- (PluginGLView *)openGLView;
- (BOOL)fullscreen;
- (void)setFullscreen:(BOOL)flag;
- (void)performFullscreenSwap;
- (void)cureAllIlls;
- (void) adaptToFrame:(NSRect)aFrame;
- (NSRect) screenFrame;
- (void)subscribeToEvents;

- (id)initWithCoder:(NSCoder *)aDecoder ;
- (id)initWithWindow:(NSWindow*)theWindow;

- (void)windowDidBecomeKey:(NSNotification*)aNotice;
- (void)windowDidResignKey:(NSNotification*)aNotice;
@end
