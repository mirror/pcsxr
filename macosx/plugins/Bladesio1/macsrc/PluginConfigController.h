#define PluginConfigController Bladesio1PluginConfigController

#import <Cocoa/Cocoa.h>

@interface Bladesio1PluginConfigController : NSWindowController
@property (weak) IBOutlet NSButton *enabledButton;
@property (weak) IBOutlet NSTextField *ipAddressField;
@property (weak) IBOutlet NSTextField *portField;
@property (weak) IBOutlet NSPopUpButton *playerMenu;
@property (weak) IBOutlet NSBox *configBox;

@property (strong) NSMutableDictionary *keyValues;

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)toggleEnabled:(id)sender;
- (IBAction)resetPreferences:(id)sender;

- (void)loadValues;

@end
