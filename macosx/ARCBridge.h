//
//  ARCBridge.h
//  PPMacho
//
//  Created by C.W. Betts on 12/23/12.
//
//

#ifndef PPMacho_ARCBridge_h
#define PPMacho_ARCBridge_h

#if __has_feature(objc_arc)

#define SUPERDEALLOC 
#define RELEASEOBJ(obj) 
#define RETAINOBJ(obj) obj
#define AUTORELEASEOBJ(obj) obj
#define AUTORELEASEOBJNORETURN(obj)
#define BRIDGE(toType, obj) (__bridge toType)(obj)

#else

#define SUPERDEALLOC [super dealloc]
#define RELEASEOBJ(obj) [obj release]
#define RETAINOBJ(obj) [obj retain]
#define AUTORELEASEOBJ(obj) [obj autorelease]
#define AUTORELEASEOBJNORETURN(obj) [obj autorelease]
#define BRIDGE(toType, obj) (toType)obj

#endif

#endif
