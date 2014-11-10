/***************************************************************************
    PluginGLView.m
    PeopsSoftGPU
  
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

#import "PluginGLView.h"
#import "SGPUPreferences.h"
#include "externals.h"
#undef BOOL
#include "gpu.h"
#include "swap.h"

#include <time.h>
extern time_t tStart;

@implementation PluginGLView
{
	GLubyte  *image_base;
	GLubyte  *image[IMAGE_COUNT];
	
	GLboolean useShader;
	float	  shaderQuality;
	GLint     buffers;
	GLuint    vertexShader;
	GLuint    fragmentShader;
	GLuint	  program;
	//GLint     frame_rate;
	
	GLenum    texture_hint;
	GLboolean rect_texture;
	GLboolean client_storage;
	GLboolean texture_range;
	
	struct timeval cycle_time;
	
	BOOL noDisplay;
	BOOL drawBG;
	
	int image_width;
	int image_height;
	int image_width2;
	int image_height2;
	int image_depth;
	int image_type;
	float image_tx;
	float image_ty;
	int whichImage;
	BOOL isFullscreen;
	NSOpenGLPixelFormatAttribute oglProfile;
}
@synthesize glLock;

//- (id)initWithFrame:(NSRect)frameRect
- (id) initWithCoder: (NSCoder *) coder
{
	if ((self = [super initWithCoder:coder]) == nil)
		return nil;
	
	glLock = [[NSLock alloc] init];
	if (nil == glLock) {
		return nil;
	}
	
	if ([self setupOpenGL3]) {
		oglProfile = NSOpenGLProfileVersion3_2Core;
	} else if ([self setupOpenGL2]) {
		oglProfile = NSOpenGLProfileVersionLegacy;
	} else
		return nil;
	
	return self;
}

- (void)dealloc
{
	if (oglProfile == NSOpenGLProfileVersionLegacy) {
		[self cleanupGL2];
	} else if (oglProfile == NSOpenGLProfileVersion3_2Core) {
		[self cleanupGL3];
	}
	
	if (image_base) {
		free(image_base);
	}
}

- (BOOL)isOpaque
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return NO;
}

- (void)drawRect:(NSRect)aRect
{
	// Check if an update has occured to the buffer
	if ([self lockFocusIfCanDraw]) {
		
		// Make this context current
		if (drawBG) {
			[[NSColor blackColor] setFill];
			[NSBezierPath fillRect:[self visibleRect]];
		}
		
		//glFinish() ;
		// Swap buffer to screen
		//[[self openGLContext] flushBuffer];
		
		[self unlockFocus];
	}
}

#if 0
- (void)update  // moved or resized
{
	NSRect rect;

	[super update];

	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] update];

	rect = [self bounds];
	
	glViewport(0, 0, (int) rect.size.width, (int) rect.size.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); 

	//[self setNeedsDisplay:true];
}
#endif

- (void)reshape	// scrolled, moved or resized
{
	[glLock lock];
	
	[super reshape];
	
	if (oglProfile == NSOpenGLProfileVersionLegacy) {
		[self reshapeGL2];
		[self renderScreenGL2];
	} else if (oglProfile == NSOpenGLProfileVersion3_2Core) {
		[self reshapeGL3];
		[self renderScreenGL3];
	}
	
	//[self setNeedsDisplay:true];
	
	[glLock unlock];
}

- (void)renderScreen
{
	if (oglProfile == NSOpenGLProfileVersionLegacy) {
		[self renderScreenGL2];
	} else if (oglProfile == NSOpenGLProfileVersion3_2Core) {
		[self renderScreenGL3];
	}
}

- (void)loadTextures:(GLboolean)first
{
	if (oglProfile == NSOpenGLProfileVersionLegacy) {
		[self loadTexturesGL2:first];
	} else if (oglProfile == NSOpenGLProfileVersion3_2Core) {
		[self loadTexturesGL3:first];
	}
}

- (void)swapBuffer
{
	if (oglProfile == NSOpenGLProfileVersionLegacy) {
		RunOnMainThreadSync(^{
			[self swapBufferGL2];
		});
	} else if (oglProfile == NSOpenGLProfileVersion3_2Core) {
		RunOnMainThreadSync(^{
			[self swapBufferGL3];
		});
	}
}

- (void)clearBuffer:(BOOL)display
{
	if (display == NO) {
		//[[self openGLContext] makeCurrentContext];
		//glClear(GL_COLOR_BUFFER_BIT);
		//[self loadTextures:NO];
	} else {
		noDisplay = YES;
		//[self setNeedsDisplay:true];
	}
}

+ (char*)loadSource:(NSURL *)filename
{
	//Since we're passing Cocoa NSURLs, let's use Cocoa's methods
	if (filename == nil) {
		return NULL;
	}
	
	NSUInteger len;
	NSMutableData *shaderData = [[NSMutableData alloc] initWithContentsOfURL:filename];
	[shaderData appendBytes:"\0" length:1];
	len = [shaderData length];
	char *shaderText = malloc(len);
	[shaderData getBytes:shaderText length:len];
	return shaderText;
}

@end
