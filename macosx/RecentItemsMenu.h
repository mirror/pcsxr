//
//  RecentItemsMenu.h
//  Pcsxr
//
//  Created by Nicolas Pepin-Perreault on 12-12-16.
//
//

#import <Cocoa/Cocoa.h>

@class PcsxrController;
@interface RecentItemsMenu : NSMenu {
    IBOutlet PcsxrController *pcsxr;
}

- (IBAction)clearRecentDocuments:(id)sender;
- (void)addRecentItem:(NSURL*)documentURL;
- (NSMenuItem*)createMenuItem:(NSURL*)documentURL;
- (void)openRecentItem:(NSMenuItem*)sender;
- (void)addMenuItem:(NSMenuItem*)item;

@end
