/**
 * HotkeyController 
 * Nicolas Pépin-Perreault - npepinpe - 2012
 */

#import <Cocoa/Cocoa.h>

@interface HotkeyController : NSView
{
	NSButton *lastConfigButton;
	NSInteger configInput;
	NSMutableDictionary *hotkeysList;
	NSDictionary *keyNameTable;
    NSMutableDictionary *hotkeyOutlets;
    
    IBOutlet NSTextField *FastForward;
    IBOutlet NSTextField *SaveState;
    IBOutlet NSTextField *LoadState;
    IBOutlet NSTextField *NextState;
    IBOutlet NSTextField *PrevState;
}

@property (assign) NSInteger configInput;

- (void) initialize;
- (BOOL) handleMouseDown:(NSEvent *)mouseEvent;
- (IBAction) hotkeySet:(id)sender;
- (void) hotkeyCancel;

@end
