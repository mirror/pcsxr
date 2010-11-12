/*  Copyright (c) 2010, shalma.
 *  Portions Copyright (c) 2002, Pete Bernert.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include "psxhw.h"
#include "gpu.h"
#include "psxdma.h"

extern unsigned int hSyncCount;

#define GPUSTATUS_ODDLINES            0x80000000
#define GPUSTATUS_DMABITS             0x60000000 // Two bits
#define GPUSTATUS_READYFORCOMMANDS    0x10000000
#define GPUSTATUS_READYFORVRAM        0x08000000
#define GPUSTATUS_IDLE                0x04000000

#define GPUSTATUS_DISPLAYDISABLED     0x00800000
#define GPUSTATUS_INTERLACED          0x00400000
#define GPUSTATUS_RGB24               0x00200000
#define GPUSTATUS_PAL                 0x00100000
#define GPUSTATUS_DOUBLEHEIGHT        0x00080000
#define GPUSTATUS_WIDTHBITS           0x00070000 // Three bits
#define GPUSTATUS_MASKENABLED         0x00001000
#define GPUSTATUS_MASKDRAWN           0x00000800
#define GPUSTATUS_DRAWINGALLOWED      0x00000400
#define GPUSTATUS_DITHER              0x00000200


static __inline int prim__( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain ?? - unknown command\n", addr );
#endif

	return 0;
}


static __inline int prim00( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 00 - reset\n", addr );
#endif

	return 0;
}


static __inline int prim01( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 01 - clear cache\n", addr );
#endif

	return 0;
}


static __inline int prim02( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 02 - clear screen [#2]\n", addr );
#endif

	return 2;
}


static __inline int prim03( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 03 - enable display [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim04( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 04 - dma setup [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim05( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 05 - start display area [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim06( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 06 - h-display area [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim07( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 07 - v-display area [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim08( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 08 - display mode [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim10( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 10 - gpu info [#???]\n", addr );
#endif

	return 0;
}


static __inline int prim20( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 20 - 3-poly mono [#3]\n", addr );
#endif

	return 3;
}


static __inline int prim24( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 24 - 3-poly texture [#6]\n", addr );
#endif

	return 6;
}


static __inline int prim28( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 28 - 4-poly mono [#4]\n", addr );
#endif

	return 4;
}


static __inline int prim2C( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 2c - 4-poly texture [#8]\n", addr );
#endif

	return 8;
}


static __inline int prim30( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 30 - Gouraud tri [#5]\n", addr );
#endif

	return 5;
}


static __inline int prim34( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 34 - Gouraud tri texture [#8]\n", addr );
#endif

	return 8;
}


static __inline int prim38( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 38 - Gouraud quad [#7]\n", addr );
#endif

	return 7;
}


static __inline int prim3C( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 3c - Gouraud quad texture [#11]\n", addr );
#endif

	return 11;
}


static __inline int prim40( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 40 - mono 2-line [#2]\n", addr );
#endif

	return 2;
}


static __inline int prim48( u32 addr )
{
	int lcv;

#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 48 - mono 3-line [#_#]\n", addr );
#endif

	lcv = 0;

	while( psxMu32( addr ) != 0x55555555 )
	{
		addr += 4;
		lcv++;
	}

	return lcv;
}


static __inline int prim4C( u32 addr )
{
	int lcv;

#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 4c - mono 4-line [#_#]\n", addr );
#endif

	lcv = 0;

	while( psxMu32( addr ) != 0x55555555 )
	{
		addr += 4;
		lcv++;
	}

	return lcv;
}


static __inline int prim50( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 50 - 2-point gradated [#3]\n", addr );
#endif

	return 3;
}


static __inline int prim58( u32 addr )
{
	int lcv;

#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 58 - 3-point gradated [#__#]\n", addr );
#endif

	lcv = 0;

	while( psxMu32( addr ) != 0x55555555 )
	{
		addr += 4;
		lcv++;
	}

	return lcv;
}


static __inline int prim5C( u32 addr )
{
	int lcv;

#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 5c - 4-point gradated [#__#]\n", addr );
#endif

	lcv = 0;

	while( psxMu32( addr ) != 0x55555555 )
	{
		addr += 4;
		lcv++;
	}

	return lcv;
}


static __inline int prim60( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 60 - rect [#2]\n", addr );
#endif

	return 2;
}


static __inline int prim64( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 64 - sprite [#3]\n", addr );
#endif

	return 3;
}


static __inline int prim68( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 68 - dot [#1]\n", addr );
#endif

	return 1;
}


static __inline int prim70( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 70 - 8x8 rectangle [#1]\n", addr );
#endif

	return 1;
}


static __inline int prim74( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 74 - 8x8 sprite [#2]\n", addr );
#endif

	return 2;
}


static __inline int prim78( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 78 - 16x16 rectangle [#1]\n", addr );
#endif

	return 1;
}


static __inline int prim7C( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 7c - 16x16 sprite [#2]\n", addr );
#endif

	return 2;
}


static __inline int prim80( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain 80 - move image [#3]\n", addr );
#endif

	return 3;
}


static __inline int primA0( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain a0 - send image to [#2+__#]\n", addr );
#endif

	// count transfer bytes 2 + (h*w)
	return 2+0;
}


static __inline int primC0( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain c0 - copy image from [#2+__#]\n", addr );
#endif

	// count transfer bytes 2 + (h*w)
	return 2+0;
}


static __inline int primE1( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain E1 - draw mode setting\n", addr );
#endif

	return 0;
}


static __inline int primE2( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain E2 - texture window setting\n", addr );
#endif

	return 0;
}


static __inline int primE3( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain E3 - set draw area top left\n", addr );
#endif

	return 0;
}


static __inline int primE4( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain E4 - set draw area bottom right\n", addr );
#endif

	return 0;
}


static __inline int primE5( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain E5 - drawing offset\n", addr );
#endif

	return 0;
}


static __inline int primE6( u32 addr )
{
#ifdef CHAIN_LOG
	PSXDMA_LOG( "%X @ Chain E6 - mask setting\n", addr );
#endif

	return 0;
}



// Agemo's idea for chain unrolling
int (*gpuPrim[0x100])( u32 addr ) = 
{
	//  00      01      02      03      04       05     06      07
	prim00, prim01, prim02, prim03, prim04, prim05, prim06, prim07, //00-07
	prim08, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //08-0F
	prim10, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //10-17
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //18-1F
	prim20, prim20, prim20, prim20, prim24, prim24, prim24, prim24, //20-27
	prim28, prim28, prim28, prim28, prim2C, prim2C, prim2C, prim2C, //28-2F
	prim30, prim30, prim30, prim30, prim34, prim34, prim34, prim34, //30-37
	prim38, prim38, prim38, prim38, prim3C, prim3C, prim3C, prim3C, //38-3F
	prim40, prim40, prim40, prim40, prim__, prim__, prim__, prim__, //40-47
	prim48, prim48, prim48, prim48, prim4C, prim4C, prim4C, prim4C, //48-4F
	prim50, prim50, prim50, prim50, prim__, prim__, prim__, prim__, //50-57
	prim58, prim58, prim58, prim58, prim5C, prim5C, prim5C, prim5C, //58-5F
	prim60, prim60, prim60, prim60, prim64, prim64, prim64, prim64, //60-67
	prim68, prim68, prim68, prim68, prim__, prim__, prim__, prim__, //68-6F
	prim70, prim70, prim70, prim70, prim74, prim74, prim74, prim74, //70-77
	prim78, prim78, prim78, prim78, prim7C, prim7C, prim7C, prim7C, //78-7F
	prim80, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //80-87
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //88-8F
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //90-97
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //98-9F
	primA0, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //A0-A7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //A8-AF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //B0-B7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //B8-BF
	primC0, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //C0-C7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //C8-CF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //D0-D7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //D8-DF
	prim__, primE1, primE2, primE3, primE4, primE5, primE6, prim__, //E0-E7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //E8-EF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //F0-F7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__  //F8-FF
};
	

u32 chain_ptr_addr;
u32 chain_ptr_mem;
u8 chain_infinite_loop;


/*
DMA2 Chain slicing
	
Let GPU run x bytes. Then let game edit linked lists.


TODO:
- Monitor chain_ptr_addr for r/w
- Find better way to do sync timing (speed hit)


Einhander: 10-
- art gallery images + corruption
- note: pretty picky about timing

Vampire Hunter D: 5-
- opening menus (perfect - zero black screen flicker)
- note: really picky about timing
*/

