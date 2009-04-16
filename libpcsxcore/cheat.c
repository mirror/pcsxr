/*  Cheat Support for PCSX-Reloaded
 *
 *  Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"

#include "cheat.h"

Cheat *Cheats = NULL;
int NumCheats = 0;
static int NumCheatsAllocated = 0;

CheatCode *CheatCodes = NULL;
int NumCodes = 0;
static int NumCodesAllocated = 0;

#define ALLOC_INCREMENT		100

// cheat types
#define CHEAT_CONST8		0x30	/* 8-bit Constant Write */
#define CHEAT_CONST16		0x80	/* 16-bit Constant Write */
#define CHEAT_INC16			0x10	/* 16-bit Increment */
#define CHEAT_DEC16			0x11	/* 16-bit Decrement */
#define CHEAT_INC8			0x20	/* 8-bit Increment */
#define CHEAT_DEC8			0x21	/* 8-bit Decrement */
#define CHEAT_SLIDE			0x50	/* Slide Codes */
#define CHEAT_MEMCPY		0xC2	/* Memory Copy */

#define CHEAT_EQU8			0xE0	/* 8-bit Equal To */
#define CHEAT_NOTEQU8		0xE1	/* 8-bit Not Equal To */
#define CHEAT_LESSTHAN8		0xE2	/* 8-bit Less Than */
#define CHEAT_GREATERTHAN8  0xE3	/* 8-bit Greater Than */
#define CHEAT_EQU16			0xD0	/* 16-bit Equal To */
#define CHEAT_NOTEQU16		0xD1	/* 16-bit Not Equal To */
#define CHEAT_LESSTHAN16	0xD2	/* 16-bit Less Than */
#define CHEAT_GREATERTHAN16 0xD3	/* 16-bit Greater Than */

void ClearAllCheats() {
	int i;

	if (Cheats != NULL) {
		for (i = 0; i < NumCheats; i++) {
			free(Cheats[i].Descr);
		}
		free(Cheats);
	}

	Cheats = NULL;
	NumCheats = 0;
	NumCheatsAllocated = 0;

	if (CheatCodes != NULL) {
		free(CheatCodes);
	}

	CheatCodes = NULL;
	NumCodes = 0;
	NumCodesAllocated = 0;
}

// load cheats from the specific filename
void LoadCheats(const char *filename) {
	FILE				*fp;
	char				buf[256];
	int					count = 0;
	unsigned int		t1, t2;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		return;
	}

	ClearAllCheats();

	while (fgets(buf, 255, fp) != NULL) {
		buf[255] = '\0';
		trim(buf);

		// Skip comment or blank lines
		if (buf[0] == '#' || buf[0] == ';' || buf[0] == '/' || buf[0] == '\"' || buf[0] == '\0')
			continue;

		if (buf[0] == '[' && buf[strlen(buf) - 1] == ']') {
			if (NumCheats > 0)
				Cheats[NumCheats - 1].n = count;

			if (NumCheats >= NumCheatsAllocated) {
				NumCheatsAllocated += ALLOC_INCREMENT;

				if (Cheats == NULL) {
					assert(NumCheats == 0);
					assert(NumCheatsAllocated == ALLOC_INCREMENT);
					Cheats = (Cheat *)malloc(sizeof(Cheat) * NumCheatsAllocated);
				} else {
					Cheats = (Cheat *)realloc(Cheats, sizeof(Cheat) * NumCheatsAllocated);
				}
			}

			buf[strlen(buf) - 1] = '\0';
			count = 0;

			if (buf[1] == '*') {
				Cheats[NumCheats].Descr = strdup(buf + 2);
				Cheats[NumCheats].Enabled = 1;
			} else {
				Cheats[NumCheats].Descr = strdup(buf + 1);
				Cheats[NumCheats].Enabled = 0;
			}

			Cheats[NumCheats].First = NumCodes;

			NumCheats++;
			continue;
		}

		if (NumCheats <= 0)
			continue;

		if (NumCodes >= NumCodesAllocated) {
			NumCodesAllocated += ALLOC_INCREMENT;

			if (CheatCodes == NULL) {
				assert(NumCodes == 0);
				assert(NumCodesAllocated == ALLOC_INCREMENT);
				CheatCodes = (CheatCode *)malloc(sizeof(CheatCode) * NumCodesAllocated);
			} else {
				CheatCodes = (CheatCode *)realloc(CheatCodes, sizeof(CheatCode) * NumCodesAllocated);
			}
		}

		sscanf(buf, "%x %x", &t1, &t2);

		CheatCodes[NumCodes].Addr = t1;
		CheatCodes[NumCodes].Val = t2;

		NumCodes++;
		count++;
	}

	if (NumCheats > 0)
		Cheats[NumCheats - 1].n = count;

	fclose(fp);

	SysPrintf("Cheats loaded from: %s\n", filename);
}

