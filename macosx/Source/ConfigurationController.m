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

@interface ConfigurationController ()
@property (strong) NSMutableDictionary *checkBoxDefaults;
- (NSString *)keyForSender:(id)sender;
@end

@implementation ConfigurationController
@synthesize autoVTypeCell;
@synthesize bwMdecCell;
@synthesize checkBoxDefaults = _checkBoxDefaults;
@synthesize consoleOutputCell;
@synthesize enableNetPlayCell;
@synthesize noCDAudioCell;
@synthesize noFastBootCell;
@synthesize noXaAudioCell;
@synthesize rCountFixCell;
@synthesize sioIrqAlwaysCell;
@synthesize spuIrqAlwaysCell;
@synthesize usesDynarecCell;
@synthesize usesHleCell;
@synthesize vSyncWAFixCell;
@synthesize vTypePALCell;
@synthesize widescreen;
@synthesize cdromPlugin;
@synthesize graphicsPlugin;
@synthesize padPlugin;
@synthesize soundPlugin;
@synthesize netPlugin;
@synthesize sio1Plugin;
@synthesize memCardEdit;
@synthesize hkController;
@synthesize hkTab;

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

- (IBAction)mcdChangeClicked:(id)sender
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
	
	path = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:mcd length:strlen(mcd)];
	
	[openDlg setAllowedFileTypes:[PcsxrMemCardHandler supportedUTIs]];
    [openDlg setDirectoryURL:[NSURL fileURLWithPath:[path stringByDeletingLastPathComponent] isDirectory:YES]];
    [openDlg setNameFieldStringValue:[path lastPathComponent]];
	[openDlg beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
		if (result == NSFileHandlingPanelOKButton) {
			NSURL *mcdURL = [openDlg URLs][0];
			
			[ConfigurationController setMemoryCard:tag toURL:mcdURL];
		}
	}];
}

- (IBAction)mcdNewClicked:(id)sender
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
    
	[openDlg beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
		if (result == NSFileHandlingPanelOKButton) {
			NSURL *mcdURL = [openDlg URL];
			const char *fileSysRep;
			
			if ([mcdURL respondsToSelector:@selector(fileSystemRepresentation)]) {
				fileSysRep = [mcdURL fileSystemRepresentation];
			} else {
				fileSysRep = [[mcdURL path] fileSystemRepresentation];
			}
			
			//Workaround/kludge to make sure we create a memory card before posting a notification
			strlcpy(mcd, fileSysRep, MAXPATHLEN);
			CreateMcd(mcd);
			
			[ConfigurationController setMemoryCard:tag toURL:mcdURL];
		}
	}];
}

- (IBAction)setVideoType:(id)sender
{
	NSInteger tag = [[sender selectedItem] tag];
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	if (3 == tag) {
		[defaults setBool:YES forKey:@"AutoDetectVideoType"];
	} else if (1 == tag || 2 == tag) {
		[defaults setBool:NO forKey:@"AutoDetectVideoType"];
		[defaults setBool:tag==2 forKey:@"VideoTypePAL"];
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
	self.checkBoxDefaults = [[NSMutableDictionary alloc] init];

	// check that the outlets are active before adding them
	if (noXaAudioCell)
		_checkBoxDefaults[@"NoXaAudio"] = noXaAudioCell;
	if (enableNetPlayCell)
		_checkBoxDefaults[@"NetPlay"] = enableNetPlayCell;
	if (sioIrqAlwaysCell)
		_checkBoxDefaults[@"SioIrqAlways"] = sioIrqAlwaysCell;
	if (bwMdecCell)
		_checkBoxDefaults[@"BlackAndWhiteMDECVideo"] = bwMdecCell;
	if (autoVTypeCell)
		_checkBoxDefaults[@"AutoDetectVideoType"] = autoVTypeCell;
	if (vTypePALCell)
		_checkBoxDefaults[@"VideoTypePAL"] = vTypePALCell;
	if (noCDAudioCell)
		_checkBoxDefaults[@"NoCDAudio"] = noCDAudioCell;
	if (usesHleCell)
		_checkBoxDefaults[@"UseHLE"] = usesHleCell;
	if (usesDynarecCell)
		_checkBoxDefaults[@"NoDynarec"] = usesDynarecCell;
	if (consoleOutputCell)
		_checkBoxDefaults[@"ConsoleOutput"] = consoleOutputCell;
	if (spuIrqAlwaysCell)
		_checkBoxDefaults[@"SpuIrqAlways"] = spuIrqAlwaysCell;
	if (rCountFixCell)
		_checkBoxDefaults[@"RootCounterFix"] = rCountFixCell;
	if (vSyncWAFixCell)
		_checkBoxDefaults[@"VideoSyncWAFix"] = vSyncWAFixCell;
	if (noFastBootCell)
		_checkBoxDefaults[@"NoFastBoot"] = noFastBootCell;
	if (widescreen)
		_checkBoxDefaults[@"Widescreen"] = widescreen;

	// make the visuals match the defaults
	
	for (NSString* key in _checkBoxDefaults) {
		if ([defaults integerForKey:key]) {
			[_checkBoxDefaults[key] setNextState];
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
	for (NSString *key in [self.checkBoxDefaults keyEnumerator]) {
		id object = (self.checkBoxDefaults)[key];
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
