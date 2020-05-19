/* NetSfPeopsSPUPluginController */

#import <Cocoa/Cocoa.h>
#import "SPUPluginController.h"

#ifdef USEOPENAL
#define PluginController NetSfPeopsSPUALPluginController
#else
#define PluginController NetSfPeopsSPUSDLPluginController
#endif

@interface PluginController : SPUPluginController
@end
