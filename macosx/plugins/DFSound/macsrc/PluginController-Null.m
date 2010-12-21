#import "PluginController-Null.h"
#include "stdafx.h"
#include "externals.h"

#define APP_ID @"com.codeplex.pcsxr.null.SPUPlugin"
#define PrefsKey APP_ID @" Settings"

static PluginController *pluginController;
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


long DoConfiguration()
{
	NSWindow *window;
	
	if (pluginController == nil) {
		pluginController = [[PluginController alloc] initWithWindowNibName:@"NullSpuPluginMain"];
	}
	window = [pluginController window];
	
	/* load values */
	//[pluginController loadValues];
	
	[window center];
	[window makeKeyAndOrderFront:nil];
	
	return 0;
}

void ReadConfig(void)
{
	iVolume = 1;
	iUseTimer = 2;
	iSPUIRQWait = 1;
	iXAPitch = 0;
	iUseReverb = 1;
	iUseInterpolation = 0;

}

@implementation PluginController

- (IBAction)ok:(id)sender
{
	[self close];
}

@end
