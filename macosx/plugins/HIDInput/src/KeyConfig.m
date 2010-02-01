/***************************************************************************
    KeyConfig.h
    HIDInput
  
    Created by Gil Pedersen on Sat May 29 2004.
    Copyright (c) 2004 Gil Pedersen.
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#import "KeyConfig.h"
#include "PlugPAD.h"


/*
	<Set 1>  -  [Player 1]  -  [Current Type]
									-  <Type 1 Keys>
									-  <Type 2 Keys>
									-  ...
									
				-  [Player 2]  -  ...
				
	<Set 2>  -  ...
	...
*/

static KeyConfig *sKeyconfig;

int LoadConfig()
{
	if (!sKeyconfig)
		sKeyconfig = [[KeyConfig alloc] init];

	return 0;
}

// get name of element for display in window;
// try names first then default to more generic derived names if device does not provide explicit names
static void GetDeviceElementNameString(pRecDevice pDevice, pRecElement pElement, char * cstr)
{
	char cstrElement[256] = "----", cstrDevice[256] = "----";

	if (!HIDIsValidElement(pDevice, pElement))
		return;

	if (HIDGetElementNameFromVendorProductUsage (pDevice->vendorID, pDevice->productID, pElement->usagePage, pElement->usage, cstr))
		return;

	if (*(pDevice->product))
		BlockMoveData(pDevice->product, cstrDevice, 256);
	else
	{
		HIDGetUsageName(pDevice->usagePage, pDevice->usage, cstrDevice);
		if (!*cstrDevice) // if usage
			sprintf(cstrDevice, "Device");
	}

	if (*(pElement->name))
		BlockMoveData(pElement->name, cstrElement, 256);
	else // if no name
	{
		HIDGetUsageName(pElement->usagePage, pElement->usage, cstrElement);
		if (!*cstrElement) // if not usage
			sprintf(cstrElement, "Element");
	}
	sprintf(cstr, "%s: %s", cstrDevice, cstrElement);
}

@implementation KeyConfig

- (id)init
{
	if ((self = [super init]) == nil)
		return nil;

	/* Using the defaults system means that we will effectively piggy-back on
		the parent process' preferences. This behaviour is ok, since it allows
		for seperate preferences for each application that will use the plugin */
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
			@"Default", @"net.pcsx.HIDInputPlugin Current Set",
			[[NSMutableDictionary alloc] initWithCapacity:1], @"net.pcsx.HIDInputPlugin Pad Settings",
			nil]];

	/* load from preferences */
	keyValues = [[defaults dictionaryForKey:@"net.pcsx.HIDInputPlugin Pad Settings"] retain];

	list = [[ControllerList alloc] initWithConfig:self];

	[self setCurrentSet:[self currentSet]];
	[self updateKeys];

	return self;
}

- (void)dealloc
{
	[keyValues release];
	[list release];
}

+ (KeyConfig *)current
{
	if (!sKeyconfig)
		LoadConfig();
	
	return sKeyconfig;
}

- (ControllerList *)controllerList
{
	return list;
}

#if 0
- (void)setDefaultMappingsWithType:(NSString *)type device:(NSString *)device player:(int)player
{
	int len = [list elementCountForType:type];
	int i;
	
	for (i=0; i<len; i++) {
		NSArray *mappings = [list defaultMappingsAtIndex:i type:type];
		
	}
}

- (void)setDefaultMappingsInSet:(NSString *)name
{
	NSArray *devices = [self systemDeviceList];
	
	/* assign all keyboards to player 1 */
	//if (
	
	/* assign the first joypad to player 1 */
	
	/* assign second joypad to player 2 */
}
#endif

- (void)setCurrentSet:(NSString *)name
{
	[[NSUserDefaults standardUserDefaults] setObject:name forKey:@"net.pcsx.HIDInputPlugin Current Set"];
	
	/* create the entry if neccesary */
	NSDictionary *dict = [keyValues objectForKey:name];
	if (nil == dict) {
		dict = [NSMutableDictionary dictionaryWithCapacity:2];
		if (![keyValues respondsToSelector:@selector(setObject)]) {
			[keyValues autorelease];
			keyValues = [[NSMutableDictionary alloc] initWithDictionary:keyValues];
		}
		[(NSMutableDictionary *)keyValues setObject:dict forKey:name];
	}
}

