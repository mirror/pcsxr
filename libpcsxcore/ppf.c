/*  PPF/SBI Support for PCSX-Reloaded
 *  Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *  Copyright (c) 2010, shalma.
 *
 *  PPF code based on P.E.Op.S CDR Plugin by Pete Bernert.
 *  Copyright (c) 2002, Pete Bernert.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "psxcommon.h"
#include "ppf.h"
#include "cdrom.h"

typedef struct tagPPF_DATA {
	s32					addr;
	s32					pos;
	s32					anz;
	struct tagPPF_DATA	*pNext;
} PPF_DATA;

typedef struct tagPPF_CACHE {
	s32					addr;
	struct tagPPF_DATA	*pNext;
} PPF_CACHE;

static PPF_CACHE		*ppfCache = NULL;
static PPF_DATA			*ppfHead = NULL, *ppfLast = NULL;
static int				iPPFNum = 0;

// using a linked data list, and address array
static void FillPPFCache() {
	PPF_DATA		*p;
	PPF_CACHE		*pc;
	s32				lastaddr;

	p = ppfHead;
	lastaddr = -1;
	iPPFNum = 0;

	while (p != NULL) {
		if (p->addr != lastaddr) iPPFNum++;
		lastaddr = p->addr;
		p = p->pNext;
	}

	if (iPPFNum <= 0) return;

	pc = ppfCache = (PPF_CACHE *)malloc(iPPFNum * sizeof(PPF_CACHE));

	iPPFNum--;
	p = ppfHead;
	lastaddr = -1;

	while (p != NULL) {
		if (p->addr != lastaddr) {
			pc->addr = p->addr;
			pc->pNext = p;
			pc++;
		}
		lastaddr = p->addr;
		p = p->pNext;
	}
}

void FreePPFCache() {
	PPF_DATA *p = ppfHead;
	void *pn;

	while (p != NULL) {
		pn = p->pNext;
		free(p);
		p = (PPF_DATA *)pn;
	}
	ppfHead = NULL;
	ppfLast = NULL;

	if (ppfCache != NULL) free(ppfCache);
	ppfCache = NULL;
}

void CheckPPFCache(unsigned char *pB, unsigned char m, unsigned char s, unsigned char f) {
	PPF_CACHE *pcstart, *pcend, *pcpos;
	int addr = MSF2SECT(btoi(m), btoi(s), btoi(f)), pos, anz, start;

	if (ppfCache == NULL) return;

	pcstart = ppfCache;
	if (addr < pcstart->addr) return;
	pcend = ppfCache + iPPFNum;
	if (addr > pcend->addr) return;

	while (1) {
		if (addr == pcend->addr) { pcpos = pcend; break; }

		pcpos = pcstart + (pcend - pcstart) / 2;
		if (pcpos == pcstart) break;
		if (addr < pcpos->addr) {
			pcend = pcpos;
			continue;
		}
		if (addr > pcpos->addr) {
			pcstart = pcpos;
			continue;
		}
		break;
	}

	if (addr == pcpos->addr) {
		PPF_DATA *p = pcpos->pNext;
		while (p != NULL && p->addr == addr) {
			pos = p->pos - (CD_FRAMESIZE_RAW - DATA_SIZE);
			anz = p->anz;
			if (pos < 0) { start = -pos; pos = 0; anz -= start; }
			else start = 0;
			memcpy(pB + pos, (unsigned char *)(p + 1) + start, anz);
			p = p->pNext;
		}
	}
}

static void AddToPPF(s32 ladr, s32 pos, s32 anz, unsigned char *ppfmem) {
	if (ppfHead == NULL) {
		ppfHead = (PPF_DATA *)malloc(sizeof(PPF_DATA) + anz);
		ppfHead->addr = ladr;
		ppfHead->pNext = NULL;
		ppfHead->pos = pos;
		ppfHead->anz = anz;
		memcpy(ppfHead + 1, ppfmem, anz);
		iPPFNum = 1;
		ppfLast = ppfHead;
	} else {
		PPF_DATA *p = ppfHead;
		PPF_DATA *plast = NULL;
		PPF_DATA *padd;

		if (ladr > ppfLast->addr || (ladr == ppfLast->addr && pos > ppfLast->pos)) {
			p = NULL;
			plast = ppfLast;
		} else {
			while (p != NULL) {
				if (ladr < p->addr) break;
				if (ladr == p->addr) {
					while (p && ladr == p->addr && pos > p->pos) {
						plast = p;
						p = p->pNext;
					}
					break;
				}
				plast = p;
				p = p->pNext;
			}
		}

		padd = (PPF_DATA *)malloc(sizeof(PPF_DATA) + anz);
		padd->addr = ladr;
		padd->pNext = p;
		padd->pos = pos;
		padd->anz = anz;
		memcpy(padd + 1, ppfmem, anz);
		iPPFNum++;
		if (plast == NULL) ppfHead = padd;
		else plast->pNext = padd;

		if (padd->pNext == NULL) ppfLast = padd;
	}
}

void BuildPPFCache() {
	FILE			*ppffile;
	char			buffer[12];
	char			method, undo = 0, blockcheck = 0;
	int				dizlen = 0, dizyn;
	unsigned char	ppfmem[512];
	char			szPPF[MAXPATHLEN];
	int				count, seekpos, pos;
	u32				anz; // use 32-bit to avoid stupid overflows
	s32				ladr, off, anx;

	FreePPFCache();

    if (CdromId[0] == '\0') return;

	// Generate filename in the format of SLUS_123.45
	buffer[0] = toupper(CdromId[0]);
	buffer[1] = toupper(CdromId[1]);
	buffer[2] = toupper(CdromId[2]);
	buffer[3] = toupper(CdromId[3]);
	buffer[4] = '_';
	buffer[5] = CdromId[4];
	buffer[6] = CdromId[5];
	buffer[7] = CdromId[6];
	buffer[8] = '.';
	buffer[9] = CdromId[7];
	buffer[10] = CdromId[8];
	buffer[11] = '\0';

	sprintf(szPPF, "%s/%s", Config.PatchesDir, buffer);

	ppffile = fopen(szPPF, "rb");
	if (ppffile == NULL) return;

	memset(buffer, 0, 5);
	fread(buffer, 3, 1, ppffile);

	if (strcmp(buffer, "PPF") != 0) {
		SysPrintf(_("Invalid PPF patch: %s.\n"), szPPF);
		fclose(ppffile);
		return;
	}

	fseek(ppffile, 5, SEEK_SET);
	method = fgetc(ppffile);

	switch (method) {
		case 0: // ppf1
			fseek(ppffile, 0, SEEK_END);
			count = ftell(ppffile);
			count -= 56;
			seekpos = 56;
			break;

		case 1: // ppf2
			fseek(ppffile, -8, SEEK_END);

			memset(buffer, 0, 5);
			fread(buffer, 4, 1, ppffile);

			if (strcmp(".DIZ", buffer) != 0) {
				dizyn = 0;
			} else {
				fread(&dizlen, 4, 1, ppffile);
				dizlen = SWAP32(dizlen);
				dizyn = 1;
			}

			fseek(ppffile, 0, SEEK_END);
			count = ftell(ppffile);

			if (dizyn == 0) {
				count -= 1084;
				seekpos = 1084;
			} else {
				count -= 1084;
				count -= 38;
				count -= dizlen;
				seekpos = 1084;
			}
			break;

		case 2: // ppf3
			fseek(ppffile, 57, SEEK_SET);
			blockcheck = fgetc(ppffile);
			undo = fgetc(ppffile);

			fseek(ppffile, -6, SEEK_END);
			memset(buffer, 0, 5);
			fread(buffer, 4, 1, ppffile);
			dizlen = 0;

			if (strcmp(".DIZ", buffer) == 0) {
				fseek(ppffile, -2, SEEK_END);
				fread(&dizlen, 2, 1, ppffile);
				dizlen = SWAP32(dizlen);
				dizlen += 36;
			}

			fseek(ppffile, 0, SEEK_END);
			count = ftell(ppffile);
			count -= dizlen;

			if (blockcheck) {
				seekpos = 1084;
				count -= 1084;
			} else {
				seekpos = 60;
				count -= 60;
			}
			break;

		default:
			fclose(ppffile);
			SysPrintf(_("Unsupported PPF version (%d).\n"), method + 1);
			return;
	}

	// now do the data reading
	do {                                                
		fseek(ppffile, seekpos, SEEK_SET);
		fread(&pos, 4, 1, ppffile);
		pos = SWAP32(pos);

		if (method == 2) fread(buffer, 4, 1, ppffile); // skip 4 bytes on ppf3 (no int64 support here)

		anz = fgetc(ppffile);
		fread(ppfmem, anz, 1, ppffile);   

		ladr = pos / CD_FRAMESIZE_RAW;
		off = pos % CD_FRAMESIZE_RAW;

		if (off + anz > CD_FRAMESIZE_RAW) {
			anx = off + anz - CD_FRAMESIZE_RAW;
			anz -= (unsigned char)anx;
			AddToPPF(ladr + 1, 0, anx, &ppfmem[anz]);
		}

		AddToPPF(ladr, off, anz, ppfmem); // add to link list

		if (method == 2) {
			if (undo) anz += anz;
			anz += 4;
		}

		seekpos = seekpos + 5 + anz;
		count = count - 5 - anz;
	} while (count != 0); // loop til end

	fclose(ppffile);

	FillPPFCache(); // build address array

	SysPrintf(_("Loaded PPF %d.0 patch: %s.\n"), method + 1, szPPF);
}

// redump.org SBI files
static u8 sbitime[256][3], sbicount;

int LoadSBI(const char *filename) {
	FILE *sbihandle;
	char buffer[16], sbifile[MAXPATHLEN];

	if (filename == NULL) {
		if (CdromId[0] == '\0') return -1;

		// Generate filename in the format of SLUS_123.45.sbi
		buffer[0] = toupper(CdromId[0]);
		buffer[1] = toupper(CdromId[1]);
		buffer[2] = toupper(CdromId[2]);
		buffer[3] = toupper(CdromId[3]);
		buffer[4] = '_';
		buffer[5] = CdromId[4];
		buffer[6] = CdromId[5];
		buffer[7] = CdromId[6];
		buffer[8] = '.';
		buffer[9] = CdromId[7];
		buffer[10] = CdromId[8];
		buffer[11] = '.';
		buffer[12] = 's';
		buffer[13] = 'b';
		buffer[14] = 'i';
		buffer[15] = '\0';

		sprintf(sbifile, "%s%s", Config.PatchesDir, buffer);
		filename = sbifile;
	}

	sbihandle = fopen(filename, "rb");
	if (sbihandle == NULL) return -1;

	// init
	sbicount = 0;

	// 4-byte SBI header
	fread(buffer, 1, 4, sbihandle);
	while (!feof(sbihandle)) {
		fread(sbitime[sbicount++], 1, 3, sbihandle);
		fread(buffer, 1, 11, sbihandle);
	}

	fclose(sbihandle);

	SysPrintf(_("Loaded SBI file: %s.\n"), filename);

	return 0;
}

boolean CheckSBI(const u8 *time) {
	int lcv;

	// both BCD format
	for (lcv = 0; lcv < sbicount; lcv++) {
		if (time[0] == sbitime[lcv][0] && 
				time[1] == sbitime[lcv][1] && 
				time[2] == sbitime[lcv][2])
			return TRUE;
	}

	return FALSE;
}

void UnloadSBI(void) {
	sbicount = 0;
}
