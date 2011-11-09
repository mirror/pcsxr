/***************************************************************************
    PluginGLView.m
    a view within game window, rudimentary OpenGL setup + maintainence
    Also, I clear the gl screen with a beautiful yellow color for
    debugging purposes.
    
    PeopsOpenGLGPU
  
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

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>
//#import <GLUT/glut.h>
//#import <Carbon/Carbon.h>
#import "PluginGLView.h"
#include "externals.h" // for PSXDisplay.disable -- should move it elsewhere really
#undef BOOL

@implementation PluginGLView

- (BOOL)isOpaque {	return YES; } 
- (BOOL)acceptsFirstResponder { return NO; }


- (id) initWithCoder: (NSCoder *) coder
{
    // Set up pixel format on creation
    // and, well, that's about it.
    if ((self = [super initWithCoder:coder]) == nil)
		return nil;

	glLock = [[NSLock alloc] init];
	if (nil == glLock) {
		[self release];
		return nil;
	}

	// Init pixel format attribs
	NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
//      NSOpenGLPFASampleBuffers, 1,  // For full screen AA when implemented
//      NSOpenGLPFASamples, 2,
		0
	};

	// Get pixel format from OpenGL
	NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	if (!pixFmt)
	{
		NSLog(@"No Accelerated OpenGL pixel format found\n");
		
		NSOpenGLPixelFormatAttribute attrs2[] =
		{
			NSOpenGLPFANoRecovery,
			0
		};

		// Get pixel format from OpenGL
		pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs2];
		if (!pixFmt) {
			NSLog(@"No OpenGL pixel format found!\n");
			
			[self release];
			return nil;
		}
	}
	
	[self setPixelFormat:[pixFmt autorelease]];

	[[self openGLContext] makeCurrentContext];

    // we're done, dude.

	// Call for a redisplay
	noDisplay = YES; // hm, this can be deleted I think
	PSXDisplay.Disabled = 1;
	[self setNeedsDisplay:true];
	
	return self;
}

- (void)dealloc
{
	[[self openGLContext] makeCurrentContext]; // just in case
	[NSOpenGLContext clearCurrentContext];
	[glLock release];
	[super dealloc];
}


- (void)reshape	// scrolled, moved or resized
{
	[super reshape];
    
    [glLock lock];  // not sure if needed, but hey
    [[self openGLContext] makeCurrentContext];
    
    NSRect rect = [self bounds];
    rect.size = [self convertSize:rect.size toView:nil];
    glViewport(0.0, 0.0, NSWidth(rect), NSHeight(rect));
 
    glClearColor (1.0, 0.5, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
   [[self openGLContext] flushBuffer];

//    [NSOpenGLContext clearCurrentContext]; // this makes bad things happen, so screw it.
	[glLock unlock];
    return;
}

- (void)swapBuffer
{
    // actually not much to do here.
	[[self openGLContext] flushBuffer];
    return;
}

// don't know what this does, pasted it in from PeopsSoftGPU's PluginGLView because something was calling it
- (void)clearBuffer:(BOOL)display
{
	if (display == NO) {
		//[[self openGLContext] makeCurrentContext];
		//glClear(GL_COLOR_BUFFER_BIT);
		//[self loadTextures:NO];
	} else {
		noDisplay = YES;
        //		[self setNeedsDisplay:true];
	}
}

@end
