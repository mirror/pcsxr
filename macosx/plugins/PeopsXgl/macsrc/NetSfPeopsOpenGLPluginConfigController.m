
/* All the various stuff needed for configuration is done here, 
   including reading the Config file and displaying a dialog box
   AboutDlgProc() is a plug-in function called from the PCSXR app, as is
   DlgProc()
*/

#import "NetSfPeopsOpenGLPluginConfigController.h"
#include "gpu.h"
#include "cfg.h"
#include "menu.h"
#include <OpenGL/gl.h> // bah, "externals.h" thinks include files are for wimps; OpenGL header, in fact, is needed
#include "externals.h"
#import "ARCBridge.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
//If running under Mac OS X, use the Localizable.strings file instead.
#elif defined(_MACOSX)
#ifdef PCSXRCORE
extern char* Pcsxr_locale_text(char* toloc);
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
extern char* PLUGLOC(char* toloc);
#define _(String) PLUGLOC(String)
#define N_(String) String
#endif
#endif
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

#define APP_ID @"net.sf.peops.GpuOpenGLPlugin"
#define PrefsKey APP_ID @" Settings"

static NetSfPeopsOpenGLPluginConfigController *windowController = nil;
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
		credits = AUTORELEASEOBJ([[NSAttributedString alloc] initWithPath: path documentAttributes:NULL]);
	} else {
		credits = AUTORELEASEOBJ([[NSAttributedString alloc] initWithString:@""]);
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

void DlgProc()
{
	NSWindow *window;

    PrepFactoryDefaultPreferences(); // Must do here to avoid a "when does such-and-such bind" issue
	
	if (windowController == nil) {
		windowController = [[PluginConfigController alloc] initWithWindowNibName:@"NetSfPeopsOpenGLConfig"];
	}
	window = [windowController window];
	
	/* load values */
	[windowController loadValues];
	
	[window center];
	[window makeKeyAndOrderFront:nil];
}

void PrepFactoryDefaultPreferences(void)
{
    // THE place to find the names of settings.
    // If it's not here, you can't set it.

    // create or read a sub-dictionary beneath the main PCSXR app prefs.
    // dictionary is named "net.sf.GpuOpenGLPlugin Settings"
    // and contains all our key/values
    // the prefs .plist will store this dictionary ("net.sf...") as an object
    
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
								 [NSDictionary dictionaryWithObjectsAndKeys:
								  @NO, @"FPS Counter",
								  @NO, @"Auto Full Screen",
								  @NO, @"Frame Skipping",
								  @YES, @"Frame Limit",
								  @NO, @"VSync",
								  @NO, @"Enable Hacks",
								  @0, @"Dither Mode",
								  @((unsigned int)0), @"Hacks",
								  
								  @YES, @"Proportional Resize",
								  //[NSSize stringWithCString: @"default"], @"Fullscreen Resolution",
								  @2, @"Offscreen Drawing Level",
								  @0, @"Texture Color Depth Level",
								  @0, @"Texture Enhancement Level",
								  @0, @"Texture Filter Level",
								  @0, @"Frame Buffer Level",
								  @NO, @"Draw Scanlines",
								  // nasty:
								  [NSArchiver archivedDataWithRootObject: [NSColor colorWithCalibratedRed:0  green:0 blue:0 alpha:0.25]], @"Scanline Color",
								  @NO, @"Advanced Blending",
								  @NO, @"Opaque Pass",
								  @NO, @"Blur",
								  @YES, @"Z Mask Clipping",
								  @NO, @"Wireframe Mode",
								  @YES, @"Emulate mjpeg decoder", // helps remove unsightly vertical line in movies
								  @NO, @"Fast mjpeg decoder",
								  nil],  PrefsKey, nil]];
}

