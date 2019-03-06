#define PluginConfigController DFCdromPluginConfigController

#import <Cocoa/Cocoa.h>

@interface PluginConfigController : NSWindowController
@property (weak) IBOutlet NSControl *Cached;
@property (weak) IBOutlet NSSlider *CacheSize;
@property (weak) IBOutlet NSPopUpButton *CdSpeed;
@property (strong) NSMutableDictionary *keyValues;

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;

- (void)loadValues;

@end
