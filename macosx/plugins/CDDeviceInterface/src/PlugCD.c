/***************************************************************************
    PlugCD.c
    CDDeviceInterface
  
    Created by Gil Pedersen on Fri July 18 2003.
    Copyright (c) 2003,2004 Gil Pedersen.
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

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <paths.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pthread.h>
#include <mach/mach_port.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#include <IOKit/scsi/SCSITaskLib.h>
#include <CoreFoundation/CoreFoundation.h>

//#include "plugins.h"
#include "PlugCD.h"

//#define USE_DEVICE_INTERFACE

long CDRclose(void);

/////////////////////////////////////////////////////////
typedef void* HWND;
#include "psemu_plugin_defs.h"

const char *LibName = "CD-ROM Device Interface";
const int version = 0;
const int revision = 1;
const int build = 0;

const char *PSEgetLibName(void) {
	return LibName;
}

unsigned long PSEgetLibType(void) {
	return PSE_LT_CDR;
}

unsigned long PSEgetLibVersion(void) {
	return version << 16 | revision << 8 | build;
}
/////////////////////////////////////////////////////////

//#define SysPrintf printf
#define SysPrintf(...) ((void)0)

#define UseMultiThreaded	1
#define NoIdleSleep			1

struct CdrStat {
	unsigned long Type;
	unsigned long Status;
	unsigned char Time[3];
};

MMCDeviceInterface **cdInterface;
pthread_t readThread;
pthread_cond_t readCond;
pthread_mutex_t readMutex;
char deviceFilePath[ 256 ];

kern_return_t FindEjectableCDMedia( io_iterator_t *mediaIterator, mach_port_t *masterPort )
{
    kern_return_t kernResult; 
    CFMutableDictionaryRef classesToMatch;
	int i;

    kernResult = IOMasterPort( bootstrap_port, masterPort );
    if ( kernResult != KERN_SUCCESS )
    {
        SysPrintf( "IOMasterPort returned %d\n", kernResult );
        return kernResult;
    }

    for (i = 0; i < 2; i++)
    {
        // CD media are instances of class kIOCDMediaClass/kIODVDMediaClass.
        classesToMatch = IOServiceMatching( (i == 0) ? kIOCDMediaClass : kIODVDMediaClass );

        if ( classesToMatch == NULL )
        {
            SysPrintf( "IOServiceMatching returned a NULL dictionary.\n" );
            continue;
        }
        else
        {
            // Each IOMedia object has a property with key kIOMediaEjectable 
            // which is true if the media is indeed ejectable. So add property
            // to CFDictionary for matching.
            CFDictionarySetValue( classesToMatch, 
                                CFSTR( kIOMediaEjectableKey ), kCFBooleanTrue );
        }
        kernResult = IOServiceGetMatchingServices( *masterPort,
                                    classesToMatch, mediaIterator );
        if ( (kernResult != KERN_SUCCESS) || (*mediaIterator == NULL) )
            SysPrintf( "No ejectable CD media found.\n kernResult = %d\n", kernResult );
        else
            break;
    }

    return kernResult;
}

kern_return_t GetDeviceFilePath( io_service_t media, 
                        char *deviceFilePath, CFIndex maxPathSize )
{
    kern_return_t kernResult = KERN_FAILURE;
	CFTypeRef   deviceFilePathAsCFString;

	deviceFilePathAsCFString = IORegistryEntryCreateCFProperty( 
								media, CFSTR( kIOBSDNameKey ), 
								kCFAllocatorDefault, 0 );
	*deviceFilePath = '\0';
	if ( deviceFilePathAsCFString )
	{
		size_t devPathLength = strlen( _PATH_DEV )+0;//+1;

		strcpy( deviceFilePath, _PATH_DEV );
		// Add "r" before the BSD node name from the I/O Registry
		// to specify the raw disk node. The raw disk node receives
		// I/O requests directly and does not go through the
		// buffer cache.
		// strcat( deviceFilePath, "r"); // apparently som cd-rom drives don't like this

		if ( CFStringGetCString( deviceFilePathAsCFString,
								 deviceFilePath + devPathLength,
								 maxPathSize - devPathLength, 
								 kCFStringEncodingASCII ) )
		{
			SysPrintf( "BSD path: %s\n", deviceFilePath );
			kernResult = KERN_SUCCESS;
		}
		CFRelease( deviceFilePathAsCFString );
	}

    return kernResult;
}

MMCDeviceInterface ** GetMMCInterfaceForDevice(io_service_t service)
{
    SInt32                          score;
    HRESULT                         herr;
    kern_return_t                   err;
    IOCFPlugInInterface             **plugInInterface = NULL;
    MMCDeviceInterface              **mmcInterface = NULL;
 
    // Create the IOCFPlugIn interface so we can query it.
    err = IOCreatePlugInInterfaceForService (   service,
                                                kIOMMCDeviceUserClientTypeID,
                                                kIOCFPlugInInterfaceID,
                                                &plugInInterface,
                                                &score );
        
    if ( err != noErr )
    {
        SysPrintf("IOCreatePlugInInterfaceForService returned %d\n", err);
        return NULL;
    }
 
    // Query the interface for the MMCDeviceInterface.
    herr = ( *plugInInterface )->QueryInterface ( plugInInterface,
                                        CFUUIDGetUUIDBytes ( kIOMMCDeviceInterfaceID ),
                                        ( LPVOID ) &mmcInterface );
    if ( herr != S_OK )
    {
        SysPrintf("QueryInterface returned %ld\n", herr);
        return NULL;
    }
        
//    ( *mmcInterface )->Release ( mmcInterface );
//    IODestroyPlugInInterface ( plugInInterface );
	
	return mmcInterface;
}

int GetCDROMServices()
{
	mach_port_t         masterPort;
	CFMutableDictionaryRef  matchingDict;
	CFMutableDictionaryRef  subDict;
	io_service_t nextDevice;
	io_iterator_t iterator;
	kern_return_t       kr;
	int retVal = -1;
			
	// first create a master_port for my task
	kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (kr || !masterPort)
	{
		SysPrintf("ERR: Couldn't create a master IOKit Port(%08x)\n", kr);
		return -1;
	}

	// Create the dictionaries
	matchingDict = CFDictionaryCreateMutable ( kCFAllocatorDefault, 0, NULL, NULL );
	subDict      = CFDictionaryCreateMutable ( kCFAllocatorDefault, 0, NULL, NULL );

	// Create a dictionary with the "SCSITaskDeviceCategory" key = "SCSITaskAuthoringDevice"
	// TODO: examine if this also work on non-authoring capable devices
	CFDictionarySetValue (  subDict,
								 CFSTR ( kIOPropertySCSITaskDeviceCategory ),
								 CFSTR ( kIOPropertySCSITaskAuthoringDevice ) );

	// Add the dictionary to the main dictionary with the key "IOPropertyMatch" to
	// narrow the search to the above dictionary.
	CFDictionarySetValue (  matchingDict,
								 CFSTR ( kIOPropertyMatchKey ),
								 subDict );


	kr = IOServiceGetMatchingServices( masterPort, 
									  matchingDict, &iterator );    
	if ( (kr != KERN_SUCCESS) || (iterator == NULL) ) {
		SysPrintf( "No CDROM drives found.\n kernResult = %d\n", kr );
		goto error;
	}

	// Find best cdrom drive - i.e. the first :-)
	while(nextDevice = IOIteratorNext( iterator )) {
		cdInterface = GetMMCInterfaceForDevice(nextDevice);
		kr = IOObjectRelease(nextDevice);
		break; // hack
	}
	IOObjectRelease( iterator );
	
	retVal = 0;
error:
	// Now done with the master_port
	if (masterPort)
		mach_port_deallocate(mach_task_self(), masterPort);
	
	return retVal;
}

/* determine if the tray is open for our cd-drive */
int TrayIsOpen()
{
#ifdef USE_DEVICE_INTERFACE
	if (cdInterface) {
		if ((*cdInterface)->GetTrayState) {
			IOReturn res;
			char state;
			
			res = (*cdInterface)->GetTrayState(cdInterface, &state);
			if (kIOReturnSuccess == res) {
				if (kMMCDeviceTrayOpen == state)
					return true;
			}
		}
	}
#endif
	return true;
}

