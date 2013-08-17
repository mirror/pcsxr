//
//  CheatController.h
//  Pcsxr
//

#import <Cocoa/Cocoa.h>
#import "ARCBridge.h"
#include "psxcommon.h"
#include "cheat.h"

@class PcsxrHexadecimalFormatter;

@interface PcsxrCheatTempObject : NSObject <NSCopying>
{
	uint32_t address;
	uint16_t value;
}
@property (readwrite) uint32_t address;
@property (readwrite, arcweak) NSNumber* addressNS;
@property (readwrite) uint16_t value;
@property (readwrite, arcweak) NSNumber* valueNS;

- (id)initWithAddress:(uint32_t)add value:(uint16_t)val;
- (id)initWithCheatCode:(CheatCode *)theCheat;
@end

@interface PcsxrCheatTemp : NSObject
{
	NSMutableArray *cheatValues;
	NSString *cheatName;
	BOOL enabled;
}
@property (readwrite, retain) NSMutableArray *cheatValues;
@property (readwrite, retain, nonatomic) NSString *cheatName;
@property (readwrite, getter = isEnabled) BOOL enabled;

- (id)initWithCheat:(Cheat *)theCheat;
@end

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
@property (readwrite, retain) NSMutableArray *tempCheatCodes;
@property (readwrite, retain) NSMutableArray *cheats;

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
