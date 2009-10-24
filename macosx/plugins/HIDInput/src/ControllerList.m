/***************************************************************************
    ControllerList.m
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

#import "ControllerList.h"

@implementation ControllerList

- (id)initWithConfig:(KeyConfig *)keyconfig
{
	if (!(self = [super init]) || nil == keyconfig)
		return nil;
	
	config = keyconfig;
	
	/* load keyinfo plist */
	NSString *path = [[NSBundle bundleWithIdentifier:@"net.pcsx.HIDInputPlugin"] pathForResource:@"psxKeys" ofType:@"plist"];
	NSData *plistData;
	NSString *error;
	int i;

	plistData = [NSData dataWithContentsOfFile:path];
	plist = [NSPropertyListSerialization propertyListFromData:plistData
											  mutabilityOption:NSPropertyListImmutable
											  format:nil
											  errorDescription:&error];
	if(!plist)
	{
		 NSLog(error);
		 [error release];
		 return nil;
	}
	
	[plist retain];
	//[plistData retain];
	
	/* build typelist */
	NSMutableArray *tmpList = [NSMutableArray arrayWithCapacity:[plist count]];
	for (i=0; i<[plist count]; i++) {
		id name = [plist objectAtIndex:i];
		if ([name isKindOfClass:[NSString class]]) {
			[tmpList addObject:name];
		}
	}
	typeList = [[NSArray alloc] initWithArray:tmpList];
	
	return self;
}

- (void)dealloc
{
	[typeList release];
	
	//[plistData release];
	[plist release];
	//[currentSet release];
}



/*
- (NSString *)typeDefaultKey
{
	return [NSString stringWithFormat:@"TypeForPad%iSet%@", currentController, [[NSUserDefaults standardUserDefaults] stringForKey:@"CurrentPadSet"]];
}

- (void)setType:(NSString *)aType
{
	if (type)
		[type release];
	if (keys)
		[keys release];
	
	type = aType ? [aType retain] : [typeList objectAtIndex:0];
	keys = [[plist objectForKey:type] objectForKey:@"Buttons"];
	if (keys)
		[keys retain];
	
	[[NSUserDefaults standardUserDefaults] setObject:type forKey:[self typeDefaultKey]];
}
*/
- (NSArray *)controllerTypes
{
	return typeList;
}
/*
- (NSString *)controllerType
{
	return type;
}*/

- (NSDictionary *)dictForType:(NSString *)type
{
	int index = [plist indexOfObject:type];
	if (NSNotFound == index)
		return [NSDictionary dictionary];
	
	id dict = [plist objectAtIndex:index+1];
	if (![dict isKindOfClass:[NSDictionary class]])
		return [NSDictionary dictionary];

	return dict;
}

- (NSArray *)elementsForType:(NSString *)type
{
	return [[self dictForType:type] objectForKey:@"Elements"];
}

- (NSString *)elementNameAtIndex:(int)index type:(NSString *)type
{
	if (nil == type || index < 0)
		return @"";
	
	NSArray *list = [self elementsForType:type];
	if (index*2 >= [list count])
		return @"";
	
	return [list objectAtIndex:index*2];
}

- (int)elementCountForType:(NSString *)type
{
	if (nil == type)
		return 0;
	
	return [[self elementsForType:type] count]/2;
}

- (int)controllerTypeIdForType:(NSString *)type
{
	if (nil == type)
		return 0;
	
	NSNumber *number = [[self dictForType:type] objectForKey:@"Type ID"];
	if (nil == number)
		return 0;
	
	return [number intValue];
}

- (int)buttonIdAtIndex:(int)index type:(NSString *)type
{
	if (nil == type || index < 0)
		return -1;
	
	NSDictionary *dict = [[self elementsForType:type] objectAtIndex:index*2+1];
	NSNumber *number = [dict objectForKey:@"Button"];
	if (nil == number)
		return -1;
	
	return [number intValue];
}

- (int)axisIdAtIndex:(int)index type:(NSString *)type
{
	if (nil == type || index < 0)
		return -1;
	
	NSDictionary *dict = [[self elementsForType:type] objectAtIndex:index*2+1];
	NSNumber *number = [dict objectForKey:@"Axis"];
	if (nil == number)
		return -1;
	
	return [number intValue];
}

- (int)axisDirectionAtIndex:(int)index type:(NSString *)type
{
	if (nil == type || index < 0)
		return -1;
	
	NSDictionary *dict = [[self elementsForType:type] objectAtIndex:index*2+1];
	NSNumber *number = [dict objectForKey:@"Axis-Positive"];
	if (nil == number)
		return 1;
	
	return [number boolValue] ? 1 : -1;
}

- (NSArray *)defaultMappingsAtIndex:(int)index type:(NSString *)type
{
	if (nil == type || index < 0)
		return -1;
	
	NSDictionary *dict = [[self elementsForType:type] objectAtIndex:index*2+1];
	return [dict objectForKey:@"Default Mappings"];
}


/* sets current controller data returned by data source */
- (void)setCurrentController:(int)which
{
	currentController = which;
}

- (int)currentController
{
	return currentController;
}

/* NSDataSource */
- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	return [self elementCountForType:[config currentTypeForPlayer:currentController]];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn 
		row:(int)rowIndex
{
	NSString *type = [config currentTypeForPlayer:currentController];
	if (type) {
		NSString *name = [self elementNameAtIndex:rowIndex type:type];
		
		if ([((NSString *)[aTableColumn identifier]) isEqualToString:@"key"])
			return NSLocalizedString(name, @"");
		else {
			// actual keys
			NSString *mappingName = [config mappingNamesForElement:name player:currentController];
			if (nil == mappingName) {
				return @"";
				//return NSLocalizedString(@"Double-Click to Set", @"");
			}
			return mappingName;
		}
	}
	
	return @"";
}

@end
