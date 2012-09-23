#import "PluginController.h"
#import "PcsxrPlugin.h"
#import "PcsxrController.h"

@implementation PluginController

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
	if (sender==pluginMenu) {
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
	NSUInteger i;

	// remember the list
	pluginType = type;
	plugins = [list retain];
	defaultKey = [[PcsxrPlugin defaultKeyForType:pluginType] retain];

	// clear the previous menu items
	[pluginMenu removeAllItems];

	// load the currently selected plugin
	sel = [[NSUserDefaults standardUserDefaults] stringForKey:defaultKey];

	// add the menu entries
	for (i = 0; i < [plugins count]; i++) {
		[pluginMenu addItemWithTitle:[[plugins objectAtIndex:i] description]];

		// make sure the currently selected is set as such
		if ([sel isEqualToString:[[plugins objectAtIndex:i] path]]) {
			[pluginMenu selectItemAtIndex:i];
		}
	}

	[self selectPlugin:pluginMenu];
}

- (void)dealloc
{
	if (plugins) [plugins release];
	if (defaultKey) [defaultKey release];
	[super dealloc];
}

@end
