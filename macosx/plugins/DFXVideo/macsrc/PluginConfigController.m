#import "PluginConfigController.h"
#include "gpu.h"
#include "cfg.h"
#include "menu.h"
#include "externals.h"
#import "SGPUPreferences.h"
#import "PluginGLView.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
//If running under Mac OS X, use the Localizable.strings file instead.
#elif defined(_MACOSX)
#ifdef PCSXRCORE
__private_extern char* Pcsxr_locale_text(char* toloc);
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
__private_extern char* PLUGLOC(char* toloc);
#define _(String) PLUGLOC(String)
#define N_(String) String
#endif
#endif
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

#define APP_ID @"net.sf.peops.SoftGpuGLPlugin"
#define PrefsKey APP_ID @" Settings"

static PluginConfigController *windowController = nil;

#define kWindowSize @"Window Size"

void AboutDlgProc()
{
	// Get parent application instance
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];
	
	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (!path) {
		path = [bundle pathForResource:@"Credits" ofType:@"rtfd"];
	}
	if (path) {
		credits = [[NSAttributedString alloc] initWithPath:path documentAttributes:NULL];
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

void SoftDlgProc()
{
	RunOnMainThreadSync(^{
		NSWindow *window;
		
		if (windowController == nil) {
			windowController = [[PluginConfigController alloc] initWithWindowNibName:@"NetSfPeopsSoftGPUConfig"];
		}
		window = [windowController window];
		
		/* load values */
		[windowController loadValues];
		
		[window center];
		[window makeKeyAndOrderFront:nil];
	});
}

BOOL isShaderEnabled()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	return [keyValues[@"UseShader"] boolValue];
}

NSURL *PSXVertexShader()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	return [NSURL URLByResolvingBookmarkData:keyValues[@"VertexShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil];
}

NSURL *PSXFragmentShader()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	return [NSURL URLByResolvingBookmarkData:keyValues[@"FragmentShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil];
}

float PSXShaderQuality()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	return (float)[keyValues[@"ShaderQuality"] intValue];
}

void ReadConfig(void)
{
	NSDictionary *keyValues;
	NSBundle *selfBundle = [NSBundle bundleWithIdentifier:APP_ID];
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults registerDefaults:@{PrefsKey: @{@"FPS Counter": @NO,
								 @"Auto Full Screen": @NO,
								 @"Frame Skipping": @NO,
								 @"Frame Limit": @YES,
								 @"VSync": @NO,
								 @"Enable Hacks": @NO,
								 @"Dither Mode": @1,
								 @"Hacks": @((unsigned int)0),
								 @"VertexShader": [[selfBundle URLForResource:@"gpuPeteOGL2" withExtension:@"slv"] bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:nil],
								 @"FragmentShader": [[selfBundle URLForResource:@"gpuPeteOGL2" withExtension:@"slf"] bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:nil],
								 @"UseShader": @NO,
								 @"ShaderQuality": @4,
								 kWindowSize: NSStringFromSize(NSMakeSize(640, 480))}}];
	
	keyValues = [defaults dictionaryForKey:PrefsKey];
	BOOL windowSizeNeedsReset = NO;
	if (keyValues) {
		NSSize size = NSSizeFromString(keyValues[kWindowSize]);
		if (!keyValues[kWindowSize]) {
			windowSizeNeedsReset = YES;
		} else if ([keyValues[kWindowSize] isKindOfClass:[NSNumber class]]) {
			windowSizeNeedsReset = YES;
		} else if (size.height == 0 || size.width == 0) {
			windowSizeNeedsReset = YES;
		}
	}
	if (windowSizeNeedsReset) {
		NSMutableDictionary *tmpDict = [[NSMutableDictionary alloc] initWithDictionary:keyValues];
		tmpDict[kWindowSize] = NSStringFromSize(NSMakeSize(640, 480));
		[defaults setObject:tmpDict forKey:PrefsKey];
		[defaults synchronize];
	}
	
	iShowFPS = [keyValues[@"FPS Counter"] boolValue];
	iWindowMode = [keyValues[@"Auto Full Screen"] boolValue] ? 0 : 1;
	UseFrameSkip = [keyValues[@"Frame Skipping"] boolValue];
	UseFrameLimit = [keyValues[@"Frame Limit"] boolValue];
	//??? = [[keyValues objectForKey:@"VSync"] boolValue];
	iUseFixes = [keyValues[@"Enable Hacks"] boolValue];

	iUseDither = [keyValues[@"Dither Mode"] intValue];
	dwCfgFixes = [keyValues[@"Hacks"] unsignedIntValue];
	
	NSSize windowSize = NSSizeFromString(keyValues[kWindowSize]);
	
	iResX = windowSize.width;
	iResY = windowSize.height;
	iUseNoStretchBlt = 1;

	fFrameRate = 60;
	iFrameLimit = 2;
	
	if (iShowFPS)
		ulKeybits |= KEY_SHOWFPS;
	else
		ulKeybits &= ~KEY_SHOWFPS;
	
	// additional checks
	if(!iColDepth)
		iColDepth = 32;
	if(iUseFixes) {
		dwActFixes = dwCfgFixes;
	} else {
		dwActFixes = 0;
	}
	SetFixes();
	
	if(iFrameLimit == 2)
		SetAutoFrameCap();
	bSkipNextFrame = FALSE;
	
	szDispBuf[0] = 0;
	BuildDispMenu(0);
}

