//
//  PcsxrMemCardManager.m
//  Pcsxr
//
//  Created by Charles Betts on 11/23/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PcsxrMemCardController.h"
#import "PcsxrMemoryObject.h"
#import "ConfigurationController.h"
#import "PcsxrMemCardHandler.h"
#import "PcsxrMemCardArray.h"
#include "sio.h"
#import "ARCBridge.h"

#define MAX_MEMCARD_BLOCKS 15

@interface PcsxrMemCardController ()
@property (readwrite, arcretain) PcsxrMemCardArray *memCard1Array;
@property (readwrite, arcretain) PcsxrMemCardArray *memCard2Array;
@end

@implementation PcsxrMemCardController

@synthesize memCard1Array, memCard2Array;

- (void)setupValues:(int)theCards
{
	NSParameterAssert(theCards < 4 && theCards > 0);
	if (theCards == 3) {
		LoadMcds(Config.Mcd1, Config.Mcd2);
	} else {
		LoadMcd(theCards, theCards == 1 ? Config.Mcd1 : Config.Mcd2);
	}
	NSFileManager *fm = [NSFileManager defaultManager];
	NSUserDefaults *def = [NSUserDefaults standardUserDefaults];
	NSString *fullPath = nil;
	NSString *fileName = nil;
	
	if (theCards & 1) {
		fullPath = [[def URLForKey:@"Mcd1"] path];
		fileName = [fm displayNameAtPath:fullPath];
		
		[memCard1Label setTitleWithMnemonic:fileName];
		[memCard1Label setToolTip:fullPath];
		
		[self loadMemoryCardInfoForCard:1];
	}
	
	if (theCards & 2) {
		fullPath = [[def URLForKey:@"Mcd2"] path];
		fileName = [fm displayNameAtPath:fullPath];
		
		[memCard2Label setTitleWithMnemonic:fileName];
		[memCard2Label setToolTip:fullPath];
		
		[self loadMemoryCardInfoForCard:2];
	}
}

#if 0
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil;
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        //LoadMcds(Config.Mcd1, Config.Mcd2);
    }
    
    return self;
}
#endif

- (void)loadMemoryCardInfoForCard:(int)theCard
{
	PcsxrMemCardArray *newArray = [[PcsxrMemCardArray alloc] initWithMemoryCardNumber:theCard];
	
	if (theCard == 1) {
		[self setMemCard1Array:newArray];
	} else {
		[self setMemCard2Array:newArray];
	}
	RELEASEOBJ(newArray);
}

- (void)memoryCardDidChangeNotification:(NSNotification *)aNote
{
	NSDictionary *dict = [aNote userInfo];
	NSNumber *theNum = [dict objectForKey:memCardChangeNumberKey];
	[self setupValues: theNum ? [theNum intValue] : 3];
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	[self setupValues:3];
    
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(memoryCardDidChangeNotification:) name:memChangeNotifier object:nil];

	imageAnimateTimer = [[NSTimer alloc] initWithFireDate:[NSDate date] interval:3.0/10.0 target:self selector:@selector(animateMemCards:) userInfo:nil repeats:YES];
	[[NSRunLoop mainRunLoop] addTimer:imageAnimateTimer forMode:NSRunLoopCommonModes];
}

- (void)animateMemCards:(NSTimer*)theTimer
{
	[[NSNotificationCenter defaultCenter] postNotificationName:memoryAnimateTimerKey object:self];
}

