#import <Cocoa/Cocoa.h>
#import "PcsxrController.h"
#import "ConfigurationController.h"
#import "CheatController.h"
#import "EmuThread.h"
#import "PcsxrMemCardHandler.h"
#import "PcsxrPluginHandler.h"
#import "PcsxrDiscHandler.h"
#import "PcsxrFreezeStateHandler.h"
#include "psxcommon.h"
#include "plugins.h"
#include "misc.h"
#include "cdrom.h"
#include "ExtendedKeys.h"
#import "ARCBridge.h"

NSDictionary *prefStringKeys;
NSDictionary *prefByteKeys;
NSMutableArray *biosList;
NSString *saveStatePath;

#define HELPSTR "\n" \
"At least one of these must be passed:\n"\
"\t--iso path    \tlaunch with selected ISO\n" \
"\t--cdrom       \tlaunch with a CD-ROM\n" \
"\t--bios        \tlaunch into the BIOS\n\n" \
"Additional options:\n" \
"\t--exitAtClose \tcloses PCSX-R at when the emulation has ended\n" \
"\t--mcd1 path   \tsets the fist memory card to path\n" \
"\t--mcd2 path   \tsets the second memory card to path\n" \
"\t--freeze path \tloads freeze state from path\n\n" \
"Help:\n" \
"\t--help        \tshows this message\n\n" \


void ShowHelpAndExit(FILE* output, int exitCode)
{
	fprintf(output, HELPSTR);
	exit(exitCode);
}

@interface PcsxrController ()
@property (readwrite) BOOL endAtEmuClose;
@property (readwrite) BOOL sleepInBackground;
@property (readwrite) BOOL wasPausedBeforeBGSwitch;
@end

@implementation PcsxrController

@synthesize recentItems;
- (BOOL)endAtEmuClose
{
	return PSXflags.endAtEmuClose;
}

- (void)setEndAtEmuClose:(BOOL)endAtEmuClose
{
	PSXflags.endAtEmuClose = endAtEmuClose;
}

- (BOOL)sleepInBackground
{
	return PSXflags.sleepInBackground;
}

- (void)setSleepInBackground:(BOOL)sleepInBackground
{
	PSXflags.sleepInBackground = sleepInBackground;
}

- (BOOL)wasPausedBeforeBGSwitch
{
	return PSXflags.wasPausedBeforeBGSwitch;
}

- (void)setWasPausedBeforeBGSwitch:(BOOL)wasPausedBeforeBGSwitch
{
	PSXflags.wasPausedBeforeBGSwitch = wasPausedBeforeBGSwitch;
}

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
			SetIsoFile((const char *)[[[files objectAtIndex:0] path] fileSystemRepresentation]);
			SetCdOpenCaseTime(time(NULL) + 2);
			LidInterrupt();
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

- (IBAction)showCheatsWindow:(id)sender
{
	/* load the nib if it hasn't yet */
	if (cheatWindow == nil) {
		if (cheatController == nil) {
			cheatController = [[CheatController alloc] initWithWindowNibName:@"CheatWindow"];
		}
		cheatWindow = [cheatController window];
	}
    
	/* show the window */
	//[cheatWindow makeKeyAndOrderFront:sender];
	[cheatController showWindow:sender];
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
	//[preferenceWindow makeKeyAndOrderFront:sender];
	[preferencesController showWindow:sender];
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
			SetIsoFile([[url path] fileSystemRepresentation]);
			SetCdOpenCaseTime(time(NULL) + 2);
			LidInterrupt();
		} else {
			NSBeep();
		}
	} else {
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"NetPlay"]) {
			[pluginList enableNetPlug];
		} else {
			[pluginList disableNetPlug];
		}
		SetIsoFile([[url path] fileSystemRepresentation]);
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
	self.sleepInBackground = !self.sleepInBackground;
	[[NSUserDefaults standardUserDefaults] setBool:self.sleepInBackground forKey:@"PauseInBackground"];
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
        
        if (cheatWindow != nil)
            if ([cheatWindow isVisible])
                return NO;
		
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
		[menuItem setState:(self.sleepInBackground ? NSOnState : NSOffState)];
		return YES;
	}

	return YES;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
	self.wasPausedBeforeBGSwitch = [EmuThread isPaused];

	if (self.sleepInBackground) {
		 [EmuThread pause];
	}
}

- (void)applicationDidBecomeActive:(NSNotification *)aNotification
{
	if (self.sleepInBackground && !self.wasPausedBeforeBGSwitch) {
		[EmuThread resume];
	}
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	RELEASEOBJ(skipFiles);
	skipFiles = nil;
}

static void ParseErrorStr(NSString *errStr) __dead2;
static void ParseErrorStr(NSString *errStr)
{
	NSLog(@"%@", errStr);
	NSRunCriticalAlertPanel(@"Parsing error", @"%@\n\nPlease check the command line options and try again", nil, nil, nil, errStr);
	ShowHelpAndExit(stderr, EXIT_FAILURE);
}

