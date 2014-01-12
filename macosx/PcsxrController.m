#import <Cocoa/Cocoa.h>
#import "PcsxrController.h"
#import "ConfigurationController.h"
#import "CheatController.h"
#import "EmuThread.h"
#import "PcsxrMemCardHandler.h"
#import "PcsxrPluginHandler.h"
#import "PcsxrDiscHandler.h"
#import "PcsxrFreezeStateHandler.h"
#import "PcsxrCheatHandler.h"
#import "LaunchArg.h"
#include <DiskArbitration/DiskArbitration.h>
#include <IOKit/storage/IOCDMedia.h>
#include "psxcommon.h"
#include "plugins.h"
#include "misc.h"
#include "cdrom.h"
#include "ExtendedKeys.h"

NSDictionary *prefStringKeys = nil;
NSDictionary *prefByteKeys = nil;
NSDictionary *prefURLKeys = nil;
NSMutableArray *biosList = nil;
NSString *saveStatePath = nil;
BOOL wasFinderLaunch = NO;


#define HELPSTR "\n" \
"At least one of these must be passed:\n" \
"\t--iso path    launch with selected ISO\n" \
"\t--cdrom       launch with a CD-ROM\n" \
"\t--bios        launch into the BIOS\n" \
"\n" \
"Additional options:\n" \
"\t--exitAtClose closes PCSX-R at when the emulation has ended\n" \
"\t--mcd1 path   sets the fist memory card to path\n" \
"\t--mcd2 path   sets the second memory card to path\n" \
"\t--freeze path loads freeze state from path\n" \
"\n" \
"Help:\n" \
"\t--help        shows this message\n" \
"\n" \


void ShowHelpAndExit(FILE* output, int exitCode)
{
	fprintf(output, HELPSTR);
	if (!NSApp) {
		exit(exitCode);
	} else {
		[NSApp stop:nil];
	}
}

@interface PcsxrController ()
@property (readwrite) BOOL endAtEmuClose;
@property BOOL sleepInBackground;
@property BOOL wasPausedBeforeBGSwitch;
@property BOOL wasPausedBeforeDiscEject;
@property (strong) NSMutableArray *skipFiles;
@property (strong) NSWindow *preferenceWindow;
@property (strong) NSWindow *cheatWindow;
@property (nonatomic) DASessionRef diskSession;
@end

@implementation PcsxrController
@synthesize recentItems;
@synthesize skipFiles;
@synthesize cheatController;
@synthesize cheatWindow;
@synthesize preferenceWindow;

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

- (BOOL)wasPausedBeforeDiscEject
{
	return PSXflags.wasPausedBeforeDiscEject;
}

-(void)setWasPausedBeforeDiscEject:(BOOL)wasPausedBeforeDiscEject
{
	PSXflags.wasPausedBeforeDiscEject = wasPausedBeforeDiscEject;
}

@synthesize diskSession = _diskSession;
- (void)setDiskSession:(DASessionRef)diskSession
{
	if (diskSession == _diskSession) {
		return;
	}
	if (_diskSession) {
		CFRelease(_diskSession);
		_diskSession = NULL;
	}if (diskSession) {
		_diskSession = diskSession;
		CFRetain(diskSession);
	}
}

static void PSXDiscAppearedCallback(DADiskRef disk, void *context)
{
	PcsxrController *theSelf = (__bridge PcsxrController*)context;
	//sleep(3); //Is this needed?
	SetCdOpenCaseTime(time(NULL) + 2);
	LidInterrupt();
	
	/* and open new cd */
	if ([EmuThread active])
		CDR_open();
	
	if (!theSelf.wasPausedBeforeDiscEject) {
		[EmuThread resume];
	}

	DASessionUnscheduleFromRunLoop(theSelf.diskSession, CFRunLoopGetMain(), kCFRunLoopCommonModes);
	theSelf.diskSession = NULL;
}

