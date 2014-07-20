//
//  PcsxrMemCardManager.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
@class PcsxrMemCardArray;

@interface PcsxrMemCardController : NSViewController
{
	IBOutlet NSCollectionView *memCard1view;
	IBOutlet NSCollectionView *memCard2view;
	IBOutlet NSTextField *memCard1Label;
	IBOutlet NSTextField *memCard2Label;
}
@property (readonly, strong) PcsxrMemCardArray *memCard1Array;
@property (readonly, strong) PcsxrMemCardArray *memCard2Array;

- (IBAction)moveBlock:(id)sender;
- (IBAction)formatCard:(id)sender;
- (IBAction)deleteMemoryObject:(id)sender;
- (void)loadMemoryCardInfoForCard:(int)theCard;

- (void)beginMemoryAnimation;
- (void)stopMemoryAnimation;

@end
