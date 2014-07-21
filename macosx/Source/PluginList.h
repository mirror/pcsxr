//
//  PluginList.h
//  Pcsxr
//
//  Created by Gil Pedersen on Sun Sep 21 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PcsxrPlugin.h"

//extern NSMutableArray *plugins;

@interface PluginList : NSObject

+ (PluginList *)list;

- (void)refreshPlugins;
- (NSArray *)pluginsForType:(int)typeMask;
- (BOOL)hasPluginAtPath:(NSString *)path;
@property (readonly) BOOL configured;
- (PcsxrPlugin *)activePluginForType:(int)type;
- (BOOL)setActivePlugin:(PcsxrPlugin *)plugin forType:(int)type;

- (void)disableNetPlug;
- (void)enableNetPlug;

@end
