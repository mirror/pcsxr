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
#import "ARCBridge.h"
#include "pad.h"
#include "cfg.h"

static int currentController;
static NSArray *labelText;

@implementation ControllerList

- (id)initWithConfig
{
	if (self = [super init]) {
		static dispatch_once_t onceToken;
		dispatch_once(&onceToken, ^{
			NSBundle *plugBundle = [NSBundle bundleForClass:[ControllerList class]];
			labelText = @[[plugBundle localizedStringForKey:@"D-Pad Up" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"D-Pad Down" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"D-Pad Left" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"D-Pad Right" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Cross" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Circle" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Square" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Triangle" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"L1" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"R1" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"L2" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"R2" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Select" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Start" value:@"" table:nil],
				 
				 [plugBundle localizedStringForKey:@"L3" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"R3" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"Analog" value:@"" table:nil],
				 
				 [plugBundle localizedStringForKey:@"L-Stick Right" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"L-Stick Left" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"L-Stick Down" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"L-Stick Up" value:@"" table:nil],
				 
				 [plugBundle localizedStringForKey:@"R-Stick Right" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"R-Stick Left" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"R-Stick Down" value:@"" table:nil],
				 [plugBundle localizedStringForKey:@"R-Stick Up" value:@"" table:nil]];
			RETAINOBJNORETURN(labelText);
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
		return [labelText objectAtIndex:rowIndex];
	} else {
		char buf[256] = {0};
		
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