// save all cheats to the specified filename
void SaveCheats(const char *filename) {
	FILE		*fp;
	int			i, j;

	fp = fopen(filename, "w");
	if (fp == NULL) {
		return;
	}

	for (i = 0; i < NumCheats; i++) {
		// write the description
		if (Cheats[i].Enabled)
			fprintf(fp, "[*%s]\n", Cheats[i].Descr);
		else
			fprintf(fp, "[%s]\n", Cheats[i].Descr);

		// write all cheat codes
		for (j = 0; j < Cheats[i].n; j++) {
			fprintf(fp, "%.8X %.4X\n",
				CheatCodes[Cheats[i].First + j].Addr,
				CheatCodes[Cheats[i].First + j].Val);
		}

		fprintf(fp, "\n");
	}

	fclose(fp);

	SysPrintf("Cheats saved to: %s\n", filename);
}

// apply all enabled cheats
void ApplyCheats() {
	int		i, j, k, endindex;

	for (i = 0; i < NumCheats; i++) {
		if (!Cheats[i].Enabled) {
			continue;
		}

		// process all cheat codes
		endindex = Cheats[i].First + Cheats[i].n;

		for (j = Cheats[i].First; j < endindex; j++) {
			u8		type = (uint8_t)(CheatCodes[j].Addr >> 24);
			u32		addr = (CheatCodes[j].Addr & 0x001FFFFF);
			u16		val = CheatCodes[j].Val;
			u32		taddr;

			switch (type) {
				case CHEAT_CONST8:
					psxMemWrite8(addr, (u8)val);
					break;

				case CHEAT_CONST16:
					psxMemWrite16(addr, (u16)val);
					break;

				case CHEAT_INC16:
					psxMemWrite16(addr, psxMemRead16(addr) + val);
					break;

				case CHEAT_DEC16:
					psxMemWrite16(addr, psxMemRead16(addr) - val);
					break;

				case CHEAT_INC8:
					psxMemWrite8(addr, psxMemRead8(addr) + (u8)val);
					break;

				case CHEAT_DEC8:
					psxMemWrite8(addr, psxMemRead8(addr) - (u8)val);
					break;

				case CHEAT_SLIDE:
					j++;
					if (j >= endindex)
						break;

					type = (uint8_t)(CheatCodes[j].Addr >> 24);
					taddr = (CheatCodes[j].Addr & 0x001FFFFF);
					val = CheatCodes[j].Val;

					if (type == CHEAT_CONST8) {
						for (k = 0; k < ((addr >> 8) & 0xFF); k++) {
							psxMemWrite8(taddr, (u8)val);
							taddr += (s8)(addr & 0xFF);
							val += (s8)(CheatCodes[j - 1].Val & 0xFF);
						}
					} else if (type == CHEAT_CONST16) {
						for (k = 0; k < ((addr >> 8) & 0xFF); k++) {
							psxMemWrite16(taddr, val);
							taddr += (s8)(addr & 0xFF);
							val += (s8)(CheatCodes[j - 1].Val & 0xFF);
						}
					}
					break;

				case CHEAT_MEMCPY:
					j++;
					if (j >= endindex)
						break;

					taddr = (CheatCodes[j].Addr & 0x001FFFFF);
					for (k = 0; k < val; k++) {
						psxMemWrite8(taddr + k, psxMemRead8(addr + k));
					}
					break;

				case CHEAT_EQU8:
					if (psxMemRead8(addr) != (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_NOTEQU8:
					if (psxMemRead8(addr) == (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_LESSTHAN8:
					if (psxMemRead8(addr) >= (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_GREATERTHAN8:
					if (psxMemRead8(addr) <= (u8)val)
						j++; // skip the next code
					break;

				case CHEAT_EQU16:
					if (psxMemRead16(addr) != val)
						j++; // skip the next code
					break;

				case CHEAT_NOTEQU16:
					if (psxMemRead16(addr) == val)
						j++; // skip the next code
					break;

				case CHEAT_LESSTHAN16:
					if (psxMemRead16(addr) >= val)
						j++; // skip the next code
					break;

				case CHEAT_GREATERTHAN16:
					if (psxMemRead16(addr) <= val)
						j++; // skip the next code
					break;
			}
		}
	}
}

int AddCheat(const char *descr, char *code) {
	int c = 1;
	char *p1, *p2;

	if (NumCheats >= NumCheatsAllocated) {
		NumCheatsAllocated += ALLOC_INCREMENT;

		if (Cheats == NULL) {
			assert(NumCheats == 0);
			assert(NumCheatsAllocated == ALLOC_INCREMENT);
			Cheats = (Cheat *)malloc(sizeof(Cheat) * NumCheatsAllocated);
		} else {
			Cheats = (Cheat *)realloc(Cheats, sizeof(Cheat) * NumCheatsAllocated);
		}
	}

	Cheats[NumCheats].Descr = strdup(descr[0] ? descr : _("(Untitled)"));
	Cheats[NumCheats].Enabled = 0;
	Cheats[NumCheats].First = NumCodes;
	Cheats[NumCheats].n = 0;

	p1 = code;
	p2 = code;

	while (c) {
		unsigned int t1, t2;

		while (*p2 != '\n' && *p2 != '\0')
			p2++;

		if (*p2 == '\0')
			c = 0;

		*p2 = '\0';
		p2++;

		t1 = 0;
		t2 = 0;
		sscanf(p1, "%x %x", &t1, &t2);

		if (t1 > 0x10000000) {
			if (NumCodes >= NumCodesAllocated) {
				NumCodesAllocated += ALLOC_INCREMENT;

				if (CheatCodes == NULL) {
					assert(NumCodes == 0);
					assert(NumCodesAllocated == ALLOC_INCREMENT);
					CheatCodes = (CheatCode *)malloc(sizeof(CheatCode) * NumCodesAllocated);
				} else {
					CheatCodes = (CheatCode *)realloc(CheatCodes, sizeof(CheatCode) * NumCodesAllocated);
				}
			}

			CheatCodes[NumCodes].Addr = t1;
			CheatCodes[NumCodes].Val = t2;
			NumCodes++;
			Cheats[NumCheats].n++;
		}

		p1 = p2;
	}

	if (Cheats[NumCheats].n == 0) {
		return -1;
	}

	NumCheats++;
	return 0;
}

void RemoveCheat(int index) {
	assert(index >= 0 && index < NumCheats);

	free(Cheats[index].Descr);

	while (index < NumCheats - 1) {
		Cheats[index] = Cheats[index + 1];
		index++;
	}

	NumCheats--;
}

int EditCheat(int index, const char *descr, char *code) {
	int c = 1;
	int prev = NumCodes;
	char *p1, *p2;

	assert(index >= 0 && index < NumCheats);

	p1 = code;
	p2 = code;

	while (c) {
		unsigned int t1, t2;

		while (*p2 != '\n' && *p2 != '\0')
			p2++;

		if (*p2 == '\0')
			c = 0;

		*p2 = '\0';
		p2++;

		t1 = 0;
		t2 = 0;
		sscanf(p1, "%x %x", &t1, &t2);

		if (t1 > 0x10000000) {
			if (NumCodes >= NumCodesAllocated) {
				NumCodesAllocated += ALLOC_INCREMENT;

				if (CheatCodes == NULL) {
					assert(NumCodes == 0);
					assert(NumCodesAllocated == ALLOC_INCREMENT);
					CheatCodes = (CheatCode *)malloc(sizeof(CheatCode) * NumCodesAllocated);
				} else {
					CheatCodes = (CheatCode *)realloc(CheatCodes, sizeof(CheatCode) * NumCodesAllocated);
				}
			}

			CheatCodes[NumCodes].Addr = t1;
			CheatCodes[NumCodes].Val = t2;
			NumCodes++;
		}

		p1 = p2;
	}

	if (NumCodes == prev) {
		return -1;
	}

	free(Cheats[index].Descr);
	Cheats[index].Descr = strdup(descr[0] ? descr : _("(Untitled)"));
	Cheats[index].First = prev;
	Cheats[index].n = NumCodes - prev;

	return 0;
}