@implementation NetSfPeopsSoftGPUPluginConfigController

@synthesize fragmentPath;
@synthesize vertexPath;

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];
	writeDic[@"FPS Counter"] = ([fpsCounter intValue] ? @YES : @NO);
	writeDic[@"Auto Full Screen"] = ([autoFullScreen intValue] ? @YES : @NO);
	writeDic[@"Frame Skipping"] = ([frameSkipping intValue] ? @YES : @NO);
	//[writeDic setObject:@([frameLimit intValue]) forKey:@"Frame Limit"];
	writeDic[@"VSync"] = ([vSync intValue] ? @YES : @NO);
	writeDic[@"Enable Hacks"] = ([hackEnable intValue] ? @YES : @NO);
	writeDic[@"UseShader"] = ([shaders intValue] ? @YES : @NO);
	writeDic[@"ShaderQuality"] = @([shaderQualitySelector indexOfSelectedItem] + 1);
	writeDic[@"Dither Mode"] = @([ditherMode indexOfSelectedItem]);
	
	unsigned int hackValues = 0;
	for (NSCell *control in [hacksMatrix cells]) {
			hackValues |= [control intValue] << ([control tag] - 1);
	}
	
	writeDic[@"Hacks"] = @(hackValues);

	writeDic[@"VertexShader"] = [vertexPath bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:nil];
	writeDic[@"FragmentShader"] = [fragmentPath bookmarkDataWithOptions:0 includingResourceValuesForKeys:nil relativeToURL:nil error:nil];
	writeDic[kWindowSize] = NSStringFromSize(NSMakeSize(self.displayWidth.integerValue, self.displayHeight.integerValue));
	
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

- (IBAction)hackToggle:(id)sender
{
	BOOL enable = [sender intValue] ? YES : NO;
	NSArray *views = [[hacksView subviews][0] subviews];

	for (NSView *control in views) {
		if ([control isKindOfClass:[NSControl class]]) {
			if ([control isKindOfClass:[NSTextField class]]) {
				[(NSTextField*)control setTextColor:enable ? [NSColor controlTextColor] : [NSColor disabledControlTextColor] ];
			}
			[(NSControl *)control setEnabled:enable];
		}
	}
}

