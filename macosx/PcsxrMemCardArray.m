//
//  PcsxrMemCardArray.m
//  Pcsxr
//
//  Created by C.W. Betts on 7/6/13.
//
//

#import "PcsxrMemCardArray.h"
#import "ARCBridge.h"
#import "ConfigurationController.h"

#define MAX_MEMCARD_BLOCKS 15

static inline void CopyMemcardData(char *from, char *to, int srci, int dsti, char *str)
{
	// header
	memmove(to + (dsti + 1) * 128, from + (srci + 1) * 128, 128);
	SaveMcd(str, to, (dsti + 1) * 128, 128);
	
	// data
	memmove(to + (dsti + 1) * 1024 * 8, from + (srci+1) * 1024 * 8, 1024 * 8);
	SaveMcd(str, to, (dsti + 1) * 1024 * 8, 1024 * 8);
	
	//printf("data = %s\n", from + (srci+1) * 128);
}


@interface PcsxrMemCardArray ()
@property (arcretain) NSArray *rawArray;

@end

@implementation PcsxrMemCardArray
@synthesize rawArray;

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
			McdBlock memBlock, tmpBlock;
			GetMcdBlockInfo(carNum, i + 1, &memBlock);
			
			if ([PcsxrMemoryObject memFlagsFromBlockFlags:memBlock.Flags] == memFlagFree) {
				//Free space: ignore
				i++;
				continue;
			}
			do {
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
			PcsxrMemoryObject *obj = [[PcsxrMemoryObject alloc] initWithMcdBlock:&memBlock startingIndex:i size:x];
			[tmpMemArray addObject:obj];
			RELEASEOBJ(obj);
			i += x;
		}
		self.rawArray = [NSArray arrayWithArray:tmpMemArray];
		RELEASEOBJ(tmpMemArray);
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
		NSLog(@"Trying to get an object one more than the length of the raw array. Perhaps you were trying to \"move\" the free blocks");
#endif
		return NO;
	}

	
	//TODO: Implement!
	return NO;
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
		return [rawArray arrayByAddingObject:AUTORELEASEOBJ(freeObj)];
	} else
		return rawArray;
}

- (NSURL*)memCardLocation
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
		NSLog(@"Trying to get an object one more than the length of the raw array. Perhaps you were trying to \"delete\" the free blocks");
#endif
		return [self freeBlocks];
	}

	return [[rawArray objectAtIndex:idx] blockSize];
}

- (void)compactMemory
{
	
#if 0
	LoadMcd(cardNumber, cardNumber == 1 ? Config.Mcd1 : Config.Mcd2);
	[[NSNotificationCenter defaultCenter] postNotificationName:memChangeNotifier object:nil userInfo:[NSDictionary dictionaryWithObject:@(cardNumber) forKey:memCardChangeNumberKey]];
#endif
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
		NSLog(@"Trying to get an object one more than the length of the raw array. Perhaps you were trying to \"delete\" the free blocks");
#endif
		return;
	}
	
	PcsxrMemoryObject *theObj = [rawArray objectAtIndex:slotnum];
	
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
