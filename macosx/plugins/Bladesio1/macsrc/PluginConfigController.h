#define PluginConfigController Bladesio1PluginConfigController

#import <Cocoa/Cocoa.h>

@interface Bladesio1PluginConfigController : NSWindowController
{
	IBOutlet NSButton *enabledButton;
	IBOutlet NSTextField *ipAddressField;
	IBOutlet NSTextField *portField;
	IBOutlet NSPopUpButton *playerMenu;
	IBOutlet NSBox *configBox;

	NSMutableDictionary *keyValues;
}
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)toggleEnabled:(id)sender;
- (IBAction)resetPreferences:(id)sender;

- (void)loadValues;

@end
