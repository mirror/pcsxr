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
#define kPCSXRSaveNames		@"com_codeplex_pcsxr_memcard_savenames"
#define kPCSXRMemCount		@"com_codeplex_pcsxr_memcard_memcount"
#define kPCSXRFreeBlocks	@"com_codeplex_pcsxr_memcard_freeblocks"
#define kPCSXRMemNames		@"com_codeplex_pcsxr_memcard_memnames"
#define kPCSXRMemIDs		@"com_codeplex_pcsxr_memcard_memids"

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
	PCSXRMemFlagDeleted = 0,
	PCSXRMemFlagFree,
	PCSXRMemFlagUsed,
	PCSXRMemFlagLink,
	PCSXRMemFlagEndLink
};

static void GetSoloBlockInfo(unsigned char *data, int block, McdBlock *Info)
{
	unsigned char *ptr = data + block * 8192 + 2;
	unsigned char *str = Info->Title;
	unsigned char *sstr = Info->sTitle;
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
	
	ptr = data + block * 128;
	
	Info->Flags = *ptr;
	
	ptr += 0xa;
	strlcpy(Info->ID, ptr, 13);
	ptr += 12;
	strlcpy(Info->Name, ptr, 17);
}

static inline PCSXRMemFlags MemBlockFlag(unsigned char blockFlags)
{
	if ((blockFlags & 0xF0) == 0xA0) {
		if ((blockFlags & 0xF) >= 1 && (blockFlags & 0xF) <= 3)
			return PCSXRMemFlagDeleted;
		else
			return PCSXRMemFlagFree;
	} else if ((blockFlags & 0xF0) == 0x50) {
		if ((blockFlags & 0xF) == 0x1)
			return PCSXRMemFlagUsed;
		else if ((blockFlags & 0xF) == 0x2)
			return PCSXRMemFlagLink;
		else if ((blockFlags & 0xF) == 0x3)
			return PCSXRMemFlagEndLink;
	} else
		return PCSXRMemFlagFree;
	
	//Xcode complains unless we do this...
	//NSLog(@"Unknown flag %x", blockFlags);
	return PCSXRMemFlagFree;
}

Boolean GetMetadataForFile(void *thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI, CFStringRef pathToFile)
{
    Boolean ok = FALSE;
    @autoreleasepool {
		int i = 0, x;
		NSCharacterSet *theCharSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
		short freeBlocks = MAX_MEMCARD_BLOCKS;
		short memCount = 0;
		//NSMutableArray *enNames = [[NSMutableArray alloc] initWithCapacity:MAX_MEMCARD_BLOCKS];
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
			//NSString *enName;
			NSString *jpName;
			NSString *memName;
			NSString *memID;

			GetSoloBlockInfo((unsigned char *)fileCData, i + 1, &memBlock);
			
			if (MemBlockFlag(memBlock.Flags) == PCSXRMemFlagFree) {
				//Free space: ignore
				i++;
				continue;
			}
			while (i + x < MAX_MEMCARD_BLOCKS)  {
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
			};
			i += x;
			// Ignore deleted blocks
			if (MemBlockFlag(memBlock.Flags) == PCSXRMemFlagDeleted) {
				continue;
			}
			memCount++;
			freeBlocks -= x;
			jpName = [[NSString alloc] initWithCString:memBlock.sTitle encoding:NSShiftJISStringEncoding];
			jpName = [jpName stringByTrimmingCharactersInSet:theCharSet];
			memName = @(memBlock.Name);
			memID = @(memBlock.ID);
			
			[jpNames addObject:jpName];
			[memNames addObject:memName];
			[memIDs addObject:memID];
		}
		
		attr[kPCSXRSaveNames] = jpNames;
		attr[kPCSXRMemCount] = @(memCount);
		attr[kPCSXRFreeBlocks] = @(freeBlocks);
		attr[kPCSXRMemNames] = memNames;
		attr[kPCSXRMemIDs] = memIDs;
		ok = TRUE;
	}
    
	// Return the status
	return ok;
}
