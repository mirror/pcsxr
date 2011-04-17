/***************************************************************************
                            dma.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "stdafx.h"

#define _IN_DMA

#include "externals.h"
#include "registers.h"

////////////////////////////////////////////////////////////////////////
// READ DMA (one value)
////////////////////////////////////////////////////////////////////////

unsigned short CALLBACK SPUreadDMA(void)
{
 unsigned short s=spuMem[spuAddr>>1];
 spuAddr+=2;
 if(spuAddr>0x7ffff) spuAddr=0;

 iSpuAsyncWait=0;

 return s;
}

////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

void CALLBACK SPUreadDMAMem(unsigned short * pusPSXMem,int iSize)
{
 int i;

 spuStat |= STAT_DATA_BUSY;

 for(i=0;i<iSize;i++)
  {
	 Check_IRQ( spuAddr, 0 );

		
	 *pusPSXMem++=spuMem[spuAddr>>1];                    // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr

	 // guess based on Vib Ribbon (below)
   if(spuAddr>0x7ffff) break;
  }

 iSpuAsyncWait=0;

 spuStat &= ~STAT_DATA_BUSY;
 spuStat &= ~STAT_DMA_NON;
 spuStat &= ~STAT_DMA_W;
 spuStat |= STAT_DMA_R;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// to investigate: do sound data updates by writedma affect spu
// irqs? Will an irq be triggered, if new data is written to
// the memory irq address?

////////////////////////////////////////////////////////////////////////
// WRITE DMA (one value)
////////////////////////////////////////////////////////////////////////
  
void CALLBACK SPUwriteDMA(unsigned short val)
{
 spuMem[spuAddr>>1] = val;                             // spu addr got by writeregister

 spuAddr+=2;                                           // inc spu addr
 if(spuAddr>0x7ffff) spuAddr=0;                        // wrap

 iSpuAsyncWait=0;
}

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////

void CALLBACK SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize)
{
 int i;

 spuStat |= STAT_DATA_BUSY;

 for(i=0;i<iSize;i++)
  {
	 Check_IRQ( spuAddr, 0 );

	 spuMem[spuAddr>>1] = *pusPSXMem++;                  // spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr

	 // Vib Ribbon - stop transfer (reverb playback)
   if(spuAddr>0x7ffff) break;
  }
 
 iSpuAsyncWait=0;


 spuStat &= ~STAT_DATA_BUSY;
 spuStat &= ~STAT_DMA_NON;
 spuStat &= ~STAT_DMA_R;
 spuStat |= STAT_DMA_W;
}

////////////////////////////////////////////////////////////////////////
