//
//  PcsxrMemCardArray.m
//  Pcsxr
//
//  Created by C.W. Betts on 7/6/13.
//
//

#import "PcsxrMemCardArray.h"
#import "ConfigurationController.h"
#include "sio.h"

#define MAX_MEMCARD_BLOCKS 15

static inline void CopyMemcardData(char *from, char *to, int srci, int dsti, char *str)
{
	// header
	memmove(to + (dsti + 1) * 128, from + (srci + 1) * 128, 128);
	SaveMcd(str, to, (dsti + 1) * 128, 128);
	
	// data
	memmove(to + (dsti + 1) * 1024 * 8, from + (srci+1) * 1024 * 8, 1024 * 8);
	SaveMcd(str, to, (dsti + 1) * 1024 * 8, 1024 * 8);
}

static inline char* BlankHeader()
{
	struct PSXMemHeader {
		unsigned int allocState;
		unsigned int fileSize;
		unsigned short nextBlock;
		char fileName[21];
		unsigned char garbage[96];
		unsigned char checksum;
	};
	
	static struct PSXMemHeader *toReturn = NULL;
	if (!toReturn) {
		toReturn = calloc(sizeof(struct PSXMemHeader), 1);
		
		//FIXME: Which value is right?
		toReturn->allocState = 0x000000a0;
		//toReturn->allocState = 0xa0000000;
		toReturn->nextBlock = 0xFFFF;
		unsigned char *bytePtr = (unsigned char*)toReturn;
		for (int i = 0; i < sizeof(struct PSXMemHeader) - sizeof(unsigned char); i++) {
			toReturn->checksum = toReturn->checksum ^ bytePtr[i];
		}
	}
	
	return (char*)toReturn;
}

static inline void ClearMemcardData(char *to, int dsti, char *str)
{
	// header
	char *header = BlankHeader();
	memcpy(to + (dsti + 1) * 128, header, 128);
	SaveMcd(str, to, (dsti + 1) * 128, 128);
	
	// data
	memset(to + (dsti + 1) * 1024 * 8, 0, 1024 * 8);
	SaveMcd(str, to, (dsti + 1) * 1024 * 8, 1024 * 8);
}

@interface PcsxrMemCardArray ()
@property (strong) NSArray *rawArray;
@property (readonly) char* memDataPtr;

@end

@implementation PcsxrMemCardArray
@synthesize rawArray;

- (char*)memDataPtr
{
	if (cardNumber == 1) {
		return Mcd1Data;
	} else {
		return Mcd2Data;
	}
}

- (const char *)memCardCPath
{
	if (cardNumber == 1) {
		return Config.Mcd1;
	} else {
		return Config.Mcd2;
	}
}

- (id)initWithMemoryCardNumber:(int)carNum
{
	NSParameterAssert(carNum == 1 || carNum == 2);
	if (self = [super init]) {
		NSMutableArray *tmpMemArray = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		cardNumber = carNum;
		int i, x;
		i = 0;
		while (i < MAX_MEMCARD_BLOCKS) {
			x = 1;
			McdBlock memBlock;
			GetMcdBlockInfo(carNum, i + 1, &memBlock);
			
			if ([PcsxrMemoryObject memFlagsFromBlockFlags:memBlock.Flags] == memFlagFree) {
				//Free space: ignore
				i++;
				continue;
			}
			do {
				McdBlock tmpBlock;
				GetMcdBlockInfo(carNum, i + x + 1, &tmpBlock);
				if ((tmpBlock.Flags & 0x3) == 0x3) {
					x++;
					break;
				} else if ((tmpBlock.Flags & 0x2) == 0x2) {
					x++;
				} else {
					break;
				}
			} while (i + x - 1 < MAX_MEMCARD_BLOCKS);
			@autoreleasepool {
				PcsxrMemoryObject *obj = [[PcsxrMemoryObject alloc] initWithMcdBlock:&memBlock startingIndex:i size:x];
				[tmpMemArray addObject:obj];
			}
			i += x;
		}
		self.rawArray = [NSArray arrayWithArray:tmpMemArray];
	}
	return self;
}

