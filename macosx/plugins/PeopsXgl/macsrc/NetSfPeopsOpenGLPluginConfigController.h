/* NetSfPeopsSoftGPUPluginConfigController */
/* All the various stuff needed for configuration is done here, including reading 
   the preferences and displaying a dialog box for the user
*/

#define PluginConfigController NetSfPeopsOpenGLPluginConfigController

#import <Cocoa/Cocoa.h>

@interface NetSfPeopsOpenGLPluginConfigController : NSWindowController
@property (weak) IBOutlet NSCell *autoFullScreen;
@property (weak) IBOutlet NSPopUpButton *ditherMode;
@property (weak) IBOutlet NSCell *fpsCounter;
@property (weak) IBOutlet NSCell *frameSkipping;
@property (weak) IBOutlet NSCell *vSync;
@property (weak) IBOutlet NSControl *proportionalResize;
@property (weak) IBOutlet NSPopUpButton *fullscreenSize;
@property (weak) IBOutlet NSFormCell *windowWidth;
@property (weak) IBOutlet NSFormCell *windowHeighth;
@property (weak) IBOutlet NSPopUpButton *offscreenDrawing;
@property (weak) IBOutlet NSPopUpButton *texColorDepth;
@property (weak) IBOutlet NSSlider *texFiltering;
@property (weak) IBOutlet NSSlider *texEnhancment;
@property (weak) IBOutlet NSPopUpButton *frameBufferEffects;
@property (weak) IBOutlet NSCell *drawScanlines;
@property (weak) IBOutlet NSCell *advancedBlending;
@property (weak) IBOutlet NSCell *opaquePass;
@property (weak) IBOutlet NSCell *zMaskClipping;
@property (weak) IBOutlet NSCell *wireframeOnly;
@property (weak) IBOutlet NSCell *blurEffect;
@property (weak) IBOutlet NSCell *mjpegDecoder;
@property (weak) IBOutlet NSCell *mjpegDecoder15bit;
@property (weak) IBOutlet NSCell *gteAccuracy;
@property (weak) IBOutlet NSColorWell *scanlineColorWell;
@property (weak) IBOutlet NSMatrix *hacksMatrix;
@property (weak) IBOutlet NSControl *hackEnable;
@property (weak) IBOutlet NSWindow *hacksWindow;
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
