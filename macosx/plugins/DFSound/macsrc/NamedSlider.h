/* NetSfPeopsSPUPluginNamedSlider */

#import <Cocoa/Cocoa.h>

@interface NamedSlider : NSSlider
@property (strong) NSArray *strings;
@property (unsafe_unretained) Class pluginClass;
@end
