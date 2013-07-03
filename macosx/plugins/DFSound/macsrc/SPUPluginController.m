//
//  SPUPluginController.m
//  PeopsSPU
//
//  Created by C.W. Betts on 7/2/13.
//
//

#import "SPUPluginController.h"

@implementation SPUPluginController

static inline void FuncNotAvailable(id sel, id sender, SEL theCmd)
{
#ifdef DEBUG
	NSLog(@"Class %@ does not implement %@, and was sent a(n) %@ with the description %@", [sel class], NSStringFromSelector(theCmd), [sender class], [sender description]);
	if ([sel isMemberOfClass:[SPUPluginController class]]) {
		NSLog(@"For one thing, the class %@ isn't supposed to be accessed directly, just subclassed!", [SPUPluginController class]); \
	} else {
		NSLog(@"You should implement %@ for your class %@. As it is, you are calling %@ from the superclass %@.", NSStringFromSelector(theCmd), [sel class], NSStringFromSelector(theCmd), [SPUPluginController class]);
	}
#endif
	[sel doesNotRecognizeSelector:theCmd];
}

#define NotAvailableWarn() FuncNotAvailable(self, sender, _cmd)

- (IBAction)cancel:(id)sender
{
	NotAvailableWarn();
	
}

- (IBAction)ok:(id)sender
{
	NotAvailableWarn();
}

- (IBAction)reset:(id)sender
{
	NotAvailableWarn();
}

- (void)loadValues
{
#ifdef DEBUG
	NSLog(@"Class %@ does not implement %@", [self class], NSStringFromSelector(_cmd));
	if ([self isMemberOfClass:[SPUPluginController class]]) {
		NSLog(@"For one thing, the class %@ isn't supposed to be accessed directly, just subclassed!", [SPUPluginController class]);
	} else {
		NSLog(@"You should implement %@ for your class %@. As it is, you are calling %@ from the superclass %@.", NSStringFromSelector(_cmd), [self class], NSStringFromSelector(_cmd), [SPUPluginController class]);
	}
#endif
	[self doesNotRecognizeSelector:_cmd];
}

@end