/* opens tray if no media is in the device */
void TrayOpen()
{
	if (TrayIsOpen())
		return;
	
	if (cdInterface) {
		if ((*cdInterface)->SetTrayState) {
			(*cdInterface)->SetTrayState(cdInterface, kMMCDeviceTrayOpen);
		}
	}
}

long openDisc()
{
    mach_port_t masterPort = NULL;
    kern_return_t kernResult;
    io_iterator_t mediaIterator;
	io_service_t media;
	CFTypeRef toc_cf;
    //dk_cd_read_disc_info_t cd_read_disc_info;
    //dk_cd_read_track_info_t cd_read_track_info;
    //CDDiscInfo di; CDTrackInfo ti;
    u_int16_t speed;
    int i;

    CD.cd = 0;
    CD.status = 0x10;
    
    // Find 1st CD
    kernResult = FindEjectableCDMedia( &mediaIterator, &masterPort );
    if ( kernResult != KERN_SUCCESS )
        return -1;
    
	media = IOIteratorNext( mediaIterator );
	if (NULL == media)
		return -1;

    // Release the iterator.
    IOObjectRelease( mediaIterator );
	
    // Get The device path
    kernResult = GetDeviceFilePath( media, deviceFilePath, 
                    sizeof( deviceFilePath ) );
    if ( kernResult != KERN_SUCCESS )
        return -1;
    
	toc_cf = IORegistryEntryCreateCFProperty(media,CFSTR(kIOCDMediaTOCKey),kCFAllocatorDefault,0);
	if(toc_cf != nil)
	{
		CDTOC *toc = (CDTOC *)CFDataGetBytePtr(toc_cf);
		int ndesc;
		
        ndesc = CDTOCGetDescriptorCount(toc);
		if (CD.tl) free(CD.tl);
		CD.tl = calloc(ndesc, sizeof(Track));
		if (NULL == CD.tl)
			return -1;
		
		CD.numtracks = 0;
		for (i=0; i<ndesc; i++)
		{
			CDTOCDescriptor	*desc = &toc->descriptors[i];
			
			if(desc->point < 100)
			{
				CD.tl[CD.numtracks].type = ((desc->control & 0x0f) != 0) ? Mode2 : Audio; // TODO: set correct type
				CD.tl[CD.numtracks].num = desc->point;
				CD.tl[CD.numtracks].start[2] = desc->p.frame;
				CD.tl[CD.numtracks].start[1] = desc->p.second;
				CD.tl[CD.numtracks].start[0] = desc->p.minute;
				
				if (CD.numtracks) {
					CD.tl[CD.numtracks-1].end[2] = desc->p.frame;
					CD.tl[CD.numtracks-1].end[1] = desc->p.second;
					CD.tl[CD.numtracks-1].end[0] = desc->p.minute;
				}
				//normalizeTime(CD.tl[0].start);
				//normalizeTime(CD.tl[0].end);
				CD.numtracks++;
			}
		}
		CDMSF end = CDConvertTrackNumberToMSF(0xa2, toc);
		CD.tl[CD.numtracks-1].end[2] = end.frame;
		CD.tl[CD.numtracks-1].end[1] = end.second;
		CD.tl[CD.numtracks-1].end[0] = end.minute;
		
		CFRelease(toc_cf);
	} else {
		SysPrintf("failed to read cdrom toc information\n");
		IOObjectRelease( mediaIterator );
		return -1;
	}
	
    IOObjectRelease( media );

    // Free master port if we created one.
    if (masterPort)
        mach_port_deallocate(mach_task_self(), masterPort);
    
    // Now open it
    CD.cd = open(deviceFilePath, O_RDONLY, 0);
    if (CD.cd <= 0) {
        perror("failed to open cd: ");
        CD.cd = 0;
        return -1;
    }
    
    // get number of tracks
/*    memset(&cd_read_disc_info, 0, sizeof(dk_cd_read_disc_info_t));
    cd_read_disc_info.bufferLength = sizeof(CDDiscInfo);
    cd_read_disc_info.buffer = &di;
    if (ioctl(CD.cd, DKIOCCDREADDISCINFO, &cd_read_disc_info) < 0) {
        perror("error reading cd info: ");
        CD.numtracks = 1;
    }
    else {
        CD.numtracks = di.lastTrackNumberInLastSessionLSB-di.firstTrackNumberInLastSessionLSB+1;
	 }
	 
    CD.tl = calloc(CD.numtracks, sizeof(Track));
    for (i=0; i<CD.numtracks; i++)
    {
        int blocks;
        memset(&cd_read_track_info, 0, sizeof(dk_cd_read_track_info_t));
        cd_read_track_info.address = di.firstTrackNumberInLastSessionLSB+i;
        cd_read_track_info.addressType = kCDTrackInfoAddressTypeTrackNumber;
        cd_read_track_info.bufferLength = sizeof(CDTrackInfo);
        cd_read_track_info.buffer = &ti;
        if (ioctl(CD.cd, DKIOCCDREADTRACKINFO, &cd_read_track_info) < 0) {
            perror("error reading track info");
            close(CD.cd); CD.cd = 0;
            return -1;
        }
			
        //SysPrintf("Track %i: %i - %i\n", di.firstTrackNumberInLastSessionLSB+i,
        //        ti.trackStartAddress, ti.trackSize);
        
        CD.tl[i].type = ti.trackMode ? Mode2 : Audio; // TODO: set correct type
        CD.tl[i].num = ti.trackNumberLSB;
        blocks = ti.trackStartAddress;
        CD.tl[i].start[2] = blocks % 75;
        CD.tl[i].start[1] = ((blocks - CD.tl[i].end[2]) / 75) % 60;
        CD.tl[i].start[0] = (((blocks - CD.tl[i].end[2]) / 75) - CD.tl[i].end[1]) / 60;
        blocks += ti.trackSize;
        CD.tl[i].end[2] = blocks % 75;
        CD.tl[i].end[1] = ((blocks - CD.tl[i].end[2]) / 75) % 60;
        CD.tl[i].end[0] = (((blocks - CD.tl[i].end[2]) / 75) - CD.tl[i].end[1]) / 60;

        normalizeTime(CD.tl[i].start);
        normalizeTime(CD.tl[i].end);
    }*/
    
    speed = kCDSpeedMin*4; // 4x
    if (ioctl(CD.cd, DKIOCCDSETSPEED, &speed) < 0)
        perror("couldn't set cd speed");
    
    CD.sectorType = kCDSectorTypeMode2Form1;
    CD.bufferSize = 0;
    CD.bufferPos = 0x7FFFFFFF;
    CD.status = 0x00;
    
//    fcntl(CD.cd, F_RDAHEAD, 1);
//    fcntl(CD.cd, F_NOCACHE, 0);
    fcntl(CD.cd, F_RDAHEAD, 0);
    fcntl(CD.cd, F_NOCACHE, 1);

    return 0;
}

