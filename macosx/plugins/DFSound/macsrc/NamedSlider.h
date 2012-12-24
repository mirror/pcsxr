/* NetSfPeopsSPUPluginNamedSlider */

#import <Cocoa/Cocoa.h>

#ifdef USEOPENAL
#define NamedSlider NetSfPeopsSPUALPluginNamedSlider
#else
#define NamedSlider NetSfPeopsSPUSDLPluginNamedSlider
#endif

@interface NamedSlider : NSSlider
{
	NSArray *strings;
}

- (void)setStrings:(NSArray *)theStrings;
@end