- (IBAction)ejectCD:(id)sender
{
	self.wasPausedBeforeDiscEject = [EmuThread pauseSafe];
	
	/* close connection to current cd */
	if ([EmuThread active])
		CDR_close();
	
	// switch to another ISO if using internal image reader, otherwise eject the CD
	if (UsingIso()) {
		NSOpenPanel* openDlg = [NSOpenPanel openPanel];
		[openDlg setAllowedFileTypes:[PcsxrDiscHandler supportedUTIs]];
		
		if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
			NSArray* files = [openDlg URLs];
			SetIsoFile([[files[0] path] fileSystemRepresentation]);
			SetCdOpenCaseTime(time(NULL) + 2);
			LidInterrupt();
		}
		
		if ([EmuThread active])
			CDR_open();
		
		if (!self.wasPausedBeforeDiscEject) {
			[EmuThread resume];
		}
	} else {
		NSMutableString *deviceName;
		NSTask *ejectTask;
		NSRange rdiskRange;
		char *driveLetter = CDR_getDriveLetter();
        
		if (driveLetter != NULL) {
			deviceName = [NSMutableString stringWithString:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:driveLetter length:strlen(driveLetter)]];
			
			// delete the 'r' in 'rdisk'
			rdiskRange = [deviceName rangeOfString:@"rdisk"];
			if (rdiskRange.length != 0) {
				rdiskRange.length = 1;
				[deviceName deleteCharactersInRange:rdiskRange];
			}
			// execute hdiutil to eject the device
			ejectTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/hdiutil" arguments:@[@"eject", deviceName]];
			[ejectTask waitUntilExit];
		}
		DASessionRef tmpSession = DASessionCreate(kCFAllocatorDefault);
		CFDictionaryRef match = CFBridgingRetain(@{(NSString*)kDADiskDescriptionMediaKindKey : @(kIOCDMediaClass),
												 (NSString*)kDADiskDescriptionMediaWholeKey : @YES});
		DARegisterDiskAppearedCallback(tmpSession, match, PSXDiscAppearedCallback, (__bridge void*)self);
		CFRelease(match);
		
		DASessionScheduleWithRunLoop(tmpSession, CFRunLoopGetMain(), kCFRunLoopCommonModes);
		
		self.diskSession = tmpSession;
		CFRelease(tmpSession);
	}
}

- (void)emuWindowDidClose:(NSNotification*)theNot
{
	if (self.diskSession) {
		DASessionUnscheduleFromRunLoop(self.diskSession, CFRunLoopGetMain(), kCFRunLoopCommonModes);
		self.diskSession = NULL;
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
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg setAllowedFileTypes:[PcsxrDiscHandler supportedUTIs]];

	if ([openDlg runModal] == NSFileHandlingPanelOKButton) {
        NSURL *url = [openDlg URLs][0];
        [recentItems addRecentItem:url];
		[self runURL:url];
    }
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
	[PcsxrController saveState:(int)num];
}

+ (void)saveState:(int)num
{
	[EmuThread freezeAt:[PcsxrController saveStatePath:num] which:num];
}

- (IBAction)defrost:(id)sender
{
	NSInteger num = [sender tag];
	[PcsxrController loadState:(int)num];
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
	self.skipFiles = nil;
}

static void ParseErrorStr(NSString *errStr)
{
	NSLog(@"Parse error: %@", errStr);
	NSRunCriticalAlertPanel(@"Parsing error", @"%@\n\nPlease check the command line options and try again.\n\nPCSXR will now quit.", nil, nil, nil, errStr);
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
ParseErrorStr([NSString stringWithFormat:@"The options %@ and %@ are exclusive.", arg, runtimeStr]); \
} \
if(launchable) { \
isLaunchable = YES; \
runtimeStr = arg; \
} \
otherblock();\
}

