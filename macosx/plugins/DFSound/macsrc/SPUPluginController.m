//
//  SPUPluginController.m
//  PeopsSPU
//
//  Created by C.W. Betts on 7/2/13.
//
//

#import "SPUPluginController.h"

#ifdef DEBUG
static inline Class GetSPUBaseClass()
{
	static Class spuBaseClass;
	if (!spuBaseClass) {
		spuBaseClass = [SPUPluginController class];
	}
	return spuBaseClass;
}
#endif

static void FuncNotAvailable(id sel, id sender, SEL theCmd)
{
#ifdef DEBUG
	NSString *selString = NSStringFromSelector(theCmd);
	if (sender) {
		NSLog(@"Class %@ does not implement %@, and was sent a(n) %@ with the description %@", [sel class], selString, [sender class], [sender description]);
	} else {
		NSLog(@"Class %@ does not implement %@", [sel class], selString);
	}
	if ([sel class] == GetSPUBaseClass()) {
		NSLog(@"For one thing, the class %@ isn't supposed to be accessed directly, just subclassed!", GetSPUBaseClass()); \
	} else {
		NSLog(@"You should implement %@ for your class %@. As it is, you are calling %@ from the superclass %@.", selString, [sel class], selString, GetSPUBaseClass());
	}
#endif
	[sel doesNotRecognizeSelector:theCmd];
}

#define NotAvailableWarn(sender) FuncNotAvailable(self, sender, _cmd)

@implementation SPUPluginController
@synthesize keyValues;

- (IBAction)cancel:(id)sender
{
	NotAvailableWarn(sender);
}

- (IBAction)ok:(id)sender
{
	NotAvailableWarn(sender);
}

- (IBAction)reset:(id)sender
{
	NotAvailableWarn(sender);
}

- (void)loadValues
{
	NotAvailableWarn(nil);
}

@end