// gets track 
long getTN(unsigned char* buffer)
{
	int numtracks = getNumTracks();

	if (-1 == numtracks)
	{
		buffer[0]=buffer[1]=1;
		return -1;
	}

	buffer[0]=CD.tl[0].num;
	buffer[1]=numtracks;
   return 0;
}

 // if track==0 -> return total length of cd
 // otherwise return start in bcd time format
long getTD(int track, unsigned char* buffer)
{
      // lasttrack just keeps track of which track TD was requested last (go fig)

   if (track > getNumTracks())
   {
//      SysPrintf("getTD bad %2d\n", track);
      return -1;
   }

   if (track == 0)
   {
      buffer[0] = CD.tl[CD.numtracks-1].end[0];
      buffer[1] = CD.tl[CD.numtracks-1].end[1];
      buffer[2] = CD.tl[CD.numtracks-1].end[2];
   }
   else
   {
      buffer[0] = CD.tl[track-1].start[0];
      buffer[1] = CD.tl[track-1].start[1];
      buffer[2] = CD.tl[track-1].start[2];
   }
//   SysPrintf("getTD %2d %02d:%02d:%02d\n", track, (int)buffer[0],
//         (int)buffer[1], (int)buffer[2]);

   // bcd encode it
   buffer[0] = intToBCD(buffer[0]);
   buffer[1] = intToBCD(buffer[1]);
   buffer[2] = intToBCD(buffer[2]);
//   SysPrintf("end getTD()\r\n");
   return 0;
}

