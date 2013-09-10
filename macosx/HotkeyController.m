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

@implementation HotkeyController

@synthesize configInput;

- (void)initialize
{
	lastConfigButton = nil;
	configInput = 0;
	hotkeysList = [[NSMutableDictionary alloc] initWithCapacity:16];
    keyNameTable = [[NSDictionary alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KeyNames" ofType:@"plist"]];
    hotkeyOutlets = [[NSMutableDictionary alloc] initWithCapacity:8];
    
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

- (void)mapOutletToIdentifier:(id)outlet forIdentifier:(NSString*)identifier
{
    [hotkeyOutlets setObject:outlet forKey:identifier];
    [self setHotkeyDisplay:identifier];
}

- (void)setHotkeyDisplay:(NSString*)keyIdent
{
    NSString *label = [self parseMappingDisplayString:keyIdent];
    NSTextField *displayField = [hotkeyOutlets objectForKey:keyIdent];
    
    if(displayField) {
        [[displayField cell] setStringValue:label];
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
	if (configInput != 0)
	{
		[self hotkeyCancel];
	}
	
	return YES;
}

- (void)keyDown:(NSEvent *)theEvent
{
	NSString *keyCode = [NSString stringWithFormat:@"%d", [theEvent keyCode]];
	NSString *keyLabel = (NSString *)[keyNameTable valueForKey:keyCode];
	
	if (configInput != 0)
	{
		// Save input
        NSString *ident = [lastConfigButton identifier];
        [self saveHotkey:ident device:@"NSEventKeyboard" deviceLabel:@"Keyboard" code:keyCode label:keyLabel];
        [self setHotkeyDisplay:ident];
		[self hotkeyCancel];
	}
}

- (void)saveHotkey:(NSString*)keyIdent device:(NSString*)device deviceLabel:(NSString*)deviceLabel code:(NSString*)keyCode label:(NSString*)keyLabel
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults] ;
	NSMutableDictionary *tempUserMappings = [NSMutableDictionary dictionaryWithDictionary:[defaults dictionaryForKey:@"HotkeyBindings"]];
	[tempUserMappings setValue:[NSDictionary dictionaryWithObjectsAndKeys:
        device, @"device",
        deviceLabel, @"deviceName",
        keyCode, @"keyCode",
        keyLabel, @"keyLabel",
        nil] forKey:keyIdent];
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
	
	if (configInput && lastConfigButton != theButton)
	{
		[lastConfigButton setState:NSOffState];
	}
	
	if ([theButton state] == NSOnState)
	{
		lastConfigButton = theButton;
		[hotkeysList removeAllObjects];
		configInput = [theButton tag];
	}
	else
	{
		[self hotkeyCancel];
	}
    
}

- (void) hotkeyCancel
{
	if (lastConfigButton != nil)
	{
		[lastConfigButton setState:NSOffState];
		lastConfigButton = nil;
	}
	
	configInput = 0;
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
