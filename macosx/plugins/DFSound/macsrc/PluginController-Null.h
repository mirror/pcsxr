/* NetSfPeopsSPUPluginController */

#import <Cocoa/Cocoa.h>
#import "NamedSlider.h"

void DoAbout();
long DoConfiguration();
void LoadConfiguration();

#define PluginController NullSPUPluginController

@interface PluginController : NSWindowController
{
}
- (IBAction)ok:(id)sender;
@end
