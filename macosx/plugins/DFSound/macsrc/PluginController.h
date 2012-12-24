/* NetSfPeopsSPUPluginController */

#import <Cocoa/Cocoa.h>
#import "NamedSlider.h"

#ifdef USEOPENAL
#define PluginController NetSfPeopsSPUALPluginController
#else
#define PluginController NetSfPeopsSPUSDLPluginController
#endif

@interface PluginController : NSWindowController
{
    IBOutlet NSControl *hiCompBox;
    IBOutlet NamedSlider *interpolValue;
    IBOutlet NSControl *irqWaitBox;
    IBOutlet NSControl *monoSoundBox;
    IBOutlet NamedSlider *reverbValue;
    IBOutlet NSControl *xaEnableBox;
    IBOutlet NSControl *xaSpeedBox;
    IBOutlet NamedSlider *volumeValue;

	NSMutableDictionary *keyValues;
}
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;

- (void)loadValues;
@end
