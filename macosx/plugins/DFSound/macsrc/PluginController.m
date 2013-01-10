#import "PluginController.h"
#include "stdafx.h"
#include "externals.h"
#include "maccfg.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
//If running under Mac OS X, use the Localizable.strings file instead.
#elif defined(_MACOSX)
#ifdef PCSXRCORE
extern const char* Pcsxr_locale_text(char* toloc);
#define _(String) Pcsxr_locale_text(String)
#define N_(String) String
#else
#ifndef PCSXRPLUG
#warning please define the plug being built to use Mac OS X localization!
#define _(msgid) msgid
#define N_(msgid) msgid
#else
//Kludge to get the preprocessor to accept PCSXRPLUG as a variable.
#define PLUGLOC_x(x,y) x ## y
#define PLUGLOC_y(x,y) PLUGLOC_x(x,y)
#define PLUGLOC PLUGLOC_y(PCSXRPLUG,_locale_text)
extern const char* PLUGLOC(char* toloc);
#define _(String) PLUGLOC(String)
#define N_(String) String
#endif
#endif
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

#ifdef USEOPENAL
#define APP_ID @"net.sf.peops.SPUALPlugin"
#else
#define APP_ID @"net.sf.peops.SPUSDLPlugin"
#endif
#define PrefsKey APP_ID @" Settings"

static PluginController *pluginController = nil;
char * pConfigFile=NULL;

void DoAbout()
{
	// Get parent application instance
	NSApplication *app = [NSApplication sharedApplication];
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];

	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (path) {
		credits = [[[NSAttributedString alloc] initWithPath: path documentAttributes:NULL] autorelease];
	} else {
		credits = [[[NSAttributedString alloc] initWithString:@""] autorelease];
	}
	
	// Get Application Icon
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[bundle bundlePath]];
	NSSize size = NSMakeSize(64, 64);
	[icon setSize:size];
		
	[app orderFrontStandardAboutPanelWithOptions:[NSDictionary dictionaryWithObjectsAndKeys:
												  [bundle objectForInfoDictionaryKey:@"CFBundleName"], @"ApplicationName",
												  icon, @"ApplicationIcon",
												  [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"], @"ApplicationVersion",
												  [bundle objectForInfoDictionaryKey:@"CFBundleVersion"], @"Version",
												  [bundle objectForInfoDictionaryKey:@"NSHumanReadableCopyright"], @"Copyright",
												  credits, @"Credits",
												  nil]];
}


long DoConfiguration()
{
	NSWindow *window;
	
	if (pluginController == nil) {
#ifdef USEOPENAL
		pluginController = [[PluginController alloc] initWithWindowNibName:@"NetSfPeopsSpuALPluginMain"];
#else
		pluginController = [[PluginController alloc] initWithWindowNibName:@"NetSfPeopsSpuSDLPluginMain"];
#endif
	}
	window = [pluginController window];

	/* load values */
	[pluginController loadValues];

	[window center];
	[window makeKeyAndOrderFront:nil];

	return 0;
}

void ReadConfig(void)
{
	NSDictionary *keyValues;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
								[NSDictionary dictionaryWithObjectsAndKeys:
								 [NSNumber numberWithBool:YES], @"High Compatibility Mode",
								 [NSNumber numberWithBool:YES], @"SPU IRQ Wait",
								 [NSNumber numberWithBool:NO], @"XA Pitch",
								 [NSNumber numberWithBool:NO], @"Mono Sound Output",
								 [NSNumber numberWithInt:0], @"Interpolation Quality",
								 [NSNumber numberWithInt:1], @"Reverb Quality",
								 [NSNumber numberWithInt:3], @"Volume",
								 nil], PrefsKey,
								nil]];

	keyValues = [defaults dictionaryForKey:PrefsKey];

	iUseTimer = [[keyValues objectForKey:@"High Compatibility Mode"] boolValue] ? 2 : 0;
	iSPUIRQWait = [[keyValues objectForKey:@"SPU IRQ Wait"] boolValue];
	iDisStereo = [[keyValues objectForKey:@"Mono Sound Output"] boolValue];
	iXAPitch = [[keyValues objectForKey:@"XA Pitch"] boolValue];

	iUseInterpolation = [[keyValues objectForKey:@"Interpolation Quality"] intValue];
	iUseReverb = [[keyValues objectForKey:@"Reverb Quality"] intValue];

	iVolume = 5 - [[keyValues objectForKey:@"Volume"] intValue];
}

