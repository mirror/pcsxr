//
//  PSXMemEnumerator.c
//  Pcsxr
//
//  Created by C.W. Betts on 7/20/14.
//
//

#include <stdio.h>
#import "PSXMemEnumerator.h"

#define MAX_MEMCARD_BLOCKS 15

static void GetSoloBlockInfo(unsigned char *data, int block, McdBlock *Info)
{
	unsigned char *ptr = data + block * 8192 + 2;
	unsigned char *str = Info->Title;
	unsigned short clut[16];
	unsigned char *	sstr = Info->sTitle;
	unsigned short c;
	int i, x = 0;
	
	memset(Info, 0, sizeof(McdBlock));
	Info->IconCount = *ptr & 0x3;
	ptr += 2;
	
	for (i = 0; i < 48; i++) {
		c = *(ptr) << 8;
		c |= *(ptr + 1);
		if (!c)
			break;
		
		// Convert ASCII characters to half-width
		if (c >= 0x8281 && c <= 0x829A) {
			c = (c - 0x8281) + 'a';
		} else if (c >= 0x824F && c <= 0x827A) {
			c = (c - 0x824F) + '0';
		} else if (c == 0x8140) {
			c = ' ';
		} else if (c == 0x8143) {
			c = ',';
		} else if (c == 0x8144) {
			c = '.';
		} else if (c == 0x8146) {
			c = ':';
		} else if (c == 0x8147) {
			c = ';';
		} else if (c == 0x8148) {
			c = '?';
		} else if (c == 0x8149) {
			c = '!';
		} else if (c == 0x815E) {
			c = '/';
		} else if (c == 0x8168) {
			c = '"';
		} else if (c == 0x8169) {
			c = '(';
		} else if (c == 0x816A) {
			c = ')';
		} else if (c == 0x816D) {
			c = '[';
		} else if (c == 0x816E) {
			c = ']';
		} else if (c == 0x817C) {
			c = '-';
		} else {
			str[i] = ' ';
			sstr[x++] = *ptr++;
			sstr[x++] = *ptr++;
			continue;
		}
		
		str[i] = sstr[x++] = c;
		ptr += 2;
	}
	
	ptr = data + block * 8192 + 0x60; // icon palette data
	
	for (i = 0; i < 16; i++) {
		clut[i] = *((unsigned short *)ptr);
		ptr += 2;
	}
	
	for (i = 0; i < Info->IconCount; i++) {
		short *icon = &Info->Icon[i * 16 * 16];
		
		ptr = data + block * 8192 + 128 + 128 * i; // icon data
		
		for (x = 0; x < 16 * 16; x++) {
			icon[x++] = clut[*ptr & 0xf];
			icon[x] = clut[*ptr >> 4];
			ptr++;
		}
	}

	ptr = data + block * 128;
	
	Info->Flags = *ptr;
	
	ptr += 0xa;
	strlcpy(Info->ID, ptr, 12);
	ptr += 12;
	strlcpy(Info->Name, ptr, 16);
}

static inline PCSXRMemFlags MemBlockFlag(unsigned char blockFlags)
{
	if ((blockFlags & 0xF0) == 0xA0) {
		if ((blockFlags & 0xF) >= 1 && (blockFlags & 0xF) <= 3)
			return memFlagDeleted;
		else
			return memFlagFree;
	} else if ((blockFlags & 0xF0) == 0x50) {
		if ((blockFlags & 0xF) == 0x1)
			return memFlagUsed;
		else if ((blockFlags & 0xF) == 0x2)
			return memFlagLink;
		else if ((blockFlags & 0xF) == 0x3)
			return memFlagEndLink;
	} else
		return memFlagFree;
	
	//Xcode complains unless we do this...
	//NSLog(@"Unknown flag %x", blockFlags);
	return memFlagFree;
}


NSArray *CreateArrayByEnumeratingMemoryCardAtURL(NSURL *location)
{
	NSMutableArray *memArray = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
	if (!location) {
		return nil;
	}
	NSData *fileData = [[NSData alloc] initWithContentsOfURL:location options:NSDataReadingMappedIfSafe error:NULL];
	if (!fileData) {
		return nil;
	}
	
	const unsigned char *memPtr = [fileData bytes];
	if ([fileData length] == MCD_SIZE + 64)
		memPtr += 64;
	else if([fileData length] == MCD_SIZE + 3904)
		memPtr += 3904;
	else if ([fileData length] != MCD_SIZE)
		return nil;
	
	int i = 0, x;
	while (i < MAX_MEMCARD_BLOCKS) {
		x = 1;
		McdBlock memBlock;
		GetSoloBlockInfo((unsigned char *)memPtr, i + 1, &memBlock);
		
		if (MemBlockFlag(memBlock.Flags) == memFlagFree) {
			//Free space: ignore
			i++;
			continue;
		}
		do {
			McdBlock tmpBlock;
			GetSoloBlockInfo((unsigned char *)memPtr, i + x + 1, &tmpBlock);
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
			[memArray addObject:obj];
		}
		i += x;
	}
	
	return [[NSArray alloc] initWithArray:memArray];
}
