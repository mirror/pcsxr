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
#include "sio.h"

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

typedef NS_ENUM(char, PCSXRMemFlags) {
	memFlagDeleted,
	memFlagFree,
	memFlagUsed,
	memFlagLink,
	memFlagEndLink
};

#if 0
static void trimPriv(char *str)
{
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
#endif

static void GetSoloBlockInfo(unsigned char *data, int block, McdBlock *Info)
{
	unsigned char *ptr = data + block * 8192 + 2;
	unsigned char *str = Info->Title;
	unsigned char *sstr = Info->sTitle;
	unsigned short c;
	unsigned short jisTitle[48] = {0};
	int i, x = 0;
	
	memset(Info, 0, sizeof(McdBlock));
	Info->IconCount = *ptr & 0x3;
	ptr += 2;
	
	for (i = 0; i < 48; i++) {
		c = *(ptr) << 8;
		c |= *(ptr + 1);
		if (!c)
			break;
		jisTitle[i] = c;
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
		
		str[i] = c;
		ptr += 2;
	}
	memcpy(Info->sTitle, jisTitle, sizeof(jisTitle));
	
#if 0
	trimPriv(str);
	trimPriv(sstr);
#endif
	
	ptr = data + block * 8192 + 0x60; // icon palette data
	
	for (i = 0; i < 16; i++) {
		ptr += 2;
	}
	
	for (i = 0; i < Info->IconCount; i++) {
		
		ptr = data + block * 8192 + 128 + 128 * i; // icon data
		
		for (x = 0; x < 16 * 16; x++) {
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

Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI, CFStringRef pathToFile)
{
    Boolean ok = FALSE;
    @autoreleasepool {
		int i = 0, x;
		NSCharacterSet *theCharSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
		short freeBlocks = MAX_MEMCARD_BLOCKS;
		short memCount = 0;
		NSMutableArray *enNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableArray *jpNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableArray *memNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableArray *memIDs = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
		NSMutableDictionary *attr = (__bridge NSMutableDictionary*)attributes;
		NSString *path = (__bridge NSString*)pathToFile;
		const unsigned char* fileCData = NULL;
		NSData *fileData = [[NSData alloc] initWithContentsOfFile:path options:NSDataReadingMappedIfSafe error:NULL];
		if (!fileData) {
			return FALSE;
		}
		fileCData = [fileData bytes];
		if ([fileData length] == MCD_SIZE + 64)
			fileCData += 64;
		else if([fileData length] == MCD_SIZE + 3904)
			fileCData += 3904;
		else if ([fileData length] != MCD_SIZE)
			return FALSE;

		while (i < MAX_MEMCARD_BLOCKS) {
			x = 1;
			McdBlock memBlock;
			GetSoloBlockInfo((unsigned char *)fileCData, i + 1, &memBlock);
			
			if (MemBlockFlag(memBlock.Flags) == memFlagFree) {
				//Free space: ignore
				i++;
				continue;
			}
			do {
				McdBlock tmpBlock;
				GetSoloBlockInfo((unsigned char *)fileCData, i + x + 1, &tmpBlock);
				if ((tmpBlock.Flags & 0x3) == 0x3) {
					x++;
					break;
				} else if ((tmpBlock.Flags & 0x2) == 0x2) {
					x++;
				} else {
					break;
				}
			} while (i + x - 1 < MAX_MEMCARD_BLOCKS);
			// Ignore deleted blocks
			if (MemBlockFlag(memBlock.Flags) == memFlagDeleted) {
				continue;
			}
			memCount++;
			i += x;
			freeBlocks -= x;
			NSString *enName = [@(memBlock.Title) stringByTrimmingCharactersInSet:theCharSet];
			NSString *jpName = [[NSString alloc] initWithCString:memBlock.sTitle encoding:NSShiftJISStringEncoding];
			NSString *memName = @(memBlock.Name);
			NSString *memID = @(memBlock.ID);
			
			jpName = [jpName stringByTrimmingCharactersInSet:theCharSet];
			[enNames addObject:enName];
			[jpNames addObject:jpName];
			[memNames addObject:memName];
			[memIDs addObject:memID];
		}
		
		attr[kPCSXRSaveNames] = @{@"en": [enNames copy],
								  @"ja": [jpNames copy]};
		attr[kPCSXRMemCount] = @(memCount);
		attr[kPCSXRFreeBlocks] = @(freeBlocks);
		attr[kPCSXRMemNames] = [memNames copy];
		attr[kPCSXRMemIDs] = [memIDs copy];
		ok = TRUE;
	}
    
	// Return the status
    return ok;
}
