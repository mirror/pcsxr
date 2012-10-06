#import "PluginConfigController.h"
#include "gpu.h"
#include "cfg.h"
#include "menu.h"
#include "externals.h"
#include "SGPUPreferences.h"

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

#define APP_ID @"net.sf.peops.SoftGpuGLPlugin"
#define PrefsKey APP_ID @" Settings"

static PluginConfigController *windowController;
char * pConfigFile=NULL;

void AboutDlgProc()
{
	// Get parent application instance
	NSApplication *app = [NSApplication sharedApplication];
	NSBundle *bundle = [NSBundle bundleWithIdentifier:APP_ID];

	// Get Credits.rtf
	NSString *path = [bundle pathForResource:@"Credits" ofType:@"rtf"];
	NSAttributedString *credits;
	if (path) {
		credits = [[[NSAttributedString alloc] initWithPath: path
				documentAttributes:NULL] autorelease];
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


void SoftDlgProc()
{
	NSWindow *window;
	
	if (windowController == nil) {
		windowController = [[PluginConfigController alloc] initWithWindowNibName:@"NetSfPeopsSoftGPUConfig"];
	}
	window = [windowController window];
	
	/* load values */
	[windowController loadValues];
	
	[window center];
	[window makeKeyAndOrderFront:nil];
}

BOOL isShaderEnabled()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	return [[keyValues objectForKey:@"UseShader"] boolValue];
}

NSURL *PSXVertexShader()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	NSURL *returnURL = [NSURL URLByResolvingBookmarkData:[keyValues objectForKey:@"VertexShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil];
	return returnURL;
}

NSURL *PSXFragmentShader()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	NSURL *returnURL = [NSURL URLByResolvingBookmarkData:[keyValues objectForKey:@"FragmentShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil];
	return returnURL;

}

float PSXShaderQuality()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSDictionary *keyValues = [defaults dictionaryForKey:PrefsKey];
	return (float)[[keyValues objectForKey:@"ShaderQuality"] intValue];
}


void ReadConfig(void)
{
	NSDictionary *keyValues;
	NSBundle *selfBundle = [NSBundle bundleWithIdentifier:APP_ID];
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
								[NSMutableDictionary dictionaryWithObjectsAndKeys:
								 [NSNumber numberWithBool:NO], @"FPS Counter",
								 [NSNumber numberWithBool:NO], @"Auto Full Screen",
								 [NSNumber numberWithBool:NO], @"Frame Skipping",
								 [NSNumber numberWithBool:YES], @"Frame Limit",
								 [NSNumber numberWithBool:NO], @"VSync",
								 [NSNumber numberWithBool:NO], @"Enable Hacks",
								 [NSNumber numberWithInt:1], @"Dither Mode",
								 [NSNumber numberWithUnsignedInt:0], @"Hacks",
								 [[selfBundle URLForResource:@"gpuPeteOGL2" withExtension:@"slv"] bookmarkDataWithOptions:NSURLBookmarkCreationPreferFileIDResolution includingResourceValuesForKeys:nil relativeToURL:nil error:nil], @"VertexShader",
								 [[selfBundle URLForResource:@"gpuPeteOGL2" withExtension:@"slf"] bookmarkDataWithOptions:NSURLBookmarkCreationPreferFileIDResolution includingResourceValuesForKeys:nil relativeToURL:nil error:nil], @"FragmentShader",
								 [NSNumber numberWithBool:NO], @"UseShader",
								 [NSNumber numberWithInt:4], @"ShaderQuality",
								 nil], PrefsKey,
								nil]];
	
	keyValues = [defaults dictionaryForKey:PrefsKey];

	iShowFPS = [[keyValues objectForKey:@"FPS Counter"] boolValue];
	iWindowMode = [[keyValues objectForKey:@"Auto Full Screen"] boolValue] ? 0 : 1;
	UseFrameSkip = [[keyValues objectForKey:@"Frame Skipping"] boolValue];
	UseFrameLimit = [[keyValues objectForKey:@"Frame Limit"] boolValue];
	//??? = [[keyValues objectForKey:@"VSync"] boolValue];
	iUseFixes = [[keyValues objectForKey:@"Enable Hacks"] boolValue];

	iUseDither = [[keyValues objectForKey:@"Dither Mode"] intValue];
	dwCfgFixes = [[keyValues objectForKey:@"Hacks"] unsignedIntValue];
	
	iResX = 640;
	iResY = 480;
	iUseNoStretchBlt = 1;

	fFrameRate = 60;
	iFrameLimit = 2;
	
	if (iShowFPS)
		ulKeybits|=KEY_SHOWFPS;
	else
		ulKeybits&=~KEY_SHOWFPS;
	
	// additional checks
	if(!iColDepth)       iColDepth=32;
	if(iUseFixes) {
		dwActFixes=dwCfgFixes;
	} else {
		dwActFixes=0;
	}
	SetFixes();
	
	if(iFrameLimit==2) SetAutoFrameCap();
	bSkipNextFrame = FALSE;
	
	szDispBuf[0]=0;
	BuildDispMenu(0);
}