#define kPCSXRArgumentCDROM @"--cdrom"
#define kPCSXRArgumentBIOS @"--bios"
#define kPCSXRArgumentISO @"--iso"
#define kPCSXRArgumentMcd @"--mcd"
#define kPCSXRArgumentMcd1 kPCSXRArgumentMcd @"1"
#define kPCSXRArgumentMcd2 kPCSXRArgumentMcd @"2"
#define kPCSXRArgumentFreeze @"--freeze"
#define kPCSXRArgumentExitAtClose @"--exitAtClose"

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)awakeFromNib
{
	[[NSNotificationCenter defaultCenter]
	 addObserver:self selector:@selector(emuWindowDidClose:)
	 name:@"emuWindowDidClose" object:nil];
	
	pluginList = [[PluginList alloc] init];
	if (![pluginList configured] /*!Config.Gpu[0] || !Config.Spu[0] || !Config.Pad1[0] || !Config.Cdr[0]*/) {
		// configure plugins
		[self preferences:nil];
		
		NSRunCriticalAlertPanel(NSLocalizedString(@"Missing plugins!", nil),
								NSLocalizedString(@"Pcsxr is missing one or more critical plugins. You will need to install these in order to play games.", nil),
								nil, nil, nil);
	}
	
	if (![PcsxrController biosAvailable]) {
		NSFileManager *manager = [NSFileManager defaultManager];
		NSURL *supportURL = [manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
		NSURL *biosURL = [[supportURL URLByAppendingPathComponent:@"Pcsxr"] URLByAppendingPathComponent:@"Bios"];
		NSInteger retVal = NSRunInformationalAlertPanel(NSLocalizedString(@"Missing BIOS!", nil),
														NSLocalizedString(@"Pcsxr wasn't able to locate any Playstation BIOS ROM files. This means that it will run in BIOS simulation mode which is less stable and compatible than using a real Playstation BIOS.\nIf you have a BIOS available, please copy it to\n%@", nil),
														NSLocalizedString(@"Okay", @"OK"), NSLocalizedString(@"Show Folder", @"Show Folder"), nil, [[biosURL path] stringByAbbreviatingWithTildeInPath]);
		if (retVal == NSAlertAlternateReturn) {
			[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[biosURL]];
		}
	}
	
	self.sleepInBackground = [[NSUserDefaults standardUserDefaults] boolForKey:@"PauseInBackground"];
	
	NSArray *progArgs = [[NSProcessInfo processInfo] arguments];
	if ([progArgs count] > 1 && !wasFinderLaunch) {
		self.skipFiles = [NSMutableArray array];
		
		BOOL isLaunchable = NO;
		NSString *runtimeStr = nil;
		
		__block short memcardHandled = 0;
		__block BOOL hasParsedAnArgument = NO;
		__block NSString *(^FileTestBlock)() = NULL;
		__block NSMutableDictionary *argDict = [[NSMutableDictionary alloc] initWithCapacity:[progArgs count]];
		
		
		NSMutableArray *unknownOptions = [NSMutableArray array];
		
		dispatch_block_t cdromBlock = ^{
			hasParsedAnArgument = YES;
			LaunchArg *larg = [[LaunchArg alloc] initWithLaunchOrder:LaunchArgRun argument:kPCSXRArgumentCDROM block:^{
				[self runCD:nil];
			}];
			[larg addToDictionary:argDict];
		};
		
		dispatch_block_t biosBlock = ^{
			hasParsedAnArgument = YES;
			LaunchArg *larg = [[LaunchArg alloc] initWithLaunchOrder:LaunchArgRun argument:kPCSXRArgumentBIOS block:^{
				[self runBios:nil];
			}];
			[larg addToDictionary:argDict];
		};
		
		//This block/argument does not need to be sorted
		dispatch_block_t emuCloseAtEnd = ^{
			hasParsedAnArgument = YES;
			LaunchArg *larg = [[LaunchArg alloc] initWithLaunchOrder:LaunchArgPreRun argument:kPCSXRArgumentExitAtClose block:^{
				self.endAtEmuClose = YES;
			}];
			[larg addToDictionary:argDict];
		};
		
		dispatch_block_t isoBlock = ^{
			hasParsedAnArgument = YES;
			NSString *path = FileTestBlock();
			LaunchArg *larg = [[LaunchArg alloc] initWithLaunchOrder:LaunchArgRun argument:kPCSXRArgumentISO block:^{
				[self runURL:[NSURL fileURLWithPath:path isDirectory:NO]];
			}];
			[larg addToDictionary:argDict];
		};
		
		void (^mcdBlock)(int mcdNumber) = ^(int mcdnumber){
			hasParsedAnArgument = YES;
			if (memcardHandled & (1 << mcdnumber)) {
				NSLog(@"Memory card %i has already been defined. The latest one passed will be used.", mcdnumber);
			} else {
				memcardHandled |= (1 << mcdnumber);
			}
			
			NSString *path = FileTestBlock();
			NSString *mcdArg = [kPCSXRArgumentMcd stringByAppendingFormat:@"%i", mcdnumber];
			LaunchArg *larg = [[LaunchArg alloc] initWithLaunchOrder:LaunchArgPreRun argument:mcdArg block:^{
				LoadMcd(mcdnumber, (char*)[path fileSystemRepresentation]);
			}];
			[larg addToDictionary:argDict];
		};
		
		dispatch_block_t freezeBlock = ^{
			hasParsedAnArgument = YES;
			NSString *path = FileTestBlock();
			LaunchArg *larg = [[LaunchArg alloc] initWithLaunchOrder:LaunchArgPostRun argument:kPCSXRArgumentFreeze block:^{
				if (![EmuThread isRunBios]) {
					//Make sure the emulator is running
					sleep(5);
					[EmuThread defrostAt:path];
				}
			}];
			[larg addToDictionary:argDict];
		};
		
		BOOL hasFileTestBlock = NO;
		
		for (__block int i = 1; i < [progArgs count]; i++) {
			if (!hasFileTestBlock)
			{
				FileTestBlock = ^NSString *(){
					if ([progArgs count] <= ++i) {
						ParseErrorStr(@"Not enough arguments.");
					}
					NSString *path = [progArgs[i] stringByExpandingTildeInPath];
					if (![[NSFileManager defaultManager] fileExistsAtPath:path])
					{
						ParseErrorStr([NSString stringWithFormat:@"The file \"%@\" does not exist.", path]);
						return nil;
					}
					[skipFiles addObject:path];
					return path;
				};
				hasFileTestBlock = YES;
			}
			
			//DO NOT END these MACROS WITH A SIMICOLON! It will break the if-else if process
			HandleArg(kPCSXRArgumentISO, YES, isoBlock)
			HandleArgElse(kPCSXRArgumentCDROM, YES, cdromBlock)
			HandleArgElse(kPCSXRArgumentBIOS, YES, biosBlock)
			HandleArgElse(kPCSXRArgumentExitAtClose, NO, emuCloseAtEnd)
			HandleArgElse(kPCSXRArgumentMcd1, NO, ^{mcdBlock(1);})
			HandleArgElse(kPCSXRArgumentMcd2, NO, ^{mcdBlock(2);})
			HandleArgElse(kPCSXRArgumentFreeze, NO, freezeBlock)
			else {
				[unknownOptions addObject:progArgs[i]];
			}
		}
#ifdef DEBUG
		if ([unknownOptions count]) {
			NSString *unknownString = [unknownOptions componentsJoinedByString:@" "];
			
			NSLog(@"The following options weren't recognized by PCSX-R: %@. This may be due to extra arguments passed by the OS or debugger.", unknownString);
		}
#endif
		unknownOptions = nil;
		if (!isLaunchable && hasParsedAnArgument) {
			NSMutableArray *mutProgArgs = [NSMutableArray arrayWithArray:progArgs];
			NSString *appRawPath = mutProgArgs[0];
			//Remove the app file path from the array
			[mutProgArgs removeObjectAtIndex:0];
			NSString *arg = [mutProgArgs componentsJoinedByString:@" "];
			NSString *recognizedArgs = [[argDict allKeys] componentsJoinedByString:@" "];
			
			NSString *tmpStr = [NSString stringWithFormat:@"A launch command wasn't found in the command line and one or more arguments that PCSX-R recognizes were: %@.\nThe following command line arguments were passed with the application launch file at %@: %@.\n\nThe valid launch commands are %@, %@, and %@.", recognizedArgs, appRawPath, arg, kPCSXRArgumentISO, kPCSXRArgumentCDROM, kPCSXRArgumentBIOS];
			ParseErrorStr(tmpStr);
		} else if (hasParsedAnArgument){
			NSArray *argArray = [[argDict allValues] sortedArrayWithOptions:NSSortStable usingComparator:^NSComparisonResult(id obj1, id obj2) {
				LaunchArg *LA1 = obj1;
				LaunchArg *LA2 = obj2;
				if (LA1.launchOrder > LA2.launchOrder) {
					return NSOrderedDescending;
				} else if (LA1.launchOrder < LA2.launchOrder) {
					return NSOrderedAscending;
				} else {
					return NSOrderedSame;
				}
			}];
			for (LaunchArg *arg in argArray) {
				arg.theBlock();
			}
		}
	}
}

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
		u8 *dst = (u8 *)[prefByteKeys[key] pointerValue];
		if (dst != NULL) *dst = [defaults boolForKey:key];
	}

	// special cases
	//str = [[defaults stringForKey:@"PluginPAD"] fileSystemRepresentation];
	//if (str != nil) strncpy(Config.Pad2, str, 255);

	str = [[defaults stringForKey:@"Bios"] fileSystemRepresentation];
	if (str) {
		NSString *path = [defaults stringForKey:@"Bios"];
		NSInteger index = [biosList indexOfObject:path];

		if (-1 == index) {
			[biosList insertObject:path atIndex:0];
		} else if (0 < index) {
			[biosList exchangeObjectAtIndex:index withObjectAtIndex:0];
		}
	}

	{
		NSFileManager *manager = [NSFileManager defaultManager];
		NSURL *memoryURL = [[[manager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL] URLByAppendingPathComponent:@"Pcsxr"] URLByAppendingPathComponent:@"Memory Cards"];
		
		str = [[[defaults URLForKey:@"Mcd1"] path] fileSystemRepresentation];
		if (str) {
			strlcpy(Config.Mcd1, str, MAXPATHLEN);
		} else {
			NSURL *url = [memoryURL URLByAppendingPathComponent:@"Mcd001.mcr"];
			[defaults setURL:url forKey:@"Mcd1"];
			str = [[url path] fileSystemRepresentation];
			if (str != nil) strlcpy(Config.Mcd1, str, MAXPATHLEN);
		}
		
		str = [[[defaults URLForKey:@"Mcd2"] path] fileSystemRepresentation];
		if (str) {
			strlcpy(Config.Mcd2, str, MAXPATHLEN);
		} else {
			NSURL *url = [memoryURL URLByAppendingPathComponent:@"Mcd002.mcr"];
			[defaults setURL:url forKey:@"Mcd2"];
			str = [[url path] fileSystemRepresentation];
			if (str != nil) strlcpy(Config.Mcd2, str, MAXPATHLEN);
		}
	}

	if ([defaults boolForKey:@"UseHLE"] || 0 == [biosList count]) {
		strcpy(Config.Bios, "HLE");
	} else {
		str = [(NSString *)biosList[0] fileSystemRepresentation];
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

	char *str = (char *)[prefStringKeys[defaultKey] pointerValue];
	if (str) {
		NSString *tmpNSStr = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:str length:strlen(str)];
		if (!tmpNSStr) {
			tmpNSStr = @(str);
		}
		
		[defaults setObject:tmpNSStr forKey:defaultKey];
		return;
	}
	
	str = (char *)[prefURLKeys[defaultKey] pointerValue];
	if (str) {
		NSString *tmpNSStr = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:str length:strlen(str)];
		if (!tmpNSStr) {
			tmpNSStr = @(str);
		}
		[defaults setURL:[NSURL fileURLWithPath:tmpNSStr isDirectory:NO] forKey:defaultKey];
		return;
	}

	u8 *val = (u8 *)[prefByteKeys[defaultKey] pointerValue];
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
	NSDictionary *appDefaults = @{@"NoDynarec": @YES,
								 @"AutoDetectVideoType": @YES,
								 @"UseHLE": @NO,
								 @"PauseInBackground": @YES,
								 @"Widescreen": @NO,
								 @"NetPlay": @NO};
	
	[defaults registerDefaults:appDefaults];

	prefStringKeys = @{@"PluginGPU": [NSValue valueWithPointer:Config.Gpu],
		@"PluginSPU": [NSValue valueWithPointer:Config.Spu],
		@"PluginPAD": [NSValue valueWithPointer:Config.Pad1],
		@"PluginCDR": [NSValue valueWithPointer:Config.Cdr],
		@"PluginNET": [NSValue valueWithPointer:Config.Net],
		@"PluginSIO1": [NSValue valueWithPointer:Config.Sio1]};
	
	prefURLKeys = @{@"Mcd1": [NSValue valueWithPointer:Config.Mcd1],
				   @"Mcd2": [NSValue valueWithPointer:Config.Mcd2]};

	prefByteKeys = @{@"NoXaAudio": [NSValue valueWithPointer:&Config.Xa],
		@"SioIrqAlways": [NSValue valueWithPointer:&Config.SioIrq],
		@"BlackAndWhiteMDECVideo": [NSValue valueWithPointer:&Config.Mdec],
		@"AutoDetectVideoType": [NSValue valueWithPointer:&Config.PsxAuto],
		@"VideoTypePAL": [NSValue valueWithPointer:&Config.PsxType],
		@"NoCDAudio": [NSValue valueWithPointer:&Config.Cdda],
		@"NoDynarec": [NSValue valueWithPointer:&Config.Cpu],
		@"ConsoleOutput": [NSValue valueWithPointer:&Config.PsxOut],
		@"SpuIrqAlways": [NSValue valueWithPointer:&Config.SpuIrq],
		@"RootCounterFix": [NSValue valueWithPointer:&Config.RCntFix],
		@"VideoSyncWAFix": [NSValue valueWithPointer:&Config.VSyncWA],
		@"Widescreen": [NSValue valueWithPointer:&Config.Widescreen]};

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
			[manager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:NULL];

        MemCardPath = [PcsxrAppSupport URLByAppendingPathComponent:@"Memory Cards"];
		url = MemCardPath;
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [PcsxrAppSupport URLByAppendingPathComponent:@"Patches"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:NULL];
		
		url = [PcsxrAppSupport URLByAppendingPathComponent:@"PlugIns"];
		if (![url checkResourceIsReachableAndReturnError:NULL])
            [manager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:NULL];
        
        saveStatePath = [[[PcsxrAppSupport URLByAppendingPathComponent:@"Save States"] path] copy];
		if (![manager fileExistsAtPath:saveStatePath isDirectory:&dir])
			[manager createDirectoryAtPath:saveStatePath withIntermediateDirectories:YES attributes:nil error:NULL];

        url = [MemCardPath URLByAppendingPathComponent:@"Mcd001.mcr"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil)
			strlcpy(Config.Mcd1, str, MAXPATHLEN);

		url = [MemCardPath URLByAppendingPathComponent:@"Mcd002.mcr"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil)
			strlcpy(Config.Mcd2, str, MAXPATHLEN);

		url = [PcsxrAppSupport URLByAppendingPathComponent:@"Bios"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil)
			strlcpy(Config.BiosDir, str, MAXPATHLEN);

		url = [PcsxrAppSupport URLByAppendingPathComponent:@"Patches"];
		str = [[url path] fileSystemRepresentation];
		if (str != nil) {
			strlcpy(Config.PatchesDir, str, MAXPATHLEN);
		}
	} else {
		strcpy(Config.BiosDir, "Bios/");
		strcpy(Config.PatchesDir, "Patches/");

		//NSString constants don't need to be retained/released. In fact, retain/releasing them does nothing.
		saveStatePath = @"sstates";
	}

	// set plugin path
	path = [[NSBundle mainBundle] builtInPlugInsPath];
	str = [path fileSystemRepresentation];
	if (str != nil)
		strlcpy(Config.PluginsDir, str, MAXPATHLEN);

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
	NSFileManager *fm = [NSFileManager defaultManager];
	if (skipFiles && [skipFiles count]) {
		for (NSString *parsedFile in skipFiles) {
			if ([filename isEqualToString:parsedFile]) {
				return YES;
			}
		}
	}
	
	if (![fm fileExistsAtPath:filename]) {
		NSLog(@"Nonexistant file %@ was passed to open.", filename );
		return NO;
	}
	
	if ([[filename pathExtension] compare:@"bin" options:(NSCaseInsensitiveSearch | NSWidthInsensitiveSearch)]) {
		NSDictionary *attrib = [fm attributesOfItemAtPath:filename error:NULL];
		if ([[attrib fileType] isEqualToString:NSFileTypeRegular] && ([attrib fileSize] % (256 * 1024)) == 0 && [attrib fileSize] > 0 ) {
			NSAlert *biosInfo = [NSAlert alertWithMessageText:NSLocalizedString(@"PlayStation BIOS File", @"PSX BIOS File") defaultButton:NSLocalizedString(@"BIOS_Copy", @"copy the BIOS over") alternateButton:NSLocalizedString(@"Cancel", @"Cancel") otherButton:NSLocalizedString(@"BIOS_Move", @"Move the bios over") informativeTextWithFormat:NSLocalizedString(@"The file \"%@\" seems to be a BIOS file. Do you want PCSX-R to copy it to the proper location?", @"Can we copy the BIOS?")];
			biosInfo.alertStyle = NSInformationalAlertStyle;
			switch ([biosInfo runModal]) {
				case NSAlertFirstButtonReturn:
				case NSAlertDefaultReturn:
				{
					NSError *theErr = nil;
					NSURL *biosDirPath = [NSURL fileURLWithPath:[fm stringWithFileSystemRepresentation:Config.BiosDir length:strlen(Config.BiosDir)] isDirectory:YES];
					NSURL *biosPath = [biosDirPath URLByAppendingPathComponent:[filename lastPathComponent]];
					if ([biosPath checkResourceIsReachableAndReturnError:NULL]) {
						NSAlert *alreadyThere = [NSAlert alertWithMessageText:NSLocalizedString(@"BIOS Already Exists", @"BIOS file already there.") defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:NSLocalizedString(@"There already exists a BIOS file at \"%1$@\": not copying the file at \"%2$@\".\n\nIf you do want to use the BIOS file at \"%2$@\", delete the BIOS at \"%1$@\".", @"What to do"), [biosPath path], filename];
						alreadyThere.alertStyle = NSCriticalAlertStyle;
						[alreadyThere runModal];
						return NO;
					}
					if (![fm copyItemAtURL:[NSURL fileURLWithPath:filename isDirectory:NO] toURL:biosPath error:&theErr]) {
						[[NSAlert alertWithError:theErr] runModal];
						return NO;
					}
				}
					break;
					
				case NSAlertThirdButtonReturn:
				case NSAlertOtherReturn:
				{
					NSError *theErr = nil;
					NSURL *biosDirPath = [NSURL fileURLWithPath:[fm stringWithFileSystemRepresentation:Config.BiosDir length:strlen(Config.BiosDir)] isDirectory:YES];
					NSURL *biosPath = [biosDirPath URLByAppendingPathComponent:[filename lastPathComponent]];
					if ([biosPath checkResourceIsReachableAndReturnError:NULL]) {
						NSAlert *alreadyThere = [NSAlert alertWithMessageText:NSLocalizedString(@"BIOS Already Exists", @"BIOS file already there.") defaultButton:nil alternateButton:nil otherButton:nil informativeTextWithFormat:NSLocalizedString(@"There already exists a BIOS file at \"%1$@\": not moving the file at \"%2$@\".\n\nIf you do want to use the BIOS file at \"%2$@\", delete the BIOS at \"%1$@\".", @"What to do"), [biosPath path], filename];
						alreadyThere.alertStyle = NSCriticalAlertStyle;
						[alreadyThere runModal];
						return NO;
					}
					if (![fm moveItemAtURL:[NSURL fileURLWithPath:filename isDirectory:NO] toURL:biosPath error:&theErr]) {
						[[NSAlert alertWithError:theErr] runModal];
						return NO;
					}
				}
					break;

				default:
					break;
			}
			return YES;
		}
	}
	
	NSError *err = nil;
	NSString *utiFile = [[NSWorkspace sharedWorkspace] typeOfFile:filename error:&err];
	if (err) {
		NSRunAlertPanel(NSLocalizedString(@"Error opening file", nil), NSLocalizedString(@"Unable to open %@: %@", nil), nil, nil, nil, [filename lastPathComponent], err);
		return NO;
	}
	static NSArray *handlers = nil;
	if (handlers == nil) {
		handlers = @[[PcsxrPluginHandler class], [PcsxrMemCardHandler class], [PcsxrFreezeStateHandler class], [PcsxrDiscHandler class], [PcsxrCheatHandler class]];
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
			break;
		}
	}
	
	return isHandled;
}

@end
