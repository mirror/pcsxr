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
@property (weak) IBOutlet NSCell *hiCompBox;
@property (weak) IBOutlet NamedSlider *interpolValue;
@property (weak) IBOutlet NSCell *irqWaitBox;
@property (weak) IBOutlet NSCell *monoSoundBox;
@property (weak) IBOutlet NamedSlider *reverbValue;
@property (weak) IBOutlet NSCell *xaEnableBox;
@property (weak) IBOutlet NSCell *xaSpeedBox;
@property (weak) IBOutlet NamedSlider *volumeValue;
@property (readwrite, strong) NSMutableDictionary *keyValues;

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;

- (void)loadValues;
@end