// return the sector address - the buffer address + 12 bytes for subheader offset.
unsigned char* getSector(int subchannel)
{
	SysPrintf("getSector()\n");
	
	if (readThread) {
		int err;
		// wait until we can obtain a lock */
		err = pthread_mutex_lock(&readMutex);
		if (err != 0) {
			SysPrintf("failed to lock mutex, error = %i\n", err);
		} else {
			err = pthread_mutex_unlock(&readMutex);
			if (err != 0) {
				SysPrintf("failed to unlock mutex, error = %i\n", err);
			}
		}
	}
	
	if (CD.sector == -1)
		return NULL;
	else {
		return CD.buffer /*+ (CD.sector - CD.bufferPos)*/ + ((subchannel) ? 0 : 12);
	}
}

// returns the number of tracks
char getNumTracks()
{
//    SysPrintf("start getNumTracks()\r\n");
    // if there's no open cd, return -1
    if (CD.cd == 0) {
        return -1;
    }

//    SysPrintf("numtracks %d\n",CD.numtracks);
//    SysPrintf("end getNumTracks()\r\n");
    return CD.numtracks;
}

// read the sector pointed to by pos
int readSector(off_t pos, unsigned char *buffer)
{
	int len;
	
	SysPrintf("start readit()\n");
	
	if (0 == CD.cd)
		return 0;
	
	// go to the sector
	pos = lseek(CD.cd, pos, SEEK_SET);
	if (pos < 0)
		goto error;
   
	// and read it into the buffer
	len = read(CD.cd, buffer, 2352);
	if (len < 2352)
		goto error;

	SysPrintf("end readit()\n");
	return 2352;
	
error:
	perror("CD read error");
	return 0;
}

