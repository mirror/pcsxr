#import <Cocoa/Cocoa.h>
#import "PcsxrController.h"
#import "ConfigurationController.h"
#import "EmuThread.h"
#import "PcsxrMemCardHandler.h"
#import "PcsxrPluginHandler.h"
#import "PcsxrDiscHandler.h"
#import "PcsxrFreezeStateHandler.h"
#include "psxcommon.h"
#include "plugins.h"
#include "misc.h"
#include "ExtendedKeys.h"

NSDictionary *prefStringKeys;
NSDictionary *prefByteKeys;
NSMutableArray *biosList;
NSString *saveStatePath;

static NSString *HandleBinCue(NSString *toHandle)
{
	NSURL *tempURL = [[NSURL alloc] initFileURLWithPath:toHandle];
	NSString *extension = [tempURL pathExtension];
	NSString *newName = toHandle;
	if ([extension caseInsensitiveCompare:@"cue"] == NSOrderedSame) {
		NSURL *temp1 = [tempURL URLByDeletingLastPathComponent];
		NSURL *temp2 = nil;
		//Get the bin file name from the cue.
		NSString *cueFile = [NSString stringWithContentsOfURL:tempURL encoding:NSUTF8StringEncoding error:nil];
		if (!cueFile) {
			cueFile = [NSString stringWithContentsOfURL:tempURL encoding:NSASCIIStringEncoding error:nil];
			if (!cueFile) {
				goto badCue;
			}
		}
		
		NSRange firstQuote, lastQuote, filePath;
		firstQuote = [cueFile rangeOfString:@"\""];
		if (firstQuote.location == NSNotFound) {
			goto badCue;
		}
		lastQuote = [cueFile rangeOfString:@".bin\"" options:NSCaseInsensitiveSearch];
		if (lastQuote.location == NSNotFound) {
			goto badCue;
		}
		
		filePath.location = firstQuote.location + 1; //Don't include the quote symbol
		filePath.length = (lastQuote.location + 4) - (firstQuote.location + 1 ); //Include the .bin but not the first quote symbol
		temp2 = [temp1 URLByAppendingPathComponent:[cueFile substringWithRange:filePath]];

		goto goodCue;
		
	badCue:
		//Fallback if the cue couldn't be loaded
		temp1 = [tempURL URLByDeletingPathExtension];
		//TODO: handle case-sensitive filesystems better
		temp2 = [temp1 URLByAppendingPathExtension:@"bin"];
		if (![[NSFileManager defaultManager] fileExistsAtPath:[temp2 path]]) {
			temp2 = [temp1 URLByAppendingPathExtension:@"BIN"];
		}
		
	goodCue:
		newName = [temp2 path];
	}
	[tempURL release];
	return newName;
}

@implementation PcsxrController

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
	if (UsingIso()) {
		NSOpenPanel* openDlg = [NSOpenPanel openPanel];
		[openDlg retain];

		[openDlg setCanChooseFiles:YES];
		[openDlg setCanChooseDirectories:NO];
		[openDlg setAllowedFileTypes:[PcsxrDiscHandler supportedUTIs]];

		if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
			NSArray* files = [openDlg URLs];
			SetCdOpenCaseTime(time(NULL) + 2);
			SetIsoFile((const char *)[HandleBinCue([[files objectAtIndex:0] path]) fileSystemRepresentation]);
		}
		[openDlg release];
	} else {
        char *driveLetter = CDR_getDriveLetter();
        
		if (driveLetter != nil) {
			deviceName = [NSMutableString stringWithString:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:driveLetter length:strlen(driveLetter)]];

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
	SetIsoFile(NULL);
	[EmuThread run];
}

- (IBAction)runIso:(id)sender
{
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg retain];

	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowedFileTypes:[PcsxrDiscHandler supportedUTIs]];

	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
		NSArray* urls = [openDlg URLs];
		SetIsoFile((const char *)[HandleBinCue([[urls objectAtIndex:0] path]) fileSystemRepresentation]);
		[EmuThread run];
    }
	[openDlg release];
}

- (IBAction)runBios:(id)sender
{
	SetIsoFile(NULL);
	[EmuThread runBios];
}

- (IBAction)freeze:(id)sender
{
	int num = [sender tag];
    NSString *path = [saveStatePath stringByAppendingPathComponent:[NSString stringWithFormat:@"%s-%3.3d.pcsxrstate", CdromId, num]];

	[EmuThread freezeAt:path which:num-1];
}

- (IBAction)defrost:(id)sender
{
	NSString *path = [saveStatePath stringByAppendingPathComponent:[NSString stringWithFormat:@"%s-%3.3d.pcsxrstate", CdromId, [sender tag]]];
	[EmuThread defrostAt:path];
}

