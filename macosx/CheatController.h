//
//  CheatController.h
//  Pcsxr
//

#import <Cocoa/Cocoa.h>
#import "ARCBridge.h"

@class PcsxrHexadecimalFormatter;

@interface CheatController : NSWindowController <NSWindowDelegate, NSTableViewDelegate>
{
    IBOutlet NSTableView *cheatView;
	
	IBOutlet NSWindow *editCheatWindow;
	IBOutlet NSTableView *editCheatView;
	IBOutlet PcsxrHexadecimalFormatter *addressFormatter;
	IBOutlet PcsxrHexadecimalFormatter *valueFormatter;
	NSMutableArray *tempCheatCodes;
	NSMutableArray *cheats;
}
@property (readwrite, arcretain) NSMutableArray *tempCheatCodes;
@property (readwrite, arcretain) NSMutableArray *cheats;

- (void)refresh;

- (IBAction)saveCheats:(id)sender;
- (IBAction)loadCheats:(id)sender;
- (IBAction)clear:(id)sender;
- (IBAction)editCheat:(id)sender;
- (IBAction)addCheat:(id)sender;
- (IBAction)applyCheats:(id)sender;
- (IBAction)removeCheats:(id)sender;
- (IBAction)changeCheat:(id)sender;

- (IBAction)closeCheatEdit:(id)sender;
- (IBAction)removeCheatValue:(id)sender;
- (IBAction)addCheatValue:(id)sender;

@end
