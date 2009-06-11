#import <Cocoa/Cocoa.h>
#import "PcsxController.h"
#import "ConfigurationController.h"
#import "EmuThread.h"
#include "psxcommon.h"
#include "plugins.h"
#include "misc.h"
#include "ExtendedKeys.h"

NSDictionary *prefStringKeys;
NSDictionary *prefLongKeys;
NSMutableArray *biosList;
NSString *saveStatePath;

@implementation PcsxController

- (IBAction)ejectCD:(id)sender
{
	NSMutableString *deviceName;
	NSTask *ejectTask;
	NSRange rdiskRange;
	
	BOOL wasPaused = [EmuThread pauseSafe];

	/* close connection to current cd */
	if ([EmuThread active])
		CDR_close();

	// switch to another ISO if using internal image reader, otherwise eject the CD
	if (cdrfilename[0] != '\0') {
		NSOpenPanel* openDlg = [NSOpenPanel openPanel];

		[openDlg setCanChooseFiles:YES];
		[openDlg setCanChooseDirectories:NO];

		if ([openDlg runModalForDirectory:nil file:nil] == NSOKButton) {
			NSArray* files = [openDlg filenames];
			strcpy(cdrfilename, (const char *)[[files objectAtIndex:0] UTF8String]);
		}

        cdOpenCase = time(NULL) + 2;
	} else {
		if (CDR_getDriveLetter() != nil) {
			deviceName = [NSMutableString stringWithCString:CDR_getDriveLetter()];

			// delete the 'r' in 'rdisk'
			rdiskRange = [deviceName rangeOfString:@"rdisk"];
			if (rdiskRange.length != 0) {
				rdiskRange.length = 1;
				[deviceName deleteCharactersInRange:rdiskRange];
			}
			// execute hdiutil to eject the device
			ejectTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/hdiutil" arguments:[NSArray arrayWithObjects:@"eject", deviceName, nil]];
			[ejectTask waitUntilExit];
		}
	}

	/* and open new cd */
	if ([EmuThread active])
		CDR_open();

	if (!wasPaused) {
		[EmuThread resume];
	}
}

- (IBAction)memCardConfig:(id)sender
{
	NSRunAlertPanel(NSLocalizedString(@"Unimplemented feature", nil),
			NSLocalizedString(@"Configuration of memory cards has not yet been implemented, but will be available in the future.", nil), 
			nil, nil, nil);
}

- (IBAction)pause:(id)sender
{
    if ([EmuThread isPaused]) {
        //[sender setState:NSOffState];
        [EmuThread resume];
    }
    else {
        //[sender setState:NSOnState];
        [EmuThread pause];
    }
}

- (IBAction)preferences:(id)sender
{
	/* load the nib if it hasn't yet */
	if (preferenceWindow == nil) {
		if (preferencesController == nil) {
			preferencesController = [[ConfigurationController alloc] initWithWindowNibName:@"Configuration"];
		}
		preferenceWindow = [preferencesController window];
	}
	
	/* show the window */
	[preferenceWindow makeKeyAndOrderFront:self];
	[preferencesController showWindow:self];
}

- (IBAction)reset:(id)sender
{
    [EmuThread reset];
}

- (IBAction)runCD:(id)sender
{
	cdrfilename[0] = '\0';
	[EmuThread run];
}

- (IBAction)runIso:(id)sender
{
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];

	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];

	if ([openDlg runModalForDirectory:nil file:nil] == NSOKButton) {
		NSArray* files = [openDlg filenames];
		strcpy(cdrfilename, (const char *)[[files objectAtIndex:0] UTF8String]);
		[EmuThread run];
    }
}

- (IBAction)runExe:(id)sender
{
}

- (IBAction)freeze:(id)sender
{
	int num = [sender tag];
	NSString *path = [NSString stringWithFormat:@"%@/%s-%3.3d.pcsxstate", saveStatePath, CdromId, num];

	[EmuThread freezeAt:path which:num-1];
}

- (IBAction)defrost:(id)sender
{
	NSString *path = [NSString stringWithFormat:@"%@/%s-%3.3d.pcsxstate", saveStatePath, CdromId, [sender tag]];
	[EmuThread defrostAt:path];
}

- (IBAction)fullscreen:(id)sender
{
	GPU_keypressed(GPU_FULLSCREEN_KEY);
}

- (BOOL)validateMenuItem:(id <NSMenuItem>)menuItem
{
	if ([menuItem action] == @selector(pause:)) {
		[menuItem setState:([EmuThread isPaused] ? NSOnState : NSOffState)];
	}

	if ([menuItem action] == @selector(reset:) || [menuItem action] == @selector(pause:) ||
		 [menuItem action] == @selector(ejectCD:) || [menuItem action] == @selector(freeze:) ||
		 [menuItem action] == @selector(fullscreen:))
		return [EmuThread active];

    if ([menuItem action] == @selector(runCD:) || [menuItem action] == @selector(runIso:))
        return ![EmuThread active];

	if ([menuItem action] == @selector(defrost:)) {
		if (![EmuThread active])
			return NO;

		NSString *path = [NSString stringWithFormat:@"%@/%s-%3.3d.pcsxstate", saveStatePath, CdromId, [menuItem tag]];
		return (CheckState((char *)[path fileSystemRepresentation]) == 0);
	}
	
	return YES;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
	wasPausedBeforeBGSwitch = [EmuThread isPaused];

	if (sleepInBackground) {
		 [EmuThread pause];
	}
}