- (IBAction)toggleShader:(id)sender {
	BOOL enable = [sender intValue] ? YES : NO;
	NSArray *views = [[shadersView subviews][0] subviews];
	
	for (NSView *control in views) {
		if ([control isKindOfClass:[NSControl class]]) {
			if ([control isKindOfClass:[NSTextField class]]) {
				[(NSTextField*)control setTextColor:enable ? [NSColor controlTextColor] : [NSColor disabledControlTextColor] ];
			}
			[(NSControl *)control setEnabled:enable];
		}
	}
}

- (void)setFragmentPathInfo:(NSURL *)_fragmentPath
{
	self.fragmentPath = _fragmentPath;
	if (_fragmentPath) {
		[fragmentShaderViewablePath setStringValue:[fragmentPath lastPathComponent]];
		[fragmentShaderViewablePath setToolTip:[fragmentPath path]];
	}
}

- (void)setVertexPathInfo:(NSURL *)_vertexPath
{
	self.vertexPath = _vertexPath;
	if (_vertexPath) {
		[vertexShaderViewablePath setStringValue:[vertexPath lastPathComponent]];
		[vertexShaderViewablePath setToolTip:[vertexPath path]];
	}
}

- (IBAction)selectShader:(id)sender {
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	[openPanel setAllowsMultipleSelection:NO];
	[openPanel setCanChooseDirectories:NO];
	[openPanel setCanChooseFiles:YES];
	if ([openPanel runModal] == NSFileHandlingPanelOKButton) {
		if ([sender tag] == 1) {
			[self setVertexPathInfo:[openPanel URL]];
		} else {
			[self setFragmentPathInfo:[openPanel URL]];
		}
	}
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSSize theSize;
	
	ReadConfig();
	
	/* load from preferences */
	keyValues = [[defaults dictionaryForKey:PrefsKey] mutableCopy];
	
	{
		BOOL resetPrefs = NO;
		[self setVertexPathInfo:[NSURL URLByResolvingBookmarkData:keyValues[@"VertexShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil]];
		if (!vertexPath) {
			resetPrefs = YES;
		}
		[self setFragmentPathInfo:[NSURL URLByResolvingBookmarkData:keyValues[@"FragmentShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil]];
		if (!fragmentPath) {
			resetPrefs = YES;
		}
		if (resetPrefs) {
			NSBundle *selfBundle = [NSBundle bundleForClass:[self class]];
			[self setVertexPathInfo:[selfBundle URLForResource:@"gpuPeteOGL2" withExtension:@"slv"]];
			[self setFragmentPathInfo:[selfBundle URLForResource:@"gpuPeteOGL2" withExtension:@"slf"]];
		}
	}
	[fpsCounter setIntValue:[keyValues[@"FPS Counter"] intValue]];
	[autoFullScreen setIntValue:[keyValues[@"Auto Full Screen"] intValue]];
	[frameSkipping setIntValue:[keyValues[@"Frame Skipping"] intValue]];
	[vSync setIntValue:[keyValues[@"VSync"] intValue]];
	[hackEnable setIntValue:[keyValues[@"Enable Hacks"] intValue]];
	[shaders setIntValue:[keyValues[@"UseShader"] intValue]];

	[ditherMode selectItemAtIndex:[keyValues[@"Dither Mode"] intValue]];
	[shaderQualitySelector selectItemAtIndex:[keyValues[@"ShaderQuality"] intValue] - 1];
	
	unsigned int hackValues = [keyValues[@"Hacks"] unsignedIntValue];
	
	for (NSCell *control in [hacksMatrix cells]) {
			[control setIntValue:(hackValues >> ([control tag] - 1)) & 1];
	}
	theSize = NSSizeFromString(keyValues[kWindowSize]);
	[self.displayWidth setIntegerValue:theSize.width];
	[self.displayHeight setIntegerValue:theSize.height];
	
	[self hackToggle:hackEnable];
	[self toggleShader:shaders];
}

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginConfigController class])
