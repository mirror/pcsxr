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


// Taken from PEOPS SOFTGPU
unsigned long lUsedAddr[3];

char CheckForEndlessLoop(unsigned long laddr)
{
 if(laddr==lUsedAddr[1]) return 1;
 if(laddr==lUsedAddr[2]) return 1;

 if(laddr<lUsedAddr[0]) lUsedAddr[1]=laddr;
 else                   lUsedAddr[2]=laddr;
 lUsedAddr[0]=laddr;
 return 0;
}

long gpuDmaChain(unsigned long *baseAddrL, unsigned long addr)
{
 unsigned long dmaMem;
 unsigned char *baseAddrB;
 short count;unsigned int DMACommandCounter = 0;
 int size;

 size = 0;
 lUsedAddr[0]=lUsedAddr[1]=lUsedAddr[2]=0xffffff;

 baseAddrB = (unsigned char*) baseAddrL;

 do
 {
	 // Only Zinc = 1024
   //if(iGPUHeight==512) addr&=0x1FFFFC;

	 addr&=0x1FFFFC;
   if(DMACommandCounter++ > 2000000) break;
   if(CheckForEndlessLoop(addr)) break;

   count = baseAddrB[addr+3];
	 size += 4;
	 size += count;

   dmaMem=addr+4;

   //if(count>0) GPUwriteDataMem(&baseAddrL[dmaMem>>2],count);

   addr = baseAddrL[addr>>2]&0xffffff;
  }
 while (addr != 0xffffff);


 return size;
}


int gpuReadStatus()
{
	int hard;


	// GPU plugin
	hard = GPU_readStatus();


#if 0
	// ePSXe 1.7.0 - Chrono Cross interlace hack
	if (hard & 0x400000) {
		switch (Config.PsxType) {
			case PSX_TYPE_NTSC:
				if (hSyncCount > 262 - 240) hard ^= 0x80000000;
				break;

			case PSX_TYPE_PAL:
				if (hSyncCount > 312 - 256) hard ^= 0x80000000;
				break;
		}
	}
#endif


	// NOTE:
	// Backup option when plugins fail to simulate 'busy gpu'

	// TODO:
	// Check this with
	// - Hot wheels turbo racing
	// - Dukes of Hazzard

#if 1
	if( HW_DMA2_CHCR & 0x01000000 )
	{
		hard &= ~GPUSTATUS_IDLE;
		hard &= ~GPUSTATUS_READYFORCOMMANDS;
	}
#endif


	// Gameshark Lite - wants to see VRAM busy
	// - Must enable GPU 'Fake Busy States' hack
	if( (hard & GPUSTATUS_IDLE) == 0 )
		hard &= ~GPUSTATUS_READYFORVRAM;

	return hard;
}



void psxDma2(u32 madr, u32 bcr, u32 chcr) { // GPU
	u32 *ptr;
	u32 size;

	switch(chcr) {
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
			size = (bcr >> 16) * (bcr & 0xffff);
			GPU_readDataMem(ptr, size);
			psxCpu->Clear(madr, size);

			GPUDMA_INT( size / 4 );
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
			size = (bcr >> 16) * (bcr & 0xffff);
			GPU_writeDataMem(ptr, size);

			GPUDMA_INT( size / 4 );
			return;



		case 0x01000401: // dma chain
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 2 - GPU dma chain *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			GPU_dmaChain((u32 *)psxM, madr & 0x1fffff);

			// FIXME!! GPU DMA chain walking
			GPUDMA_INT( 0x4000 / 4 );
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
	HW_DMA2_CHCR &= SWAP32(~0x01000000);
	DMA_INTERRUPT(2);
}
