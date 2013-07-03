//
//  SPUPluginController.h
//  PeopsSPU
//
//  Created by C.W. Betts on 7/2/13.
//
//

#import <Cocoa/Cocoa.h>
#import "NamedSlider.h"

@interface SPUPluginController : NSWindowController
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
