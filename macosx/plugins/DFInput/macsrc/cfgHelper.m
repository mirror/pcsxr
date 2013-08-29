//
//  cfgHelper.m
//  Pcsxr
//
//  Created by C.W. Betts on 8/28/13.
//
//

#include "cfg.h"
#import <Foundation/Foundation.h>
#import "ARCBridge.h"
#import "PadController.h"

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

NSDictionary *DefaultPadArray(int padnum)
{
	NSMutableDictionary *mutArray = [NSMutableDictionary dictionaryWithObjectsAndKeys:
									 @(padnum), deviceNumber,
									 @(PSE_PAD_TYPE_STANDARD), padType,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@8, joyVal, @(BUTTON), joyType, nil], dSelect,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@9, joyVal, @(BUTTON), joyType, nil], dStart,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@(-2), joyVal, @(AXIS), joyType, nil], dUp,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@1, joyVal, @(AXIS), joyType, nil], dRight,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@2, joyVal, @(AXIS), joyType, nil], dDown,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@(-1), joyVal, @(AXIS), joyType, nil], dLeft,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@4, joyVal, @(BUTTON), joyType, nil], dL2,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@6, joyVal, @(BUTTON), joyType, nil], dL1,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@5, joyVal, @(BUTTON), joyType, nil], dR2,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@7, joyVal, @(BUTTON), joyType, nil], dR1,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@0, joyVal, @(BUTTON), joyType, nil], dTriangle,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@1, joyVal, @(BUTTON), joyType, nil], dCircle,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@2, joyVal, @(BUTTON), joyType, nil], dCross,
									 [NSMutableDictionary dictionaryWithObjectsAndKeys:@3, joyVal, @(BUTTON), joyType, nil], dSquare,
									 nil];
	if (padnum == 0) {
		[[mutArray objectForKey:dSelect] setObject:@9 forKey:dfKey];
		[[mutArray objectForKey:dStart] setObject:@10 forKey:dfKey];
		[[mutArray objectForKey:dUp] setObject:@127 forKey:dfKey];
		[[mutArray objectForKey:dRight] setObject:@125 forKey:dfKey];
		[[mutArray objectForKey:dDown] setObject:@126 forKey:dfKey];
		[[mutArray objectForKey:dLeft] setObject:@124 forKey:dfKey];
		[[mutArray objectForKey:dL2] setObject:@16 forKey:dfKey];
		[[mutArray objectForKey:dR2] setObject:@18 forKey:dfKey];
		[[mutArray objectForKey:dL1] setObject:@14 forKey:dfKey];
		[[mutArray objectForKey:dR1] setObject:@15 forKey:dfKey];
		[[mutArray objectForKey:dTriangle] setObject:@3 forKey:dfKey];
		[[mutArray objectForKey:dCircle] setObject:@8 forKey:dfKey];
		[[mutArray objectForKey:dCross] setObject:@7 forKey:dfKey];
		[[mutArray objectForKey:dSquare] setObject:@2 forKey:dfKey];
		[mutArray setObject:[NSDictionary dictionaryWithObject:@12 forKey:dfKey] forKey:dAnalog];
	}
	return [NSDictionary dictionaryWithDictionary:mutArray];
}

