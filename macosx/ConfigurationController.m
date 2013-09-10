#import "ConfigurationController.h"
#import "PcsxrController.h"
#import "PluginList.h"
#import "PcsxrPlugin.h"
#import "PcsxrMemCardController.h"
#import "PcsxrMemCardHandler.h"
#include "psxcommon.h"
#include "plugins.h"

NSString *const memChangeNotifier = @"PcsxrMemoryCardDidChangeNotifier";
NSString *const memCardChangeNumberKey = @"PcsxrMemoryCardThatChangedKey";

@implementation ConfigurationController

+ (void)setMemoryCard:(NSInteger)theCard toURL:(NSURL *)theURL;
{
	if (theCard == 1) {
		[[NSUserDefaults standardUserDefaults] setURL:theURL forKey:@"Mcd1"];
		strlcpy(Config.Mcd1, [[theURL path] fileSystemRepresentation], MAXPATHLEN );
	} else {
		[[NSUserDefaults standardUserDefaults] setURL:theURL forKey:@"Mcd2"];
		strlcpy(Config.Mcd2, [[theURL path] fileSystemRepresentation], MAXPATHLEN );
	}
	
	[[NSNotificationCenter defaultCenter] postNotificationName:memChangeNotifier object:nil userInfo:
	 @{memCardChangeNumberKey: @(theCard)}];
}

+ (void)setMemoryCard:(NSInteger)theCard toPath:(NSString *)theFile
{
	[self setMemoryCard:theCard toURL:[NSURL fileURLWithPath:theFile isDirectory:NO]];
}

- (IBAction)setCheckbox:(id)sender
{
	if ([sender isKindOfClass:[NSMatrix class]]) {
		sender = [sender selectedCell];
	}

	NSString *key = [self keyForSender:sender];
	if (key) {
		[[NSUserDefaults standardUserDefaults] setBool:[sender intValue] ? YES : NO forKey:key];
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
		[[NSUserDefaults standardUserDefaults] setBool:[sender intValue] ? NO : YES forKey:key];
		[PcsxrController setConfigFromDefaults];
	}
}

+ (void)mcdChangeClicked:(id)sender
{
	NSInteger tag = [sender tag];
	char *mcd;
	NSOpenPanel *openDlg = [NSOpenPanel openPanel];
	NSString *path;
	
	if (tag == 1) {
		mcd = Config.Mcd1;
	} else {
		mcd = Config.Mcd2;
	}
	
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowedFileTypes:[PcsxrMemCardHandler supportedUTIs]];
	
	path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:mcd length:strlen(mcd)];
    
    [openDlg setDirectoryURL:[NSURL fileURLWithPath:[path stringByDeletingLastPathComponent] isDirectory:YES]];
    [openDlg setNameFieldStringValue:[path lastPathComponent]];
	
	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
		NSURL *mcdURL = [openDlg URLs][0];
        
		[ConfigurationController setMemoryCard:tag toURL:mcdURL];
    }
}

+ (void)mcdNewClicked:(id)sender
{
	NSInteger tag = [sender tag];
	char *mcd;
	NSSavePanel *openDlg = [NSSavePanel savePanel];
	NSString *path;
	
	if (tag == 1) {
		mcd = Config.Mcd1;
	} else {
		mcd = Config.Mcd2;
	}
	
    path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:mcd length:strlen(mcd)];
	
    [openDlg setDirectoryURL:[NSURL fileURLWithPath:[path stringByDeletingLastPathComponent] isDirectory:YES]];
    [openDlg setNameFieldStringValue:NSLocalizedString(@"New Memory Card.mcd", nil)];
	[openDlg setAllowedFileTypes:[PcsxrMemCardHandler supportedUTIs]];
    
	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
        NSURL *mcdURL = [openDlg URL];
		
		//Workaround/kludge to make sure we create a memory card before posting a notification
		strlcpy(mcd, [[mcdURL path] fileSystemRepresentation], MAXPATHLEN);
		CreateMcd(mcd);
		
		[ConfigurationController setMemoryCard:tag toURL:mcdURL];
    }
}

- (IBAction)setVideoType:(id)sender
{
	NSInteger tag = [[sender selectedItem] tag];

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

- (void)awakeFromNib
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[[self window] center];
	
	// setup checkboxes
	checkBoxDefaults = [[NSMutableDictionary alloc] init];

	// check that the outlets are active before adding them
	if (noXaAudioCell) checkBoxDefaults[@"NoXaAudio"] = noXaAudioCell;
	if (enableNetPlayCell) checkBoxDefaults[@"NetPlay"] = enableNetPlayCell;
	if (sioIrqAlwaysCell) checkBoxDefaults[@"SioIrqAlways"] = sioIrqAlwaysCell;
	if (bwMdecCell) checkBoxDefaults[@"BlackAndWhiteMDECVideo"] = bwMdecCell;
	if (autoVTypeCell) checkBoxDefaults[@"AutoDetectVideoType"] = autoVTypeCell;
	if (vTypePALCell) checkBoxDefaults[@"VideoTypePAL"] = vTypePALCell;
	if (noCDAudioCell) checkBoxDefaults[@"NoCDAudio"] = noCDAudioCell;
	if (usesHleCell) checkBoxDefaults[@"UseHLE"] = usesHleCell;
	if (usesDynarecCell) checkBoxDefaults[@"NoDynarec"] = usesDynarecCell;
	if (consoleOutputCell) checkBoxDefaults[@"ConsoleOutput"] = consoleOutputCell;
	if (spuIrqAlwaysCell) checkBoxDefaults[@"SpuIrqAlways"] = spuIrqAlwaysCell;
	if (rCountFixCell) checkBoxDefaults[@"RootCounterFix"] = rCountFixCell;
	if (vSyncWAFixCell) checkBoxDefaults[@"VideoSyncWAFix"] = vSyncWAFixCell;
	if (noFastBootCell) checkBoxDefaults[@"NoFastBoot"] = noFastBootCell;
	if (widescreen) checkBoxDefaults[@"Widescreen"] = widescreen;

	// make the visuals match the defaults
	
	for (NSString* key in checkBoxDefaults) {
		if ([defaults integerForKey:key]) {
			[checkBoxDefaults[key] setNextState];
		}
	}
	
	// special cases
	if (![PcsxrController biosAvailable]) {
		// no bios means always use HLE
		[usesHleCell setState:NSOnState];
		[usesHleCell setEnabled:NO];
	}
	

	// setup labels

	NSInteger tag = [defaults integerForKey:@"AutoDetectVideoType"];
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
	[sio1Plugin setPluginsTo:[list pluginsForType:PSE_LT_SIO1] withType:PSE_LT_SIO1];
    
    // Setup hotkey view
    [hkController initialize];
}

- (NSString *)keyForSender:(id)sender
{
	for (NSString *key in checkBoxDefaults) {
		id object = checkBoxDefaults[key];
		if ([object isEqual:sender])
			return key;
	}
	
	return nil;
}

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSWindow *window = [self window];
    if(tabViewItem == hkTab) {
        [window makeFirstResponder:(NSView*)hkController];
    }
    else if([window firstResponder] == (NSView*)hkController) {
        [hkController resignFirstResponder];
    }
}

@end
