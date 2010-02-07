/***************************************************************************
		pulseaudiosimple.c  -  description
		     -------------------
begin                : Thu Feb 04 2010
copyright            : (C) 2010 by Tristin Celestin
email                : cetris1@umbc.edu
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

#ifdef USEPULSEAUDIOSIMPLE

#define _IN_OSS

#include "externals.h"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/timeval.h>

////////////////////////////////////////////////////////////////////////
// pulseaudio globals
////////////////////////////////////////////////////////////////////////

#define PLAYBACK_RATE 44100
#define LATENCY_IN_MS 20

static pa_simple *playback_stream;
static pa_sample_spec sample_specification;

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
     int error_number = 0;
     int buffer_length;
     pa_buffer_attr buffer_attributes;

     // Set sample specification ///////////////////////////////////////////////
     sample_specification.format = PA_SAMPLE_S16LE;
     sample_specification.rate = PLAYBACK_RATE;
     if (iDisStereo)
	  sample_specification.channels = 1;
     else
	  sample_specification.channels = 2;

     // Set buffer attributes //////////////////////////////////////////////////
     // See http://0pointer.de/lennart/projects/pulseaudio/doxygen/streams.html
     // for parameter explanations
     ///////////////////////////////////////////////////////////////////////////
     int mixlen = pa_usec_to_bytes(LATENCY_IN_MS * PA_USEC_PER_MSEC, &sample_specification);
     buffer_attributes.maxlength = -1;
     buffer_attributes.tlength = mixlen;
     buffer_attributes.prebuf = -1;
     buffer_attributes.minreq = -1;

     // Get connection from PulseAudio server
     playback_stream = pa_simple_new(NULL, "PCSX", PA_STREAM_PLAYBACK, NULL, "PCSX", &sample_specification, NULL, &buffer_attributes, &error_number);
     if (playback_stream == NULL)
     {
	  fprintf(stderr, "Failed to connect to Pulseaudio server with pa_simple_new(): %s\n", pa_strerror(error_number));
	  return;
     }
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{
     int error_number = 0;

     if (pa_simple_drain(playback_stream, &error_number) < 0)
	  fprintf(stderr, "Unable to drain audio: %s\n", pa_strerror (error_number));
     if (playback_stream)
	  pa_simple_free(playback_stream);
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
  return 0;
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////

void SoundFeedStreamData(unsigned char *pSound, long lBytes)
{
  int error_number = 0;

  if (pa_simple_write(playback_stream, pSound, (ssize_t)lBytes, &error_number) < 0)
    fprintf(stderr, "Could not write - pa_simple_write() failed: %s\n", pa_strerror(error_number));
}

#endif
