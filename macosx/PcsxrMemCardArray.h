//
//  PcsxrMemCardArray.h
//  Pcsxr
//
//  Created by C.W. Betts on 7/6/13.
//
//

#import <Foundation/Foundation.h>
#import "PcsxrMemoryObject.h"

@interface PcsxrMemCardArray : NSObject
{
	@private
	NSArray *rawArray;
	int cardNumber;
	NSURL *fileURL;
}

- (id)initWithMemoryCardNumber:(int)carNum;

- (void)deleteMemoryBlocksAtIndex:(int)slotnum;
- (void)compactMemory;
//Blocks that are free
- (int)freeBlocks;
//Blocks that have been deleted and are free
- (int)availableBlocks;
- (int)memorySizeAtIndex:(int)idx;
- (BOOL)moveBlockAtIndex:(int)idx toMemoryCard:(PcsxrMemCardArray*)otherCard;
- (int)indexOfFreeBlocksWithSize:(int)asize;

@property (nonatomic, readonly, unsafe_unretained) NSArray *memoryArray;
@property (nonatomic, readonly, unsafe_unretained) NSURL *memCardURL;
@property (nonatomic, readonly) const char *memCardCPath;

@end