- (IBAction)fullscreen:(id)sender
{
	GPU_keypressed(GPU_FULLSCREEN_KEY);
}

- (IBAction)pauseInBackground:(id)sender
{
	sleepInBackground = !sleepInBackground;
	[[NSUserDefaults standardUserDefaults] setBool:sleepInBackground forKey:@"PauseInBackground"];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
	if ([menuItem action] == @selector(pause:)) {
		[menuItem setState:([EmuThread isPaused] ? NSOnState : NSOffState)];
	}

	if ([menuItem action] == @selector(pause:) || [menuItem action] == @selector(fullscreen:))
		return [EmuThread active];

	if ([menuItem action] == @selector(reset:) || [menuItem action] == @selector(ejectCD:) ||
		 [menuItem action] == @selector(freeze:))
		return [EmuThread active] && ![EmuThread isRunBios];

	if ([menuItem action] == @selector(runCD:) || [menuItem action] == @selector(runIso:) ||
		 [menuItem action] == @selector(runBios:)) {
		if (preferenceWindow != nil)
			if ([preferenceWindow isVisible])
				return NO;

		if (preferencesController != nil) {
			if ([preferencesController memoryCardWindowIsVisible] == YES)
				return NO;
		}
		
		if ([menuItem action] == @selector(runBios:) && strcmp(Config.Bios, "HLE") == 0)
			return NO;

		return ![EmuThread active];
	}

	if ([menuItem action] == @selector(defrost:)) {
		if (![EmuThread active] || [EmuThread isRunBios])
			return NO;

		NSString *path = [saveStatePath stringByAppendingPathComponent:[NSString stringWithFormat:@"%s-%3.3d.pcsxrstate", CdromId, [menuItem tag]]];
		return (CheckState((char *)[path fileSystemRepresentation]) == 0);
	}

	if ([menuItem action] == @selector(preferences:))
		return ![EmuThread active];

	if ([menuItem action] == @selector(pauseInBackground:)) {
		[menuItem setState:(sleepInBackground ? NSOnState : NSOffState)];
		return YES;
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
				NSLocalizedString(@"Pcsxr is missing one or more critical plugins. You will need to install these in order to play games.", nil), 
				nil, nil, nil);
	}

	if (![PcsxrController biosAvailable]) {
		NSRunInformationalAlertPanel(NSLocalizedString(@"Missing BIOS!", nil),
				NSLocalizedString(@"Pcsxr wasn't able to locate any Playstation BIOS ROM files. This means that it will run in BIOS simulation mode which is less stable and compatible than using a real Playstation BIOS.\nIf you have a BIOS available, please copy it to\n~/Library/Application Support/Pcsxr/Bios/", nil), 
				nil, nil, nil);
	}

	sleepInBackground = [[NSUserDefaults standardUserDefaults] boolForKey:@"PauseInBackground"];
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

	enumerator = [prefByteKeys keyEnumerator];
	while ((key = [enumerator nextObject])) {
		u8 *dst = (u8 *)[[prefByteKeys objectForKey:key] pointerValue];
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

	str = [[defaults stringForKey:@"Mcd1"] fileSystemRepresentation];
	if (str) strncpy(Config.Mcd1, str, MAXPATHLEN);

	str = [[defaults stringForKey:@"Mcd2"] fileSystemRepresentation];
	if (str) strncpy(Config.Mcd2, str, MAXPATHLEN);

	if ([defaults boolForKey:@"UseHLE"] || 0 == [biosList count]) {
		strcpy(Config.Bios, "HLE");
	} else {
		str = [(NSString *)[biosList objectAtIndex:0] fileSystemRepresentation];
		if (str != nil) strncpy(Config.Bios, str, MAXPATHLEN);
		else strcpy(Config.Bios, "HLE");
	}

	str = [[defaults stringForKey:@"Net"] fileSystemRepresentation];
	if (str) strncpy(Config.Net, str, MAXPATHLEN);
	else {
			strcpy(Config.Net, "Disabled");
	}
}

