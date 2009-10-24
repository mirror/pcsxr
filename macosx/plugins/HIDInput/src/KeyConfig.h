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

#define KeyConfig NetPcsxHIDInputPluginKeyConfig

#import <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include "HID_Utilities.h"
#import "ControllerList.h"

@class ControllerList;

@interface KeyConfig : NSObject {
	ControllerList *list;
	NSDictionary *keyValues;
}

- (id)init;
- (void)dealloc;

+ (KeyConfig *)current;

+ (NSString *)mappingIdForElement:(pRecElement)element onDevice:(pRecDevice)device reverse:(BOOL)reverse;
+ (BOOL)reverseMappingForId:(NSString *)mappingId outElement:(pRecElement *)element outDevice:(pRecDevice *)device;
+ (NSString *)nameFromMapping:(NSString *)mappingId;
+ (NSString *)deviceNameFromMapping:(NSString *)mappingId;

- (ControllerList *)controllerList;
- (void)setCurrentSet:(NSString *)name;
- (NSString *)currentSet;
- (NSDictionary *)players;
- (NSDictionary *)typesForPlayer:(int)player;
- (void)setCurrentType:(NSString *)type player:(int)player;
- (NSString *)currentTypeForPlayer:(int)player;
- (NSDictionary *)currentKeysForPlayer:(int)player;

- (void)addMapping:(NSString *)mappingId forElement:(NSString *)name player:(int)player;
- (void)removeMappingsForElement:(NSString *)name player:(int)player;
- (NSArray *)currentMappingsForElement:(NSString *)name player:(int)player;
- (NSString *)mappingNamesForElement:(NSString *)name player:(int)player;

- (void)updateKeys;
- (void)releaseKeys;

@end
