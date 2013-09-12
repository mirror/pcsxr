//
//  cfgHelper.m
//  Pcsxr
//
//  Created by C.W. Betts on 8/28/13.
//
//

#import <Foundation/Foundation.h>
#import "PadController.h"
#include "cfg.h"

#define padType @"Pad Type"
#define deviceNumber @"Device Number"
#define dfKey @"Key Value"
#define joyType @"Joystick Type"
#define joyVal @"Joystick Value"

#define dSelect @"DKEY_SELECT"
#define dStart @"DKEY_START"
#define dUp @"DKEY_UP"
#define dRight @"DKEY_RIGHT"
#define dDown @"DKEY_DOWN"
#define dLeft @"DKEY_LEFT"
#define dL1 @"DKEY_L1"
#define dL2 @"DKEY_L2"
#define dL3 @"DKEY_L3"
#define dR1 @"DKEY_R1"
#define dR2 @"DKEY_R2"
#define dR3 @"DKEY_R3"
#define dTriangle @"DKEY_TRIANGLE"
#define dCircle @"DKEY_CIRCLE"
#define dCross @"DKEY_CROSS"
#define dSquare @"DKEY_SQUARE"
#define dAnalog @"DKEY_ANALOG"
#define dLeftAnalogXP @"LeftAnalogXP"
#define dLeftAnalogXM @"LeftAnalogXM"
#define dLeftAnalogYP @"LeftAnalogYP"
#define dLeftAnalogYM @"LeftAnalogYM"
#define dRightAnalogXP @"RightAnalogXP"
#define dRightAnalogXM @"RightAnalogXM"
#define dRightAnalogYP @"RightAnalogYP"
#define dRightAnalogYM @"RightAnalogYM"

#define VibrateOn @"Visual Vibration"

NSDictionary *DefaultPadArray(int padnum)
{
	NSMutableDictionary *mutArray =
	[NSMutableDictionary dictionaryWithDictionary:@{VibrateOn: @NO,
									 deviceNumber: @(padnum),
										  padType: @(PSE_PAD_TYPE_STANDARD),
										  dSelect: [NSMutableDictionary dictionaryWithObjectsAndKeys:@8, joyVal, @(BUTTON), joyType, nil],
										   dStart: [NSMutableDictionary dictionaryWithObjectsAndKeys:@9, joyVal, @(BUTTON), joyType, nil],
											  dUp: [NSMutableDictionary dictionaryWithObjectsAndKeys:@(-2), joyVal, @(AXIS), joyType, nil],
										   dRight: [NSMutableDictionary dictionaryWithObjectsAndKeys:@1, joyVal, @(AXIS), joyType, nil],
											dDown: [NSMutableDictionary dictionaryWithObjectsAndKeys:@2, joyVal, @(AXIS), joyType, nil],
											dLeft: [NSMutableDictionary dictionaryWithObjectsAndKeys:@(-1), joyVal, @(AXIS), joyType, nil],
											  dL2: [NSMutableDictionary dictionaryWithObjectsAndKeys:@4, joyVal, @(BUTTON), joyType, nil],
											  dL1: [NSMutableDictionary dictionaryWithObjectsAndKeys:@6, joyVal, @(BUTTON), joyType, nil],
											  dR2: [NSMutableDictionary dictionaryWithObjectsAndKeys:@5, joyVal, @(BUTTON), joyType, nil],
											  dR1: [NSMutableDictionary dictionaryWithObjectsAndKeys:@7, joyVal, @(BUTTON), joyType, nil],
										dTriangle: [NSMutableDictionary dictionaryWithObjectsAndKeys:@0, joyVal, @(BUTTON), joyType, nil],
										  dCircle: [NSMutableDictionary dictionaryWithObjectsAndKeys:@1, joyVal, @(BUTTON), joyType, nil],
										   dCross: [NSMutableDictionary dictionaryWithObjectsAndKeys:@2, joyVal, @(BUTTON), joyType, nil],
										  dSquare: [NSMutableDictionary dictionaryWithObjectsAndKeys:@3, joyVal, @(BUTTON), joyType, nil]}];
	if (padnum == 0) {
		mutArray[dSelect][dfKey] = @9;
		mutArray[dStart][dfKey] = @10;
		mutArray[dUp][dfKey] = @127;
		mutArray[dRight][dfKey] = @125;
		mutArray[dDown][dfKey] = @126;
		mutArray[dLeft][dfKey] = @124;
		mutArray[dL2][dfKey] = @16;
		mutArray[dR2][dfKey] = @18;
		mutArray[dL1][dfKey] = @14;
		mutArray[dR1][dfKey] = @15;
		mutArray[dTriangle][dfKey] = @3;
		mutArray[dCircle][dfKey] = @8;
		mutArray[dCross][dfKey] = @7;
		mutArray[dSquare][dfKey] = @2;
		mutArray[dAnalog] = @{dfKey: @12};
	}
	return [NSDictionary dictionaryWithDictionary:mutArray];
}

