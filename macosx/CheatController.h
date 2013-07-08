//
//  CheatController.h
//  Pcsxr
//

#import <Cocoa/Cocoa.h>

@interface CheatController : NSWindowController <NSWindowDelegate>
{
    IBOutlet NSTableView *cheatView;
}

-(IBAction)SaveCheats:(id)sender;
-(IBAction)LoadCheats:(id)sender;
-(IBAction)clear:(id)sender;
-(IBAction)close:(id)sender;
@end
