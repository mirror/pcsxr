/***************************************************************************
		pulseaudio.c  -  description
		     -------------------
begin                : Thu Feb 04 2010
copyright            : (C) 2010 by Tristin Celestin
email                : cetris1@umbc.edu
comment              : Much of this was taken from pulseaudio.cpp (authored
                       by slouken) in SDL 
                       (http://lists.libsdl.org/pipermail/svn-libsdl.org/2009-September/001809.html()
                       and in pulseaudio.cpp (authored by RedDwarf) in bsnes
                       (http://www.byuu.org/bsnes)
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

#ifdef USEPULSEAUDIO

#define _IN_OSS

#include "externals.h"
#include <pulse/pulseaudio.h>

////////////////////////////////////////////////////////////////////////
// declarations for pulseaudio callbacks
////////////////////////////////////////////////////////////////////////
void connection_state_callback (pa_context *context, const char *name, pa_proplist *property_list, void *user_data);

////////////////////////////////////////////////////////////////////////
// pulseaudio structs
////////////////////////////////////////////////////////////////////////

typedef struct {
     pa_mainloop *mainloop;
     pa_context *context;
     pa_mainloop_api *api;
     pa_stream *stream;
     pa_sample_spec spec;
     pa_buffer_attr buffer_attr;
     int first;
} Device;

typedef struct {
     unsigned int frequency;
     unsigned int latency_in_msec;
} Settings;

////////////////////////////////////////////////////////////////////////
// pulseaudio globals
////////////////////////////////////////////////////////////////////////

static Device device = {
     .mainloop = NULL,
     .api = NULL,
     .context = NULL,
     .stream = NULL
};

static Settings settings = {
     .frequency = 44100,
     .latency_in_msec = 40,
};

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound (void)
{
     int error_number;

     // Acquire mainloop ///////////////////////////////////////////////////////
     device.mainloop = pa_mainloop_new ();
     if (device.mainloop == NULL)
     {
	  fprintf (stderr, "Could not acquire PulseAudio main loop\n");
	  return;
     }

     // Acquire context ////////////////////////////////////////////////////////
     device.api = pa_mainloop_get_api (device.mainloop);
     device.context = pa_context_new (device.api, "PCSX");
     if (device.context == NULL)
     {
	  fprintf (stderr, "Could not acquire PulseAudio device context\n");
	  return;
     }

     // Connect to PulseAudio server ///////////////////////////////////////////
     error_number = pa_context_connect (device.context, NULL, 0, NULL);
     if (error_number < 0) {
	  fprintf (stderr, "Could not connect to PulseAudio server: %s\n", pa_strerror(error_number));
	  return;
     }
     else
     {
	  fprintf (stderr, "Connected to PulseAudio asynchronously.\n");
     }

     // Run mainloop until sever is ready //////////////////////////////////////
     pa_context_state_t context_state;
     do 
     {
	  error_number = pa_mainloop_iterate (device.mainloop, 1, NULL);
	  if (error_number < 0) {
	       fprintf (stderr, "Could not run pa_mainloop_iterate ():%s\n", pa_strerror (error_number));
	       return;
	  }

	  context_state = pa_context_get_state (device.context);
	  if (! PA_CONTEXT_IS_GOOD (context_state))
	  {
	       fprintf (stderr, "Context state is not good.\n");
	       return;
	  }
	  else
	  {
	       fprintf (stderr, "PulseAudio context state is %d\n", context_state);
	  }
     } while (context_state != PA_CONTEXT_READY);

     // Set sample spec ////////////////////////////////////////////////////////
     device.spec.format = PA_SAMPLE_S16LE;
     if (iDisStereo)
	  device.spec.channels = 1;
     else
	  device.spec.channels = 2;
     device.spec.rate = settings.frequency;

     // Set buffer attributes //////////////////////////////////////////////////
     int mixlen = pa_usec_to_bytes (settings.latency_in_msec * PA_USEC_PER_MSEC, &device.spec);
     fprintf (stderr, "Size of buffer is: %ld\n", mixlen);
     device.buffer_attr.maxlength = (uint32_t) -1;
     device.buffer_attr.tlength = mixlen;
     device.buffer_attr.prebuf = 0;
     device.buffer_attr.minreq = (uint32_t) -1;
     //device.buffer_attr.minreq = mixlen;

     // Acquire new stream using spec and buffer attributes ////////////////////
     device.stream = pa_stream_new (device.context, "PCSX", &device.spec, NULL);
     if (device.stream == NULL)
     {
	  fprintf (stderr, "Could not get new PulseAudio stream.\n");
	  return;
     }

     pa_stream_flags_t flags = (pa_stream_flags_t) (PA_STREAM_ADJUST_LATENCY | PA_STREAM_VARIABLE_RATE);
     error_number = pa_stream_connect_playback (device.stream, NULL, &device.buffer_attr, flags, NULL, NULL);
     if (error_number < 0) {
	  fprintf (stderr, "Could not connect for playback successfully :%s\n", pa_strerror (error_number));
	  return;
     }

     // Run mainloop until stream is ready /////////////////////////////////////
     pa_stream_state_t stream_state;
     do {
	  error_number = pa_mainloop_iterate (device.mainloop, 1, NULL); 
	  if (error_number < 0) {
	       fprintf (stderr, "Could not run pa_mainloop_iterate ():%s\n", pa_strerror (error_number));
	       return;
	  }

	  stream_state = pa_stream_get_state (device.stream);
	  if (! PA_STREAM_IS_GOOD (stream_state))
	  {
	       fprintf (stderr, "Could not acquire PulseAudio stream.\n");
	       return;
	  }
	  else
	  {
	       fprintf (stderr, "PulseAudio stream state is %d.\n", stream_state);
	  }
     } while (stream_state != PA_STREAM_READY);

     fprintf  (stderr, "PulseAudio should be connected.\n");
     return;
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound (void)
{

     if (device.stream != NULL)
     {
	  pa_stream_disconnect (device.stream);
	  pa_stream_unref (device.stream);
	  device.stream = NULL;
     }

     if (device.context != NULL)
     {
	  pa_context_disconnect (device.context);
	  pa_context_unref (device.context);
	  device.context = NULL;
     }

     if (device.mainloop != NULL)
     {
	  pa_mainloop_free (device.mainloop);
	  device.mainloop = NULL;
     }
}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered (void)
{
     int size;
     int error_code;

     size = pa_stream_writable_size (device.stream);
     fprintf (stderr, "Writable size: %d\n", size);

     while (size < device.buffer_attr.tlength)
     {
	  size = pa_stream_writable_size (device.stream);
	  fprintf (stderr, "Looping - Writable size: %d\n", size);
	  pa_mainloop_iterate (device.mainloop, 1, &error_code);
	  if (error_code < 0)
	  {
	       fprintf (stderr, "Error on iterating loop while getting bytes buffered: %s\n", pa_strerror (error_code));
	       return SOUNDSIZE;
	  }
     }
     return 0;
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////

void SoundFeedStreamData (unsigned char *pSound, long lBytes)
{
     fprintf (stderr, "Number of bytes to write: %ld\n", lBytes);

     if (pa_stream_write (device.stream, pSound, lBytes, NULL, 0LL, PA_SEEK_RELATIVE) < 0)
	  fprintf (stderr, "Error: Could not perform write with PulseAudio\n");
}

///////////////////////////////////////////////////////////////////////
// CALLBACK TO NOTIFY US OF PA CONTEXT CHANGES
///////////////////////////////////////////////////////////////////////

void connection_state_callback (pa_context *context, const char *name, pa_proplist *property_list, void *user_data)
{
     return;
}

#endif
