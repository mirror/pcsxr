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
__private_extern__ char* Pcsxr_locale_text(char* toloc);
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
__private_extern__ char* PLUGLOC(char* toloc);
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

static SPUPluginController *pluginController = nil;

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

void DoAbout()
{
	// Get parent application instance
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];

	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (path) {
		credits =  [[NSAttributedString alloc] initWithPath: path documentAttributes:NULL];
	} else {
		credits = [[NSAttributedString alloc] initWithString:@""];
	}
	
	// Get Application Icon
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[bundle bundlePath]];
	NSSize size = NSMakeSize(64, 64);
	[icon setSize:size];
		
	NSDictionary *infoPaneDict =
	@{@"ApplicationName": [bundle objectForInfoDictionaryKey:@"CFBundleName"],
	 @"ApplicationIcon": icon,
	 @"ApplicationVersion": [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
	 @"Version": [bundle objectForInfoDictionaryKey:@"CFBundleVersion"],
	 @"Copyright": [bundle objectForInfoDictionaryKey:@"NSHumanReadableCopyright"],
	 @"Credits": credits};
	dispatch_async(dispatch_get_main_queue(), ^{
		[NSApp orderFrontStandardAboutPanelWithOptions:infoPaneDict];
	});
}

long DoConfiguration()
{
	RunOnMainThreadSync(^{
		NSWindow *window;
		
		if (pluginController == nil) {
			pluginController = [[PluginController alloc] initWithWindowNibName:@"NetSfPeopsSpuPluginMain"];
		}
		window = [pluginController window];
		
		/* load values */
		[pluginController loadValues];
		
		[window center];
		[window makeKeyAndOrderFront:nil];
	});

	return 0;
}

void ReadConfig(void)
{
	NSDictionary *keyValues;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults registerDefaults:@{PrefsKey: @{@"High Compatibility Mode": @YES,
								 @"SPU IRQ Wait": @YES,
								 @"XA Pitch": @NO,
								 @"Mono Sound Output": @NO,
								 @"Interpolation Quality": @0,
								 @"Reverb Quality": @1,
								 @"Volume": @3}}];
	
	keyValues = [defaults dictionaryForKey:PrefsKey];
	
	iUseTimer = [keyValues[@"High Compatibility Mode"] boolValue] ? 2 : 0;
	iSPUIRQWait = [keyValues[@"SPU IRQ Wait"] boolValue];
	iDisStereo = [keyValues[@"Mono Sound Output"] boolValue];
	iXAPitch = [keyValues[@"XA Pitch"] boolValue];
	
	iUseInterpolation = [keyValues[@"Interpolation Quality"] intValue];
	iUseReverb = [keyValues[@"Reverb Quality"] intValue];
	
	iVolume = 5 - [keyValues[@"Volume"] intValue];
}

@implementation PluginController

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:self.keyValues];
	writeDic[@"High Compatibility Mode"] = ([hiCompBox intValue] ? @YES : @NO);
	writeDic[@"SPU IRQ Wait"] = ([irqWaitBox intValue] ? @YES : @NO);
	writeDic[@"Mono Sound Output"] = ([monoSoundBox intValue] ? @YES : @NO);
	writeDic[@"XA Pitch"] = ([xaSpeedBox intValue] ? @YES : @NO);

	writeDic[@"Interpolation Quality"] = @([interpolValue intValue]);
	writeDic[@"Reverb Quality"] = @([reverbValue intValue]);

	writeDic[@"Volume"] = @([volumeValue intValue]);

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
	self.keyValues = [NSMutableDictionary dictionaryWithDictionary:[defaults dictionaryForKey:PrefsKey]];

	[hiCompBox setIntValue:[keyValues[@"High Compatibility Mode"] boolValue]];
	[irqWaitBox setIntValue:[keyValues[@"SPU IRQ Wait"] boolValue]];
	[monoSoundBox setIntValue:[keyValues[@"Mono Sound Output"] boolValue]];
	[xaSpeedBox setIntValue:[keyValues[@"XA Pitch"] boolValue]];

	[interpolValue setIntValue:[keyValues[@"Interpolation Quality"] intValue]];
	[reverbValue setIntValue:[keyValues[@"Reverb Quality"] intValue]];
	[volumeValue setIntValue:[keyValues[@"Volume"] intValue]];
}

- (void)awakeFromNib
{
	Class thisClass = [self class];
	
	NSBundle *spuBundle = [NSBundle bundleForClass:thisClass];
	
	[interpolValue setStrings:@[
		[spuBundle localizedStringForKey:@"(No Interpolation)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Simple Interpolation)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Gaussian Interpolation)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Cubic Interpolation)" value:@"" table:nil]]];
	interpolValue.pluginClass = thisClass;

	[reverbValue setStrings:@[
		[spuBundle localizedStringForKey:@"(No Reverb)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Simple Reverb)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(PSX Reverb)" value:@"" table:nil]]];
	reverbValue.pluginClass = thisClass;

	[volumeValue setStrings:@[
		[spuBundle localizedStringForKey:@"(Muted)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Low)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Medium)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Loud)" value:@"" table:nil],
		[spuBundle localizedStringForKey:@"(Loudest)" value:@"" table:nil]]];
	volumeValue.pluginClass = thisClass;
}

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginController class]);
