/***************************************************************************
 *   Copyright (C) 2011 by Blade_Arma                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include "stdafx.h"

#define _IN_OSS

#include "externals.h"

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

/******************************************************************************/
/* Defines.
 */

#define BUFFER_SIZE_TOTAL   (32768)
#define BUFFER_SIZE         (1024)
#define BUFFER_QUANTITY     (BUFFER_SIZE_TOTAL / BUFFER_SIZE)

/******************************************************************************/
/* Variables.
 */

static ALCdevice  *pDevice;
static ALCcontext *pContext;

// Buffers hold sound data.
static ALuint buffers[BUFFER_QUANTITY];

// Sources are points emitting sound.
static ALuint source;

// Position of the source sound.
static ALfloat SourcePos[]   = {0.0, 0.0, 0.0};

// Velocity of the source sound.
static ALfloat SourceVel[]   = {0.0, 0.0, 0.0};

// Direction of the source sound.
static ALfloat SourceDir[]   = {0.0, 0.0, 0.0};

// Position of the listener.
static ALfloat ListenerPos[] = {0.0, 0.0, 0.0};

// Velocity of the listener.
static ALfloat ListenerVel[] = {0.0, 0.0, 0.0};

// Orientation of the listener. (first 3 elements are "at", second 3 are "up")
static ALfloat ListenerOri[] = {0.0, 0.0, -1.0,  0.0, 1.0, 0.0};

static ALenum format         = AL_FORMAT_STEREO16;
static ALuint sampleRate     = 44100;
 
static ALuint UNUSED_VARIABLE sampleQuality  = 16;
static ALuint UNUSED_VARIABLE channels       = 2;

/******************************************************************************/
/* Error handling.
 */

char* GetALErrorString(ALenum error)
{
    switch(error)
    {
        case AL_NO_ERROR:
            return 0;
        case AL_INVALID_NAME:
            return "AL_INVALID_NAME";
        case AL_INVALID_ENUM:
            return "AL_INVALID_ENUM";
        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE";
        case AL_INVALID_OPERATION:
            return "AL_INVALID_OPERATION";
        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY";
    };
    
    return "AL_UNKNOWN_ERROR";
}

char* GetALCErrorString(ALenum error)
{
    switch(error)
    {
        case ALC_NO_ERROR:
            return 0;
        case ALC_INVALID_DEVICE:
            return "ALC_INVALID_DEVICE";
        case ALC_INVALID_CONTEXT:
            return "ALC_INVALID_CONTEXT";
        case ALC_INVALID_ENUM:
            return "ALC_INVALID_ENUM";
        case ALC_INVALID_VALUE:
            return "ALC_INVALID_VALUE";
        case ALC_OUT_OF_MEMORY:
            return "ALC_OUT_OF_MEMORY";
    };

    return "ALC_UNKNOWN_ERROR";
}

int checkALError()
{
    char *pErrorString = GetALErrorString(alGetError());
    if(pErrorString)
    {
        fprintf(stderr, "[SPU] AL: %s.\n", pErrorString);
        return -1;
    }
    
    return 0;
}

int checkALCError()
{
    char *pErrorString = GetALCErrorString(alcGetError(pDevice));
    if(pErrorString)
    {
        fprintf(stderr, "[SPU] ALC: %s.\n", pErrorString);
        return -1;
    }
    
    return 0;
}

/******************************************************************************/