- (NSString *)currentSet
{
	NSString *set = [[NSUserDefaults standardUserDefaults] stringForKey:@"net.pcsx.HIDInputPlugin Current Set"];
	if (nil == set) {
		//[self setDefaultMappingsInSet:@"Default"];
		return @"Default";
	}
	
	return set;
}

- (NSDictionary *)players
{
	return [keyValues objectForKey:[self currentSet]];
}
- (NSMutableDictionary *)mutablePlayers
{
	NSDictionary *players = [self players];
	
	if (![players respondsToSelector:@selector(setObject)]) {
		players = [NSMutableDictionary dictionaryWithDictionary:players];
		if (![keyValues respondsToSelector:@selector(setObject)]) {
			[keyValues autorelease];
			keyValues = [[NSMutableDictionary alloc] initWithDictionary:keyValues];
		}
		[(NSMutableDictionary *)keyValues setObject:players forKey:[self currentSet]];
	}
	
	return (NSMutableDictionary *)players;
}


- (NSDictionary *)typesForPlayer:(int)player
{
	NSDictionary *types = [[self players] objectForKey:[NSString stringWithFormat:@"Player %i", player+1]];
	if (nil == types)
		return [NSDictionary dictionary];
	
	return types;
}
- (NSMutableDictionary *)mutableTypesForPlayer:(int)player
{
	NSDictionary *types = [self typesForPlayer:player];
	
	if (![types respondsToSelector:@selector(setObject)]) {
		types = [NSMutableDictionary dictionaryWithDictionary:types];
		[[self mutablePlayers] setObject:types forKey:[NSString stringWithFormat:@"Player %i", player+1]];
	}
	
	return (NSMutableDictionary *)types;
}

- (void)setCurrentType:(NSString *)type player:(int)player
{
/*	NSString *playerKey = [NSString stringWithFormat:@"Player %i", player+1];
	NSDictionary *types = [[self players] objectForKey:playerKey];
	if (nil == types) {
		types = [NSMutableDictionary dictionaryWithCapacity:2];
		[[self mutablePlayers] setObject:types forKey:playerKey];
	}*/

	if (![type isEqualToString:[self currentTypeForPlayer:player]]) {
		NSMutableDictionary *types = [self mutableTypesForPlayer:player];

		/* set the type */
		[types setObject:type forKey:@"Current Type"];
	
		/* make sure its dictionary is created */
		NSDictionary *keys = [types objectForKey:type];
		if (nil == keys) {
			keys = [NSMutableDictionary dictionaryWithCapacity:1];
			[types setObject:keys forKey:type];
		}
	}
}

- (NSString *)currentTypeForPlayer:(int)player
{
	return [[self typesForPlayer:player] objectForKey:@"Current Type"];
}


- (NSDictionary*)currentKeysForPlayer:(int)player
{
	NSString *typeKey = [self currentTypeForPlayer:player];
	if (nil == typeKey)
		return nil;
	
	return [[self typesForPlayer:player] objectForKey:typeKey];
}
- (NSMutableDictionary *)mutableKeysForPlayer:(int)player
{
	NSDictionary *keys = [self currentKeysForPlayer:player];
	
	if (![keys respondsToSelector:@selector(setObject)]) {
		keys = [NSMutableDictionary dictionaryWithDictionary:keys];
		[[self mutableTypesForPlayer:player] setObject:keys forKey:[self currentTypeForPlayer:player]];
	}
	
	return (NSMutableDictionary *)keys;
}


- (void)addMapping:(NSString *)mappingId forElement:(NSString *)name player:(int)player
{
	NSMutableDictionary *keys = [self mutableKeysForPlayer:player];
	NSMutableArray *mappings = [keys objectForKey:name];
	if (nil == mappings) {
		mappings = [NSMutableArray arrayWithCapacity:1];
		[keys setObject:mappings forKey:name];
	} else {
		/* check if it's new */
		int i;
		for (i=0; i<[mappings count]; i++) {
			if ([[mappings objectAtIndex:i] isEqualToString:mappingId])
				return;
		}
	}
	[mappings addObject:mappingId];
}

