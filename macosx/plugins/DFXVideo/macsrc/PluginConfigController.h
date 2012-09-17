/* NetSfPeopsSoftGPUPluginConfigController */

#define PluginConfigController NetSfPeopsSoftGPUPluginConfigController

#import <Cocoa/Cocoa.h>

@interface NetSfPeopsSoftGPUPluginConfigController : NSWindowController
{
    IBOutlet NSControl *autoFullScreen;
    IBOutlet NSPopUpButton *ditherMode;
    IBOutlet NSControl *fpsCounter;
    IBOutlet NSControl *frameSkipping;
    IBOutlet NSControl *hackEnable;
    IBOutlet NSView *hacksView;
    IBOutlet NSControl *vSync;
	IBOutlet NSControl *shaders;
	IBOutlet NSTextField *vertexShaderViewablePath;
	IBOutlet NSTextField *fragmentShaderViewablePath;
	IBOutlet NSControl *vertexChooser;
	IBOutlet NSControl *fragmentChooser;
	IBOutlet NSView *shadersView;
	
	NSURL *vertexPath;
	NSURL *fragmentPath;
	 
	 NSMutableDictionary *keyValues;
}
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
- (IBAction)reset:(id)sender;
- (IBAction)hackToggle:(id)sender;
- (IBAction)toggleShader:(id)sender;
- (IBAction)selectShader:(id)sender;

- (void)loadValues;

@end
