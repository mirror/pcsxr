/* NetSfPeopsSPUPluginNamedSlider */

#import <Cocoa/Cocoa.h>
#import "ARCBridge.h"

@interface NamedSlider : NSSlider
{
	NSArray *strings;
	__unsafe_unretained Class pluginClass;
}
@property (retain) NSArray *strings;
@property (unsafe_unretained) Class pluginClass;
@end