- (void)removeMappingsForElement:(NSString *)name player:(int)player
{
	NSMutableDictionary *keys = [self mutableKeysForPlayer:player];
	[keys removeObjectForKey:name];
}

- (NSArray *)currentMappingsForElement:(NSString *)name player:(int)player
{
	NSArray *mappings = [[self currentKeysForPlayer:player] objectForKey:name];
	if (nil == mappings)
		return [NSArray array];
	
	return mappings;
}

- (NSString *)mappingNamesForElement:(NSString *)name player:(int)player
{
	NSMutableArray *mappings = [NSMutableArray arrayWithArray:[self currentMappingsForElement:name player:player]];
	NSMutableString *mappingName = [NSMutableString stringWithCapacity:256];
	int i;
	
	if (0 == [mappings count])
		return nil;
	
	while ([mappings count] > 0) {
		NSString *deviceName = [KeyConfig deviceNameFromMapping:[mappings objectAtIndex:0]];
		if (0 != [mappingName length])
			[mappingName appendString:@" and "];
		
		[mappingName appendString:deviceName];
		[mappingName appendString:@": "];
		[mappingName appendString:[KeyConfig nameFromMapping:[mappings objectAtIndex:0]]];
		[mappings removeObjectAtIndex:0];
		
		for (i=0; i<[mappings count]; i++) {
			if ([deviceName isEqualToString:[KeyConfig deviceNameFromMapping:[mappings objectAtIndex:0]]]) {
				[mappingName appendString:@", "];
				[mappingName appendString:[KeyConfig nameFromMapping:[mappings objectAtIndex:i]]];
				[mappings removeObjectAtIndex:i]; i--;
			}
		}
	}
	
	return [NSString stringWithString:mappingName];
}

+ (NSString *)mappingIdForElement:(pRecElement)element onDevice:(pRecDevice)device reverse:(BOOL)reverse
{
	return [NSString stringWithFormat:@"d:{v:%ld, p:%ld, l:%ld, p:%ld, u:%ld}, e:{t:%ld, p:%ld, u:%ld, c:%ld, r:%ld}", 
							device->vendorID, device->productID, device->locID, device->usagePage, device->usage, 
							element->type, element->usagePage, element->usage, element->cookie, reverse];
}

+ (BOOL)reverseMappingForId:(NSString *)mappingId outElement:(pRecElement *)element outDevice:(pRecDevice *)device
{
	recDevice	searchDevice;
	recElement	searchElement;
	long reverse;
	int count = sscanf([mappingId cString], "d:{v:%ld, p:%ld, l:%ld, p:%ld, u:%ld}, e:{t:%ld, p:%ld, u:%ld, c:%ld, r:%ld}", 
			&searchDevice.vendorID, &searchDevice.productID, &searchDevice.locID, &searchDevice.usagePage, &searchDevice.usage, 
			&searchElement.type, &searchElement.usagePage, &searchElement.usage, (long*)&searchElement.cookie, &reverse);

	if (9 == count || 10 == count) {
		if (HIDFindActionDeviceAndElement(&searchDevice, &searchElement,device, element)) {
			return YES;
		}
	}
	
	return NO;
}

+ (BOOL)mappingIsReverse:(NSString *)mappingId
{
	recDevice	searchDevice;
	recElement	searchElement;
	long reverse;
	int count = sscanf([mappingId cString], "d:{v:%ld, p:%ld, l:%ld, p:%ld, u:%ld}, e:{t:%ld, p:%ld, u:%ld, c:%ld, r:%ld}", 
			&searchDevice.vendorID, &searchDevice.productID, &searchDevice.locID, &searchDevice.usagePage, &searchDevice.usage, 
			&searchElement.type, &searchElement.usagePage, &searchElement.usage, (long*)&searchElement.cookie, &reverse);

	if (10 == count) {
		return (reverse != 0);
	}
	
	return NO;
}