static NSDictionary *DictionaryFromButtonDef(KEYDEF theKey)
{
	NSMutableDictionary *mutDict = [NSMutableDictionary dictionaryWithCapacity:3];
	if (theKey.Key) {
		[mutDict setObject:@(theKey.Key) forKey:dfKey];
	}
	if (theKey.JoyEvType != NONE) {
		[mutDict setObject:@(theKey.JoyEvType) forKey:joyType];
		switch (theKey.JoyEvType) {
			case BUTTON:
				[mutDict setObject:@(theKey.J.Button) forKey:joyVal];
				break;
				
			case HAT:
				[mutDict setObject:@(theKey.J.Hat) forKey:joyVal];
				break;
				
			case AXIS:
				[mutDict setObject:@(theKey.J.Axis) forKey:joyVal];
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
	NSNumber *theJoyType = [inDict objectForKey:joyType];
	if (theJoyType) {
		NSNumber *theJoyVal = [inDict objectForKey:joyVal];
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
	NSNumber *keyVal = [inDict objectForKey:dfKey];
	if (keyVal) {
		outDef->Key = [keyVal unsignedShortValue];
	}
}

void LoadPadArray(int padnum, NSDictionary *nsPrefs)
{
	PADDEF *curDef = &g.cfg.PadDef[padnum];
	curDef->DevNum = [[nsPrefs objectForKey:deviceNumber] charValue];
	curDef->Type = [[nsPrefs objectForKey:padType] unsignedShortValue];
	curDef->VisualVibration = 0; //Not implemented on OS X right now.
	
	//Analog buttons
	SetKeyFromDictionary([nsPrefs objectForKey:dL3], &curDef->KeyDef[DKEY_L3]);
	SetKeyFromDictionary([nsPrefs objectForKey:dR3], &curDef->KeyDef[DKEY_R3]);
	SetKeyFromDictionary([nsPrefs objectForKey:dAnalog], &curDef->KeyDef[DKEY_ANALOG]);
	
	//Analog sticks
	SetKeyFromDictionary([nsPrefs objectForKey:dLeftAnalogXP], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_XP]);
	SetKeyFromDictionary([nsPrefs objectForKey:dLeftAnalogXM], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_XM]);
	SetKeyFromDictionary([nsPrefs objectForKey:dLeftAnalogYP], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_YP]);
	SetKeyFromDictionary([nsPrefs objectForKey:dLeftAnalogYM], &curDef->AnalogDef[ANALOG_LEFT][ANALOG_YM]);
	
	SetKeyFromDictionary([nsPrefs objectForKey:dRightAnalogXP], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XP]);
	SetKeyFromDictionary([nsPrefs objectForKey:dRightAnalogXM], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XM]);
	SetKeyFromDictionary([nsPrefs objectForKey:dRightAnalogYP], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YP]);
	SetKeyFromDictionary([nsPrefs objectForKey:dRightAnalogYM], &curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YM]);
	
	//Digital shouldurs
	SetKeyFromDictionary([nsPrefs objectForKey:dL1], &curDef->KeyDef[DKEY_L1]);
	SetKeyFromDictionary([nsPrefs objectForKey:dL2], &curDef->KeyDef[DKEY_L2]);
	SetKeyFromDictionary([nsPrefs objectForKey:dR1], &curDef->KeyDef[DKEY_R1]);
	SetKeyFromDictionary([nsPrefs objectForKey:dR2], &curDef->KeyDef[DKEY_R2]);
	
	//Digital buttons
	SetKeyFromDictionary([nsPrefs objectForKey:dSelect], &curDef->KeyDef[DKEY_SELECT]);
	SetKeyFromDictionary([nsPrefs objectForKey:dStart], &curDef->KeyDef[DKEY_START]);
	SetKeyFromDictionary([nsPrefs objectForKey:dUp], &curDef->KeyDef[DKEY_UP]);
	SetKeyFromDictionary([nsPrefs objectForKey:dRight], &curDef->KeyDef[DKEY_RIGHT]);
	SetKeyFromDictionary([nsPrefs objectForKey:dDown], &curDef->KeyDef[DKEY_DOWN]);
	SetKeyFromDictionary([nsPrefs objectForKey:dLeft], &curDef->KeyDef[DKEY_LEFT]);
	SetKeyFromDictionary([nsPrefs objectForKey:dTriangle], &curDef->KeyDef[DKEY_TRIANGLE]);
	SetKeyFromDictionary([nsPrefs objectForKey:dCircle], &curDef->KeyDef[DKEY_CIRCLE]);
	SetKeyFromDictionary([nsPrefs objectForKey:dCross], &curDef->KeyDef[DKEY_CROSS]);
	SetKeyFromDictionary([nsPrefs objectForKey:dSquare], &curDef->KeyDef[DKEY_SQUARE]);
}

NSDictionary *SavePadArray(int padnum)
{
	NSMutableDictionary *mutArray = [NSMutableDictionary dictionary];
	PADDEF *curDef = &g.cfg.PadDef[padnum];
	[mutArray setObject:@(curDef->DevNum) forKey:deviceNumber];
	[mutArray setObject:@(curDef->Type) forKey:padType];
	
	switch (curDef->Type) {
		case PSE_PAD_TYPE_ANALOGPAD:
		{
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_L3]) forKey:dL3];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_R3]) forKey:dR3];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_ANALOG]) forKey:dAnalog];
			
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_XP]) forKey:dLeftAnalogXP];
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_XM]) forKey:dLeftAnalogXM];
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_YP]) forKey:dLeftAnalogYP];
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_LEFT][ANALOG_YM]) forKey:dLeftAnalogYM];
			
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XP]) forKey:dRightAnalogXP];
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_XM]) forKey:dRightAnalogXM];
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YP]) forKey:dRightAnalogYP];
			[mutArray setObject:DictionaryFromButtonDef(curDef->AnalogDef[ANALOG_RIGHT][ANALOG_YM]) forKey:dRightAnalogYM];
		}
			//Fall through
			
		case PSE_PAD_TYPE_STANDARD:
		{
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_L1]) forKey:dL1];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_L2]) forKey:dL2];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_R1]) forKey:dR1];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_R2]) forKey:dR2];
			
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_SELECT]) forKey:dSelect];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_START]) forKey:dStart];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_UP]) forKey:dUp];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_RIGHT]) forKey:dRight];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_DOWN]) forKey:dDown];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_LEFT]) forKey:dLeft];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_TRIANGLE]) forKey:dTriangle];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_CIRCLE]) forKey:dCircle];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_CROSS]) forKey:dCross];
			[mutArray setObject:DictionaryFromButtonDef(curDef->KeyDef[DKEY_SQUARE]) forKey:dSquare];
		}
			break;
			
		default:
			break;
	}

	return [NSDictionary dictionaryWithDictionary:mutArray];
}
