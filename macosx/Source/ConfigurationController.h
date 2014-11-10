/* ConfigurationController */

#import <Cocoa/Cocoa.h>
#import "PluginController.h"
#import "HotkeyController.h"
#import "PluginList.h"

extern NSString *const memChangeNotifier;
extern NSString *const memCardChangeNumberKey;

@class PcsxrMemCardController;

@interface ConfigurationController : NSWindowController <NSWindowDelegate, NSTabViewDelegate>
@property (weak) IBOutlet PluginController *cdromPlugin;
@property (weak) IBOutlet PluginController *graphicsPlugin;
@property (weak) IBOutlet PluginController *padPlugin;
@property (weak) IBOutlet PluginController *soundPlugin;
@property (weak) IBOutlet PluginController *netPlugin;
@property (weak) IBOutlet PluginController *sio1Plugin;

@property (weak) IBOutlet PcsxrMemCardController *memCardEdit;

// Hotkeys
@property (weak) IBOutlet HotkeyController *hkController;
@property (weak) IBOutlet NSTabViewItem *hkTab;

@property (weak) IBOutlet NSButtonCell *noXaAudioCell;
@property (weak) IBOutlet NSButtonCell *sioIrqAlwaysCell;
@property (weak) IBOutlet NSButtonCell *bwMdecCell;
@property (weak) IBOutlet NSButtonCell *autoVTypeCell;
@property (weak) IBOutlet NSPopUpButton *vTypePALCell;
@property (weak) IBOutlet NSButtonCell *noCDAudioCell;
@property (weak) IBOutlet NSButtonCell *usesHleCell;
@property (weak) IBOutlet NSButtonCell *usesDynarecCell;
@property (weak) IBOutlet NSButtonCell *consoleOutputCell;
@property (weak) IBOutlet NSButtonCell *spuIrqAlwaysCell;
@property (weak) IBOutlet NSButtonCell *rCountFixCell;
@property (weak) IBOutlet NSButtonCell *vSyncWAFixCell;
@property (weak) IBOutlet NSButtonCell *noFastBootCell;
@property (weak) IBOutlet NSButtonCell *enableNetPlayCell;
@property (weak) IBOutlet NSButtonCell *widescreen;

- (IBAction)setCheckbox:(id)sender;
- (IBAction)setCheckboxInverse:(id)sender;
- (IBAction)setVideoType:(id)sender;

+ (void)setMemoryCard:(NSInteger)theCard toPath:(NSString *)theFile;
+ (void)setMemoryCard:(NSInteger)theCard toURL:(NSURL *)theURL;

- (IBAction)mcdNewClicked:(id)sender;
- (IBAction)mcdChangeClicked:(id)sender;

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem;

@end
