#import "ConfigurationController.h"
#import "PcsxrController.h"
#import "PluginList.h"
#import "PcsxrPlugin.h"
#import "PcsxrMemCardController.h"
#import "PcsxrMemCardHandler.h"
#include "psxcommon.h"
#include "plugins.h"

NSString *memChangeNotifier = @"PcsxrMemoryCardDidChangeNotifier";

@implementation ConfigurationController

+ (void)setMemoryCard:(int)theCard toPath:(NSString *)theFile
{
	if (theCard == 1) {
		[[NSUserDefaults standardUserDefaults] setObject:theFile forKey:@"Mcd1"];
		strlcpy(Config.Mcd1, [theFile fileSystemRepresentation], MAXPATHLEN );
	} else {
		[[NSUserDefaults standardUserDefaults] setObject:theFile forKey:@"Mcd2"];
		strlcpy(Config.Mcd2, [theFile fileSystemRepresentation], MAXPATHLEN );
	}
	
	[[NSNotificationCenter defaultCenter] postNotificationName:memChangeNotifier object:nil];
}

- (IBAction)setCheckbox:(id)sender
{
	if ([sender isKindOfClass:[NSMatrix class]]) {
		sender = [sender selectedCell];
	}

	NSString *key = [self keyForSender:sender];
	if (key) {
		[[NSUserDefaults standardUserDefaults] setInteger:[sender intValue] forKey:key];
		[PcsxrController setConfigFromDefaults];
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
		[PcsxrController setConfigFromDefaults];
	}
}

- (IBAction)mcdChangeClicked:(id)sender
{
	int tag = [sender tag];
	char *mcd;
	NSTextField *label;
	NSOpenPanel *openDlg = [NSOpenPanel openPanel];
	[openDlg retain];
	NSString *path;

	if (tag == 1) { mcd = Config.Mcd1; label = mcd1Label; }
	else { mcd = Config.Mcd2; label = mcd2Label; }

	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowedFileTypes:[PcsxrMemCardHandler supportedUTIs]];

	path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:mcd length:strlen(mcd)];
    
    [openDlg setDirectoryURL:[NSURL fileURLWithPath:[path stringByDeletingLastPathComponent]]];
    [openDlg setNameFieldStringValue:[path lastPathComponent]];

	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
		NSArray* urls = [openDlg URLs];
        NSString *mcdPath = [[urls objectAtIndex:0] path];
        
		[ConfigurationController setMemoryCard:tag toPath:mcdPath];
    }
	[openDlg release];
}

- (IBAction)mcdNewClicked:(id)sender
{
	int tag = [sender tag];
	char *mcd;
	NSTextField *label;
	NSSavePanel *openDlg = [NSSavePanel savePanel];
	[openDlg retain];
	NSString *path;

	if (tag == 1) { mcd = Config.Mcd1; label = mcd1Label; }
	else { mcd = Config.Mcd2; label = mcd2Label; }

    path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:mcd length:strlen(mcd)];

    [openDlg setDirectoryURL:[NSURL fileURLWithPath:[path stringByDeletingLastPathComponent]]];
    [openDlg setNameFieldStringValue:@"New Memory Card File.mcr"];
	[openDlg setAllowedFileTypes:[PcsxrMemCardHandler supportedUTIs]];
    
	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
        NSString *mcdPath = [[openDlg URL] path];
		
		//Workaround/kludge to make sure we create a memory card before posting a notification
		strcpy(mcd, [mcdPath fileSystemRepresentation]);
		
		CreateMcd(mcd);

		[ConfigurationController setMemoryCard:tag toPath:mcdPath];
    }
	[openDlg release];
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
	[PcsxrController setConfigFromDefaults];

	if ([sender pullsDown]) {
		NSArray *items = [sender itemArray];
		for (id object in items) {
			[object setState:NSOffState];
		}
		
		[[sender selectedItem] setState:NSOnState];
	}
}

- (void)memoryCardDidChangeNotification:(NSNotification *)aNote
{
	NSString *path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:Config.Mcd1 length:strlen(Config.Mcd1)];
	[mcd1Label setTitleWithMnemonic:path];
	path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:Config.Mcd2 length:strlen(Config.Mcd2)];
	[mcd2Label setTitleWithMnemonic:path];
}

- (void)awakeFromNib
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[[self window] center];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(memoryCardDidChangeNotification:) name:memChangeNotifier object:nil];
	
	// setup checkboxes
	checkBoxDefaults = [[NSMutableDictionary alloc] init];

	// check that the outlets are active before adding them
	if (noXaAudioCell) [checkBoxDefaults setObject:noXaAudioCell forKey:@"NoXaAudio"];
	if (enableNetPlayCell) [checkBoxDefaults setObject:enableNetPlayCell forKey:@"NetPlay"];
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
	if (widescreen) [checkBoxDefaults setObject:widescreen forKey:@"Widescreen"];

	// make the visuals match the defaults
	NSEnumerator *enumerator= [checkBoxDefaults keyEnumerator];
	id key;
	while ((key = [enumerator nextObject])) {
		if ([defaults integerForKey:key]) {
			[[checkBoxDefaults objectForKey:key] setNextState];
		}
	}

	// special cases
	if (![PcsxrController biosAvailable]) {
		// no bios means always use HLE
		[usesHleCell setState:NSOnState];
		[usesHleCell setEnabled:NO];
	}

	// setup labels
	[mcd1Label setTitleWithMnemonic:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:Config.Mcd1 length:strlen(Config.Mcd1)]];
	[mcd2Label setTitleWithMnemonic:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:Config.Mcd2 length:strlen(Config.Mcd2)]];

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
	[netPlugin setPluginsTo:[list pluginsForType:PSE_LT_NET] withType: PSE_LT_NET];

}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[checkBoxDefaults release];
	if (memCardEdit) {
		[memCardEdit close];
		[memCardEdit release];
	}
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

- (IBAction)mcdEditClicked:(id)sender
{
	if (!memCardEdit) {
		memCardEdit = [[PcsxrMemCardController alloc] init];
	}
	[[memCardEdit window] center];
	[memCardEdit showWindow:sender];
}

- (BOOL)isMemoryCardWindowVisible
{
	if (!memCardEdit) {
		return NO;
	} else {
		return [[memCardEdit window] isVisible];
	}
}

@end