+ (NSString *)nameFromMapping:(NSString *)mappingId
{
	pRecElement element;
	pRecDevice device;
	
	if ([self reverseMappingForId:mappingId outElement:&element outDevice:&device]) {
		if (device->usagePage == kHIDPage_GenericDesktop && device->usage == kHIDUsage_GD_Mouse) {
			if (element->usagePage == kHIDPage_Button) {
				return [NSString stringWithFormat:@"Button %i", element->usage];
			}
		} else if (element->usagePage == kHIDPage_KeyboardOrKeypad) {
			int key = element->usage;
			if (key >= kHIDUsage_KeyboardA && key <= kHIDUsage_KeyboardZ) {
				return [NSString stringWithFormat:@"%c", (key-kHIDUsage_KeyboardA)+'A'];
			} else if (key >= kHIDUsage_Keyboard1 && key <= kHIDUsage_Keyboard0) {
				if (key == kHIDUsage_Keyboard0)
					return [NSString stringWithFormat:@"%c", '0'];
				
				return [NSString stringWithFormat:@"%c", (key-kHIDUsage_Keyboard1)+'1'];
			} else if (key >= kHIDUsage_KeyboardF1 && key <= kHIDUsage_KeyboardF12) {
				return [NSString stringWithFormat:@"F%i", (key-kHIDUsage_KeyboardF1)+1];
			} else if (key >= kHIDUsage_Keypad1 && key <= kHIDUsage_Keypad9) {
				return [NSString stringWithFormat:@"Keypad %i", (key-kHIDUsage_Keypad1)+1];
			} else {
				NSString *s = nil;
				switch (key) {
					case kHIDUsage_KeyboardReturnOrEnter: s=@"Return"; break;
					case kHIDUsage_KeyboardEscape: s=@"Escape"; break;
					case kHIDUsage_KeyboardDeleteOrBackspace: s=@"Delete"; break;
					case kHIDUsage_KeyboardTab: s=@"Tab"; break;
					case kHIDUsage_KeyboardSpacebar: s=@"Space"; break;
					case kHIDUsage_KeyboardHyphen: s=@"-"; break;
					case kHIDUsage_KeyboardEqualSign: s=@"="; break;
					case kHIDUsage_KeyboardOpenBracket: s=@"["; break;
					case kHIDUsage_KeyboardCloseBracket: s=@"]"; break;
					case kHIDUsage_KeyboardBackslash: s=@"\\"; break;
					case kHIDUsage_KeyboardSemicolon: s=@";"; break;
					case kHIDUsage_KeyboardQuote: s=@"'"; break;
					case kHIDUsage_KeyboardGraveAccentAndTilde: s=@"´"; break;
					case kHIDUsage_KeyboardComma: s=@","; break;
					case kHIDUsage_KeyboardPeriod: s=@"."; break;
					case kHIDUsage_KeyboardSlash: s=@"/"; break;

					case kHIDUsage_KeyboardCapsLock: s=@"Caps Lock"; break;
					case kHIDUsage_KeyboardRightArrow: s=@"Right Arrow"; break;
					case kHIDUsage_KeyboardLeftArrow: s=@"Left Arrow"; break;
					case kHIDUsage_KeyboardDownArrow: s=@"Down Arrow"; break;
					case kHIDUsage_KeyboardUpArrow: s=@"Up Arrow"; break;
					
					case kHIDUsage_KeypadNumLock: s=@"NumLock"; break;
					case kHIDUsage_KeypadSlash: s=@"Keypad /"; break;
					case kHIDUsage_KeypadAsterisk: s=@"Keypad *"; break;
					case kHIDUsage_KeypadHyphen: s=@"Keypad -"; break;
					case kHIDUsage_KeypadPlus: s=@"Keypad +"; break;
					case kHIDUsage_KeypadEnter: s=@"Keypad Enter"; break;
					case kHIDUsage_Keypad0: s=@"Keypad 0"; break;
					case kHIDUsage_KeypadPeriod: s=@"Keypad ."; break;
					case kHIDUsage_KeypadEqualSign: s=@"Keypad ="; break;

					case kHIDUsage_KeyboardLeftControl: s=@"Left Control"; break;
					case kHIDUsage_KeyboardLeftShift: s=@"Left Shift"; break;
					case kHIDUsage_KeyboardLeftAlt: s=@"Left Option"; break;
					case kHIDUsage_KeyboardLeftGUI: s=@"Left Command"; break;
					case kHIDUsage_KeyboardRightControl: s=@"Right Control"; break;
					case kHIDUsage_KeyboardRightShift: s=@"Right Shift"; break;
					case kHIDUsage_KeyboardRightAlt: s=@"Right Option"; break;
					case kHIDUsage_KeyboardRightGUI: s=@"Right Command"; break;
				}
				if (s) {
					return s;
				}
			}
		}
		
		char name[256];

		if (HIDGetElementNameFromVendorProductUsage (device->vendorID, device->productID, element->usagePage, element->usage, name))
			return [NSString stringWithCString:name];
		return [NSString stringWithCString:element->name];
	} else {
		return NSLocalizedString(@"Unknown Key", @"");
	}
}

