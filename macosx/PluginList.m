//
//  PluginList.m
//  Pcsxr
//
//  Created by Gil Pedersen on Sun Sep 21 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import "EmuThread.h"
#import "PluginList.h"
#import "PcsxrPlugin.h"
#include "psxcommon.h"
#include "plugins.h"

static PluginList __weak *sPluginList = nil;
const static int typeList[] = {PSE_LT_GPU, PSE_LT_SPU, PSE_LT_CDR, PSE_LT_PAD, PSE_LT_NET, PSE_LT_SIO1};

@interface PluginList ()
@property (strong) NSMutableArray *pluginList;
@property BOOL missingPlugins;
@property (strong) PcsxrPlugin *activeGpuPlugin;
@property (strong) PcsxrPlugin *activeSpuPlugin;
@property (strong) PcsxrPlugin *activeCdrPlugin;
@property (strong) PcsxrPlugin *activePadPlugin;
@property (strong) PcsxrPlugin *activeNetPlugin;
@property (strong) PcsxrPlugin *activeSIO1Plugin;

@end

@implementation PluginList
@synthesize missingPlugins;

+ (PluginList *)list
{
	return sPluginList;
}

- (id)init
{
	NSUInteger i;
	
	if (!(self = [super init]))
	{
		return nil;
	}
	
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	self.pluginList = [[NSMutableArray alloc] initWithCapacity:20];

	self.activeGpuPlugin = self.activeSpuPlugin = self.activeCdrPlugin = self.activePadPlugin = self.activeNetPlugin = self.activeSIO1Plugin = nil;
	
	missingPlugins = NO;
	for (i = 0; i < sizeof(typeList) / sizeof(typeList[0]); i++) {
		NSString *path = [defaults stringForKey:[PcsxrPlugin defaultKeyForType:typeList[i]]];
		if (nil == path) {
			missingPlugins = YES;
			continue;
		}
		if ([path isEqualToString:@"Disabled"])
			continue;
		
		if (![self hasPluginAtPath:path]) {
			@autoreleasepool {
				PcsxrPlugin *plugin = [[PcsxrPlugin alloc] initWithPath:path];
				if (plugin) {
					[self.pluginList addObject:plugin];
					if (![self setActivePlugin:plugin forType:typeList[i]])
						missingPlugins = YES;
				} else {
					missingPlugins = YES;
				}
			}
		}
	}
		
	if (missingPlugins) {
		[self refreshPlugins];
	}
	
	sPluginList = self;
	
	return self;
}

- (void)refreshPlugins
{
	NSDirectoryEnumerator *dirEnum;
	NSString *pname;
	NSUInteger i;
	
	// verify that the ones that are in list still works
	for (i=0; i < [self.pluginList count]; i++) {
		if (![(self.pluginList)[i] verifyOK]) {
			[self.pluginList removeObjectAtIndex:i]; i--;
		}
	}
	
	for (NSString *plugDir in [PcsxrPlugin pluginsPaths])
	{
		// look for new ones in the plugin directory
		dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:plugDir];
		
		while ((pname = [dirEnum nextObject])) {
			if ([[pname pathExtension] isEqualToString:@"psxplugin"] || 
				[[pname pathExtension] isEqualToString:@"so"]) {
				[dirEnum skipDescendents]; /* don't enumerate this
											directory */
				
				if (![self hasPluginAtPath:pname]) {
					@autoreleasepool {
						PcsxrPlugin *plugin = [[PcsxrPlugin alloc] initWithPath:pname];
						if (plugin != nil) {
							[self.pluginList addObject:plugin];
						}
					}
				}
			}
		}
	}
	
	// check the we have the needed plugins
	missingPlugins = NO;
	for (i=0; i < 4 /*sizeof(*typeList)*/; i++) {
		PcsxrPlugin *plugin = [self activePluginForType:typeList[i]];
		if (nil == plugin) {
			NSArray *list = [self pluginsForType:typeList[i]];
			NSUInteger j;
			
			for (j=0; j < [list count]; j++) {
				if ([self setActivePlugin:list[j] forType:typeList[i]])
					break;
			}
			if (j == [list count])
				missingPlugins = YES;
		}
	}
}

- (NSArray *)pluginsForType:(int)typeMask
{
	NSMutableArray *types = [NSMutableArray array];
	
	for (PcsxrPlugin *plugin in self.pluginList) {
		if ([plugin type] & typeMask) {
			[types addObject:plugin];
		}
	}
		
	return types;
}