void seekSector(const unsigned char m, const unsigned char s, const unsigned char f)
{
	int err;
	SysPrintf("start seekSector()\n");
	
	if (0 == CD.cd)
		return;
	
	if (readThread) {
		// wait until we're done reading
		err = pthread_mutex_lock(&readMutex);
		if (err != 0) {
			SysPrintf("failed to lock mutex, error = %i\n", err);
		} else {
			// calc byte to search for
			CD.sector = (( (m * 60) + (s - 2)) * 75 + f) * 2352;

			// unlock again, since the mutex won't be set until 
			// we signal a cond later in this function
			err = pthread_mutex_unlock(&readMutex);
			if (err != 0) {
				SysPrintf("failed to unlock mutex, error = %i\n", err);
			}
		}
	} else {
		// calc byte to search for
		CD.sector = (( (m * 60) + (s - 2)) * 75 + f) * 2352;
	}
//    SysPrintf("seek %d %02d:%02d:%02d",CD.sector, (int)m, (int)s, (int)f);

	// is it cached?
#if 0
	if ((CD.sector >= CD.bufferPos) &&
		(CD.sector < (CD.bufferPos + CD.bufferSize)) ) {
//		SysPrintf(" cached %d %d\n",CD.sector - CD.bufferPos,BUFFER_SIZE);
//		SysPrintf("end seekSector()\r\n");
		return;
	}
	// not cached - read a few blocks into the cache
	else
#endif
	{
		if (readThread) {
			SysPrintf("end seekSector()\n");

			// signal that a new sector is ready to be read
			if (pthread_cond_broadcast(&readCond) == 0)
				return;
			
			SysPrintf("failed to signal 'readCond'\n");
		}
		
		CD.bufferSize = readSector(CD.sector, CD.buffer);
		if (CD.bufferSize==0) CD.sector = -1;
		else CD.bufferPos = CD.sector;
	}
	SysPrintf("end seekSector()\n");
}

