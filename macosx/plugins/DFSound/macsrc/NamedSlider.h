/* NetSfPeopsSPUPluginNamedSlider */

#import <Cocoa/Cocoa.h>

@interface NamedSlider : NSSlider
{
	NSArray *strings;
	Class pluginClass;
}
@property (retain) NSArray *strings;
@property Class pluginClass;
@end
