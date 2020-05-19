/***************************************************************************
    PluginGLView.h -- a view within game window, rudimentary OpenGL setup + maintainence
    PeopsOpenGPU
  
    Created by Gil Pedersen on Sun April 18 2004.
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

#define PluginGLView NetSfPeopsOpenGLGPUPluginGLView

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#include <sys/time.h>

#define IMAGE_COUNT  2

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

@interface PluginGLView : NSOpenGLView

- (void)swapBuffer; // I wonder what this does ;-)
- (void)clearBuffer:(BOOL)display;

// overrides:
- (id) initWithCoder: (NSCoder *) coder;
- (BOOL)isOpaque;
- (BOOL)acceptsFirstResponder;
- (void)reshape;

@end