@implementation PluginController

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];
	[writeDic setObject:[NSNumber numberWithBool:[hiCompBox intValue]] forKey:@"High Compatibility Mode"];
	[writeDic setObject:[NSNumber numberWithBool:[irqWaitBox intValue]] forKey:@"SPU IRQ Wait"];
	[writeDic setObject:[NSNumber numberWithBool:[monoSoundBox intValue]] forKey:@"Mono Sound Output"];
	[writeDic setObject:[NSNumber numberWithBool:[xaSpeedBox intValue]] forKey:@"XA Pitch"];

	[writeDic setObject:[NSNumber numberWithInt:[interpolValue intValue]] forKey:@"Interpolation Quality"];
	[writeDic setObject:[NSNumber numberWithInt:[reverbValue intValue]] forKey:@"Reverb Quality"];

	[writeDic setObject:[NSNumber numberWithInt:[volumeValue intValue]] forKey:@"Volume"];

	// write to defaults
	[defaults setObject:writeDic forKey:PrefsKey];
	[defaults synchronize];

	// and set global values accordingly
	ReadConfig();

	[self close];
}

- (IBAction)reset:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults removeObjectForKey:PrefsKey];
	[self loadValues];
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	ReadConfig();

	/* load from preferences */
	[keyValues release];
	keyValues = [[defaults dictionaryForKey:PrefsKey] mutableCopy];

	[hiCompBox setIntValue:[[keyValues objectForKey:@"High Compatibility Mode"] boolValue]];
	[irqWaitBox setIntValue:[[keyValues objectForKey:@"SPU IRQ Wait"] boolValue]];
	[monoSoundBox setIntValue:[[keyValues objectForKey:@"Mono Sound Output"] boolValue]];
	[xaSpeedBox setIntValue:[[keyValues objectForKey:@"XA Pitch"] boolValue]];

	[interpolValue setIntValue:[[keyValues objectForKey:@"Interpolation Quality"] intValue]];
	[reverbValue setIntValue:[[keyValues objectForKey:@"Reverb Quality"] intValue]];
	[volumeValue setIntValue:[[keyValues objectForKey:@"Volume"] intValue]];
}

- (void)awakeFromNib
{
	NSBundle *mainBundle = [NSBundle bundleForClass:[self class]];

	[interpolValue setStrings:[NSArray arrayWithObjects:
		[mainBundle localizedStringForKey:@"(No Interpolation)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Simple Interpolation)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Gaussian Interpolation)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Cubic Interpolation)" value:nil table:nil],
		nil]];

	[reverbValue setStrings:[NSArray arrayWithObjects:
		[mainBundle localizedStringForKey:@"(No Reverb)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Simple Reverb)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(PSX Reverb)" value:nil table:nil],
		nil]];

	[volumeValue setStrings:[NSArray arrayWithObjects:
		[mainBundle localizedStringForKey:@"(Muted)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Low)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Medium)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Loud)" value:nil table:nil],
		[mainBundle localizedStringForKey:@"(Loudest)" value:nil table:nil],
		nil]];
}

@end

const char* PLUGLOC(char *toloc)
{
	NSBundle *mainBundle = [NSBundle bundleForClass:[PluginController class]];
	NSString *origString = nil, *transString = nil;
	origString = [NSString stringWithCString:toloc encoding:NSUTF8StringEncoding];
	transString = [mainBundle localizedStringForKey:origString value:nil table:nil];
	return [transString cStringUsingEncoding:NSUTF8StringEncoding];
}
