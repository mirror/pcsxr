#import "NamedSlider.h"

@implementation NamedSlider
@synthesize pluginClass;

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.strings = nil;
	
	[super dealloc];
}
#endif

@synthesize strings;

- (NSString *)stringValue
{
	NSInteger index = [self integerValue];

	if (index >= 0 && index < [strings count])
		return [strings objectAtIndex:index];

	if (!pluginClass) {
		return @"(Unknown)";
	} else {
		return [[NSBundle bundleForClass:pluginClass] localizedStringForKey:@"(Unknown)" value:@"" table:nil];
	}
}

- (void)setIntValue:(int)value
{
	[super setIntValue:value];
	[self sendAction:[self action] to:[self target]];
}

@end