void ReadConfig(void)
{
    // set up PCSXR GPU plug's global variables according to user preferences.
    // this is called from the PCSXR GPU plugin thread via GPUOpen.
    
    // has nothing to do with the Configuration dialog box, btw., other than the
    // fact that the config dialog writes to user prefs. This only reads, which
    // is important because PCSXR will change its globals on the fly
    // and saving those new ad hoc changes is Bad for the user.
    
    PrepFactoryDefaultPreferences(); // in case user deletes, or on new startup
	
	//NOTE this is NOT the "keyValues" member of the controller. Just sayin.
    NSDictionary* keyValues = [[NSUserDefaults standardUserDefaults] dictionaryForKey:PrefsKey];
	
	
    // bind all prefs settings to their PCSXR counterparts
    // with a little finagling to make it work as expected
	iShowFPS = [[keyValues objectForKey:@"FPS Counter"] boolValue];
    
    if ([[keyValues objectForKey:@"Frame Limit"] boolValue]){
        bUseFrameLimit = 1;
        iFrameLimit = 2; // required
        fFrameRate = 60; // required (some number, 60 seems ok)
    }
	
	// Dithering is either on or off in OpenGL plug, but hey
	bDrawDither = [[keyValues objectForKey:@"Dither Mode"] intValue];
	
	bChangeWinMode = [[keyValues objectForKey:@"Auto Full Screen"] boolValue] ? 2 : 1;
	bUseFrameSkip = [[keyValues objectForKey:@"Frame Skipping"] boolValue];
	
	bUseFixes = [[keyValues objectForKey:@"Enable Hacks"] boolValue];
	dwCfgFixes = [[keyValues objectForKey:@"Hacks"] unsignedIntValue];
    
	
	// we always start out at 800x600 (at least until resizing the window is implemented)
	iResX = 800;
	iResY = 600;
	
    iBlurBuffer = [[keyValues objectForKey:@"Blur"] boolValue]; // not noticeable, but doesn't harm
    iUseScanLines = [[keyValues objectForKey:@"Draw Scanlines"] boolValue]; // works
    NSColor* scanColor = [NSUnarchiver unarchiveObjectWithData: [keyValues objectForKey:@"Scanline Color"]];
    iScanlineColor[0] = [scanColor redComponent];
    iScanlineColor[1] = [scanColor greenComponent];
    iScanlineColor[2] = [scanColor blueComponent];
    iScanlineColor[3] = [scanColor alphaComponent];
    
    iScanBlend = 0; // we always draw nice since it costs nothing.
    iUseMask = [[keyValues objectForKey:@"Z Mask Clipping"] boolValue];  // works, clips polygons with primitive "Z" buffer
    bUseLines = [[keyValues objectForKey:@"Wireframe Mode"] boolValue]; // works, aka "Wireframe" mode
    iOffscreenDrawing = [[keyValues objectForKey:@"Offscreen Drawing Level"] intValue]; // draw offscreen for texture building?
    if (iOffscreenDrawing > 4) iOffscreenDrawing = 4;
    if (iOffscreenDrawing < 0) iOffscreenDrawing = 0;
    
	
	// texture quality, whatever that means (doesn't hurt), more like "texture handling" or "texture performance"
    iFrameTexType = [[keyValues objectForKey:@"Frame Buffer Level"] intValue];
    if (iFrameTexType > 3) iFrameTexType = 3;
    if (iFrameTexType < 0) iFrameTexType = 0;
    
    iTexQuality = [[keyValues objectForKey:@"Texture Color Depth Level"] intValue];
    if (iTexQuality > 4) iTexQuality = 4;
    if (iTexQuality < 0) iTexQuality = 0;
	
	// MAG_FILTER = LINEAR, etc.
    iFilterType = [[keyValues objectForKey:@"Texture Filter Level"] intValue];
    if (iFilterType > 2) iFilterType = 2;
    if (iFilterType < 0) iFilterType = 0;
    
	// stretches textures (more detail). You'd think it would look great, but it's not massively better. NEEDS iFilterType to be of any use.
    iHiResTextures = [[keyValues objectForKey:@"Texture Enhancement Level"] intValue];
    if (iHiResTextures > 2) iHiResTextures = 2;
    if (iHiResTextures < 0) iHiResTextures = 0;
    
    // well actually, the "SaI" mode is best, but is #1, so swap qualities:
    if (iHiResTextures != 0)
        iHiResTextures = 3 - iHiResTextures;
    
    if (iHiResTextures && !iFilterType)
        iFilterType = 1; // needed to see any real effect
    
    bUseFastMdec = [[keyValues objectForKey:@"Emulate mjpeg decoder"] boolValue];
    bUse15bitMdec = [[keyValues objectForKey:@"Fast mjpeg decoder"] boolValue];
    bGteAccuracy = [[keyValues objectForKey:@"GteAccuracy"] boolValue];
	
	if (iShowFPS)
		ulKeybits |= KEY_SHOWFPS;
	else
		ulKeybits &=~ KEY_SHOWFPS;
	
	// additional checks
	if(!iColDepth)       iColDepth=32;
#if 0 // was in SoftGPU, not in OpenGL
	if(iUseFixes)        dwActFixes=dwCfgFixes;
	else						 dwActFixes=0;
#else
    dwActFixes = 0; // for now... TODO
#endif
	
	
	SetFixes();
	
	// need this or you'll be playing at light speed:
	if(iFrameLimit == 2) SetAutoFrameCap();
	bSkipNextFrame = FALSE;
	
	szDispBuf[0] = 0;
	BuildDispMenu(0);
}

