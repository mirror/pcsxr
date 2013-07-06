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
#define RETAINOBJNORETURN(obj)
#define AUTORELEASEOBJ(obj) obj
#define AUTORELEASEOBJNORETURN(obj)
#define BRIDGE(toType, obj) (__bridge toType)(obj)
#define __arcweak __weak
#define arcweak weak
#define arcstrong strong

#else

#define SUPERDEALLOC [super dealloc]
#define RELEASEOBJ(obj) [obj release]
#define RETAINOBJ(obj) [obj retain]
#define RETAINOBJNORETURN(obj) [obj retain]
#define AUTORELEASEOBJ(obj) [obj autorelease]
#define AUTORELEASEOBJNORETURN(obj) [obj autorelease]
#define BRIDGE(toType, obj) (toType)obj
#define __arcweak
#define arcweak assign
#define arcstrong retain

#endif

#define arcretain arcstrong

#endif