+ (NSString *)deviceNameFromMapping:(NSString *)mappingId
{
	pRecElement element;
	pRecDevice device;
	
	if ([self reverseMappingForId:mappingId outElement:&element outDevice:&device]) {
		return [NSString stringWithCString:device->product];
	} else {
		return NSLocalizedString(@"Unknown Device", @"");
	}
}

/* called when ok is pressed */
- (void)updateKeys
{
	int i, j, k;

	/* transfer to working set */
	for (i=0; i<MAX_NUM_PADS; i++) {
		NSString *type = [self currentTypeForPlayer:i];

		gControllerType[i] = [list controllerTypeIdForType:type];
		gNumKeys[i] = gNumAxes[i] = 0;

		for (j=0; j<[list elementCountForType:type]; j++) {
			NSString *name = [list elementNameAtIndex:j type:type];
			NSArray *mappings = [self currentMappingsForElement:name player:i];
			int button = [list buttonIdAtIndex:j type:type];

			if (-1 == button) {
				int axis = [list axisIdAtIndex:j type:type];
				if (-1 != axis) {
					BOOL positive = ([list axisDirectionAtIndex:j type:type] >= 0);

					for (k=0; k<[mappings count]; k++) {
						NSString *mapId = [mappings objectAtIndex:k];

						if (gNumAxes[i] >= MAX_NUM_AXES)
							break;

						if ([KeyConfig reverseMappingForId:mapId outElement:&gAxes[i][gNumAxes[i]].element outDevice:&gAxes[i][gNumAxes[i]].device]) {
							gAxes[i][gNumAxes[i]].axis = axis;
							gAxes[i][gNumAxes[i]].reverse = [KeyConfig mappingIsReverse:mapId];
							gAxes[i][gNumAxes[i]].positive = positive;
							gAxes[i][gNumAxes[i]].lastValue = 127;
							gNumAxes[i]++;
						}
					}
				}
			} else {
				for (k=0; k<[mappings count]; k++) {
					NSString *mapId = [mappings objectAtIndex:k];

					if (gNumKeys[i] >= MAX_NUM_KEYS)
						break;

					if ([KeyConfig reverseMappingForId:mapId outElement:&gKeys[i][gNumKeys[i]].element outDevice:&gKeys[i][gNumKeys[i]].device]) {
						gKeys[i][gNumKeys[i]].button = button;
						gKeys[i][gNumKeys[i]].reverse = [KeyConfig mappingIsReverse:mapId];
						gNumKeys[i]++;
					}
				}
			}
		}
	}

	/* save to preferences */
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setObject:keyValues forKey:@"net.pcsx.HIDInputPlugin Pad Settings"];
	[defaults synchronize];
}

/* called when cancel button is pressed */
- (void)releaseKeys
{
	[keyValues release];
	keyValues = [[[NSUserDefaults standardUserDefaults] dictionaryForKey:@"net.pcsx.HIDInputPlugin Pad Settings"] retain];
}

@end
