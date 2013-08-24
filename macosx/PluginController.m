#import "PluginController.h"
#import "PcsxrPlugin.h"
#import "PcsxrController.h"
#import "ARCBridge.h"

@interface PluginController ()
@property (arcstrong) NSArray *plugins;
@property (arcstrong) NSString *defaultKey;
@end

@implementation PluginController
@synthesize defaultKey;
@synthesize plugins;

- (IBAction)doAbout:(id)sender
{
	 PcsxrPlugin *plugin = [plugins objectAtIndex:[pluginMenu indexOfSelectedItem]];
	 [plugin aboutAs:pluginType];
}

- (IBAction)doConfigure:(id)sender
{
	 PcsxrPlugin *plugin = [plugins objectAtIndex:[pluginMenu indexOfSelectedItem]];
	 [plugin configureAs:pluginType];
}

- (IBAction)selectPlugin:(id)sender
{
	if (sender == pluginMenu) {
		NSInteger index = [pluginMenu indexOfSelectedItem];
		if (index != -1) {
			PcsxrPlugin *plugin = [plugins objectAtIndex:index];

			if (![[PluginList list] setActivePlugin:plugin forType:pluginType]) {
				/* plugin won't initialize */
			}

			// write selection to defaults
			[[NSUserDefaults standardUserDefaults] setObject:[plugin path] forKey:defaultKey];

			// set button states
			[aboutButton setEnabled:[plugin hasAboutAs:pluginType]];
			[configureButton setEnabled:[plugin hasConfigureAs:pluginType]];
		} else {
			// set button states
			[aboutButton setEnabled:NO];
			[configureButton setEnabled:NO];
		}
	}
}

// must be called before anything else
- (void)setPluginsTo:(NSArray *)list withType:(int)type
{
	NSString *sel;

	// remember the list
	pluginType = type;
	self.plugins = list;
	self.defaultKey = [PcsxrPlugin defaultKeyForType:pluginType];

	// clear the previous menu items
	[pluginMenu removeAllItems];

	// load the currently selected plugin
	sel = [[NSUserDefaults standardUserDefaults] stringForKey:defaultKey];

	// add the menu entries
	for (PcsxrPlugin *plug in plugins) {
		NSString *description = [plug description];
		[pluginMenu addItemWithTitle:description];
		
		// make sure the currently selected is set as such
		if ([sel isEqualToString:[plug path]]) {
			[pluginMenu selectItemWithTitle:description];
		}
	}

	[self selectPlugin:pluginMenu];
}

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.plugins = nil;
	self.defaultKey = nil;
	
	[super dealloc];
}
#endif

@end