/* handles reading from the cd */
void *read_thread(void *arg)
{
	struct timespec dT = { 20, 0 }; // 20 s
	//struct timespec dT = { 0, 400*1000*1000 }; // 400 ms
	int err;
	
	pthread_mutex_lock(&readMutex);
	//pthread_cleanup_push(pthread_exit, 0);
	
	for (;;) {
		// wait until we're signalled
		if (NoIdleSleep) {
			err = pthread_cond_timedwait_relative_np(&readCond, &readMutex, &dT);
			pthread_testcancel();
		} else {
			err = pthread_cond_wait(&readCond, &readMutex);
		}
		if (err == EINVAL) {
			SysPrintf("failed cond wait for 'readCond', error = %i\n", err);
			return (void *)-1;
		}
		
		CD.bufferSize = readSector(CD.sector, CD.buffer);
		if (CD.bufferSize==0) CD.sector = -1;
		else CD.bufferPos = CD.sector;
	}
	//pthread_cleanup_pop(1);
	
	return 0;
}

long CDRopen(void)
{
	SysPrintf("CDR_open()\n");
	if (UseMultiThreaded && !readThread) {
		int err;
		
		err = pthread_cond_init(&readCond, NULL);
		if (err != 0) {
			SysPrintf("failed to create conditional, error=%i\n"
						 "going to single thread mode\n", err);
		} else {
			err = pthread_mutex_init(&readMutex, NULL);
			if (err != 0) {
				SysPrintf("failed to create mutex, error=%i\n"
							 "going to single thread mode\n", err);
			} else {
				struct sched_param params;
				int policy;
				
				err = pthread_create(&readThread, NULL, read_thread, NULL);
				if (err!=0) {
					SysPrintf("failed to create read thread, error=%i\n"
								 "going to single thread mode\n", err);
				} else {
					// set the thread to maximum priority
					pthread_getschedparam(readThread, &policy, &params);
					params.sched_priority = sched_get_priority_max(policy);
					pthread_setschedparam(readThread, policy, &params);
				}
			}
		}
	}

	if (openDisc() < 0) {
		TrayOpen();
		return 0;
	}
	
	seekSector(0,2,0);

	SysPrintf("end CDR_open()\n");
	return 0;
}

long CDRinit(void) {
	
#ifdef USE_DEVICE_INTERFACE
	return GetCDROMServices();
#endif
	
	return 0;
}

long CDRshutdown(void) {
	// do cleanup
	CDRclose();
	
	return 0;
}

long CDRclose(void) {
	SysPrintf("start CDR_close()\n");
	
	if (!CD.cd)
		return 0;
	
	if (readThread) {
		int termVal = 0;
		// make sure we're done reading
		pthread_mutex_lock(&readMutex);
		pthread_mutex_unlock(&readMutex);
		
		// kill read thread
		pthread_cancel(readThread);
		//pthread_kill(readThread, SIGTERM);
		pthread_cond_broadcast(&readCond);
		pthread_join(readThread, (void **)&termVal);
		
		// remove fluff
		pthread_mutex_destroy(&readMutex);
		pthread_cond_destroy(&readCond);
		
		readThread = 0;
	}
	
	close(CD.cd);
	CD.cd = 0;
	
	SysPrintf("end CDR_close()\n");
	return 0;
}

long CDRgetTN(unsigned char *buffer) {
    //SysPrintf("start CDRgetTN()\n");
    return getTN(buffer);
}

long CDRgetTD(unsigned char track, unsigned char *buffer) {
   unsigned char temp[3];
   int result = getTD((int)track, temp);

   // SysPrintf("start CDRgetTD()\n");

   if (result == -1) return -1;

   buffer[1] = temp[1];
   buffer[2] = temp[0];

   return 0;
}

