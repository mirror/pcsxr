/**
 *  HotkeyController.m
 *  Pcsxr
 *
 *  Created by Nicolas Pepin-Perreault on 12-12-10.
 *
 * Adapted from the Cocoa port of DeSMuMe
 */

#import "HotkeyController.h"

#define INPUT_HOLD_TIME		0.1

@interface HotkeyController ()
@property (strong) NSButton *lastConfigButton;
@property (strong) NSMutableDictionary *hotkeysList;
@property (strong) NSDictionary *keyNameTable;
@property (strong) NSMutableDictionary *hotkeyOutlets;
@end

@implementation HotkeyController

@synthesize FastForward;
@synthesize FrameLimit;
@synthesize LoadState;
@synthesize NextState;
@synthesize PrevState;
@synthesize SaveState;

- (void)initialize
{
	self.lastConfigButton = nil;
	self.configInput = 0;
	self.hotkeysList = [[NSMutableDictionary alloc] initWithCapacity:16];
    self.keyNameTable = [[NSDictionary alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KeyNames" ofType:@"plist"]];
    self.hotkeyOutlets = [[NSMutableDictionary alloc] initWithCapacity:8];
    
    [self mapOutletToIdentifier:FastForward forIdentifier:@"FastForward"];
	[self mapOutletToIdentifier:SaveState forIdentifier:@"SaveState"];
    [self mapOutletToIdentifier:LoadState forIdentifier:@"LoadState"];
    [self mapOutletToIdentifier:NextState forIdentifier:@"NextState"];
    [self mapOutletToIdentifier:PrevState forIdentifier:@"PrevState"];
    [self mapOutletToIdentifier:FrameLimit forIdentifier:@"FrameLimit"];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)mapOutletToIdentifier:(id)outlet forIdentifier:(NSString*)identifier1
{
    (self.hotkeyOutlets)[identifier1] = outlet;
    [self setHotkeyDisplay:identifier1];
}

- (void)setHotkeyDisplay:(NSString*)keyIdent
{
    NSString *label = [self parseMappingDisplayString:keyIdent];
    NSTextField *displayField = (self.hotkeyOutlets)[keyIdent];
    
    if(displayField) {
        [displayField setStringValue:label];
    }
}

- (void)mouseDown:(NSEvent *)theEvent
{
	BOOL isHandled = [self handleMouseDown:theEvent];
	if (!isHandled)
	{
		[super mouseDown:theEvent];
	}
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	[self mouseDown:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	BOOL isHandled = [self handleMouseDown:theEvent];
	if (!isHandled)
	{
		[super rightMouseDown:theEvent];
	}
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	[self rightMouseDown:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	BOOL isHandled = [self handleMouseDown:theEvent];
	if (!isHandled)
	{
		[super otherMouseDown:theEvent];
	}
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
	[self otherMouseDown:theEvent];
}

- (BOOL) handleMouseDown:(NSEvent *)mouseEvent
{
	if (self.configInput != 0)
	{
		[self hotkeyCancel];
	}
	
	return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
	NSString *keyCode = [NSString stringWithFormat:@"%d", [theEvent keyCode]];
	NSString *keyLabel = (NSString *) (self.keyNameTable)[keyCode];
	
	if (self.configInput != 0)
	{
		// Save input
        NSString *ident = [self.lastConfigButton identifier];
        [self saveHotkey:ident device:@"NSEventKeyboard" deviceLabel:@"Keyboard" code:keyCode label:keyLabel];
        [self setHotkeyDisplay:ident];
		[self hotkeyCancel];
	}
}

- (void)saveHotkey:(NSString*)keyIdent device:(NSString*)device deviceLabel:(NSString*)deviceLabel code:(NSString*)keyCode label:(NSString*)keyLabel
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults] ;
	NSMutableDictionary *tempUserMappings = [NSMutableDictionary dictionaryWithDictionary:[defaults dictionaryForKey:@"HotkeyBindings"]];
	[tempUserMappings setValue:@{@"device": device,
								 @"deviceName": deviceLabel,
								 @"keyCode": keyCode,
								 @"keyLabel": keyLabel} forKey:keyIdent];
	[defaults setValue:tempUserMappings forKey:@"HotkeyBindings"];
}

- (NSString *) parseMappingDisplayString:(NSString *)keyString
{
	NSDictionary *userMappings = [[NSUserDefaults standardUserDefaults] dictionaryForKey:@"HotkeyBindings"];
	NSDictionary *binding = (NSDictionary *)[userMappings valueForKey:keyString];
	
    NSString *displayString = @"";
    if(binding) {
        NSString *deviceLabel = (NSString *)[binding valueForKey:@"deviceName"];
        NSString *keyLabel = (NSString *)[binding valueForKey:@"keyLabel"];
    
        displayString = [NSString stringWithString:deviceLabel];
        displayString = [displayString stringByAppendingString:@": "];
        displayString = [displayString stringByAppendingString:keyLabel];
	}
    
	return displayString;
}

- (IBAction) hotkeySet:(id)sender
{
	NSButton *theButton = (NSButton *)sender;
	
	if (self.configInput && self.lastConfigButton != theButton)
	{
		[self.lastConfigButton setState:NSOffState];
	}
	
	if ([theButton state] == NSOnState)
	{
		self.lastConfigButton = theButton;
		[self.hotkeysList removeAllObjects];
		self.configInput = [theButton tag];
	}
	else
	{
		[self hotkeyCancel];
	}
    
}

- (void) hotkeyCancel
{
	if (self.lastConfigButton != nil)
	{
		[self.lastConfigButton setState:NSOffState];
		self.lastConfigButton = nil;
	}
	
	self.configInput = 0;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}

@end
