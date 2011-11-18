#define PluginConfigController DFNetPlayPluginConfigController

#import <Cocoa/Cocoa.h>

@interface PluginConfigController : NSWindowController
{
	IBOutlet NSTextField *ipAddress;
	IBOutlet NSTextField *portNum;
	IBOutlet NSTextField *playerNum;
}
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;

- (void)loadValues;

@end
