#include "stdafx.h"
#define _IN_OSS
#include "externals.h"

#if !defined (USEALSA) && !defined (USEOSS) && !defined (USEPULSEAUDIO)

#warning "Using NULL sound output..."

// SETUP SOUND
void SetupSound(void)
{
}

// REMOVE SOUND
void RemoveSound(void)
{
}

// GET BYTES BUFFERED
unsigned long SoundGetBytesBuffered(void)
{
  return 0;
}

// FEED SOUND DATA
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
}

#endif