- (BOOL)hasPluginAtPath:(NSString *)path
{
	if (nil == path)
		return NO;
	
	for (PcsxrPlugin *plugin in self.pluginList) {
		if ([[plugin path] isEqualToString:path])
			return YES;
	}
		
	return NO;
}

// returns if all the required plugins are available
- (BOOL)configured
{
	return !missingPlugins;
}

- (BOOL)doInitPlugins
{
	BOOL bad = NO;
	
	if ([self.activeGpuPlugin runAs:PSE_LT_GPU] != 0) bad = YES;
	if ([self.activeSpuPlugin runAs:PSE_LT_SPU] != 0) bad = YES;
	if ([self.activeCdrPlugin runAs:PSE_LT_CDR] != 0) bad = YES;
	if ([self.activePadPlugin runAs:PSE_LT_PAD] != 0) bad = YES;
	if ([self.activeNetPlugin runAs:PSE_LT_NET] != 0) bad = YES;
	if ([self.activeSIO1Plugin runAs:PSE_LT_SIO1] != 0) bad = YES;
	
	return !bad;
}

- (PcsxrPlugin *)activePluginForType:(int)type
{
	switch (type) {
		case PSE_LT_GPU: return self.activeGpuPlugin; break;
		case PSE_LT_CDR: return self.activeCdrPlugin; break;
		case PSE_LT_SPU: return self.activeSpuPlugin; break;
		case PSE_LT_PAD: return self.activePadPlugin; break;
		case PSE_LT_NET: return self.activeNetPlugin; break;
		case PSE_LT_SIO1: return self.activeSIO1Plugin; break;
	}
	
	return nil;
}

- (BOOL)setActivePlugin:(PcsxrPlugin *)plugin forType:(int)type
{
	PcsxrPlugin *pluginPtr = nil;
	
	switch (type) {
		case PSE_LT_SIO1:
		case PSE_LT_GPU:
		case PSE_LT_CDR:
		case PSE_LT_SPU:
		case PSE_LT_PAD:
		case PSE_LT_NET: pluginPtr = [self activePluginForType:type]; break;
		default: return NO; break;
	}
	if (plugin == pluginPtr) {
		return YES;
	}

	BOOL active = pluginPtr && [EmuThread active];
	BOOL wasPaused = NO;
	if (active) {
		// TODO: temporary freeze?
		wasPaused = [EmuThread pauseSafe];
		ClosePlugins();
		ReleasePlugins();
	}

	// stop the old plugin and start the new one
	if (pluginPtr) {
		[pluginPtr shutdownAs:type];
		pluginPtr = nil;
	}
	
	if ([plugin runAs:type] != 0) {
		plugin = nil;
	}
		switch (type) {
			case PSE_LT_GPU:
				self.activeGpuPlugin = plugin;
				break;
			case PSE_LT_CDR:
				self.activeCdrPlugin = plugin;
				break;
			case PSE_LT_SPU:
				self.activeSpuPlugin = plugin;
				break;
			case PSE_LT_PAD:
				self.activePadPlugin = plugin;
				break;
			case PSE_LT_NET:
				self.activeNetPlugin = plugin;
				break;
			case PSE_LT_SIO1:
				self.activeSIO1Plugin = plugin;
				break;
	}
	
	// write path to the correct config entry
	const char *str;
	if (plugin != nil) {
		str = [[plugin path] fileSystemRepresentation];
		if (str == NULL) {
			str = "Invalid Plugin";
		}
	} else {
		str = "Invalid Plugin";
	}
	
	char **dst = [PcsxrPlugin configEntriesForType:type];
	while (*dst) {
		strlcpy(*dst, str, MAXPATHLEN);
		dst++;
	}
	
	if (active) {
		LoadPlugins();
		OpenPlugins();
		
		if (!wasPaused) {
			[EmuThread resume];
		}
	}
	
	return plugin != nil;
}

- (void)disableNetPlug
{
	char **dst = [PcsxrPlugin configEntriesForType:PSE_LT_NET];
	while (*dst) {
		strcpy(*dst, "Disabled");
		dst++;
	}
}

- (void)enableNetPlug
{
	PcsxrPlugin *netPlug = [self activePluginForType:PSE_LT_NET];
	
	const char *str = NULL;
	if (netPlug) {
		str = [[netPlug path] fileSystemRepresentation];
	}
	if (str) {
		char **dst = [PcsxrPlugin configEntriesForType:PSE_LT_NET];
		while (*dst) {
			strlcpy(*dst, str, MAXPATHLEN);
			dst++;
		}
	}
	
}

@end
