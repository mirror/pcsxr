//
//  PgxpController.h
//  Pcsxr
//
//  Created by MrLavender on 29/06/2017.
//

#import <Cocoa/Cocoa.h>

@interface PgxpController : NSViewController

@property (weak) IBOutlet NSButton *vertexCreation;
@property (weak) IBOutlet NSButton *vertexCaching;
@property (weak) IBOutlet NSButton *perspectiveCorrect;
@property (weak) IBOutlet NSPopUpButton *pgxpModeButton;
@property (weak) IBOutlet NSTextField *pgxpModeLabel;

- (IBAction)onOptionChange:(NSButton*)sender;
- (IBAction)onModeChange:(NSPopUpButton*)sender;

+ (void)loadPgxpSettings;
+ (void)savePgxpSettings;

@end