static NSDictionary *DictionaryFromButtonDef(KEYDEF theKey)
{
	NSMutableDictionary *mutDict = [NSMutableDictionary dictionaryWithCapacity:3];
	if (theKey.Key) {
		mutDict[dfKey] = @(theKey.Key);
	}
	if (theKey.JoyEvType != NONE) {
		mutDict[joyType] = @(theKey.JoyEvType);
		switch (theKey.JoyEvType) {
			case BUTTON:
				mutDict[joyVal] = @(theKey.J.Button);
				break;
				
			case HAT:
				mutDict[joyVal] = @(theKey.J.Hat);
				break;
				
			case AXIS:
				mutDict[joyVal] = @(theKey.J.Axis);
				break;
				
			case NONE:
			default:
				//[mutDict setObject:@(theKey.J.d) forKey:joyVal];
				[mutDict removeObjectForKey:joyType];
				break;
		}
	}
	return [NSDictionary dictionaryWithDictionary:mutDict];
}

static void SetKeyFromDictionary(NSDictionary *inDict, KEYDEF *outDef)
{
	assert(outDef != NULL);
	if (!inDict) {
		return;
	}
	NSNumber *theJoyType = inDict[joyType];
	if (theJoyType) {
		NSNumber *theJoyVal = inDict[joyVal];
		outDef->JoyEvType = [theJoyType unsignedCharValue];
		switch (outDef->JoyEvType) {
			case BUTTON:
				outDef->J.Button = [theJoyVal unsignedShortValue];
				break;
				
			case HAT:
				outDef->J.Hat = [theJoyVal unsignedShortValue];
				break;
				
			case AXIS:
				outDef->J.Axis = [theJoyVal shortValue];
				break;
				
			default:
				break;
		}
	}
	NSNumber *keyVal = inDict[dfKey];
	if (keyVal) {
		outDef->Key = [keyVal unsignedShortValue];
	}
}

