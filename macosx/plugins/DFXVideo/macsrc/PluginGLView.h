/***************************************************************************
    PluginGLView.h
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

#define PluginGLView NetSfPeopsSoftGPUPluginGLView

#import <Cocoa/Cocoa.h>
#include <OpenGL/gltypes.h>
#include <sys/time.h>

#define IMAGE_COUNT 2

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

@interface PluginGLView : NSOpenGLView

@property (readonly, strong) NSLock *glLock;

- (void)renderScreen;
- (void)swapBuffer;
- (void)clearBuffer:(BOOL)display;
- (void)loadTextures:(GLboolean)first;
+ (char*)loadSource:(NSURL *)filename;
void printProgramInfoLog(GLuint obj);
- (GLuint)loadShader:(GLenum)type location:(NSURL*)filename;

@end
