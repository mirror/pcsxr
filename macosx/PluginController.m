#import "PluginController.h"
#import "PcsxrPlugin.h"
#import "PcsxrController.h"

@interface PluginController ()
@property (strong) NSArray *plugins;
@property (strong) NSString *defaultKey;
@property int pluginType;
@end

@implementation PluginController
@synthesize aboutButton;
@synthesize configureButton;
@synthesize pluginMenu;

- (IBAction)doAbout:(id)sender
{
	 PcsxrPlugin *plugin = (self.plugins)[[pluginMenu indexOfSelectedItem]];
	 [plugin aboutAs:self.pluginType];
}

- (IBAction)doConfigure:(id)sender
{
	 PcsxrPlugin *plugin = (self.plugins)[[pluginMenu indexOfSelectedItem]];
	 [plugin configureAs:self.pluginType];
}

- (IBAction)selectPlugin:(id)sender
{
	if (sender == pluginMenu) {
		NSInteger index = [pluginMenu indexOfSelectedItem];
		if (index != -1) {
			PcsxrPlugin *plugin = (self.plugins)[index];

			if (![[PluginList list] setActivePlugin:plugin forType:self.pluginType]) {
				/* plugin won't initialize */
			}

			// write selection to defaults
			[[NSUserDefaults standardUserDefaults] setObject:[plugin path] forKey:self.defaultKey];

			// set button states
			[aboutButton setEnabled:[plugin hasAboutAs:self.pluginType]];
			[configureButton setEnabled:[plugin hasConfigureAs:self.pluginType]];
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
	self.pluginType = type;
	self.plugins = list;
	self.defaultKey = [PcsxrPlugin defaultKeyForType:self.pluginType];

	// clear the previous menu items
	[pluginMenu removeAllItems];

	// load the currently selected plugin
	sel = [[NSUserDefaults standardUserDefaults] stringForKey:self.defaultKey];

	// add the menu entries
	for (PcsxrPlugin *plug in self.plugins) {
		NSString *description = [plug description];
		[pluginMenu addItemWithTitle:description];
		
		// make sure the currently selected is set as such
		if ([sel isEqualToString:[plug path]]) {
			[pluginMenu selectItemWithTitle:description];
		}
	}

	[self selectPlugin:pluginMenu];
}

@end