//DO NOT END THIS MACRO WITH A SIMICOLON! it will break the if-else if process
#define HandleArg(arg, launchable, otherblock) \
if ([[progArgs objectAtIndex:i] compare:arg options:NSCaseInsensitiveSearch] == NSOrderedSame) { \
HandleArgBase(arg, launchable, otherblock)

#define HandleArgElse(arg, launchable, otherblock) \
else if ([[progArgs objectAtIndex:i] compare:arg options:NSCaseInsensitiveSearch] == NSOrderedSame) { \
HandleArgBase(arg, launchable, otherblock)

#define HandleArgBase(arg, launchable, otherblock) \
if (isLaunchable && launchable) { \
NSString *messageStr = [NSString stringWithFormat:@"The options %@ and %@ are exclusive.", arg, runtimeStr];  \
ParseErrorStr(messageStr); \
} \
if(launchable) { \
isLaunchable = YES; \
runtimeStr = arg; \
} \
otherblock();\
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

	self.sleepInBackground = [[NSUserDefaults standardUserDefaults] boolForKey:@"PauseInBackground"];
	NSProcessInfo *procInfo = [NSProcessInfo processInfo];
	NSArray *progArgs = [procInfo arguments];
	if ([progArgs count] > 1 && ![[progArgs objectAtIndex:1] hasPrefix:@"-psn"]) {
		skipFiles = [[NSMutableArray alloc] init];
		BOOL isLaunchable = NO;
		NSString *runtimeStr = nil;
		__block dispatch_block_t runtimeBlock = NULL;
		__block BOOL hasParsedAnArgument = NO;

		__block NSString *(^FileTestBlock)() = NULL;
		
		NSMutableArray *unknownOptions = [NSMutableArray array];
		
		dispatch_block_t cdromBlock = ^{
			hasParsedAnArgument = YES;
			runtimeBlock = [^{
				[self runCD:nil];
			} copy];
		};
		
		dispatch_block_t biosBlock = ^{
			hasParsedAnArgument = YES;
			runtimeBlock = [^{
				[self runBios:nil];
			} copy];
		};
		
		dispatch_block_t emuCloseAtEnd = ^{
			hasParsedAnArgument = YES;
			self.endAtEmuClose = YES;
		};
		
		dispatch_block_t isoBlock = ^{
			hasParsedAnArgument = YES;
			NSString *path = FileTestBlock();
			runtimeBlock = [^{
				[self runURL:[NSURL fileURLWithPath:path]];
			} copy];
		};
		
		void (^mcdBlock)(int mcdNumber) = ^(int mcdnumber){
			hasParsedAnArgument = YES;
			NSString *path = FileTestBlock();
			LoadMcd(mcdnumber, (char*)[path fileSystemRepresentation]);
		};
		
		dispatch_block_t freezeBlock = ^{
			hasParsedAnArgument = YES;
			NSString *path = FileTestBlock();
			[EmuThread defrostAt:path];
		};

		BOOL hasFileTestBlock = NO;

		for (__block int i = 1; i < [progArgs count]; i++) {
			if (!hasFileTestBlock)
			{
				FileTestBlock = ^NSString *(){
					if ([progArgs count] <= ++i) {
						ParseErrorStr(@"Not enough arguments.");
					}
					NSString *path = [progArgs objectAtIndex:i];
					if (![[NSFileManager defaultManager] fileExistsAtPath:path])
					{
						NSString *errStr = [NSString stringWithFormat:@"The file \"%@\" does not exist.", path];
						ParseErrorStr(errStr);
						return nil;
					}
					[skipFiles addObject:path];
					return path;
				};
				hasFileTestBlock = YES;
			}
			
			//DO NOT END these MACROS WITH A SIMICOLON! it will break the if-else if process
			HandleArg(@"--iso", YES, isoBlock)
			HandleArgElse(@"--cdrom", YES, cdromBlock)
			HandleArgElse(@"--bios", YES, biosBlock)
			HandleArgElse(@"--exitAtClose", NO, emuCloseAtEnd)
			HandleArgElse(@"--mcd1", NO, ^{mcdBlock(1);})
			HandleArgElse(@"--mcd2", NO, ^{mcdBlock(2);})
			HandleArgElse(@"--freeze", NO, freezeBlock)
			else {
				[unknownOptions addObject:[progArgs objectAtIndex:i]];
			}
		}
#ifdef DEBUG
		if ([unknownOptions count]) {
			NSString *unknownString = @"The following options weren't recognized by PCSX-R:";
			@autoreleasepool {
				for (NSString *arg in unknownOptions) {
					unknownString = [NSString stringWithFormat:@"%@ %@", unknownString, arg];
				}
				RETAINOBJNORETURN(unknownString);
			}
			NSLog(@"%@", unknownString);
			RELEASEOBJ(unknownString);
			NSLog(@"This may be due to extra arguments passed by the OS or debugger.");
		}
#endif
		if (!isLaunchable && hasParsedAnArgument) {
			NSString *tmpStr = @"A launch command wasn't found in the command line and an argument that PCSX-R recognizes was.\n";
			@autoreleasepool {
				for (NSString *arg in progArgs) {
					tmpStr = [NSString stringWithFormat:@"%@ %@", tmpStr, arg];
				}
				tmpStr = [NSString stringWithFormat:@"%@\n\nThe valid launch commands are --iso, --cdrom, and --bios", tmpStr];
				RETAINOBJNORETURN(tmpStr);
			}
			ParseErrorStr(AUTORELEASEOBJ(tmpStr));
		} else if (hasParsedAnArgument){
			runtimeBlock();
			RELEASEOBJ(runtimeBlock);
		}
	}
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
		if (dst != NULL) *dst = [defaults boolForKey:key];
#ifdef __i386__
		//i386 on OS X doesn't like the dynarec core
		if ([key isEqualToString:@"NoDynarec"]) {
			if (dst != NULL) *dst = 1;
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
								 @YES, @"NoDynarec",
								 @YES, @"AutoDetectVideoType",
								 @NO, @"UseHLE",
								 @YES, @"PauseInBackground",
								 @NO, @"Widescreen",
								 @NO, @"NetPlay",
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
	if (skipFiles && [skipFiles count]) {
		for (NSString *parsedFile in skipFiles) {
			if ([filename isEqualToString:parsedFile]) {
				return YES;
			}
		}
	}
	
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