/* called when the psx requests a read */
long CDRreadTrack(unsigned char *time) {
//    SysPrintf("start CDR_readTrack()\r\n");
    //SysPrintf("readTrack at %02d:%02d:%02d\n", BCDToInt(time[0]), BCDToInt(time[1]), BCDToInt(time[2]));
	
	if (CD.cd != 0)
		seekSector(BCDToInt(time[0]), BCDToInt(time[1]), BCDToInt(time[2]));

//    SysPrintf("end CDR_readTrack()\r\n");
	return PSE_CDR_ERR_SUCCESS;
}

/* called after the read should be finished, and the data is needed */
unsigned char *CDRgetBuffer(void) {
    //SysPrintf("start CDR_getBuffer()\n");
//    SysPrintf("start CDR_getBuffer()\r\n");
	if (CD.cd == 0)
		return NULL;

    return getSector(0);
}

unsigned char *CDRgetBufferSub(void) {
    //SysPrintf("start CDR_getBuffer()\n");
//    SysPrintf("start CDR_getBuffer()\r\n");
    return getSector(1);
}

/* from PSX manual p. 83 */
#define CDR_STATUS_UNKNOWN		0x00
#define CDR_STATUS_ERROR		0x02  /* command error detected */
#define CDR_STATUS_STANDBY		0x04  /* spindle motor rotating */
#define CDR_STATUS_SEEK_ERROR 0x08  /* seek error detected */
#define CDR_STATUS_SHELL_OPEN 0x10  /* once shell open */
#define CDR_STATUS_READING		0x20  /* reading data sectors */
#define CDR_STATUS_SEEKING		0x40
#define CDR_STATUS_PLAYING		0x80  /* playing CD-DA */

// reads cdr status - from old plugin
// type:
//  0x00 - unknown
//  0x01 - data
//  0x02 - audio
//  0xff - no cdrom
// status: (only shell open supported)
//  0x00 - unknown
//  0x01 - error
//  0x04 - seek error -> no disk???
//  0x10 - shell open -> tray open
//  0x20 - reading
//  0x40 - seeking
//  0x80 - playing
// time:
//  byte 0 - minute
//  byte 1 - second
//  byte 2 - frame

long CDRgetStatus(struct CdrStat *stat)
{
    if (CD.cd == 0) {
        // no cd - check for disc
        if (openDisc() < 0) {
				if (TrayIsOpen()) {
					stat->Type = 0xff;//0x00;
					stat->Status |= CDR_STATUS_SHELL_OPEN;
				} else {
					TrayOpen();
					stat->Type = 0xff; // indicates no cd
					stat->Status = CDR_STATUS_UNKNOWN;
				}
            stat->Time[0] = 
            stat->Time[1] = 
            stat->Time[2] = 0;
            return 0;
        }
    }
    
    if (CD.tl[0].type == Mode1 || CD.tl[0].type == Mode2) {
        stat->Type = 0x01;
        stat->Status = CDR_STATUS_UNKNOWN;
    } else if (CD.tl[0].type == Audio) {
        stat->Type = 0x02;
        stat->Status = CDR_STATUS_UNKNOWN; // FIXME
    } else {
        stat->Type = 0x00;
        stat->Status = CDR_STATUS_UNKNOWN; // FIXME
    }
    
    stat->Time[0] = 
    stat->Time[1] = 
    stat->Time[2] = 0; // FIXME
    
    return 0;
}

char *CDRgetDriveLetter(void) {
    //SysPrintf("start CDR_getBuffer()\n");
//    SysPrintf("start CDR_getBuffer()\r\n");
    return deviceFilePath;
}

#if 0
AudioFilePlayID cdFilePlayID;
long CDRplay(unsigned char *sector)
{
	// TODO: find the correct track...
	
	NewAudioFilePlayID(&fsref, &cdFilePlayID);
	
	return 0;
}

long CDRstop(void)
{
	
	return 0;
}
#endif
/*
long (CALLBACK* CDRconfigure)(void);
long (CALLBACK* CDRtest)(void);
void (CALLBACK* CDRabout)(void);
*/

#ifdef TEST
int main (int argc, int *argv)
{
    CDRopen();
    CDRclose();
    return 0;
}
#endif
