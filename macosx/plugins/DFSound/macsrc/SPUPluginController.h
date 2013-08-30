//
//  SPUPluginController.h
//  PeopsSPU
//
//  Created by C.W. Betts on 7/2/13.
//
//

#import <Cocoa/Cocoa.h>
#import "NamedSlider.h"
#import "ARCBridge.h"

@interface SPUPluginController : NSWindowController
{
    IBOutlet NSCell *hiCompBox;
    IBOutlet NamedSlider *interpolValue;
    IBOutlet NSCell *irqWaitBox;
    IBOutlet NSCell *monoSoundBox;
    IBOutlet NamedSlider *reverbValue;
    IBOutlet NSCell *xaEnableBox;
    IBOutlet NSCell *xaSpeedBox;
    IBOutlet NamedSlider *volumeValue;
	
	NSMutableDictionary *keyValues;
}
@property (readwrite, arcretain) NSMutableDictionary *keyValues;
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;

- (void)loadValues;
@end
