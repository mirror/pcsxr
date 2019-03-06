/***************************************************************************
                            alsa.c  -  description
                             -------------------
    begin                : Sat Mar 01 2003
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

#define _IN_OSS

#include "externals.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

static snd_pcm_t *handle = NULL;
static snd_pcm_uframes_t buffer_size;

// SETUP SOUND
void SetupSound(void)
{
 snd_pcm_hw_params_t *hwparams;
 snd_pcm_status_t *status;
 unsigned int pspeed;
 int pchannels;
 int format;
 unsigned int buffer_time = 100000;
 unsigned int period_time = buffer_time / 4;
 int err;

 if (iDisStereo) pchannels = 1;
 else pchannels=2;

 pspeed = 44100;
 format = SND_PCM_FORMAT_S16;

 if ((err = snd_pcm_open(&handle, "default", 
                      SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
  {
   printf("Audio open error: %s\n", snd_strerror(err));
   return;
  }

 if((err = snd_pcm_nonblock(handle, 0))<0)
  {
   printf("Can't set blocking moded: %s\n", snd_strerror(err));
   return;
  }

 snd_pcm_hw_params_alloca(&hwparams);

 if((err=snd_pcm_hw_params_any(handle, hwparams))<0)
  {
   printf("Broken configuration for this PCM: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED))<0)
  {
   printf("Access type not available: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params_set_format(handle, hwparams, format))<0)
  {
   printf("Sample format not available: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params_set_channels(handle, hwparams, pchannels))<0)
  {
   printf("Channels count not available: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params_set_rate_near(handle, hwparams, &pspeed, 0))<0)
  {
   printf("Rate not available: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, 0))<0)
  {
   printf("Buffer time error: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0))<0)
  {
   printf("Period time error: %s\n", snd_strerror(err));
   return;
  }

 if((err=snd_pcm_hw_params(handle, hwparams))<0)
  {
   printf("Unable to install hw params: %s\n", snd_strerror(err));
   return;
  }

 snd_pcm_status_alloca(&status);
 if((err=snd_pcm_status(handle, status))<0)
  {
   printf("Unable to get status: %s\n", snd_strerror(err));
   return;
  }

 buffer_size = snd_pcm_status_get_avail(status);
}

// REMOVE SOUND
void RemoveSound(void)
{
 if(handle != NULL)
  {
   snd_pcm_drop(handle);
   snd_pcm_close(handle);
   handle = NULL;
  }
}

// GET BYTES BUFFERED
unsigned long SoundGetBytesBuffered(void)
{
 unsigned long l;

 if (handle == NULL)                                 // failed to open?
  return SOUNDSIZE;
 l = snd_pcm_avail(handle);
 if (l < 0) return 0;
 if (l < buffer_size / 2)                            // can we write in at least the half of fragments?
      l = SOUNDSIZE;                                 // -> no? wait
 else l = 0;                                         // -> else go on

 return l;
}

// FEED SOUND DATA
void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
 if (handle == NULL) return;

 if (snd_pcm_state(handle) == SND_PCM_STATE_XRUN)
  snd_pcm_prepare(handle);
 snd_pcm_writei(handle,pSound,
                iDisStereo ? lBytes / 2 : lBytes / 4);
}