- (int)indexOfFreeBlocksWithSize:(int)asize
{
	int foundcount = 0, i = 0;
	
	McdBlock obj;
	// search for empty (formatted) blocks first
	while (i < MAX_MEMCARD_BLOCKS && foundcount < asize) {
		GetMcdBlockInfo(cardNumber, 1 + i++, &obj);
		//&Blocks[target_card][++i];
		if ((obj.Flags & 0xFF) == 0xA0) { // if A0 but not A1
			foundcount++;
		} else if (foundcount >= 1) { // need to find n count consecutive blocks
			foundcount = 0;
		} else {
			//i++;
		}
		//printf("formatstatus=%x\n", Info->Flags);
 	}
	
	if (foundcount == asize)
		return (i-foundcount);
	
	// no free formatted slots, try to find a deleted one
	foundcount = i = 0;
	while (i < MAX_MEMCARD_BLOCKS && foundcount < asize) {
		GetMcdBlockInfo(cardNumber, 1 + i++, &obj);
		if ((obj.Flags & 0xF0) == 0xA0) { // A2 or A6 f.e.
			foundcount++;
		} else if (foundcount >= 1) { // need to find n count consecutive blocks
			foundcount = 0;
		} else {
			//i++;
		}
		//printf("delstatus=%x\n", Info->Flags);
 	}
	
	if (foundcount == asize)
		return (i-foundcount);
	
 	return -1;
}

- (BOOL)moveBlockAtIndex:(int)idx toMemoryCard:(PcsxrMemCardArray*)otherCard
{
	if (idx == [rawArray count]) {
#ifdef DEBUG
		NSLog(@"Trying to get an object one more than the length of the raw array. Perhaps you were trying to \"move\" the free blocks. We don't want to do this.");
#endif
		return NO;
	}
	PcsxrMemoryObject *tmpObj = rawArray[idx];

	int memSize = tmpObj.blockSize;
	
	if ([otherCard availableBlocks] < memSize) {
		NSLog(@"Failing because the other card does not have enough space!");
		return NO;
	}
	
	int toCopy = [otherCard indexOfFreeBlocksWithSize:memSize];
	if (toCopy == -1) {
		NSLog(@"Not enough consecutive blocks. Compacting the other card.");
		[otherCard compactMemory];
		//Since we're accessing the mem card data directly (instead of via PcsxrMemoryObject objects) using the following calls, we don't need to reload the data.
		toCopy = [otherCard indexOfFreeBlocksWithSize:memSize];
		NSAssert(toCopy != -1, @"Compacting the card should have made space!");
	}
	
	int memIdx = tmpObj.startingIndex;
	for (int i = 0; i < memSize; i++) {
		CopyMemcardData([self memDataPtr], [otherCard memDataPtr], (memIdx+i), (toCopy+i), (char*)otherCard.memCardCPath);
	}
	
	return YES;
}

- (int)freeBlocks
{
	int memSize = 15;
	for (PcsxrMemoryObject *memObj in rawArray) {
		memSize -= memObj.blockSize;
	}
	return memSize;
}

- (int)availableBlocks
{
	int memSize = MAX_MEMCARD_BLOCKS;
	for (PcsxrMemoryObject *memObj in rawArray) {
		if (memObj.flagNameIndex != memFlagDeleted) {
			memSize -= memObj.blockSize;
		}
	}
	return memSize;
}

- (NSArray*)memoryArray
{
	int freeSize = [self freeBlocks];
	
	if (freeSize) {
		McdBlock theBlock;
		//Create a blank "block" that will be used for
		theBlock.Flags = 0xA0;
		theBlock.IconCount = 0;
		PcsxrMemoryObject *freeObj = [[PcsxrMemoryObject alloc] initWithMcdBlock:&theBlock startingIndex:MAX_MEMCARD_BLOCKS - 1 - freeSize size:freeSize];
		return [rawArray arrayByAddingObject:freeObj];
	} else
		return rawArray;
}

