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

@interface PluginList : NSObject {
    @private
    NSMutableArray *pluginList;
	 
	PcsxrPlugin *activeGpuPlugin;
	PcsxrPlugin *activeSpuPlugin;
	PcsxrPlugin *activeCdrPlugin;
	PcsxrPlugin *activePadPlugin;
	PcsxrPlugin *activeNetPlugin;
	 
	BOOL missingPlugins;
}

+ (PluginList *)list;

- (void)refreshPlugins;
- (NSArray *)pluginsForType:(int)typeMask;
- (BOOL)hasPluginAtPath:(NSString *)path;
- (BOOL)configured;
- (PcsxrPlugin *)activePluginForType:(int)type;
- (BOOL)setActivePlugin:(PcsxrPlugin *)plugin forType:(int)type;

@end
