#import "ConfigurationController.h"
#import "PcsxController.h"
#import "PluginList.h"
#import "PcsxPlugin.h"
#include "psxcommon.h"
#include "plugins.h"

@implementation ConfigurationController

- (IBAction)setCheckbox:(id)sender
{
	if ([sender isKindOfClass:[NSMatrix class]]) {
		sender = [sender selectedCell];
	}

	NSString *key = [self keyForSender:sender];
	if (key) {
		[[NSUserDefaults standardUserDefaults] setInteger:[sender intValue] forKey:key];
		[PcsxController setConfigFromDefaults];
	}
}

- (IBAction)setCheckboxInverse:(id)sender
{
	if ([sender isKindOfClass:[NSMatrix class]]) {
		sender = [sender selectedCell];
	}

	NSString *key = [self keyForSender:sender];
	if (key) {
		[[NSUserDefaults standardUserDefaults] setInteger:![sender intValue] forKey:key];
		[PcsxController setConfigFromDefaults];
	}
}

- (IBAction)setVideoType:(id)sender
{
	int tag = [[sender selectedItem] tag];
	
	if (3 == tag) {
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"AutoDetectVideoType"];
	} else if (1 == tag || 2 == tag) {
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"AutoDetectVideoType"];
		[[NSUserDefaults standardUserDefaults] setBool:tag==2 forKey:@"VideoTypePAL"];
	} else {
		return;
	}
	[PcsxController setConfigFromDefaults];
	
	if ([sender pullsDown]) {
		NSArray *items = [sender itemArray];
		int i;
		
		for (i=0; i<[items count]; i++)
			[[items objectAtIndex:i] setState:NSOffState];
		
		[[sender selectedItem] setState:NSOnState];
	}
}

- (void)awakeFromNib
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[[self window] center];

	// setup checkboxes
	checkBoxDefaults = [[NSMutableDictionary alloc] init];

	// check that the outlets are active before adding them
	if (noXaAudioCell) [checkBoxDefaults setObject:noXaAudioCell forKey:@"NoXaAudio"];
	if (sioIrqAlwaysCell) [checkBoxDefaults setObject:sioIrqAlwaysCell forKey:@"SioIrqAlways"];
	if (bwMdecCell) [checkBoxDefaults setObject:bwMdecCell forKey:@"BlackAndWhiteMDECVideo"];
	if (autoVTypeCell) [checkBoxDefaults setObject:autoVTypeCell forKey:@"AutoDetectVideoType"];
	if (vTypePALCell) [checkBoxDefaults setObject:vTypePALCell forKey:@"VideoTypePAL"];
	if (noCDAudioCell) [checkBoxDefaults setObject:noCDAudioCell forKey:@"NoCDAudio"];
	if (usesHleCell) [checkBoxDefaults setObject:usesHleCell forKey:@"UseHLE"];
	if (usesDynarecCell) [checkBoxDefaults setObject:usesDynarecCell forKey:@"NoDynarec"];
	if (consoleOutputCell) [checkBoxDefaults setObject:consoleOutputCell forKey:@"ConsoleOutput"];
	if (spuIrqAlwaysCell) [checkBoxDefaults setObject:spuIrqAlwaysCell forKey:@"SpuIrqAlways"];
	if (rCountFixCell) [checkBoxDefaults setObject:rCountFixCell forKey:@"RootCounterFix"];
	if (vSyncWAFixCell) [checkBoxDefaults setObject:vSyncWAFixCell forKey:@"VideoSyncWAFix"];
	if (noFastBootCell) [checkBoxDefaults setObject:noFastBootCell forKey:@"NoFastBoot"];

	// make the visuals match the defaults
	NSEnumerator *enumerator= [checkBoxDefaults keyEnumerator];
	id key;
	while ((key = [enumerator nextObject])) {
		if ([defaults integerForKey:key]) {
			[[checkBoxDefaults objectForKey:key] setNextState];
		}
	}

	// special cases
	if (![PcsxController biosAvailable]) {
		// no bios means always use HLE
		[usesHleCell setState:NSOnState];
		[usesHleCell setEnabled:NO];
	}

	int tag = [defaults integerForKey:@"AutoDetectVideoType"];
	if (tag)
		tag = 3;
	else {
		tag = [defaults integerForKey:@"VideoTypePAL"]+1;
	}
	[vTypePALCell setAutoenablesItems:NO];
	if ([vTypePALCell pullsDown]) {
		[[vTypePALCell itemAtIndex:[vTypePALCell indexOfItemWithTag:tag]] setState:NSOnState];
	} else {
		[vTypePALCell selectItemAtIndex:[vTypePALCell indexOfItemWithTag:tag]];
	}

	// setup plugin lists
	PluginList *list = [PluginList list];

	[list refreshPlugins];
	[graphicsPlugin setPluginsTo:[list pluginsForType:PSE_LT_GPU] withType: PSE_LT_GPU];
	[soundPlugin setPluginsTo:[list pluginsForType:PSE_LT_SPU] withType: PSE_LT_SPU];
	[padPlugin setPluginsTo:[list pluginsForType:PSE_LT_PAD] withType: PSE_LT_PAD];
	[cdromPlugin setPluginsTo:[list pluginsForType:PSE_LT_CDR] withType: PSE_LT_CDR];
}

- (void)dealloc
{
	[checkBoxDefaults release];
	[super dealloc];
}

- (NSString *)keyForSender:(id)sender
{
	NSEnumerator *enumerator= [checkBoxDefaults keyEnumerator];
	id key;
	while ((key = [enumerator nextObject])) {
		id object = [checkBoxDefaults objectForKey:key];
		if ([object isEqual:sender])
			return key;
	}
	
	return nil;
}

@end
