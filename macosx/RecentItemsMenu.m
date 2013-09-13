//
//  RecentItemsMenu.m
//  Pcsxr
//
//  Created by Nicolas Pepin-Perreault on 12-12-16.
//
//

#import "RecentItemsMenu.h"
#import "PcsxrController.h"

@implementation RecentItemsMenu

@synthesize pcsxr;

// Initialization
- (void)awakeFromNib
{
    [self setAutoenablesItems:YES];
    
    // Populate the menu
    NSArray* recentDocuments = [[NSDocumentController sharedDocumentController] recentDocumentURLs];
    NSInteger index = 0;
    for (NSURL* url in recentDocuments) {
		NSMenuItem *tempItem = [self newMenuItem:url];
        [self addMenuItem:tempItem atIndex:index];
        index++;
    }
}

- (void)addRecentItem:(NSURL*)documentURL
{
    [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:documentURL];
    
    NSMenuItem* item = [self findMenuItemByURL:documentURL];
    if (item != nil) {
        [self removeItem:item];
        [self insertItem:item atIndex:0];
    } else {
		NSMenuItem *newitem = [self newMenuItem:documentURL];
        [self addMenuItem:newitem];
    }
}

- (void)addMenuItem:(NSMenuItem*)item
{
    [self addMenuItem:item atIndex:0];
    
    // Prevent menu from overflowing; the -2 accounts for the "Clear..." and the separator items
    NSInteger maxNumItems = [[NSDocumentController sharedDocumentController] maximumRecentDocumentCount];
    if (([self numberOfItems] - 2) > maxNumItems) {
        [self removeItemAtIndex:maxNumItems];
    }
}

- (NSMenuItem*)findMenuItemByURL:(NSURL*)url
{
    for(NSMenuItem* item in [self itemArray]) {
        if([[item representedObject] isEqual:url]) {
            return item;
        }
    }
    
    return nil;
}

- (void)addMenuItem:(NSMenuItem*)item atIndex:(NSInteger)index
{
    [self insertItem:item atIndex:index]; // insert at the top
}

- (NSMenuItem*)newMenuItem:(NSURL*)documentURL
{
    NSString *lastName = nil;
	[documentURL getResourceValue:&lastName forKey:NSURLLocalizedNameKey error:NULL];
	if (!lastName) {
		lastName = [documentURL lastPathComponent];
	}
	
	NSMenuItem *newItem = [[NSMenuItem alloc] initWithTitle:lastName action:@selector(openRecentItem:) keyEquivalent:@""];
    [newItem setRepresentedObject:documentURL];
    [newItem setTarget:self];
    
    return newItem;
}

- (void)openRecentItem:(NSMenuItem*)sender
{
    NSURL* url = [sender representedObject];
    [self addRecentItem:url];
    [pcsxr runURL:url];
}

- (IBAction)clearRecentDocuments:(id)sender
{
    [self removeDocumentItems];
    [[NSDocumentController sharedDocumentController] clearRecentDocuments:sender];
}

// Document items are menu items with tag 0
- (void)removeDocumentItems
{
	NSMutableArray *removeItemsArray = [[NSMutableArray alloc] initWithCapacity:10];
	for (NSMenuItem* item in [self itemArray]) {
		if([item tag] == 0) {
			[removeItemsArray addObject:item];
		}
	}
	for (NSMenuItem *item in removeItemsArray) {
		[self removeItem:item];
	}
}

@end