void SetupSound()
{
    unsigned char buf[BUFFER_SIZE];
    int i;
    
    // Get handle to device.
    pDevice = alcOpenDevice(NULL);
    if(checkALCError())
    {
        fprintf(stderr, "[SPU] alcOpenDevice failed.\n");
        return;
    }
    
    // ALC info.
    const ALCubyte* UNUSED_VARIABLE deviceName = (ALCubyte*)alcGetString(pDevice, ALC_DEVICE_SPECIFIER);
    //printf("[SPU] ALC_DEVICE_SPECIFIER = %s.\n", deviceName);
    
    const ALCubyte* UNUSED_VARIABLE extensionList = (ALCubyte*)alcGetString(pDevice, ALC_EXTENSIONS);
    //printf("[SPU] ALC_EXTENSIONS = %s.\n", extensionList);
    
    // Create audio context.
    pContext = alcCreateContext(pDevice, NULL);
    if(checkALCError())
    {
        fprintf(stderr, "[SPU] alcCreateContext failed.\n");
        return;
    }
    
    // Set active context.
    alcMakeContextCurrent( pContext );
    if( checkALCError() )
    {
        fprintf(stderr, "[SPU] alcMakeContextCurrent failed.\n");
        return;
    }
    
    // AL info.
    const ALubyte* UNUSED_VARIABLE version = (ALubyte*)alGetString(AL_VERSION);
    //printf("[SPU] AL_VERSION = %s.\n", version);
    
    const ALubyte* UNUSED_VARIABLE renderer = (ALubyte*)alGetString(AL_RENDERER);
    //printf("[SPU] AL_RENDERER = %s.\n", renderer);

    const ALubyte* UNUSED_VARIABLE vendor = (ALubyte*)alGetString(AL_VENDOR);
    //printf("[SPU] AL_VENDOR = %s.\n", vendor);
    
    // Create buffers.
    alGenBuffers(BUFFER_QUANTITY, buffers);
    checkALError();
    
    // Load sound data into a buffer.
    memset(buf, 0x00, BUFFER_SIZE);
    for(i = 0; i < BUFFER_QUANTITY; ++i)
    {
        alBufferData(buffers[i], format, buf, BUFFER_SIZE, sampleRate);
    }
    checkALError();
    
    // Create source.
    alGenSources(1, &source);
    checkALError();
    
    // Bind buffer with a source.
    alSourcef (source, AL_PITCH,           1.0f     );
    alSourcef (source, AL_GAIN,            1.0f     );
    alSourcefv(source, AL_POSITION,        SourcePos);
    alSourcefv(source, AL_VELOCITY,        SourceVel);
    alSourcefv(source, AL_DIRECTION,       SourceDir);
    alSourcei (source, AL_SOURCE_RELATIVE, AL_TRUE  );
    alSourcei (source, AL_LOOPING,         AL_FALSE );
    
    // Listener properties.
    alListenerfv(AL_POSITION,    ListenerPos);
    alListenerfv(AL_VELOCITY,    ListenerVel);
    alListenerfv(AL_ORIENTATION, ListenerOri);
    
    // Add buffers to queue.
    alSourceQueueBuffers(source, BUFFER_QUANTITY, buffers);
    checkALError();
    
    // Enable playing.
    alSourcePlay(source);
    checkALError();
}

void RemoveSound()
{
    alSourceStop(source);
    checkALError();
    alDeleteSources(1, &source);
    checkALError();
    alDeleteBuffers(BUFFER_QUANTITY, buffers);
    checkALError();
    
    // Reset the current context to NULL.
    alcMakeContextCurrent(NULL);
    checkALCError();
    
    // RELEASE the context and the device.
    alcDestroyContext(pContext);
    checkALCError();
    alcCloseDevice(pDevice);
}

/******************************************************************************/

unsigned long SoundGetBytesBuffered()
{
    ALint processed;
	int buffered;
	
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    checkALError();
	
	buffered = BUFFER_SIZE_TOTAL - processed * BUFFER_SIZE;
    //printf("[SPU] SoundGetBytesBuffered: %i\n", buffered);
    
    return buffered;
}

/******************************************************************************/

void SoundFeedStreamData(unsigned char *pData, long lBytes)
{
    ALint  processed;
    ALint  state;
    ALuint buffer;
    int    needed;
    int    i;

    //printf("[SPU] SoundFeedStreamData: %i\n", lBytes);
    
    needed = (lBytes + (BUFFER_SIZE - 1)) / BUFFER_SIZE;
    
    // Expect free buffer.
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    while(processed < needed)
    {
        usleep(1);
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    }
	
    // Add buffers to queue.
    for(i = 0; i < needed; ++i)
    {
        alSourceUnqueueBuffers(source, 1, &buffer);        
        alBufferData(buffer, format, pData, lBytes > BUFFER_SIZE ? BUFFER_SIZE : lBytes, sampleRate);        
        alSourceQueueBuffers(source, 1, &buffer);
        lBytes -= BUFFER_SIZE;
        pData += BUFFER_SIZE;
    }
    
    // Restart playing.
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if(state != AL_PLAYING)
    {
        //fprintf(stderr, "[SPU] AL_SOURCE_STATE != AL_PLAYING: %x\n", state);
        alSourcePlay(source);
        checkALError();
    }
}

/******************************************************************************/
