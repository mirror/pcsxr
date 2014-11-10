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

- (instancetype)initWithMemoryCardNumber:(int)carNum NS_DESIGNATED_INITIALIZER;

- (void)deleteMemoryBlocksAtIndex:(int)slotnum;
- (void)compactMemory;

@property (readonly) int freeBlocks;
@property (readonly) int availableBlocks;
- (int)memorySizeAtIndex:(int)idx;
- (BOOL)moveBlockAtIndex:(int)idx toMemoryCard:(PcsxrMemCardArray*)otherCard;
- (int)indexOfFreeBlocksWithSize:(int)asize;

@property (nonatomic, readonly, unsafe_unretained) NSArray *memoryArray;
@property (nonatomic, readonly, unsafe_unretained) NSURL *memCardURL;
@property (nonatomic, readonly) const char *memCardCPath;

@end
