/* NetSfPeopsSPUPluginNamedSlider */

#import <Cocoa/Cocoa.h>

@interface NamedSlider : NSSlider
{
	NSArray *strings;
	__unsafe_unretained Class pluginClass;
}
@property (strong) NSArray *strings;
@property (unsafe_unretained) Class pluginClass;
@end