#define DMA2_CHAIN_PAUSE 5



// Taken from PEOPS SOFTGPU
u32 lUsedAddr[3];

static inline boolean CheckForEndlessLoop(u32 laddr) {
	if (laddr == lUsedAddr[1]) return TRUE;
	if (laddr == lUsedAddr[2]) return TRUE;

	if (laddr < lUsedAddr[0]) lUsedAddr[1] = laddr;
	else lUsedAddr[2] = laddr;

	lUsedAddr[0] = laddr;

	return FALSE;
}

static u32 gpuDmaChainSize(u32 addr) {
	u32 size;
	u32 DMACommandCounter = 0;

	lUsedAddr[0] = lUsedAddr[1] = lUsedAddr[2] = 0xffffff;

	// initial linked list ptr (word)
	size = 1;
	chain_infinite_loop = 0;

	do {
		addr &= 0x1ffffc;

		DMACommandCounter = size;
		if (DMACommandCounter >= DMA2_CHAIN_PAUSE) break;


		//if (DMACommandCounter++ > 2000000) break;
		if (CheckForEndlessLoop(addr))
		{
			chain_infinite_loop = 1;
			break;
		}


		// # 32-bit blocks to transfer
		size += psxMu8( addr + 3 );


		if( psxMu8( addr + 3 ) != 0 ) {
			u32 lcv, stop;

			lcv = addr + 4;
			stop = addr + 4 + 4 * psxMu8( addr + 3 );


			while( lcv < stop )
			{
				u32 code;

				code = psxMu8( lcv+3 );
				
				lcv += 4;

				
				/*
				Check for frame buffer transfers (affects GPU dma timing)
				
				Final Fantasy 4: supercharge to avoid slowdown
				- $80: ~1/4 * 1x = slowest possible
				- $80: ~1/64 * 1x = fastest possible
				*/
				if( code == 0x80 )
				{
					s16 width, height;

					width = (psxMu32( lcv + 4*3 ) >> 0) & 0xffff;
					height = (psxMu32( lcv + 4*3 ) >> 16) & 0xffff;

					
					// PEOPS - check width, height
					if( width < 0 ) break;
					if( height < 0 ) break;


					size += (width * height) / 32;
				}
				else if( code == 0xa0 )
				{}
				else if( code == 0xc0 )
				{}


				lcv += 4 * gpuPrim[code & 0xff]( lcv );
			}
		}

		
		// next 32-bit pointer
		addr = psxMu32( addr & ~0x3 ) & 0xffffff;
		size += 1;
	} while (addr != 0xffffff);


	chain_ptr_addr = addr;

	if( !chain_infinite_loop && addr != 0xffffff )
	{
		// save data at stop ptr
		chain_ptr_mem = psxMu32( addr );

		// insert stop ptr
		psxMu32ref( addr ) = 0x00ffffff;
	}

	
	return size;
}

