/* ConfigurationController */

#import <Cocoa/Cocoa.h>
#import "PluginController.h"
#import "HotkeyController.h"
#import "PluginList.h"

extern NSString *const memChangeNotifier;
extern NSString *const memCardChangeNumberKey;

@class PcsxrMemCardController;

@interface ConfigurationController : NSWindowController
{
	IBOutlet PluginController *cdromPlugin;
	IBOutlet PluginController *graphicsPlugin;
	IBOutlet PluginController *padPlugin;
	IBOutlet PluginController *soundPlugin;
	IBOutlet PluginController *netPlugin;
	IBOutlet PluginController *sio1Plugin;
	
	IBOutlet PcsxrMemCardController *memCardEdit;

	IBOutlet id noXaAudioCell;
	IBOutlet id sioIrqAlwaysCell;
	IBOutlet id bwMdecCell;
	IBOutlet id autoVTypeCell;
	IBOutlet id vTypePALCell;
	IBOutlet id noCDAudioCell;
	IBOutlet id usesHleCell;
	IBOutlet id usesDynarecCell;
	IBOutlet id consoleOutputCell;
	IBOutlet id spuIrqAlwaysCell;
	IBOutlet id rCountFixCell;
	IBOutlet id vSyncWAFixCell;
	IBOutlet id noFastBootCell;
	IBOutlet id enableNetPlayCell;
	IBOutlet id widescreen;
	
	// Hotkeys
	IBOutlet HotkeyController *hkController;
	IBOutlet NSTabViewItem *hkTab;	
	
	NSMutableDictionary *checkBoxDefaults;
}
- (IBAction)setCheckbox:(id)sender;
- (IBAction)setCheckboxInverse:(id)sender;
- (IBAction)setVideoType:(id)sender;
- (IBAction)mcdChangeClicked:(id)sender;
- (IBAction)mcdNewClicked:(id)sender;

- (NSString *)keyForSender:(id)sender;
+ (void)setMemoryCard:(int)theCard toPath:(NSString *)theFile DEPRECATED_ATTRIBUTE;
+ (void)setMemoryCard:(int)theCard toURL:(NSURL *)theURL;

+ (void)mcdNewClicked:(id)sender;
+ (void)mcdChangeClicked:(id)sender;

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem;

@end