@implementation PluginConfigController

- (IBAction)cancel:(id)sender
{
    //TODO: the IB bindings have already changed everything to what the
    // user clicked on.
    // Therefore, "backup" settings should be stored before interaction, 
    // then restored here.
    // IMO, 'cancel' is not needed since the config dialog doesn't launch
    // an action when "ok" is clicked.
	[self close];
}

- (IBAction)ok:(id)sender
{

// most everything is taken care of through bindings in Interface Builder.
// note that the IB interface uses NSObjectController (a dict controller) as a proxy to
// NSUserDefaultsController because NSUserDefaultsController can't
// handle dictionaries. Yup, that's what I said. </snark>.

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
   [defaults synchronize];

// treat hacks specially:

	unsigned int hackValues = 0;
	NSArray *views = [hacksView subviews];

	for (NSView *control in views) {
		if ([control isKindOfClass:[NSButton class]]) {
			hackValues |= [(NSControl *)control intValue] << ([control tag] - 1);
		}
	}
	
	keyValues = [NSMutableDictionary dictionaryWithDictionary: [[NSUserDefaults standardUserDefaults] dictionaryForKey:PrefsKey]];

	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];
	[writeDic setObject:@((unsigned int)hackValues) forKey:@"Hacks"];
	
	// write the preferences with Hacks adjustments
	[defaults setObject:writeDic forKey:PrefsKey];
	[defaults synchronize];
	
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
    // enable the "hacks" checkboxes 
	BOOL enable = [sender intValue] ? YES : NO;
	NSArray *views = [hacksView subviews];

	for (NSView *control in views) {
		if ([control isKindOfClass:[NSButton class]]) {
			[(NSControl *)control setEnabled:enable];
		}
	}
}

- (void)loadValues
{
// set up the window with the values in the .plist

// all preferences are bound in Interface Builder.
// Though the "hacks settings" is controlled here because it disables/enables the list
// and uses a bit mask

// Note that in the .nib, an NSObjectController (aka "dict controller")
// is used as a proxy to NSUserDefaults
// because NSUserDefaults is slightly retarded about nested dictionaries
// OK, "Completely" retarded.

    PrepFactoryDefaultPreferences(); // in case we're starting anew

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	/* load from preferences */
	keyValues = [NSMutableDictionary dictionaryWithDictionary: [defaults dictionaryForKey:PrefsKey]];

	unsigned int hackValues = [[keyValues objectForKey:@"Hacks"] unsignedIntValue];

    // build refs to hacks checkboxes
	NSArray *views = [hacksView subviews];
	for (NSView *control in views) {
		if ([control isKindOfClass:[NSButton class]]) {
			[(NSControl *)control setIntValue:(hackValues >> ([control tag] - 1)) & 1];
		}
	}
	
	[self hackToggle:hackEnable];
}

- (void)awakeFromNib
{
	hacksView = [[hacksView subviews] objectAtIndex:0];
    [[NSColorPanel sharedColorPanel] setShowsAlpha:YES]; // eliminate dumb behavior!
}

@end

char* PLUGLOC(char *toloc)
{
	NSBundle *mainBundle = [NSBundle bundleForClass:[PluginConfigController class]];
	NSString *origString = nil, *transString = nil;
	origString = [NSString stringWithCString:toloc encoding:NSUTF8StringEncoding];
	transString = [mainBundle localizedStringForKey:origString value:nil table:nil];
	return (char*)[transString cStringUsingEncoding:NSUTF8StringEncoding];
}
