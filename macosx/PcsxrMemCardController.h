//
//  PcsxrMemCardManager.h
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ARCBridge.h"
@class PcsxrMemCardArray;

@interface PcsxrMemCardController : NSViewController
{
	IBOutlet NSCollectionView *memCard1view;
	IBOutlet NSCollectionView *memCard2view;
	IBOutlet NSTextField *memCard1Label;
	IBOutlet NSTextField *memCard2Label;
	
	PcsxrMemCardArray *memCard1Array;
	PcsxrMemCardArray *memCard2Array;
	NSTimer *imageAnimateTimer;
}
@property (readonly, arcretain) PcsxrMemCardArray *memCard1Array;
@property (readonly, arcretain) PcsxrMemCardArray *memCard2Array;

- (IBAction)moveBlock:(id)sender;
- (IBAction)formatCard:(id)sender;
- (IBAction)deleteMemoryObject:(id)sender;
- (IBAction)newMemCard:(id)sender;
- (IBAction)changeMemCard:(id)sender;
- (void)loadMemoryCardInfoForCard:(int)theCard;
@end