void LoadPadArray(int padnum, NSDictionary *nsPrefs)
{
	PADDEF *curDef = &g.cfg.PadDef[padnum];
	curDef->DevNum = [nsPrefs[deviceNumber] charValue];
	curDef->Type = [nsPrefs[padType] unsignedShortValue];
	curDef->VisualVibration = [nsPrefs[VibrateOn] boolValue]; //Not implemented on OS X right now.
	
	//Analog buttons
	SetKeyFromDictionary(nsPrefs[dL3], &curDef->KeyDef[DKEY_L3]);
	SetKeyFromDictionary(nsPrefs[dR3], &curDef->KeyDef[DKEY_R3]);
	SetKeyFromDictionary(nsPrefs[dAnalog], &curDef->KeyDef[DKEY_ANALOG]);
	
	//Analog sticks
	SetKeyFromDictionary(nsPrefs[dLeftAnalogXP], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_XP]);
	SetKeyFromDictionary(nsPrefs[dLeftAnalogXM], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_XM]);
	SetKeyFromDictionary(nsPrefs[dLeftAnalogYP], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_YP]);
	SetKeyFromDictionary(nsPrefs[dLeftAnalogYM], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_YM]);
	
	SetKeyFromDictionary(nsPrefs[dRightAnalogXP], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XP]);
	SetKeyFromDictionary(nsPrefs[dRightAnalogXM], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XM]);
	SetKeyFromDictionary(nsPrefs[dRightAnalogYP], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YP]);
	SetKeyFromDictionary(nsPrefs[dRightAnalogYM], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YM]);
	
	//Digital shouldurs
	SetKeyFromDictionary(nsPrefs[dL1], &curDef->KeyDef[DKEY_L1]);
	SetKeyFromDictionary(nsPrefs[dL2], &curDef->KeyDef[DKEY_L2]);
	SetKeyFromDictionary(nsPrefs[dR1], &curDef->KeyDef[DKEY_R1]);
	SetKeyFromDictionary(nsPrefs[dR2], &curDef->KeyDef[DKEY_R2]);
	
	//Digital buttons
	SetKeyFromDictionary(nsPrefs[dSelect], &curDef->KeyDef[DKEY_SELECT]);
	SetKeyFromDictionary(nsPrefs[dStart], &curDef->KeyDef[DKEY_START]);
	SetKeyFromDictionary(nsPrefs[dUp], &curDef->KeyDef[DKEY_UP]);
	SetKeyFromDictionary(nsPrefs[dRight], &curDef->KeyDef[DKEY_RIGHT]);
	SetKeyFromDictionary(nsPrefs[dDown], &curDef->KeyDef[DKEY_DOWN]);
	SetKeyFromDictionary(nsPrefs[dLeft], &curDef->KeyDef[DKEY_LEFT]);
	SetKeyFromDictionary(nsPrefs[dTriangle], &curDef->KeyDef[DKEY_TRIANGLE]);
	SetKeyFromDictionary(nsPrefs[dCircle], &curDef->KeyDef[DKEY_CIRCLE]);
	SetKeyFromDictionary(nsPrefs[dCross], &curDef->KeyDef[DKEY_CROSS]);
	SetKeyFromDictionary(nsPrefs[dSquare], &curDef->KeyDef[DKEY_SQUARE]);
}

NSDictionary *SavePadArray(int padnum)
{
	NSMutableDictionary *mutArray = [NSMutableDictionary dictionary];
	PADDEF *curDef = &g.cfg.PadDef[padnum];
	mutArray[deviceNumber] = @(curDef->DevNum);
	mutArray[padType] = @(curDef->Type);
	mutArray[VibrateOn] = curDef->VisualVibration ? @YES : @NO;
	
	switch (curDef->Type) {
		case PSE_PAD_TYPE_ANALOGPAD:
		{
			mutArray[dL3] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_L3]);
			mutArray[dR3] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_R3]);
			mutArray[dAnalog] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_ANALOG]);
			
			mutArray[dLeftAnalogXP] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_XP]);
			mutArray[dLeftAnalogXM] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_XM]);
			mutArray[dLeftAnalogYP] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_YP]);
			mutArray[dLeftAnalogYM] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_YM]);
			
			mutArray[dRightAnalogXP] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XP]);
			mutArray[dRightAnalogXM] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XM]);
			mutArray[dRightAnalogYP] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YP]);
			mutArray[dRightAnalogYM] = DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YM]);
		}
			//Fall through
			
		case PSE_PAD_TYPE_STANDARD:
		{
			mutArray[dL1] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_L1]);
			mutArray[dL2] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_L2]);
			mutArray[dR1] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_R1]);
			mutArray[dR2] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_R2]);
			
			mutArray[dSelect] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_SELECT]);
			mutArray[dStart] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_START]);
			mutArray[dUp] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_UP]);
			mutArray[dRight] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_RIGHT]);
			mutArray[dDown] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_DOWN]);
			mutArray[dLeft] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_LEFT]);
			mutArray[dTriangle] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_TRIANGLE]);
			mutArray[dCircle] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_CIRCLE]);
			mutArray[dCross] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_CROSS]);
			mutArray[dSquare] = DictionaryFromButtonDef(curDef->KeyDef[DKEY_SQUARE]);
		}
			break;
			
		default:
			break;
	}

	return [NSDictionary dictionaryWithDictionary:mutArray];
}