- (IBAction)moveBlock:(id)sender
{
	NSInteger memCardSelect = [sender tag];
	NSCollectionView *cardView;
	NSIndexSet *selection;
	PcsxrMemCardArray *toCard, *fromCard;
	int cardnum;
	if (memCardSelect == 1) {
		cardView = memCard2view;
		toCard = memCard1Array;
		fromCard = memCard2Array;
		cardnum = 1;
	} else {
		cardView = memCard1view;
		toCard = memCard2Array;
		fromCard = memCard1Array;
		cardnum = 2;
	}
	selection = [cardView selectionIndexes];
	if (!selection || [selection count] == 0) {
		NSBeep();
		return;
	}
	NSInteger selectedIndex = [selection firstIndex];
	
	int cardSize, freeConsBlocks, availBlocks;
	
	if ([[[fromCard memoryArray] objectAtIndex:selectedIndex] flagNameIndex] == memFlagFree) {
		NSBeep();
		return;
	}
	
	cardSize = [fromCard memorySizeAtIndex:(int)selectedIndex];
	freeConsBlocks = [toCard indexOfFreeBlocksWithSize:cardSize];
	availBlocks = [toCard availableBlocks];
	if (freeConsBlocks == -1 && availBlocks >= cardSize) {
		PcsxrMemoryObject *tmpmemobj = [fromCard.memoryArray objectAtIndex:selectedIndex];
		NSInteger copyOK = NSRunInformationalAlertPanel(NSLocalizedString(@"Free Size", nil), NSLocalizedString(@"Memory card %i does not have enough free consecutive blocks.\n\nIn order to copy over \"%@ (%@),\" memory card %i must be compressed. Compressing memory cards will make deleted blocks unrecoverable.\n\nDo you want to continue?", nil), NSLocalizedString(@"Yes", nil), NSLocalizedString(@"No", nil), nil, cardnum, tmpmemobj.englishName, tmpmemobj.sjisName, cardnum);
		if (copyOK != NSAlertDefaultReturn) {
			return;
		}
	} else if (cardSize > availBlocks) {
		NSRunCriticalAlertPanel(NSLocalizedString(@"No Free Space", nil), NSLocalizedString(@"Memory card %d doesn't have %d free consecutive blocks on it. Please remove some blocks on that card to continue", nil), nil, nil, nil, availBlocks, cardnum);
		return;
	}
	
	[fromCard moveBlockAtIndex:(int)selectedIndex toMemoryCard:toCard];
	
	if (cardnum == 1) {
		LoadMcd(1, Config.Mcd1);
	} else {
		LoadMcd(2, Config.Mcd2);
	}
	[self loadMemoryCardInfoForCard:cardnum];
}

- (IBAction)formatCard:(id)sender
{
	NSInteger formatOkay = NSRunAlertPanel(NSLocalizedString(@"Format Card", nil), NSLocalizedString(@"Formatting a memory card will remove all data on it.\n\nThis cannot be undone.", nil), NSLocalizedString(@"Cancel", nil), NSLocalizedString(@"Format", nil), nil);
	if (formatOkay == NSAlertAlternateReturn) {
		NSInteger memCardSelect = [sender tag];
		if (memCardSelect == 1) {
			CreateMcd(Config.Mcd1);
			LoadMcd(1, Config.Mcd1);
		} else {
			CreateMcd(Config.Mcd2);
			LoadMcd(2, Config.Mcd2);
		}
		[self loadMemoryCardInfoForCard:(int)memCardSelect];
	}
}

- (void)deleteMemoryBlocksAtIndex:(int)slotnum card:(int)cardNum
{
	PcsxrMemCardArray *cardArray;
	if (cardNum == 1) {
		cardArray = [self memCard1Array];
	} else {
		cardArray = [self memCard2Array];
	}
	[cardArray deleteMemoryBlocksAtIndex:slotnum];
}

- (IBAction)deleteMemoryObject:(id)sender
{
	PcsxrMemCardArray *curCard;
	NSInteger memCardSelect = [sender tag];
	NSIndexSet *selected;
	if (memCardSelect == 1) {
		curCard = memCard1Array;
		selected = [memCard1view selectionIndexes];
	} else {
		curCard = memCard2Array;
		selected = [memCard2view selectionIndexes];
	}
	
	if (!selected || [selected count] == 0) {
		NSBeep();
		return;
	}
	
	NSInteger selectedIndex = [selected firstIndex];

	PcsxrMemoryObject *tmpObj = [[curCard memoryArray] objectAtIndex:selectedIndex];
	
	if (tmpObj.flagNameIndex == memFlagFree) {
		NSBeep();
		return;
	}
	
	NSInteger deleteOkay = NSRunAlertPanel(NSLocalizedString(@"Delete Block", nil), NSLocalizedString(@"Deleting a block will remove all saved data on that block.\n\nThis cannot be undone.", nil), NSLocalizedString(@"Cancel", nil), NSLocalizedString(@"Delete", nil), nil);
	if (deleteOkay == NSAlertAlternateReturn) {
		[self deleteMemoryBlocksAtIndex:(int)selectedIndex card:(int)memCardSelect];
		
		if (memCardSelect == 1) {
			LoadMcd(1, Config.Mcd1);
		} else {
			LoadMcd(2, Config.Mcd2);
		}
		[self loadMemoryCardInfoForCard:(int)memCardSelect];
	}
}

- (IBAction)changeMemCard:(id)sender
{
	[ConfigurationController mcdChangeClicked:sender];
}

- (IBAction)newMemCard:(id)sender
{
	[ConfigurationController mcdNewClicked:sender];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[imageAnimateTimer invalidate];
	
#if !__has_feature(objc_arc)
	[imageAnimateTimer release];
	self.memCard1Array = nil;
	self.memCard2Array = nil;

	[super dealloc];
#endif
}

@end
