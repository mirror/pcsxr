/***************************************************************************
    ControllerList.h
    HIDInput
  
    Created by Gil Pedersen on Mon May 03 2004.
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

#define ControllerList NetPcsxHIDInputPluginControllerList

#import <Foundation/Foundation.h>
#import <AppKit/NSTableView.h>
#import "KeyConfig.h"

@class KeyConfig;

@interface ControllerList : NSObject {
	int currentController;
	KeyConfig *config;
	NSArray *plist;
	//NSArray *keys;
	NSArray *typeList;
	//NSString *type;
	//NSString *currentSet;
	//NSMutableDictionary *keyValues;
}

- (id)initWithConfig:(KeyConfig *)keyconfig;

- (NSArray *)controllerTypes;

- (NSDictionary *)dictForType:(NSString *)type;
- (NSArray *)elementsForType:(NSString *)type;
- (NSString *)elementNameAtIndex:(int)index type:(NSString *)type;
- (int)elementCountForType:(NSString *)type;
- (int)controllerTypeIdForType:(NSString *)type;
- (int)buttonIdAtIndex:(int)index type:(NSString *)type;
- (int)axisIdAtIndex:(int)index type:(NSString *)type;
- (int)axisDirectionAtIndex:(int)index type:(NSString *)type;

- (void)setCurrentController:(int)which;
- (int)currentController;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;

@end
