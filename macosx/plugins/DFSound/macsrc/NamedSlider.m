#import "NamedSlider.h"

@implementation NamedSlider
@synthesize pluginClass;
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

- (void)setIntegerValue:(NSInteger)anInteger
{
	[super setIntegerValue:anInteger];
	[self sendAction:[self action] to:[self target]];
}


@end
