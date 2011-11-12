#import "NamedSlider.h"

@implementation NamedSlider

- (void)dealloc
{
	[strings release];
	[super dealloc];
}

- (void)setStrings:(NSArray *)theStrings
{
	[theStrings retain];
	[strings release];
	strings = theStrings;
}

- (NSString *)stringValue
{
	NSInteger index = [self integerValue];

	if (index >= 0 && index < [strings count])
		return [strings objectAtIndex:index];

	return @"(Unknown)";
}

- (void)setIntValue:(int)value
{
	[super setIntValue:value];
	[self sendAction:[self action] to:[self target]];
}

@end