- (void)applicationDidBecomeActive:(NSNotification *)aNotification
{
	if (sleepInBackground && !wasPausedBeforeBGSwitch) {
		[EmuThread resume];
	}
}

- (void)awakeFromNib
{
	pluginList = [[PluginList alloc] init];
	if (![pluginList configured] /*!Config.Gpu[0] || !Config.Spu[0] || !Config.Pad1[0] || !Config.Cdr[0]*/) {
		// configure plugins
		[self preferences:nil];

		NSRunCriticalAlertPanel(NSLocalizedString(@"Missing plugins!", nil),
				NSLocalizedString(@"Pcsx is missing one or more critical plugins. You will need to install these in order to play games.", nil), 
				nil, nil, nil);
	}

	if (![PcsxController biosAvailable]) {
		NSRunInformationalAlertPanel(NSLocalizedString(@"Missing BIOS!", nil),
				NSLocalizedString(@"Pcsx wasn't able to locate any Playstation BIOS ROM files. This means that it will run in BIOS simulation mode which is less stable and compatible than using a real Playstation BIOS.\n"
										@"If you have a BIOS available, please copy it to\n~/Library/Application Support/Pcsx/Bios/", nil), 
				nil, nil, nil);
	}

	sleepInBackground = YES;
}

- (void)dealloc
{
	[pluginList release];
	
	[super dealloc];
}

+ (void)setConfigFromDefaults
{
	NSEnumerator *enumerator;
	const char *str;
	NSString *key;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	/*
	enumerator = [prefStringKeys keyEnumerator];
	while ((key = [enumerator nextObject])) {
		str = [[defaults stringForKey:key] fileSystemRepresentation];
		char *dst = (char *)[[prefStringKeys objectForKey:key] pointerValue];
		if (str != nil && dst != nil) strncpy(dst, str, 255);
	}*/
	
	enumerator = [prefLongKeys keyEnumerator];
	while ((key = [enumerator nextObject])) {
		long *dst = (long *)[[prefLongKeys objectForKey:key] pointerValue];
		if (dst != nil) *dst = [defaults integerForKey:key];
	}

	// special cases
	//str = [[defaults stringForKey:@"PluginPAD"] fileSystemRepresentation];
	//if (str != nil) strncpy(Config.Pad2, str, 255);
	
	str = [[defaults stringForKey:@"Bios"] fileSystemRepresentation];
	if (str) {
		NSString *path = [defaults stringForKey:@"Bios"];
		int index = [biosList indexOfObject:path];

		if (-1 == index) {
			[biosList insertObject:path atIndex:0];
		} else if (0 < index) {
			[biosList exchangeObjectAtIndex:index withObjectAtIndex:0];
		}
	}
	
	if ([defaults boolForKey:@"UseHLE"] || 0 == [biosList count]) {
		strcpy(Config.Bios, "HLE");
	} else {
		str = [(NSString *)[biosList objectAtIndex:0] fileSystemRepresentation];
		if (str != nil) strncpy(Config.Bios, str, 255);
		else strcpy(Config.Bios, "HLE");
	}

	// FIXME: hack
	strcpy(Config.Net, "Disabled");
}

+ (void)setDefaultFromConfig:(NSString *)defaultKey
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	char *str = (char *)[[prefStringKeys objectForKey:defaultKey] pointerValue];
	if (str) {
		[defaults setObject:[NSString stringWithCString:str] forKey:defaultKey];
		return;
	}
	
	long *val = (long *)[[prefLongKeys objectForKey:defaultKey] pointerValue];
	if (val) {
		[defaults setInteger:*val forKey:defaultKey];
		return;
	}
}

+ (BOOL)biosAvailable
{
	return ([biosList count] > 0);
}

