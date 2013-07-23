/* NetSfPeopsSoftGPUPluginConfigController */
/* All the various stuff needed for configuration is done here, including reading 
   the preferences and displaying a dialog box for the user
*/

#define PluginConfigController NetSfPeopsOpenGLPluginConfigController

#import <Cocoa/Cocoa.h>

@interface NetSfPeopsOpenGLPluginConfigController : NSWindowController
{
    // buncha controls.
    // most aren't worthy as IBOutlets since the IB interface
    // uses bindings to magically set user defaults.
    // But you can look at their grandness if you like:
    
    IBOutlet NSControl *autoFullScreen;
    IBOutlet NSPopUpButton *ditherMode;
    IBOutlet NSControl *fpsCounter;
    IBOutlet NSControl *frameSkipping;
    IBOutlet NSControl *vSync;
    
    IBOutlet NSControl *proportionalResize;
	IBOutlet NSPopUpButton *windowSize;
    IBOutlet NSPopUpButton *fullscreenSize;
    IBOutlet NSPopUpButton *offscreenDrawing;
    
    IBOutlet NSPopUpButton *texColorDepth;
    IBOutlet NSSlider *texFiltering;
    IBOutlet NSSlider *texEnhancment;
    
    IBOutlet NSPopUpButton *frameBufferEffects;

    IBOutlet NSControl *drawScanlines;
    IBOutlet NSControl *advancedBlending;
    IBOutlet NSControl *opaquePass;
    IBOutlet NSControl *zMaskClipping;
    IBOutlet NSControl *wireframeOnly;
    IBOutlet NSControl *blurEffect;
    IBOutlet NSControl *mjpegDecoder;
    IBOutlet NSControl *mjpegDecoder15bit;
	IBOutlet NSControl *gteAccuracy;
	IBOutlet NSColorWell *scanlineColorWell;
	
	IBOutlet NSMatrix *hacksMatrix;
	IBOutlet NSControl *hackEnable;
    IBOutlet NSWindow *hacksWindow;
    
	NSMutableDictionary *keyValues;
}
@property (readwrite, retain) NSMutableDictionary *keyValues;

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;
- (IBAction)toggleCheck:(id)sender;

- (IBAction)hackToggle:(id)sender;
- (IBAction)showHacks:(id)sender;
- (IBAction)closeHacks:(id)sender;

- (void)loadValues;

@end

void PrepFactoryDefaultPreferences(void);

