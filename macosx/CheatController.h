//
//  CheatController.h
//  Pcsxr
//

#import <Cocoa/Cocoa.h>

@interface CheatController : NSWindowController <NSWindowDelegate, NSTableViewDataSource, NSTableViewDelegate>
{
    IBOutlet NSTableView *cheatView;
	
	IBOutlet NSWindow *editCheatWindow;
	IBOutlet NSTableView *editCheatView;
	IBOutlet NSNumberFormatter *addressFormatter;
	IBOutlet NSNumberFormatter *valueFormatter;
	NSMutableArray *tempCheatCodes;
}
@property (readwrite, retain) NSMutableArray *tempCheatCodes;

- (void)refresh;

- (IBAction)saveCheats:(id)sender;
- (IBAction)loadCheats:(id)sender;
- (IBAction)clear:(id)sender;
- (IBAction)editCheat:(id)sender;
- (IBAction)addCheat:(id)sender;

- (IBAction)closeCheatEdit:(id)sender;
- (IBAction)removeCheatValue:(id)sender;
- (IBAction)addCheatValue:(id)sender;

@end
