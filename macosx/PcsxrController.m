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
#import "ARCBridge.h"

NSDictionary *prefStringKeys;
NSDictionary *prefByteKeys;
NSMutableArray *biosList;
NSString *saveStatePath;

@implementation PcsxrController

@synthesize recentItems;

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
		NSOpenPanel* openDlg = RETAINOBJ([NSOpenPanel openPanel]);

		[openDlg setCanChooseFiles:YES];
		[openDlg setCanChooseDirectories:NO];
		[openDlg setAllowedFileTypes:[PcsxrDiscHandler supportedUTIs]];

		if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
			NSArray* files = [openDlg URLs];
			SetCdOpenCaseTime(time(NULL) + 2);
			SetIsoFile((const char *)[[[files objectAtIndex:0] path] fileSystemRepresentation]);
		}
		RELEASEOBJ(openDlg);
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
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"NetPlay"]) {
		[pluginList enableNetPlug];
	} else {
		[pluginList disableNetPlug];
	}
	[EmuThread run];
}

- (IBAction)runIso:(id)sender
{
	NSOpenPanel* openDlg = RETAINOBJ([NSOpenPanel openPanel]);

	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowedFileTypes:[PcsxrDiscHandler supportedUTIs]];

	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
        NSURL *url = [[openDlg URLs] objectAtIndex:0];
        [recentItems addRecentItem:url];
		[self runURL:url];
    }
	RELEASEOBJ(openDlg);
}

- (IBAction)runBios:(id)sender
{
	SetIsoFile(NULL);
	[pluginList disableNetPlug];
	[EmuThread runBios];
}

- (void)runURL:(NSURL*)url
{
    if ([EmuThread active] == YES) {
		if (UsingIso()) {
			SetCdOpenCaseTime(time(NULL) + 2);
			SetIsoFile([[url path] fileSystemRepresentation]);
		} else {
			NSBeep();
		}
	} else {
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"NetPlay"]) {
			[pluginList enableNetPlug];
		} else {
			[pluginList disableNetPlug];
		}
		SetIsoFile((const char *)[[url path] fileSystemRepresentation]);
		[EmuThread run];
	}
}

- (IBAction)freeze:(id)sender
{
	NSInteger num = [sender tag];
    [PcsxrController saveState:num];
}

+ (void)saveState:(int)num
{
	[EmuThread freezeAt:[PcsxrController saveStatePath:num] which:num];
}

- (IBAction)defrost:(id)sender
{
    NSInteger num = [sender tag];
	[PcsxrController loadState:num];
}

