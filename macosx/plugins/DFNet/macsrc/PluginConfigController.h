#define PluginConfigController DFNetPlayPluginConfigController

#import <Cocoa/Cocoa.h>

@interface PluginConfigController : NSWindowController
@property (weak) IBOutlet NSTextField *ipAddress;
@property (weak) IBOutlet NSTextField *portNum;
@property (weak) IBOutlet NSTextField *playerNum;

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;

- (void)loadValues;

@end
