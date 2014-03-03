
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
#import "PluginGLView.h"

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

void DlgProc()
{
	RunOnMainThreadSync(^{
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
	});
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
		tmpDict[kWindowSize] = NSStringFromSize(NSMakeSize(800, 600));
		[defaults setObject:tmpDict forKey:PrefsKey];
		[defaults synchronize];
	}
	keyValues = nil;
	
	[defaults registerDefaults:@{PrefsKey: @{kFPSCounter: @NO,
								  kAutoFullScreen: @NO,
								  kFrameSkipping: @NO,
								  kFrameLimit: @YES,
								  kVSync: @NO,
								  kHacksEnable: @NO,
								  @"Dither Mode": @0,
								  kHacks: @0,
								  
								  @"Proportional Resize": @YES,
								  //[NSSize stringWithCString: @"default"], @"Fullscreen Resolution",
								  @"Offscreen Drawing Level": @2,
								  @"Texture Color Depth Level": @0,
								  @"Texture Enhancement Level": @0,
								  @"Texture Filter Level": @0,
								  @"Frame Buffer Level": @0,
								  kWindowSize: NSStringFromSize(NSMakeSize(800, 600)),
								  @"Draw Scanlines": @NO,
								  // nasty:
								  @"Scanline Color": [NSArchiver archivedDataWithRootObject: [NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:0.25]],
								  @"Advanced Blending": @NO,
								  @"Opaque Pass": @NO,
								  @"Blur": @NO,
								  @"Z Mask Clipping": @YES,
								  @"Wireframe Mode": @NO,
								  @"Emulate mjpeg decoder": @YES, // helps remove unsightly vertical line in movies
								  @"Fast mjpeg decoder": @NO,
								  @"GteAccuracy": @YES}}];
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
	iShowFPS = [keyValues[kFPSCounter] boolValue];
    
    if ([keyValues[kFrameLimit] boolValue]){
        bUseFrameLimit = 1;
        iFrameLimit = 2; // required
        fFrameRate = 60; // required (some number, 60 seems ok)
    }
	
	// Dithering is either on or off in OpenGL plug, but hey
	bDrawDither = [keyValues[@"Dither Mode"] intValue];
	
	bChangeWinMode = [keyValues[kAutoFullScreen] boolValue] ? 2 : 1;
	bUseFrameSkip = [keyValues[kFrameSkipping] boolValue];
	
	bUseFixes = [keyValues[kHacksEnable] boolValue];
	dwCfgFixes = [keyValues[kHacks] unsignedIntValue];
    
	
	// we always start out at 800x600 (at least until resizing the window is implemented)
	NSSize winSize = NSSizeFromString(keyValues[kWindowSize]);
	if (bChangeWinMode == 1) {
		iResX = winSize.width;
		iResY = winSize.height;
	} else {
		iResX = 800;
		iResY = 600;
	}
	
    iBlurBuffer = [keyValues[@"Blur"] boolValue]; // not noticeable, but doesn't harm
    iUseScanLines = [keyValues[@"Draw Scanlines"] boolValue]; // works
    NSColor* scanColor = [NSUnarchiver unarchiveObjectWithData:keyValues[@"Scanline Color"]];
	scanColor = [scanColor colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
    iScanlineColor[0] = [scanColor redComponent];
    iScanlineColor[1] = [scanColor greenComponent];
    iScanlineColor[2] = [scanColor blueComponent];
    iScanlineColor[3] = [scanColor alphaComponent];
    
    iScanBlend = 0; // we always draw nice since it costs nothing.
    iUseMask = [keyValues[@"Z Mask Clipping"] boolValue];  // works, clips polygons with primitive "Z" buffer
    bUseLines = [keyValues[@"Wireframe Mode"] boolValue]; // works, aka "Wireframe" mode
    iOffscreenDrawing = [keyValues[@"Offscreen Drawing Level"] intValue]; // draw offscreen for texture building?
    if (iOffscreenDrawing > 4) iOffscreenDrawing = 4;
    if (iOffscreenDrawing < 0) iOffscreenDrawing = 0;
    
	
	// texture quality, whatever that means (doesn't hurt), more like "texture handling" or "texture performance"
    iFrameTexType = [keyValues[@"Frame Buffer Level"] intValue];
    if (iFrameTexType > 3) iFrameTexType = 3;
    if (iFrameTexType < 0) iFrameTexType = 0;
    
    iTexQuality = [keyValues[@"Texture Color Depth Level"] intValue];
    if (iTexQuality > 4) iTexQuality = 4;
    if (iTexQuality < 0) iTexQuality = 0;
	
	// MAG_FILTER = LINEAR, etc.
    iFilterType = [keyValues[@"Texture Filter Level"] intValue];
    if (iFilterType > 2) iFilterType = 2;
    if (iFilterType < 0) iFilterType = 0;
    
	// stretches textures (more detail). You'd think it would look great, but it's not massively better. NEEDS iFilterType to be of any use.
    iHiResTextures = [keyValues[@"Texture Enhancement Level"] intValue];
    if (iHiResTextures > 2) iHiResTextures = 2;
    if (iHiResTextures < 0) iHiResTextures = 0;
    
    // well actually, the "SaI" mode is best, but is #1, so swap qualities:
    if (iHiResTextures != 0)
        iHiResTextures = 3 - iHiResTextures;
    
    if (iHiResTextures && !iFilterType)
        iFilterType = 1; // needed to see any real effect
    
    bUseFastMdec = [keyValues[@"Emulate mjpeg decoder"] boolValue];
    bUse15bitMdec = [keyValues[@"Fast mjpeg decoder"] boolValue];
    bGteAccuracy = [keyValues[@"GteAccuracy"] boolValue];
	
	if (iShowFPS)
		ulKeybits |= KEY_SHOWFPS;
	else
		ulKeybits &=~ KEY_SHOWFPS;
	
	// additional checks
	if(!iColDepth)
		iColDepth=32;
	if(bUseFixes) {
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
@synthesize autoFullScreen;
@synthesize ditherMode;
@synthesize fpsCounter;
@synthesize frameSkipping;
@synthesize vSync;
@synthesize proportionalResize;
@synthesize fullscreenSize;
@synthesize windowWidth;
@synthesize windowHeighth;
@synthesize offscreenDrawing;
@synthesize texColorDepth;
@synthesize texFiltering;
@synthesize texEnhancment;
@synthesize frameBufferEffects;
@synthesize drawScanlines;
@synthesize advancedBlending;
@synthesize opaquePass;
@synthesize zMaskClipping;
@synthesize wireframeOnly;
@synthesize blurEffect;
@synthesize mjpegDecoder;
@synthesize mjpegDecoder15bit;
@synthesize gteAccuracy;
@synthesize scanlineColorWell;
@synthesize hacksMatrix;
@synthesize hackEnable;
@synthesize hacksWindow;

@synthesize keyValues;

- (IBAction)cancel:(id)sender
{
	[self close];
}

- (IBAction)ok:(id)sender
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	NSMutableDictionary *writeDic = [keyValues mutableCopy];
	writeDic[kFPSCounter] = ([fpsCounter integerValue] ? @YES : @NO);
	writeDic[@"Scanline Color"] = [NSArchiver archivedDataWithRootObject:[scanlineColorWell color]];
	writeDic[kFrameSkipping] = ([frameSkipping integerValue] ? @YES : @NO);
	writeDic[kAutoFullScreen] = ([autoFullScreen integerValue] ? @YES : @NO);
	//[writeDic setObject:([frameLimit integerValue] ? @YES : @NO) forKey:kFrameLimit];
	writeDic[@"Proportional Resize"] = ([proportionalResize integerValue] ? @YES : @NO);
	writeDic[@"Dither Mode"] = @([ditherMode indexOfSelectedItem]);
	writeDic[@"Offscreen Drawing Level"] = @([offscreenDrawing indexOfSelectedItem]);
	writeDic[@"Texture Color Depth Level"] = @([texColorDepth indexOfSelectedItem]);
	writeDic[@"Texture Enhancement Level"] = @([texEnhancment integerValue]);
	writeDic[@"Texture Filter Level"] = @([texFiltering integerValue]);
	writeDic[@"Frame Buffer Level"] = @([frameBufferEffects indexOfSelectedItem]);
	writeDic[@"Draw Scanlines"] = ([drawScanlines integerValue] ? @YES : @NO);
	writeDic[@"Advanced Blending"] = ([advancedBlending integerValue] ? @YES : @NO);
	writeDic[@"Opaque Pass"] = ([opaquePass integerValue] ? @YES : @NO);
	writeDic[@"Blur"] = ([blurEffect integerValue] ? @YES : @NO);
	writeDic[@"Z Mask Clipping"] = ([zMaskClipping integerValue] ? @YES : @NO);
	writeDic[@"Wireframe Mode"] = ([wireframeOnly integerValue] ? @YES : @NO);
	writeDic[@"Emulate mjpeg decoder"] = ([mjpegDecoder integerValue] ? @YES : @NO);
	writeDic[@"Fast mjpeg decoder"] = ([mjpegDecoder15bit integerValue] ? @YES : @NO);
	writeDic[@"GteAccuracy"] = ([gteAccuracy integerValue] ? @YES : @NO);
	writeDic[kVSync] = ([vSync integerValue] ? @YES : @NO);
	writeDic[kWindowSize] = NSStringFromSize(NSMakeSize([windowWidth integerValue], [windowHeighth integerValue]));
	
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
	unsigned int hackValues = [(self.keyValues)[kHacks] unsignedIntValue];
	[hackEnable setIntegerValue:[(self.keyValues)[kHacksEnable] boolValue]];

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
	
	[autoFullScreen setIntegerValue:[keyValues[kAutoFullScreen] boolValue]];
	[ditherMode selectItemAtIndex:[keyValues[@"Dither Mode"] integerValue]];
	[fpsCounter setIntegerValue:[keyValues[kFPSCounter] boolValue]];
	[scanlineColorWell setColor:[NSUnarchiver unarchiveObjectWithData: keyValues[@"Scanline Color"]]];
	[frameSkipping setIntegerValue:[keyValues[kFrameSkipping] boolValue]];
	[advancedBlending setIntegerValue:[keyValues[@"Advanced Blending"] boolValue]];
	[texFiltering setIntegerValue:[keyValues[@"Texture Filter Level"] integerValue]];
	[texEnhancment setIntegerValue:[keyValues[@"Texture Enhancement Level"] integerValue]];
	[zMaskClipping setIntegerValue:[keyValues[@"Z Mask Clipping"] integerValue]];
	[mjpegDecoder setIntegerValue:[keyValues[@"Emulate mjpeg decoder"] boolValue]];
	[mjpegDecoder15bit setIntegerValue:[keyValues[@"Fast mjpeg decoder"] boolValue]];
	[drawScanlines setIntegerValue:[keyValues[@"Draw Scanlines"] boolValue]];
	[offscreenDrawing selectItemAtIndex:[keyValues[@"Offscreen Drawing Level"] integerValue]];
	[advancedBlending setIntegerValue:[keyValues[@"Advanced Blending"] boolValue]];
	[opaquePass setIntegerValue:[keyValues[@"Opaque Pass"] boolValue]];
	[wireframeOnly setIntegerValue:[keyValues[@"Wireframe Mode"] boolValue]];
	[blurEffect setIntegerValue:[keyValues[@"Blur"] boolValue]];
	[texColorDepth selectItemAtIndex:[keyValues[@"Texture Color Depth Level"] integerValue]];
	[gteAccuracy setIntegerValue:[keyValues[@"GteAccuracy"] boolValue]];
	[scanlineColorWell setEnabled:[keyValues[@"Draw Scanlines"] boolValue]];
	[frameBufferEffects selectItemAtIndex:[keyValues[@"Frame Buffer Level"] integerValue]];
	[vSync setIntegerValue:[keyValues[kVSync] boolValue]];
	[proportionalResize setIntegerValue:[keyValues[@"Proportional Resize"] boolValue]];
	NSSize winSize = NSSizeFromString(keyValues[kWindowSize]);
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
		NSMutableDictionary *writeDic = self.keyValues;
		writeDic[kHacks] = @(hackValues);
		writeDic[kHacksEnable] = ([hackEnable integerValue] ? @YES : @NO);
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

@end

#import "OSXPlugLocalization.h"
PLUGLOCIMP([PluginConfigController class])
