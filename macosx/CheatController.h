//
//  CheatController.h
//  Pcsxr
//

#import <Cocoa/Cocoa.h>

@class PcsxrHexadecimalFormatter;

@interface CheatController : NSWindowController <NSWindowDelegate, NSTableViewDelegate>

@property (weak) IBOutlet NSTableView *cheatView;
@property (weak) IBOutlet NSWindow *editCheatWindow;
@property (weak) IBOutlet NSTableView *editCheatView;
@property (weak) IBOutlet PcsxrHexadecimalFormatter *addressFormatter;
@property (weak) IBOutlet PcsxrHexadecimalFormatter *valueFormatter;

@property (readwrite, strong) NSMutableArray *tempCheatCodes;
@property (readwrite, strong) NSMutableArray *cheats;

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
