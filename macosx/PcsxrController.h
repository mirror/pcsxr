/* PcsxrController */

#import <Cocoa/Cocoa.h>
#import "EmuThread.h"
#import "PluginList.h"
#import "RecentItemsMenu.h"

@class ConfigurationController;
@class CheatController;

@interface PcsxrController : NSObject <NSApplicationDelegate>
{
    ConfigurationController *preferencesController;
    CheatController *cheatController;
    PluginList *pluginList;
    
    IBOutlet NSWindow *preferenceWindow;
    IBOutlet NSWindow *cheatWindow;
    IBOutlet RecentItemsMenu *recentItems;
    
    BOOL sleepInBackground;
    BOOL wasPausedBeforeBGSwitch;
}
@property (readonly) RecentItemsMenu *recentItems;

- (IBAction)ejectCD:(id)sender;
- (IBAction)pause:(id)sender;
- (IBAction)showCheatsWindow:(id)sender;
- (IBAction)preferences:(id)sender;
- (IBAction)reset:(id)sender;
- (IBAction)runCD:(id)sender;
- (IBAction)runIso:(id)sender;
- (IBAction)runBios:(id)sender;
- (IBAction)freeze:(id)sender;
- (IBAction)defrost:(id)sender;
- (IBAction)fullscreen:(id)sender;
- (IBAction)pauseInBackground:(id)sender;
- (void)runURL:(NSURL*)url;

+ (void)setConfigFromDefaults;
+ (void)setDefaultFromConfig:(NSString *)defaultKey;
+ (BOOL)biosAvailable;
+ (NSString*)saveStatePath:(int)slot;
+ (void)saveState:(int)num;
+ (void)loadState:(int)num;

@end