+ (void)setDefaultFromConfig:(NSString *)defaultKey
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	char *str = (char *)[[prefStringKeys objectForKey:defaultKey] pointerValue];
	if (str) {
		[defaults setObject:[NSString stringWithCString:str encoding:NSUTF8StringEncoding] forKey:defaultKey];
		return;
	}

	u8 *val = (u8 *)[[prefByteKeys objectForKey:defaultKey] pointerValue];
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
		[NSNumber numberWithInt:1], @"NoDynarec",
		[NSNumber numberWithInt:1], @"AutoDetectVideoType",
		[NSNumber numberWithInt:0], @"UseHLE",
		[NSNumber numberWithBool:YES], @"PauseInBackground",
		nil];

	[defaults registerDefaults:appDefaults];

	prefStringKeys = [[NSDictionary alloc] initWithObjectsAndKeys:
		[NSValue valueWithPointer:Config.Gpu], @"PluginGPU",
		[NSValue valueWithPointer:Config.Spu], @"PluginSPU",
		[NSValue valueWithPointer:Config.Pad1], @"PluginPAD",
		[NSValue valueWithPointer:Config.Cdr], @"PluginCDR",
		[NSValue valueWithPointer:Config.Net], @"PluginNET",
		[NSValue valueWithPointer:Config.Mcd1], @"Mcd1",
		[NSValue valueWithPointer:Config.Mcd2], @"Mcd2",
		nil];

	prefByteKeys = [[NSDictionary alloc] initWithObjectsAndKeys:
		[NSValue valueWithPointer:&Config.Xa], @"NoXaAudio",
		[NSValue valueWithPointer:&Config.UseNet], @"NetPlay",
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
    NSFileManager *manager = [NSFileManager defaultManager];
    NSURL *supportURL = [manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
    
    if(supportURL != nil) {
		NSURL *url;
		BOOL dir;

		// create them if needed
        url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Bios"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
			[manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Memory Cards"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Patches"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];
		
		url = [supportURL URLByAppendingPathComponent:@"Pcsxr/PlugIns"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];
        
        saveStatePath = [[[supportURL URLByAppendingPathComponent:@"Pcsxr/Save States"] path] copy];
		if (![manager fileExistsAtPath:saveStatePath isDirectory:&dir])
			[manager createDirectoryAtPath:saveStatePath withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Memory Cards/Mcd001.mcr"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strncpy(Config.Mcd1, str, MAXPATHLEN);

		url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Memory Cards/Mcd002.mcr"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strncpy(Config.Mcd2, str, MAXPATHLEN);

		url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Bios"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strncpy(Config.BiosDir, str, MAXPATHLEN);

		url = [supportURL URLByAppendingPathComponent:@"Pcsxr/Patches"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strncpy(Config.PatchesDir, str, MAXPATHLEN);
	} else {
		strcpy(Config.BiosDir, "Bios/");
		strcpy(Config.PatchesDir, "Patches/");

		saveStatePath = @"sstates";
		[saveStatePath retain];
	}

	// set plugin path
	path = [[NSBundle mainBundle] builtInPlugInsPath];
	str = [path fileSystemRepresentation];
	if (str != nil) strncpy(Config.PluginsDir, str, MAXPATHLEN);

	// locate a bios
	biosList = [[NSMutableArray alloc] init];
	
    NSString *biosDir = [manager stringWithFileSystemRepresentation:Config.BiosDir length:strlen(Config.BiosDir)];
    NSArray *bioses = [manager contentsOfDirectoryAtPath:biosDir error:NULL];
	if (bioses) {
		NSUInteger i;
		for (i = 0; i < [bioses count]; i++) {
			NSString *file = [bioses objectAtIndex:i];
            NSDictionary *attrib = [manager attributesOfItemAtPath:[[biosDir stringByAppendingPathComponent:file] stringByResolvingSymlinksInPath] error:NULL];

			if ([[attrib fileType] isEqualToString:NSFileTypeRegular]) {
				unsigned long long size = [attrib fileSize];
				if (([attrib fileSize] % (256 * 1024)) == 0 && size > 0) {
					[biosList addObject:file];
				}
			}
		}
	}

	[PcsxrController setConfigFromDefaults];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSError *err = nil;
	NSString *utiFile = [[NSWorkspace sharedWorkspace] typeOfFile:filename error:&err];
	if (err) {
		NSRunAlertPanel(NSLocalizedString(@"Error opening file",nil), [NSString stringWithFormat:NSLocalizedString(@"Unable to open %@: %@", nil), [filename lastPathComponent], [err localizedFailureReason]], nil, nil, nil);
		return NO;
	}
	NSArray *handlers = [NSArray arrayWithObjects:[PcsxrPluginHandler class], [PcsxrMemCardHandler class], [PcsxrFreezeStateHandler class], [PcsxrDiscHandler class], nil];
	BOOL isHandled = NO;
	for (Class fileHandler in handlers) {
		NSObject<PcsxrFileHandle> *hand = [[fileHandler alloc] init];
		BOOL canHandle = NO;
		for (NSString *uti in [fileHandler supportedUTIs]) {
			if ([[NSWorkspace sharedWorkspace] type:utiFile  conformsToType:uti]) {
				canHandle = YES;
			}
		}			
		if (canHandle) {
			isHandled = [hand handleFile:HandleBinCue(filename)];
		}
		[hand release];

	}
	return isHandled;
}

@end
