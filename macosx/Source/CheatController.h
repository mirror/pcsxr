//
//  CheatController.h
//  Pcsxr
//

#import <Cocoa/Cocoa.h>
#import "PcsxrHexadecimalFormatter.h"
#include "psxcommon.h"
#include "cheat.h"

@interface PcsxrCheatTempObject : NSObject <NSCopying>
@property (readwrite) uint32_t cheatAddress;
@property (readwrite) uint16_t cheatValue;

- (instancetype)initWithAddress:(uint32_t)add value:(uint16_t)val NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCheatCode:(CheatCode *)theCheat;
@end

@interface PcsxrCheatTemp : NSObject
@property (readwrite, strong) NSMutableArray *cheatValues;
@property (readwrite, strong) NSString *cheatName;
@property (readwrite, getter = isEnabled) BOOL enabled;

- (instancetype)initWithCheat:(Cheat *)theCheat NS_DESIGNATED_INITIALIZER;
@end

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
