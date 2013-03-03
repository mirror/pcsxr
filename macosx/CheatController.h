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

-(NSInteger)numberOfRowsInTableView:(NSTableView *)view;
-(id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)col row:(NSInteger)idx;
-(void)tableView:(NSTableView *)tableView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row;
@end
