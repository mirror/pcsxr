/* NetSfPeopsSPUPluginController */

#import <Cocoa/Cocoa.h>
#import "NamedSlider.h"

#ifdef USEOPENAL
#define PluginController NetSfPeopsALPluginController
#else
#define PluginController NetSfPeopsSPUPluginController
#endif

@interface PluginController : NSWindowController
{
    IBOutlet NSControl *hiCompBox;
    IBOutlet NetSfPeopsSPUPluginNamedSlider *interpolValue;
    IBOutlet NSControl *irqWaitBox;
    IBOutlet NSControl *monoSoundBox;
    IBOutlet NetSfPeopsSPUPluginNamedSlider *reverbValue;
    IBOutlet NSControl *xaEnableBox;
    IBOutlet NSControl *xaSpeedBox;
    IBOutlet NetSfPeopsSPUPluginNamedSlider *volumeValue;

	NSMutableDictionary *keyValues;
}
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;

- (void)loadValues;
@end
