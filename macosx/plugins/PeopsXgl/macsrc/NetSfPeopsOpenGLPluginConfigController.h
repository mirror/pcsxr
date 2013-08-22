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
    
    IBOutlet NSCell *autoFullScreen;
    IBOutlet NSPopUpButton *ditherMode;
    IBOutlet NSCell *fpsCounter;
    IBOutlet NSCell *frameSkipping;
    IBOutlet NSCell *vSync;
    
    IBOutlet NSControl *proportionalResize;
    IBOutlet NSPopUpButton *fullscreenSize;
	IBOutlet NSFormCell *windowWidth;
	IBOutlet NSFormCell *windowHeighth;
    IBOutlet NSPopUpButton *offscreenDrawing;
    
    IBOutlet NSPopUpButton *texColorDepth;
    IBOutlet NSSlider *texFiltering;
    IBOutlet NSSlider *texEnhancment;
    
    IBOutlet NSPopUpButton *frameBufferEffects;

    IBOutlet NSCell *drawScanlines;
    IBOutlet NSCell *advancedBlending;
    IBOutlet NSCell *opaquePass;
    IBOutlet NSCell *zMaskClipping;
    IBOutlet NSCell *wireframeOnly;
    IBOutlet NSCell *blurEffect;
    IBOutlet NSCell *mjpegDecoder;
    IBOutlet NSCell *mjpegDecoder15bit;
	IBOutlet NSCell *gteAccuracy;
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

