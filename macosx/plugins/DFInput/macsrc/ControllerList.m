/*
 * Copyright (c) 2010, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * All Rights Reserved.
 *
 * Based on HIDInput by Gil Pedersen.
 * Copyright (c) 2004, Gil Pedersen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#import "ControllerList.h"
#include "pad.h"
#include "cfg.h"

static int currentController;
static NSArray *labelText;
static NSArray *GameControllerText;

@implementation ControllerList

- (BOOL)isUsingSDL2
{
	return g.cfg.PadDef[currentController].UseSDL2;
}

- (void)setUsingSDL2:(BOOL)_usingSDL2
{
	g.cfg.PadDef[currentController].UseSDL2 = _usingSDL2;
}

- (id)initWithConfig
{
	if (self = [super init]) {
		static dispatch_once_t onceToken;
		dispatch_once(&onceToken, ^{
			NSBundle *plugBundle = [NSBundle bundleForClass:[self class]];
			labelText = @[NSLocalizedStringFromTableInBundle(@"D-Pad Up", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"D-Pad Down", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"D-Pad Left", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"D-Pad Right", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Cross", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Circle", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Square", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Triangle", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"L1", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"R1", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"L2", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"R2", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Select", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Start", nil, plugBundle, @""),
						  
						  NSLocalizedStringFromTableInBundle(@"L3", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"R3", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"Analog", nil, plugBundle, @""),
						  
						  NSLocalizedStringFromTableInBundle(@"L-Stick Right", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"L-Stick Left", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"L-Stick Down", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"L-Stick Up", nil, plugBundle, @""),
						  
						  NSLocalizedStringFromTableInBundle(@"R-Stick Right", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"R-Stick Left", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"R-Stick Down", nil, plugBundle, @""),
						  NSLocalizedStringFromTableInBundle(@"R-Stick Up", nil, plugBundle, @"")];
			
			GameControllerText = @[NSLocalizedStringFromTableInBundle(@"D-Pad Up", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"D-Pad Down", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"D-Pad Left", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"D-Pad Right", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Cross", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Circle", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Square", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Triangle", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Left Bumper", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Right Bumper", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Left Trigger", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Right Trigger", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Back", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Start", nil, plugBundle, @""),
								   
								   NSLocalizedStringFromTableInBundle(@"L3", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"R3", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"Guide", nil, plugBundle, @""),
								   
								   NSLocalizedStringFromTableInBundle(@"L-Stick Right", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"L-Stick Left", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"L-Stick Down", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"L-Stick Up", nil, plugBundle, @""),
								   
								   NSLocalizedStringFromTableInBundle(@"R-Stick Right", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"R-Stick Left", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"R-Stick Down", nil, plugBundle, @""),
								   NSLocalizedStringFromTableInBundle(@"R-Stick Up", nil, plugBundle, @"")];
		});
	}
	return self;
}

/* sets current controller data returned by data source */
+ (void)setCurrentController:(int)which
{
	currentController = which;
}

+ (int)currentController
{
	return currentController;
}

/* NSDataSource */
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
	return DKEY_TOTAL + (g.cfg.PadDef[currentController].Type == PSE_PAD_TYPE_ANALOGPAD ? 8 : -3);
}

static const int DPad[DKEY_TOTAL] = {
	DKEY_UP,
	DKEY_DOWN,
	DKEY_LEFT,
	DKEY_RIGHT,
	DKEY_CROSS,
	DKEY_CIRCLE,
	DKEY_SQUARE,
	DKEY_TRIANGLE,
	DKEY_L1,
	DKEY_R1,
	DKEY_L2,
	DKEY_R2,
	DKEY_SELECT,
	DKEY_START,
	DKEY_L3,
	DKEY_R3,
	DKEY_ANALOG
};

+ (int)buttonOfRow:(NSInteger)row
{
	return DPad[row];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn 
		row:(NSInteger)rowIndex
{
	if ([[aTableColumn identifier] isEqualToString:@"key"]) {
		return labelText[rowIndex];
	} else {
		char buf[256] = {0};
		
		if ([self isUsingSDL2]) {
			NSString *keyBoardString, *gamePadStr = GameControllerText[rowIndex];
			if (rowIndex < DKEY_TOTAL) {
				GetKeyboardKeyDescription(buf, currentController, DPad[rowIndex]);
			} else {
				NSInteger tmpRowIndex = rowIndex - DKEY_TOTAL;
				GetKeyboardAnalogDescription(buf, currentController, (int)(tmpRowIndex / 4), tmpRowIndex % 4);
			}
			keyBoardString = @(buf);
			
			return [keyBoardString isEqualToString:@""] ? gamePadStr : [gamePadStr stringByAppendingFormat:@" / %@", keyBoardString];
		} else {
		// actual keys
		if (rowIndex < DKEY_TOTAL) {
			GetKeyDescription(buf, currentController, DPad[rowIndex]);
		} else {
			rowIndex -= DKEY_TOTAL;
			GetAnalogDescription(buf, currentController, (int)(rowIndex / 4), rowIndex % 4);
		}
		
		return @(buf);
		}
	}
}

- (void)deleteRow:(NSInteger)which
{
	if (which < DKEY_TOTAL) {
		g.cfg.PadDef[currentController].KeyDef[DPad[which]].Key = 0;
		g.cfg.PadDef[currentController].KeyDef[DPad[which]].JoyEvType = NONE;
		g.cfg.PadDef[currentController].KeyDef[DPad[which]].J.d = 0;
	} else {
		which -= DKEY_TOTAL;
		g.cfg.PadDef[currentController].AnalogDef[which / 4][which % 4].Key = 0;
		g.cfg.PadDef[currentController].AnalogDef[which / 4][which % 4].JoyEvType = NONE;
		g.cfg.PadDef[currentController].AnalogDef[which / 4][which % 4].J.d = 0;
	}
}

@end