+ (void)loadState:(int)num
{
	[EmuThread defrostAt:[PcsxrController saveStatePath:num]];
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
			if ([preferencesController isMemoryCardWindowVisible] == YES)
				return NO;
		}
		
		if ([menuItem action] == @selector(runBios:) && strcmp(Config.Bios, "HLE") == 0)
			return NO;

		return ![EmuThread active];
	}

	if ([menuItem action] == @selector(defrost:)) {
		if (![EmuThread active] || [EmuThread isRunBios])
			return NO;

		NSString *path = [saveStatePath stringByAppendingPathComponent:[NSString stringWithFormat:@"%s-%3.3ld.pcsxrstate", CdromId, (long)[menuItem tag]]];
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


#if !__has_feature(objc_arc)
- (void)dealloc
{
	[pluginList release];
	[super dealloc];
}
#endif

+ (void)setConfigFromDefaults
{
	const char *str;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	/*
	enumerator = [prefStringKeys keyEnumerator];
	while ((key = [enumerator nextObject])) {
		str = [[defaults stringForKey:key] fileSystemRepresentation];
		char *dst = (char *)[[prefStringKeys objectForKey:key] pointerValue];
		if (str != nil && dst != nil) strncpy(dst, str, 255);
	}*/

	for (NSString *key in prefByteKeys) {
		u8 *dst = (u8 *)[[prefByteKeys objectForKey:key] pointerValue];
		if (dst != nil) *dst = [defaults integerForKey:key];
#ifdef __i386__
		//i386 on OS X doesn't like the dynarec core
		if ([key isEqualToString:@"NoDynarec"]) {
			*dst = 1;
			[defaults setBool:YES forKey:key];
		}
#endif
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
	if (str) strlcpy(Config.Mcd1, str, MAXPATHLEN);

	str = [[defaults stringForKey:@"Mcd2"] fileSystemRepresentation];
	if (str) strlcpy(Config.Mcd2, str, MAXPATHLEN);

	if ([defaults boolForKey:@"UseHLE"] || 0 == [biosList count]) {
		strcpy(Config.Bios, "HLE");
	} else {
		str = [(NSString *)[biosList objectAtIndex:0] fileSystemRepresentation];
		if (str != nil) strlcpy(Config.Bios, str, MAXPATHLEN);
		else strcpy(Config.Bios, "HLE");
	}

	str = [[defaults stringForKey:@"Net"] fileSystemRepresentation];
	if (str) strlcpy(Config.Net, str, MAXPATHLEN);
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
								 [NSNumber numberWithBool:YES], @"NoDynarec",
								 [NSNumber numberWithBool:YES], @"AutoDetectVideoType",
								 [NSNumber numberWithBool:NO], @"UseHLE",
								 [NSNumber numberWithBool:YES], @"PauseInBackground",
								 [NSNumber numberWithBool:NO], @"Widescreen",
								 [NSNumber numberWithBool:NO], @"NetPlay",
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
		//[NSValue valueWithPointer:&Config.UseNet], @"NetPlay",
		[NSValue valueWithPointer:&Config.SioIrq], @"SioIrqAlways",
		[NSValue valueWithPointer:&Config.Mdec], @"BlackAndWhiteMDECVideo",
		[NSValue valueWithPointer:&Config.PsxAuto], @"AutoDetectVideoType",
		[NSValue valueWithPointer:&Config.PsxType], @"VideoTypePAL",
		[NSValue valueWithPointer:&Config.Cdda], @"NoCDAudio",
		[NSValue valueWithPointer:&Config.Cpu], @"NoDynarec",
		[NSValue valueWithPointer:&Config.PsxOut], @"ConsoleOutput",
		[NSValue valueWithPointer:&Config.SpuIrq], @"SpuIrqAlways",
		[NSValue valueWithPointer:&Config.RCntFix], @"RootCounterFix",
		[NSValue valueWithPointer:&Config.VSyncWA], @"VideoSyncWAFix",
		[NSValue valueWithPointer:&Config.Widescreen], @"Widescreen",
		nil];

	// setup application support paths
    NSFileManager *manager = [NSFileManager defaultManager];
    NSURL *supportURL = [manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
    
    if(supportURL != nil) {
		NSURL *PcsxrAppSupport;
		NSURL *MemCardPath;
		NSURL *url;
		BOOL dir;
		
		PcsxrAppSupport = [supportURL URLByAppendingPathComponent:@"Pcsxr"];

		// create them if needed
        url = [PcsxrAppSupport URLByAppendingPathComponent:@"Bios"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
			[manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];

        MemCardPath = [PcsxrAppSupport URLByAppendingPathComponent:@"Memory Cards"];
		url = MemCardPath;
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [PcsxrAppSupport URLByAppendingPathComponent:@"Patches"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];
		
		url = [PcsxrAppSupport URLByAppendingPathComponent:@"PlugIns"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtPath:[url path] withIntermediateDirectories:YES attributes:nil error:NULL];
        
        saveStatePath = [[[PcsxrAppSupport URLByAppendingPathComponent:@"Save States"] path] copy];
		if (![manager fileExistsAtPath:saveStatePath isDirectory:&dir])
			[manager createDirectoryAtPath:saveStatePath withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [MemCardPath URLByAppendingPathComponent:@"Mcd001.mcr"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strlcpy(Config.Mcd1, str, MAXPATHLEN);

		url = [MemCardPath URLByAppendingPathComponent:@"Mcd002.mcr"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strlcpy(Config.Mcd2, str, MAXPATHLEN);

		url = [PcsxrAppSupport URLByAppendingPathComponent:@"Bios"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) strlcpy(Config.BiosDir, str, MAXPATHLEN);

		url = [PcsxrAppSupport URLByAppendingPathComponent:@"Patches"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) {
			strlcpy(Config.PatchesDir, str, MAXPATHLEN);
			//workaround for the fact that the PCSXR core doesn't append a forward slash to the patches dir
			strlcat(Config.PatchesDir, "/", MAXPATHLEN);
		}
	} else {
		strcpy(Config.BiosDir, "Bios/");
		strcpy(Config.PatchesDir, "Patches/");

		saveStatePath = RETAINOBJ(@"sstates");
	}

	// set plugin path
	path = [[NSBundle mainBundle] builtInPlugInsPath];
	str = [path fileSystemRepresentation];
	if (str != nil) strlcpy(Config.PluginsDir, str, MAXPATHLEN);

	// locate a bios
	biosList = [[NSMutableArray alloc] init];
	
    NSString *biosDir = [manager stringWithFileSystemRepresentation:Config.BiosDir length:strlen(Config.BiosDir)];
    NSArray *bioses = [manager contentsOfDirectoryAtPath:biosDir error:NULL];
	if (bioses) {
		for (NSString *file in bioses) {
            NSDictionary *attrib = [manager attributesOfItemAtPath:[[biosDir stringByAppendingPathComponent:file] stringByResolvingSymlinksInPath] error:NULL];

			if ([[attrib fileType] isEqualToString:NSFileTypeRegular]) {
				unsigned long long size = [attrib fileSize];
				if ((size % (256 * 1024)) == 0 && size > 0) {
					[biosList addObject:file];
				}
			}
		}
	}

	[PcsxrController setConfigFromDefaults];
}

+ (NSString*)saveStatePath:(int)slot
{
    if(slot >= 0) {
        return [saveStatePath stringByAppendingPathComponent:[NSString stringWithFormat:@"%s-%3.3d.pcsxrstate", CdromId, slot]];
    }
    
    return saveStatePath;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSError *err = nil;
	NSString *utiFile = [[NSWorkspace sharedWorkspace] typeOfFile:filename error:&err];
	if (err) {
		NSRunAlertPanel(NSLocalizedString(@"Error opening file", nil), [NSString stringWithFormat:NSLocalizedString(@"Unable to open %@: %@", nil), [filename lastPathComponent], [err localizedFailureReason]], nil, nil, nil);
		return NO;
	}
	static NSArray *handlers = nil;
	if (handlers == nil) {
		handlers = [[NSArray alloc] initWithObjects:[PcsxrPluginHandler class], [PcsxrMemCardHandler class], [PcsxrFreezeStateHandler class], [PcsxrDiscHandler class], nil];
	}
	BOOL isHandled = NO;
	for (Class fileHandler in handlers) {
		NSObject<PcsxrFileHandle> *hand = [[fileHandler alloc] init];
		BOOL canHandle = NO;
		for (NSString *uti in [fileHandler supportedUTIs]) {
			if ([[NSWorkspace sharedWorkspace] type:utiFile  conformsToType:uti]) {
				canHandle = YES;
				break;
			}
		}			
		if (canHandle) {
			isHandled = [hand handleFile:filename];
			RELEASEOBJ(hand);
			break;
		}
		RELEASEOBJ(hand);

	}
	return isHandled;
}

@end