- (NSURL*)memCardURL
{
	if (cardNumber == 1) {
		return [[NSUserDefaults standardUserDefaults] URLForKey:@"Mcd1"];
	} else {
		return [[NSUserDefaults standardUserDefaults] URLForKey:@"Mcd2"];
	}
}

- (int)memorySizeAtIndex:(int)idx
{
	if (idx == [rawArray count]) {
#ifdef DEBUG
		NSLog(@"Trying to get an object one more than the length of the raw array. Perhaps you were trying to \"count\" the free blocks.");
#endif
		return [self freeBlocks];
	}

	return [rawArray[idx] blockSize];
}

- (void)compactMemory
{
	int i = 0, x = 1;
	while (i < MAX_MEMCARD_BLOCKS && x < MAX_MEMCARD_BLOCKS) {
		x = i;
		McdBlock baseBlock;
		GetMcdBlockInfo(cardNumber, i+1, &baseBlock);
		PCSXRMemFlags theFlags = [PcsxrMemoryObject memFlagsFromBlockFlags:baseBlock.Flags];
		
		if (theFlags == memFlagDeleted || theFlags == memFlagFree) {
			PCSXRMemFlags up1Flags = theFlags;
			while ((up1Flags == memFlagDeleted || up1Flags == memFlagFree) && x < MAX_MEMCARD_BLOCKS){
				x++;
				McdBlock up1Block;
				GetMcdBlockInfo(cardNumber, x+1, &up1Block);
				up1Flags = [PcsxrMemoryObject memFlagsFromBlockFlags:up1Block.Flags];
			}
			if (x >= MAX_MEMCARD_BLOCKS) {
				
				break;
			}
			CopyMemcardData(self.memDataPtr, self.memDataPtr, x, i, (char*)[[self.memCardURL path] fileSystemRepresentation]);
			ClearMemcardData(self.memDataPtr, x, (char*)self.memCardCPath );
		}
		i++;
	}
	
	while (i < MAX_MEMCARD_BLOCKS) {
		ClearMemcardData(self.memDataPtr, i, (char*)self.memCardCPath);
		i++;
	}
	
	LoadMcd(cardNumber, (char*)self.memCardCPath);
}

- (void)deleteMemoryBlocksAtIndex:(int)slotnum
{
	int xor = 0, i, j;
	char *data, *ptr, *filename;
	if (cardNumber == 1) {
		filename = Config.Mcd1;
		data = Mcd1Data;
	} else {
		filename = Config.Mcd2;
		data = Mcd2Data;
	}
	
	if (slotnum == [rawArray count]) {
#ifdef DEBUG
		NSLog(@"Trying to get an object one more than the length of the raw array. Perhaps you were trying to \"delete\" the free blocks.");
#endif
		return;
	}
	
	PcsxrMemoryObject *theObj = rawArray[slotnum];
	
	McdBlock flagBlock;
	
	for(i = theObj.startingIndex +1; i < (theObj.startingIndex + theObj.blockSize +1); i++)
	{
		GetMcdBlockInfo(cardNumber, i, &flagBlock);
		ptr = data + i * 128;
		
		if ((flagBlock.Flags & 0xF0) == 0xA0) {
			if ((flagBlock.Flags & 0xF) >= 1 &&
				(flagBlock.Flags & 0xF) <= 3) { // deleted
				*ptr = 0x50 | (flagBlock.Flags & 0xF);
			} else return;
		} else if ((flagBlock.Flags & 0xF0) == 0x50) { // used
			*ptr = 0xA0 | (flagBlock.Flags & 0xF);
		} else { continue; }
		
		for (j = 0; j < 127; j++) xor ^= *ptr++;
		*ptr = xor;
		
		SaveMcd(filename, data, i * 128, 128);
	}
}

@end
