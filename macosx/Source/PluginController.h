/* PluginController */

#import <Cocoa/Cocoa.h>
#import "PluginList.h"

@interface PluginController : NSObject

@property (weak) IBOutlet NSButton *aboutButton;
@property (weak) IBOutlet NSButton *configureButton;
@property (weak) IBOutlet NSPopUpButton *pluginMenu;

- (IBAction)doAbout:(id)sender;
- (IBAction)doConfigure:(id)sender;
- (IBAction)selectPlugin:(id)sender;

- (void)setPluginsTo:(NSArray *)list withType:(int)type;

@end