@implementation NetSfPeopsSoftGPUPluginConfigController

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];
	[writeDic setObject:[NSNumber numberWithInt:[fpsCounter intValue]] forKey:@"FPS Counter"];
	[writeDic setObject:[NSNumber numberWithInt:[autoFullScreen intValue]] forKey:@"Auto Full Screen"];
	[writeDic setObject:[NSNumber numberWithInt:[frameSkipping intValue]] forKey:@"Frame Skipping"];
	//[writeDic setObject:[NSNumber numberWithInt:[frameLimit intValue]] forKey:@"Frame Limit"];
	[writeDic setObject:[NSNumber numberWithInt:[vSync intValue]] forKey:@"VSync"];
	[writeDic setObject:[NSNumber numberWithInt:[hackEnable intValue]] forKey:@"Enable Hacks"];
	[writeDic setObject:[NSNumber numberWithInt:[shaders intValue]] forKey:@"UseShader"];
	[writeDic setObject:[NSNumber numberWithInt:[shaderQualitySelector indexOfSelectedItem]] forKey:@"ShaderQuality"];
	[writeDic setObject:[NSNumber numberWithInt:[ditherMode indexOfSelectedItem]] forKey:@"Dither Mode"];
	
	unsigned int hackValues = 0;
	NSArray *views = [hacksView subviews];
	for (NSView *control in views) {
		if ([control isKindOfClass:[NSButton class]]) {
			hackValues |= [(NSControl *)control intValue] << ([control tag] - 1);
		}
	}
	
	[writeDic setObject:[NSNumber numberWithUnsignedInt:hackValues] forKey:@"Hacks"];

	[writeDic setObject:[vertexPath bookmarkDataWithOptions:NSURLBookmarkCreationPreferFileIDResolution includingResourceValuesForKeys:nil relativeToURL:nil error:nil] forKey:@"VertexShader"];
	[writeDic setObject:[fragmentPath bookmarkDataWithOptions:NSURLBookmarkCreationPreferFileIDResolution includingResourceValuesForKeys:nil relativeToURL:nil error:nil] forKey:@"FragmentShader"];
	
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
	NSArray *views = [hacksView subviews];

	for (NSView *control in views) {
		if ([control isKindOfClass:[NSButton class]]) {
			[(NSControl *)control setEnabled:enable];
		}
	}
}

- (IBAction)toggleShader:(id)sender {
	BOOL enable = [sender intValue] ? YES : NO;
	NSArray *views = [shadersView subviews];
	
	for (NSView *control in views) {
		[(NSControl *)control setEnabled:enable];
	}
}

- (IBAction)selectShader:(id)sender {
	NSOpenPanel *openPanel = [[NSOpenPanel openPanel] retain];
	[openPanel setAllowsMultipleSelection:NO];
	[openPanel setCanChooseDirectories:NO];
	[openPanel setCanChooseFiles:YES];
	if ([openPanel runModal] == NSFileHandlingPanelOKButton)
	{
		if ([sender tag] == 1) {
			[vertexPath release];
			vertexPath = [[openPanel URL] copy];
			[vertexShaderViewablePath setTitleWithMnemonic:[vertexPath path]];
		} else {
			[fragmentPath release];
			fragmentPath = [[openPanel URL] copy];
			[fragmentShaderViewablePath setTitleWithMnemonic:[fragmentPath path]];
		}
	}
	
	[openPanel release];
}

- (void)loadValues
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	ReadConfig();
	
	/* load from preferences */
	[keyValues release];
	keyValues = [[defaults dictionaryForKey:PrefsKey] mutableCopy];
	
	[fpsCounter setIntValue:[[keyValues objectForKey:@"FPS Counter"] intValue]];
	[autoFullScreen setIntValue:[[keyValues objectForKey:@"Auto Full Screen"] intValue]];
	[frameSkipping setIntValue:[[keyValues objectForKey:@"Frame Skipping"] intValue]];
	[vSync setIntValue:[[keyValues objectForKey:@"VSync"] intValue]];
	[hackEnable setIntValue:[[keyValues objectForKey:@"Enable Hacks"] intValue]];
	[shaders setIntValue:[[keyValues objectForKey:@"UseShader"] intValue]];

	[ditherMode selectItemAtIndex:[[keyValues objectForKey:@"Dither Mode"] intValue]];
	[shaderQualitySelector selectItemAtIndex:[[keyValues objectForKey:@"ShaderQuality"] intValue]];
	
	vertexPath = [[NSURL URLByResolvingBookmarkData:[keyValues objectForKey:@"VertexShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil] retain];
	fragmentPath = [[NSURL URLByResolvingBookmarkData:[keyValues objectForKey:@"FragmentShader"] options:NSURLBookmarkResolutionWithoutUI relativeToURL:nil bookmarkDataIsStale:NULL error:nil] retain];
	[vertexShaderViewablePath setTitleWithMnemonic:[vertexPath path]];
	[fragmentShaderViewablePath setTitleWithMnemonic:[fragmentPath path]];
	unsigned int hackValues = [[keyValues objectForKey:@"Hacks"] unsignedIntValue];
	
	
	NSArray *views = [hacksView subviews];
	for (NSView *control in views) {
		if ([control isKindOfClass:[NSButton class]]) {
			[(NSControl *)control setIntValue:(hackValues >> ([control tag] - 1)) & 1];
		}
	}
	
	[self hackToggle:hackEnable];
	[self toggleShader:shaders];
}

- (void)awakeFromNib
{
	//I don't know why we need to do this...
	hacksView = [[hacksView subviews] objectAtIndex:0];
	shadersView = [[shadersView subviews] objectAtIndex:0];
}

- (void)dealloc
{
	[vertexPath release];
	[fragmentPath release];
	
	[super dealloc];
}

@end

const char* PLUGLOC(char *toloc)
{
	NSBundle *mainBundle = [NSBundle bundleForClass:[PluginConfigController class]];
	NSString *origString = nil, *transString = nil;
	origString = [NSString stringWithCString:toloc encoding:NSUTF8StringEncoding];
	transString = [mainBundle localizedStringForKey:origString value:nil table:nil];
	return [transString cStringUsingEncoding:NSUTF8StringEncoding];
}

