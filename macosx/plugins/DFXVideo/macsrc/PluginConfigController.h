/* NetSfPeopsSoftGPUPluginConfigController */

#define PluginConfigController NetSfPeopsSoftGPUPluginConfigController

#import <Cocoa/Cocoa.h>

@interface NetSfPeopsSoftGPUPluginConfigController : NSWindowController
@property (strong) NSURL *vertexPath;
@property (strong) NSURL *fragmentPath;
@property (strong) NSMutableDictionary *keyValues;

@property (weak) IBOutlet NSFormCell *displayWidth;
@property (weak) IBOutlet NSFormCell *displayHeight;
@property (weak) IBOutlet NSControl *autoFullScreen;
@property (weak) IBOutlet NSPopUpButton *ditherMode;
@property (weak) IBOutlet NSControl *fpsCounter;
@property (weak) IBOutlet NSControl *frameSkipping;
@property (weak) IBOutlet NSControl *hackEnable;
@property (weak) IBOutlet NSView *hacksView;
@property (weak) IBOutlet NSMatrix *hacksMatrix;
@property (weak) IBOutlet NSControl *vSync;
@property (weak) IBOutlet NSControl *shaders;
@property (weak) IBOutlet NSTextField *vertexShaderViewablePath;
@property (weak) IBOutlet NSTextField *fragmentShaderViewablePath;
@property (weak) IBOutlet NSControl *vertexChooser;
@property (weak) IBOutlet NSControl *fragmentChooser;
@property (weak) IBOutlet NSView *shadersView;
@property (weak) IBOutlet NSPopUpButton *shaderQualitySelector;

- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;
- (IBAction)hackToggle:(id)sender;
- (IBAction)toggleShader:(id)sender;
- (IBAction)selectShader:(id)sender;

- (void)loadValues;

@end
