//
//  SGPUPreferences.h
//  PeopsSoftGPU
//
//  Created by C.W. Betts on 9/16/12.
//
//

#ifndef PeopsSoftGPU_SGPUPreferences_h
#define PeopsSoftGPU_SGPUPreferences_h

#import <Cocoa/Cocoa.h>

BOOL isShaderEnabled();
NSURL *PSXFragmentShader();
NSURL *PSXVertexShader();
float PSXShaderQuality();

#endif