int gpuReadStatus() {
	int hard;


	// GPU plugin
	hard = GPU_readStatus();


	// Gameshark Lite - wants to see VRAM busy
	// - Must enable GPU 'Fake Busy States' hack
	if( (hard & GPUSTATUS_IDLE) == 0 )
		hard &= ~GPUSTATUS_READYFORVRAM;

	return hard;
}

void psxDma2(u32 madr, u32 bcr, u32 chcr) { // GPU
	u32 *ptr;
	u32 size;

	switch (chcr) {
		case 0x01000200: // vram2mem
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA2 GPU - vram2mem *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			ptr = (u32 *)PSXM(madr);
			if (ptr == NULL) {
#ifdef CPU_LOG
				CPU_LOG("*** DMA2 GPU - vram2mem *** NULL Pointer!!!\n");
#endif
				break;
			}
			// BA blocks * BS words (word = 32-bits)
			size = (bcr >> 16) * (bcr & 0xffff);
			GPU_readDataMem(ptr, size);
			psxCpu->Clear(madr, size);

			// already 32-bit word size ((size * 4) / 4)
			GPUDMA_INT(size);
			return;

		case 0x01000201: // mem2vram
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 2 - GPU mem2vram *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			ptr = (u32 *)PSXM(madr);
			if (ptr == NULL) {
#ifdef CPU_LOG
				CPU_LOG("*** DMA2 GPU - mem2vram *** NULL Pointer!!!\n");
#endif
				break;
			}
			// BA blocks * BS words (word = 32-bits)
			size = (bcr >> 16) * (bcr & 0xffff);
			GPU_writeDataMem(ptr, size);

			// already 32-bit word size ((size * 4) / 4)
			GPUDMA_INT(size);
			return;

		case 0x01000401: // dma chain
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 2 - GPU dma chain *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif

			// Tekken 3 - dma2 overwrite while still running (infinite loop)
			if( !chain_infinite_loop && chain_ptr_addr != 0xffffff )
			{
				// put back old value first
				psxMu32ref( chain_ptr_addr ) = chain_ptr_mem;
			}

			size = gpuDmaChainSize(madr);
			GPU_dmaChain((u32 *)psxM, madr & 0x1fffff);
			
			GPUDMA_INT(size);
			return;

#ifdef PSXDMA_LOG
		default:
			PSXDMA_LOG("*** DMA 2 - GPU unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
			break;
#endif
	}

	HW_DMA2_CHCR &= SWAP32(~0x01000000);
	DMA_INTERRUPT(2);
}

void gpuInterrupt() {
	/*
	GPU processing during DMA2 chains

	1 - never updates linked list (99/100 games)
	
	2 - updates linked list -before- GPU gets to it
	- Einhander: fixes art gallery images + post-gallery corruption (2+ cycles / 4 bytes)
	- Vampire Hunter D: shows title screen (3+ cycles / 4 bytes)

	3 - updates linked list -after- GPU reads it, -before- chain finishes
	- Skullmonkeys: fails to show menus
	*/

	if( HW_DMA2_CHCR == 0x01000401 )
	{
		u32 size,addr;


		// check valid data left
		if( !chain_infinite_loop && chain_ptr_addr != 0xffffff )
		{
			// put back old value first
			psxMu32ref( chain_ptr_addr ) = chain_ptr_mem;


			addr = chain_ptr_addr;
			size = gpuDmaChainSize( addr );
			
			GPU_dmaChain((u32 *)psxM, addr);


			GPUDMA_INT(size);

			// continue chain
			return;
		}
	}


	HW_DMA2_CHCR &= SWAP32(~0x01000000);
	DMA_INTERRUPT(2);
}