// called when class is initialized
+ (void)initialize
{
	NSString *path;
	const char *str;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
		@"Disabled", @"PluginNET",
		[NSNumber numberWithInt:0], @"NoDynarec",
		[NSNumber numberWithInt:1], @"AutoDetectVideoType", 
//		@"HLE", @"Bios",
		[NSNumber numberWithInt:0], @"UseHLE",
		[NSNumber numberWithInt:1], @"CpuBiasShift",
		nil];

	[defaults registerDefaults:appDefaults];
	
	prefStringKeys = [[NSDictionary alloc] initWithObjectsAndKeys:
		[NSValue valueWithPointer:Config.Gpu], @"PluginGPU",
		[NSValue valueWithPointer:Config.Spu], @"PluginSPU",
		[NSValue valueWithPointer:Config.Pad1], @"PluginPAD",
		[NSValue valueWithPointer:Config.Cdr], @"PluginCDR",
		[NSValue valueWithPointer:Config.Net], @"PluginNET",
//		[NSValue valueWithPointer:Config.Bios], @"Bios",
		nil];

	prefLongKeys = [[NSDictionary alloc] initWithObjectsAndKeys:
		[NSValue valueWithPointer:&Config.Xa], @"NoXaAudio",
		[NSValue valueWithPointer:&Config.Sio], @"SioIrqAlways",
		[NSValue valueWithPointer:&Config.Mdec], @"BlackAndWhiteMDECVideo",
		[NSValue valueWithPointer:&Config.PsxAuto], @"AutoDetectVideoType",
		[NSValue valueWithPointer:&Config.PsxType], @"VideoTypePAL",
		[NSValue valueWithPointer:&Config.Cdda], @"NoCDAudio",
		[NSValue valueWithPointer:&Config.Cpu], @"NoDynarec",
		[NSValue valueWithPointer:&Config.PsxOut], @"ConsoleOutput",
		[NSValue valueWithPointer:&Config.SpuIrq], @"SpuIrqAlways",
		[NSValue valueWithPointer:&Config.RCntFix], @"RootCounterFix",
		[NSValue valueWithPointer:&Config.VSyncWA], @"VideoSyncWAFix",
		nil];
	
	// setup application support paths
	NSArray *libPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
	if ([libPaths count] > 0) {
		NSString *path;
		BOOL dir;
		
		// create them if needed
		NSFileManager *dfm = [NSFileManager defaultManager];
		NSString *supportPath = [NSString stringWithFormat:@"%@/Application Support", [libPaths objectAtIndex:0]];
		if (![dfm fileExistsAtPath:supportPath isDirectory:&dir])
			[dfm createDirectoryAtPath:supportPath attributes:nil];
	
		path = [NSString stringWithFormat:@"%@/Pcsx", supportPath];
		if (![dfm fileExistsAtPath:path isDirectory:&dir])
			[dfm createDirectoryAtPath:path attributes:nil];
		
		path = [NSString stringWithFormat:@"%@/Pcsx/Bios", supportPath];
		if (![dfm fileExistsAtPath:path isDirectory:&dir])
			[dfm createDirectoryAtPath:path attributes:nil];
		
		path = [NSString stringWithFormat:@"%@/Pcsx/Memory Cards", supportPath];
		if (![dfm fileExistsAtPath:path isDirectory:&dir])
			[dfm createDirectoryAtPath:path attributes:nil];
		
		saveStatePath = [[NSString stringWithFormat:@"%@/Pcsx/Save States", supportPath] retain];
		if (![dfm fileExistsAtPath:saveStatePath isDirectory:&dir])
			[dfm createDirectoryAtPath:saveStatePath attributes:nil];
		
		
		path = [NSString stringWithFormat:@"%@/Pcsx/Memory Cards/Mcd001.mcr", supportPath];
		str = [path fileSystemRepresentation];
		if (str != nil) strncpy(Config.Mcd1, str, 255);
		
		path = [NSString stringWithFormat:@"%@/Pcsx/Memory Cards/Mcd002.mcr", supportPath];
		str = [path fileSystemRepresentation];
		if (str != nil) strncpy(Config.Mcd2, str, 255);

		path = [NSString stringWithFormat:@"%@/Pcsx/Bios/", supportPath];
		str = [path fileSystemRepresentation];
		if (str != nil) strncpy(Config.BiosDir, str, 255);
	} else {
		strcpy(Config.BiosDir, "Bios/");
		
		saveStatePath = @"sstates";
		[saveStatePath retain];
	}

	// set plugin path
	path = [[[NSBundle mainBundle] builtInPlugInsPath] stringByAppendingString:@"/"];
	str = [path fileSystemRepresentation];
	if (str != nil) strncpy(Config.PluginsDir, str, 255);

	// locate a bios
	biosList = [[NSMutableArray alloc] init];
	NSFileManager *manager = [NSFileManager defaultManager];
	NSArray *bioses = [manager directoryContentsAtPath:[NSString stringWithCString:Config.BiosDir]];
	if (bioses) {
		int i;
		for (i=0; i<[bioses count]; i++) {
			NSString *file = [bioses objectAtIndex:i];
			NSDictionary *attrib = [manager fileAttributesAtPath:[NSString stringWithFormat:@"%s%@", Config.BiosDir, file] traverseLink:YES];

			if ([[attrib fileType] isEqualToString:NSFileTypeRegular]) {
				unsigned long long size = [attrib fileSize];
				if (([attrib fileSize] % (256*1024)) == 0 && size > 0) {
					[biosList addObject:file];
				}
			}
		}
	}
	
	[PcsxController setConfigFromDefaults];
}


@end
