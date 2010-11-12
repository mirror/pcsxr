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


u32 chain_ptr_addr;
u32 chain_ptr_mem;
u8 chain_infinite_loop;

/*
DMA2 Chain slicing
	
Let GPU run x bytes. Then let game edit linked lists.


TODO:
Have memory checks use chain_ptr_mem if that address read/write


Einhander: 4000- (+ gpu chain multiplier ~2)
- art gallery images

Vampire Hunter D: 4000- (+ gpu chain multiplier ~2.5+)
- opening menu visible
*/

#define DMA2_CHAIN_PAUSE 4000


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

			size = gpuDmaChainSize(madr);
			GPU_dmaChain((u32 *)psxM, madr & 0x1fffff);
			
			// HACK: Vampire Hunter D (title screen)
			GPUDMA_INT(size * 2.5);
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


#ifdef PSXDMA_LOG
		PSXDMA_LOG( "dma2 chain check %X [%X]\n", chain_ptr_addr, psxMu32(chain_ptr_addr) );
#endif


		// check valid data left
		if( !chain_infinite_loop && chain_ptr_addr != 0xffffff )
		{
			// put back old value first
			psxMu32ref( chain_ptr_addr ) = chain_ptr_mem;


			addr = chain_ptr_addr;
			size = gpuDmaChainSize( addr );
			
			GPU_dmaChain((u32 *)psxM, addr);


			// HACK: Vampire Hunter D (title screen)
			GPUDMA_INT(size * 2.5);

			// continue chain
			return;
		}
	}


	HW_DMA2_CHCR &= SWAP32(~0x01000000);
	DMA_INTERRUPT(2);
}
