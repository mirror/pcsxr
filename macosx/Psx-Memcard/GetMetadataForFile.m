//
//  GetMetadataForFile.m
//  Psx-Memcard
//
//  Created by C.W. Betts on 6/6/14.
//
//

#include <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#include "GetMetadataForFile.h"
#include "PcsxrMemoryObject.h"

#define MAX_MEMCARD_BLOCKS 15
#define kPCSXRSaveNames @"com_codeplex_pcsxr_memcard_savenames"
#define kPCSXRMemCount @"com_codeplex_pcsxr_memcard_memcount"
#define kPCSXRFreeBlocks @"com_codeplex_pcsxr_memcard_freeblocks"
#define kPCSXRMemNames @"com_codeplex_pcsxr_memcard_memnames"
#define kPCSXRMemIDs @"com_codeplex_pcsxr_memcard_memids"

//==============================================================================
//
//	Get metadata attributes from document files
//
//	The purpose of this function is to extract useful information from the
//	file formats for your document, and set the values into the attribute
//  dictionary for Spotlight to include.
//
//==============================================================================

static void trimPriv(char *str) {
	int pos = 0;
	char *dest = str;
	
	// skip leading blanks
	while (str[pos] <= ' ' && str[pos] > 0)
		pos++;
	
	while (str[pos]) {
		*(dest++) = str[pos];
		pos++;
	}
	
	*(dest--) = '\0'; // store the null
	
	// remove trailing blanks
	while (dest >= str && *dest <= ' ' && *dest > 0)
		*(dest--) = '\0';
}

static void GetSoloBlockInfo(unsigned char *data, int block, McdBlock *Info)
{
	unsigned char *ptr, *str, *sstr;
	unsigned short clut[16];
	unsigned short c;
	int i, x;
	
	memset(Info, 0, sizeof(McdBlock));
	
	ptr = data + block * 8192 + 2;
	
	Info->IconCount = *ptr & 0x3;
	
	ptr += 2;
	
	x = 0;
	
	str = Info->Title;
	sstr = Info->sTitle;
	
	for (i = 0; i < 48; i++) {
		c = *(ptr) << 8;
		c |= *(ptr + 1);
		if (!c) break;
		
		// Convert ASCII characters to half-width
		if (c >= 0x8281 && c <= 0x829A)
			c = (c - 0x8281) + 'a';
		else if (c >= 0x824F && c <= 0x827A)
			c = (c - 0x824F) + '0';
		else if (c == 0x8140) c = ' ';
		else if (c == 0x8143) c = ',';
		else if (c == 0x8144) c = '.';
		else if (c == 0x8146) c = ':';
		else if (c == 0x8147) c = ';';
		else if (c == 0x8148) c = '?';
		else if (c == 0x8149) c = '!';
		else if (c == 0x815E) c = '/';
		else if (c == 0x8168) c = '"';
		else if (c == 0x8169) c = '(';
		else if (c == 0x816A) c = ')';
		else if (c == 0x816D) c = '[';
		else if (c == 0x816E) c = ']';
		else if (c == 0x817C) c = '-';
		else {
			str[i] = ' ';
			sstr[x++] = *ptr++; sstr[x++] = *ptr++;
			continue;
		}
		
		str[i] = sstr[x++] = c;
		ptr += 2;
	}
	
	trimPriv(str);
	trimPriv(sstr);
	
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

static Boolean PopulateMemCards(NSData *fileData, NSArray **outArray)
{
	NSMutableArray *memArray = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
	if (!fileData) {
		*outArray = @[];
		return FALSE;
	}
	const unsigned char *memPtr = [fileData bytes];
	if ([fileData length] == MCD_SIZE + 64)
		memPtr += 64;
	else if([fileData length] == MCD_SIZE + 3904)
		memPtr += 3904;
	else if ([fileData length] != MCD_SIZE)
		return FALSE;
	
	int i = 0, x;
	while (i < MAX_MEMCARD_BLOCKS) {
		x = 1;
		McdBlock memBlock;
		GetSoloBlockInfo((unsigned char *)memPtr, i + 1, &memBlock);
		
		if ([PcsxrMemoryObject memFlagsFromBlockFlags:memBlock.Flags] == memFlagFree) {
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

	*outArray = [[NSArray alloc] initWithArray:memArray];
	return true;
}

Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI, CFStringRef pathToFile)
{
    Boolean ok = FALSE;
    @autoreleasepool {
		short freeBlocks = MAX_MEMCARD_BLOCKS;
		short memCount = 0;
		NSMutableArray *enNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableArray *jpNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableArray *memNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableArray *memIDs = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableDictionary *attr = (__bridge NSMutableDictionary*)attributes;
		NSString *path = (__bridge NSString*)pathToFile;
		NSArray *cardArrays;
		NSData *fileData = [[NSData alloc] initWithContentsOfFile:path options:NSDataReadingMappedIfSafe error:NULL];
		if (!fileData) {
			return FALSE;
		}
		ok = PopulateMemCards(fileData, &cardArrays);
		if (!ok) {
			return FALSE;
		}
		for (PcsxrMemoryObject *obj in cardArrays) {
			// Ignore deleted blocks
			if (obj.flagNameIndex == memFlagDeleted) {
				continue;
			}
			freeBlocks -= obj.blockSize;
			memCount++;
			[enNames addObject:obj.englishName];
			[jpNames addObject:obj.sjisName];
			[memNames addObject:obj.memName];
			[memIDs addObject:obj.memID];
		}
		
		
		attr[kPCSXRSaveNames] = @{@"en": [enNames copy],
								  @"jp": [jpNames copy]};
		attr[kPCSXRMemCount] = @(memCount);
		attr[kPCSXRFreeBlocks] = @(freeBlocks);
		attr[kPCSXRMemNames] = [memNames copy];
		attr[kPCSXRMemIDs] = [memIDs copy];
		ok = TRUE;
	}
    
	// Return the status
    return ok;
}
