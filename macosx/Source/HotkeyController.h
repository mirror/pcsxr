/**
 * HotkeyController 
 * Nicolas Pépin-Perreault - npepinpe - 2012
 */

#import <Cocoa/Cocoa.h>

@interface HotkeyController : NSView

@property (weak) IBOutlet NSTextField *FastForward;
@property (weak) IBOutlet NSTextField *SaveState;
@property (weak) IBOutlet NSTextField *LoadState;
@property (weak) IBOutlet NSTextField *NextState;
@property (weak) IBOutlet NSTextField *PrevState;
@property (weak) IBOutlet NSTextField *FrameLimit;


@property NSInteger configInput;

- (void) initialize;
- (BOOL) handleMouseDown:(NSEvent *)mouseEvent;
- (IBAction) hotkeySet:(id)sender;
- (void) hotkeyCancel;

@end
