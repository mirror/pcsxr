
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

void AboutDlgProc()
{
	// Get parent application instance
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
	
	NSDictionary *infoPaneDict =
	[[NSDictionary alloc] initWithObjectsAndKeys:
	 [bundle objectForInfoDictionaryKey:@"CFBundleName"], @"ApplicationName",
	 icon, @"ApplicationIcon",
	 [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"], @"ApplicationVersion",
	 [bundle objectForInfoDictionaryKey:@"CFBundleVersion"], @"Version",
	 [bundle objectForInfoDictionaryKey:@"NSHumanReadableCopyright"], @"Copyright",
	 credits, @"Credits",
	 nil];
	dispatch_async(dispatch_get_main_queue(), ^{
		[NSApp orderFrontStandardAboutPanelWithOptions:infoPaneDict];
	});
	RELEASEOBJ(infoPaneDict);
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

#define kFPSCounter @"FPS Counter"
#define kHacks @"Hacks"
#define kAutoFullScreen @"Auto Full Screen"
#define kFrameSkipping @"Frame Skipping"
#define kFrameLimit @"Frame Limit"
#define kVSync @"VSync"
#define kHacksEnable @"Enable Hacks"
#define kWindowSize @"Window Size"

void PrepFactoryDefaultPreferences(void)
{
    // THE place to find the names of settings.
    // If it's not here, you can't set it.

    // create or read a sub-dictionary beneath the main PCSXR app prefs.
    // dictionary is named "net.sf.GpuOpenGLPlugin Settings"
    // and contains all our key/values
    // the prefs .plist will store this dictionary ("net.sf...") as an object
    
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	NSDictionary* keyValues = [defaults dictionaryForKey:PrefsKey];
	BOOL windowSizeNeedsReset = NO;
	if (keyValues) {
		NSSize size = NSSizeFromString([keyValues objectForKey:kWindowSize]);
		if (![keyValues objectForKey:kWindowSize]) {
			windowSizeNeedsReset = YES;
		} else if ([[keyValues objectForKey:kWindowSize] isKindOfClass:[NSNumber class]]) {
			windowSizeNeedsReset = YES;
		} else if (size.height == 0 || size.width == 0) {
			windowSizeNeedsReset = YES;
		}
	}
	if (windowSizeNeedsReset) {
		NSMutableDictionary *tmpDict = [[NSMutableDictionary alloc] initWithDictionary:keyValues];
		[tmpDict setObject:NSStringFromSize(NSMakeSize(800, 600)) forKey:kWindowSize];
		[defaults setObject:tmpDict forKey:PrefsKey];
		[defaults synchronize];
		RELEASEOBJ(tmpDict);
	}
	keyValues = nil;
	
	[defaults registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
								 [NSDictionary dictionaryWithObjectsAndKeys:
								  @NO, kFPSCounter,
								  @NO, kAutoFullScreen,
								  @NO, kFrameSkipping,
								  @YES, kFrameLimit,
								  @NO, kVSync,
								  @NO, kHacksEnable,
								  @0, @"Dither Mode",
								  @0, kHacks,
								  
								  @YES, @"Proportional Resize",
								  //[NSSize stringWithCString: @"default"], @"Fullscreen Resolution",
								  @2, @"Offscreen Drawing Level",
								  @0, @"Texture Color Depth Level",
								  @0, @"Texture Enhancement Level",
								  @0, @"Texture Filter Level",
								  @0, @"Frame Buffer Level",
								  NSStringFromSize(NSMakeSize(800, 600)), kWindowSize,
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
								  @YES, @"GteAccuracy",
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
	
    NSDictionary* keyValues = [[NSUserDefaults standardUserDefaults] dictionaryForKey:PrefsKey];
		
    // bind all prefs settings to their PCSXR counterparts
    // with a little finagling to make it work as expected
	iShowFPS = [[keyValues objectForKey:kFPSCounter] boolValue];
    
    if ([[keyValues objectForKey:kFrameLimit] boolValue]){
        bUseFrameLimit = 1;
        iFrameLimit = 2; // required
        fFrameRate = 60; // required (some number, 60 seems ok)
    }
	
	// Dithering is either on or off in OpenGL plug, but hey
	bDrawDither = [[keyValues objectForKey:@"Dither Mode"] intValue];
	
	bChangeWinMode = [[keyValues objectForKey:kAutoFullScreen] boolValue] ? 2 : 1;
	bUseFrameSkip = [[keyValues objectForKey:kFrameSkipping] boolValue];
	
	bUseFixes = [[keyValues objectForKey:kHacksEnable] boolValue];
	dwCfgFixes = [[keyValues objectForKey:kHacks] unsignedIntValue];
    
	
	// we always start out at 800x600 (at least until resizing the window is implemented)
	NSSize winSize = NSSizeFromString([keyValues objectForKey:kWindowSize]);
	if (bChangeWinMode == 1) {
		iResX = winSize.width;
		iResY = winSize.height;
	} else {
		iResX = 800;
		iResY = 600;
	}
	
    iBlurBuffer = [[keyValues objectForKey:@"Blur"] boolValue]; // not noticeable, but doesn't harm
    iUseScanLines = [[keyValues objectForKey:@"Draw Scanlines"] boolValue]; // works
    NSColor* scanColor = [NSUnarchiver unarchiveObjectWithData:[keyValues objectForKey:@"Scanline Color"]];
	scanColor = [scanColor colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
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
	if(!iColDepth)
		iColDepth=32;
	if(bUseFixes)
	{
		dwActFixes = dwCfgFixes;
	} else {
		dwActFixes = 0;
	}
	
	SetFixes();
	
	// need this or you'll be playing at light speed:
	if(iFrameLimit == 2) SetAutoFrameCap();
	bSkipNextFrame = FALSE;
	
	szDispBuf[0] = 0;
	BuildDispMenu(0);
}

@implementation NetSfPeopsOpenGLPluginConfigController

@synthesize keyValues;

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

#if 0
	unsigned int hackValues = 0;
	NSArray *views = [hacksMatrix cells];

	for (NSControl *control in views) {
		hackValues |= [control intValue] << ([control tag] - 1);
	}
#endif
	
	//self.keyValues = [NSMutableDictionary dictionaryWithDictionary: [[NSUserDefaults standardUserDefaults] dictionaryForKey:PrefsKey]];

	NSMutableDictionary *writeDic = [NSMutableDictionary dictionaryWithDictionary:keyValues];
	//[writeDic setObject:@(hackValues) forKey:kHacks];
	[writeDic setObject:([hackEnable integerValue] ? @YES : @NO) forKey:kHacksEnable];
	[writeDic setObject:([fpsCounter integerValue] ? @YES : @NO) forKey:kFPSCounter];
	[writeDic setObject:[NSArchiver archivedDataWithRootObject:[scanlineColorWell color]] forKey:@"Scanline Color"];
	[writeDic setObject:([frameSkipping integerValue] ? @YES : @NO) forKey:kFrameSkipping];
	[writeDic setObject:([autoFullScreen integerValue] ? @YES : @NO) forKey:kAutoFullScreen];
	//[writeDic setObject:([frameLimit integerValue] ? @YES : @NO) forKey:kFrameLimit];
	[writeDic setObject:([proportionalResize integerValue] ? @YES : @NO) forKey:@"Proportional Resize"];
	[writeDic setObject:@([ditherMode indexOfItem:[ditherMode selectedItem]]) forKey:@"Dither Mode"];
	[writeDic setObject:@([offscreenDrawing indexOfItem:[offscreenDrawing selectedItem]]) forKey:@"Offscreen Drawing Level"];
	[writeDic setObject:@([texColorDepth indexOfItem:[texColorDepth selectedItem]]) forKey:@"Texture Color Depth Level"];
	[writeDic setObject:@([texEnhancment integerValue]) forKey:@"Texture Enhancement Level"];
	[writeDic setObject:@([texFiltering integerValue]) forKey:@"Texture Filter Level"];
	[writeDic setObject:@([frameBufferEffects indexOfItem:[frameBufferEffects selectedItem]]) forKey:@"Frame Buffer Level"];
	[writeDic setObject:([drawScanlines integerValue] ? @YES : @NO) forKey:@"Draw Scanlines"];
	[writeDic setObject:([advancedBlending integerValue] ? @YES : @NO) forKey:@"Advanced Blending"];
	[writeDic setObject:([opaquePass integerValue] ? @YES : @NO) forKey:@"Opaque Pass"];
	[writeDic setObject:([blurEffect integerValue] ? @YES : @NO) forKey:@"Blur"];
	[writeDic setObject:([zMaskClipping integerValue] ? @YES : @NO) forKey:@"Z Mask Clipping"];
	[writeDic setObject:([wireframeOnly integerValue] ? @YES : @NO) forKey:@"Wireframe Mode"];
	[writeDic setObject:([mjpegDecoder integerValue] ? @YES : @NO) forKey:@"Emulate mjpeg decoder"];
	[writeDic setObject:([mjpegDecoder15bit integerValue] ? @YES : @NO) forKey:@"Fast mjpeg decoder"];
	[writeDic setObject:([gteAccuracy integerValue] ? @YES : @NO) forKey:@"GteAccuracy"];
	[writeDic setObject:([vSync integerValue] ? @YES : @NO) forKey:kVSync];
	[writeDic setObject:NSStringFromSize(NSMakeSize([windowWidth integerValue], [windowHeighth integerValue])) forKey:kWindowSize];
	
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
	NSArray *views = [hacksMatrix cells];

	for (NSControl *control in views) {
		[control setEnabled:enable];
	}
}

- (void)loadHacksValues
{
	unsigned int hackValues = [[self.keyValues objectForKey:kHacks] unsignedIntValue];
	[hackEnable setIntegerValue:[[self.keyValues objectForKey:kHacksEnable] boolValue]];

    // build refs to hacks checkboxes
	for (NSControl *control in [hacksMatrix cells]) {
		[control setIntValue:(hackValues >> ([control tag] - 1)) & 1];
	}
	
	[self hackToggle:hackEnable];
}

- (void)loadValues
{
// set up the window with the values in the .plist

    PrepFactoryDefaultPreferences(); // in case we're starting anew

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	/* load from preferences */
	self.keyValues = [NSMutableDictionary dictionaryWithDictionary: [defaults dictionaryForKey:PrefsKey]];

	[self loadHacksValues];
	
	[autoFullScreen setIntegerValue:[[keyValues objectForKey:kAutoFullScreen] boolValue]];
	[ditherMode selectItemAtIndex:[[keyValues objectForKey:@"Dither Mode"] integerValue]];
	[fpsCounter setIntegerValue:[[keyValues objectForKey:kFPSCounter] boolValue]];
	[scanlineColorWell setColor:[NSUnarchiver unarchiveObjectWithData: [keyValues objectForKey:@"Scanline Color"]]];
	[frameSkipping setIntegerValue:[[keyValues objectForKey:kFrameSkipping] boolValue]];
	[advancedBlending setIntegerValue:[[keyValues objectForKey:@"Advanced Blending"] boolValue]];
	[texFiltering setIntegerValue:[[keyValues objectForKey:@"Texture Filter Level"] integerValue]];
	[texEnhancment setIntegerValue:[[keyValues objectForKey:@"Texture Enhancement Level"] integerValue]];
	[zMaskClipping setIntegerValue:[[keyValues objectForKey:@"Z Mask Clipping"] integerValue]];
	[mjpegDecoder setIntegerValue:[[keyValues objectForKey:@"Emulate mjpeg decoder"] boolValue]];
	[mjpegDecoder15bit setIntegerValue:[[keyValues objectForKey:@"Fast mjpeg decoder"] boolValue]];
	[drawScanlines setIntegerValue:[[keyValues objectForKey:@"Draw Scanlines"] boolValue]];
	[offscreenDrawing selectItemAtIndex:[[keyValues objectForKey:@"Offscreen Drawing Level"] integerValue]];
	[advancedBlending setIntegerValue:[[keyValues objectForKey:@"Advanced Blending"] boolValue]];
	[opaquePass setIntegerValue:[[keyValues objectForKey:@"Opaque Pass"] boolValue]];
	[wireframeOnly setIntegerValue:[[keyValues objectForKey:@"Wireframe Mode"] boolValue]];
	[blurEffect setIntegerValue:[[keyValues objectForKey:@"Blur"] boolValue]];
	[texColorDepth selectItemAtIndex:[[keyValues objectForKey:@"Texture Color Depth Level"] integerValue]];
	[gteAccuracy setIntegerValue:[[keyValues objectForKey:@"GteAccuracy"] boolValue]];
	[scanlineColorWell setEnabled:[[keyValues objectForKey:@"Draw Scanlines"] boolValue]];
	[frameBufferEffects selectItemAtIndex:[[keyValues objectForKey:@"Frame Buffer Level"] integerValue]];
	[vSync setIntegerValue:[[keyValues objectForKey:kVSync] boolValue]];
	[proportionalResize setIntegerValue:[[keyValues objectForKey:@"Proportional Resize"] boolValue]];
	NSSize winSize = NSSizeFromString([keyValues objectForKey:kWindowSize]);
	[windowWidth setIntegerValue:winSize.width];
	[windowHeighth setIntegerValue:winSize.height];
}

- (void)awakeFromNib
{
    [[NSColorPanel sharedColorPanel] setShowsAlpha:YES]; // eliminate dumb behavior!
}

- (void)hacksSheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	NSParameterAssert(sheet == hacksWindow);
	if (returnCode == NSCancelButton) {
		//Reset hack preferences.
		[self loadHacksValues];
	} else {
		unsigned int hackValues = 0;		
		for (NSControl *control in [hacksMatrix cells]) {
			hackValues |= [control intValue] << ([control tag] - 1);
		}
		[self.keyValues setObject:@(hackValues) forKey:kHacks];
	}
	[sheet orderOut:nil];
}

- (IBAction)closeHacks:(id)sender
{
	if ([sender tag] == 1) {
		[NSApp endSheet:hacksWindow returnCode:NSOKButton];
	} else {
		[NSApp endSheet:hacksWindow returnCode:NSCancelButton];
	}
}

- (IBAction)showHacks:(id)sender
{
	[NSApp beginSheet:hacksWindow modalForWindow:[self window] modalDelegate:self
	   didEndSelector:@selector(hacksSheetDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (IBAction)toggleCheck:(id)sender
{
	if([sender tag] == 1) {
		[scanlineColorWell setEnabled: [sender intValue] ? YES : NO];
	}
}

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.keyValues = nil;
	
	[super dealloc];
}
#endif

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginConfigController class]);
