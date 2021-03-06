/*
 *  CoreAudioPlexSupport.cpp
 *  Plex
 *
 *  Created by Ryan Walklin on 9/7/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "CoreAudioPlexSupport.h"
#include "stdafx.h"
#include "log.h"

#pragma mark Device Interface - Public

AudioDeviceArray* CoreAudioPlexSupport::GetDeviceArray()
{
	OSStatus            err = noErr;
    UInt32              i = 0, totalDeviceCount = 0, i_param_size = 0;
    AudioDeviceID       devid_def = 0;
    AudioDeviceID       *p_devices = NULL;

	/* Get number of devices */
    err = AudioHardwareGetPropertyInfo( kAudioHardwarePropertyDevices,
									   &i_param_size, NULL );
    if( err != noErr )
    {
		CLog::Log(LOGERROR, "Could not get number of devices: [%4.4s]", (char *)&err );
        return NULL;
		//goto error;
    }

    totalDeviceCount = i_param_size / sizeof( AudioDeviceID );

    if( totalDeviceCount < 1 )
    {
		CLog::Log(LOGERROR, "No audio output devices were found." );
		return NULL;
        //goto error;
    }

	CLog::Log(LOGDEBUG, "System has %ld device(s)", totalDeviceCount );

    /* Allocate DeviceID array */
	AudioDeviceArray *newDeviceArray = (AudioDeviceArray*)calloc(1, sizeof(AudioDeviceArray));
	newDeviceArray->device = (AudioDeviceInfo**)calloc(totalDeviceCount, sizeof(AudioDeviceInfo));

    p_devices = (AudioDeviceID*)calloc(totalDeviceCount, sizeof(AudioDeviceID));

	if( p_devices == NULL )
        return NULL;

    /* Populate DeviceID array */
    err = AudioHardwareGetProperty( kAudioHardwarePropertyDevices,
								   &i_param_size, p_devices );
    if( err != noErr )
    {
		CLog::Log(LOGDEBUG, "could not get the device IDs: [%4.4s]", (char *)&err );
        goto error;
    }

    /* Find the ID of the default Device */
    i_param_size = sizeof( AudioDeviceID );
    err = AudioHardwareGetProperty( kAudioHardwarePropertyDefaultOutputDevice,
								   &i_param_size, &devid_def );
    if( err != noErr )
    {
		CLog::Log(LOGDEBUG, "could not get default audio device: [%4.4s]", (char *)&err );
        return NULL;
    }
	newDeviceArray->defaultDevice = devid_def;

	for( i = 0; i < totalDeviceCount; i++ )
    {
        char *psz_name;
        i_param_size = 0;

        /* Retrieve the length of the device name */
        err = AudioDeviceGetPropertyInfo(
										 p_devices[i], 0, false,
										 kAudioDevicePropertyDeviceName,
										 &i_param_size, NULL);
        if( err ) goto error;

        /* Retrieve the name of the device */
        psz_name = (char *)malloc( i_param_size );
        err = AudioDeviceGetProperty(
									 p_devices[i], 0, false,
									 kAudioDevicePropertyDeviceName,
									 &i_param_size, psz_name);
        if( err ) goto error;

		CLog::Log(LOGDEBUG, "DevID: %#lx DevName: %s", p_devices[i], psz_name );

        if( !AudioDeviceHasOutput( p_devices[i]) )
        {
			CLog::Log(LOGDEBUG, "Skipping input-only device %i", p_devices[i]);
            continue;
        }

		// Add output device IDs to array
		AudioDeviceInfo *currentDevice = (AudioDeviceInfo*)malloc(sizeof(AudioDeviceInfo));
		currentDevice->deviceID = p_devices[i];
		currentDevice->deviceName = psz_name;

		if( newDeviceArray->defaultDevice == p_devices[i] )
        {
			CLog::Log(LOGDEBUG, "Selecting default device %s (%i)",
					  currentDevice->deviceName,
					  currentDevice->deviceID);
			newDeviceArray->selectedDevice = currentDevice->deviceID;
			newDeviceArray->selectedDeviceIndex = i;
        }

        if( AudioDeviceSupportsDigital(p_devices[i]))
        {
			currentDevice->supportsDigital = TRUE;
        }
		else currentDevice->supportsDigital = FALSE;

		newDeviceArray->device[newDeviceArray->deviceCount++] = currentDevice;
		currentDevice = NULL;
    }

    /* If we change the device we want to use, we should renegotiate the audio chain */
    //var_AddCallback( p_aout, "audio-device", AudioDeviceCallback, NULL );
#warning fix this to track device changes
    /* Attach a Listener so that we are notified of a change in the Device setup */
    //err = AudioHardwareAddPropertyListener( kAudioHardwarePropertyDevices,
	//									   HardwareListener,
	//									   (void *)p_aout );
    //if( err )
    //    goto error;

    free( p_devices );
    return newDeviceArray;

error:
    //var_Destroy( p_aout, "audio-device" );
    free( p_devices );
#warning need to free strings
    return NULL;
}

AudioDeviceID CoreAudioPlexSupport::GetAudioDeviceIDByName(const char *audioDeviceName)
{
#warning free array
	//vector <AudioDeviceInfo*> deviceArray = GetDeviceArray();
	AudioDeviceArray *deviceArray = GetDeviceArray();
	for (int i=0; i<deviceArray->deviceCount; i++)
	{
		if (strcmp(deviceArray->device[i]->deviceName, audioDeviceName) == 0)
		{
			return deviceArray->device[i]->deviceID;
		}
	}
	return deviceArray->defaultDevice;
}

/*****************************************************************************
 * AudioStreamSupportsDigital: Check i_stream_id for digital stream support.
 *****************************************************************************/
int CoreAudioPlexSupport::AudioDeviceSupportsDigital(AudioDeviceID i_dev_id)
{
    OSStatus                    err = noErr;
    UInt32                      i_param_size = 0;
    AudioStreamID               *p_streams = NULL;
    int                         i = 0, i_streams = 0;
    bool                  b_return = false;

    /* Retrieve all the output streams */
    err = AudioDeviceGetPropertyInfo( i_dev_id, 0, FALSE,
									 kAudioDevicePropertyStreams,
									 &i_param_size, NULL );
    if( err != noErr )
    {
		CLog::Log(LOGDEBUG, "could not get number of streams: [%4.4s]", (char *)&err );
        return false;
    }

    i_streams = i_param_size / sizeof( AudioStreamID );
    p_streams = (AudioStreamID *)malloc( i_param_size );
    if( p_streams == NULL )
        return false;

    err = AudioDeviceGetProperty( i_dev_id, 0, FALSE,
								 kAudioDevicePropertyStreams,
								 &i_param_size, p_streams );

    if( err != noErr )
    {
		CLog::Log(LOGDEBUG, "could not get number of streams: [%4.4s]", (char *)&err );
        return false;
    }

    for( i = 0; i < i_streams; i++ )
    {
        if( AudioStreamSupportsDigital( p_streams[i] ) )
            b_return = true;
    }

    free( p_streams );
    return b_return;
}


/**
 * Return high precision date
 *
 * Use a 1 MHz clock when possible, or 1 kHz
 *
 * Beware ! It doesn't reflect the actual date (since epoch), but can be the machine's uptime or anything (when monotonic clock is used)
 */
mtime_t CoreAudioPlexSupport::mdate()
{
    mtime_t res;

#if defined (HAVE_CLOCK_NANOSLEEP)
    struct timespec ts;

    /* Try to use POSIX monotonic clock if available */
    if( clock_gettime( CLOCK_MONOTONIC, &ts ) == EINVAL )
	/* Run-time fallback to real-time clock (always available) */
        (void)clock_gettime( CLOCK_REALTIME, &ts );

    res = ((mtime_t)ts.tv_sec * (mtime_t)1000000)
	+ (mtime_t)(ts.tv_nsec / 1000);

#elif defined( HAVE_KERNEL_OS_H )
    res = real_time_clock_usecs();

#elif defined( WIN32 ) || defined( UNDER_CE )
    /* We don't need the real date, just the value of a high precision timer */
    static mtime_t freq = INT64_C(-1);

    if( freq == INT64_C(-1) )
    {
        /* Extract from the Tcl source code:
         * (http://www.cs.man.ac.uk/fellowsd-bin/TIP/7.html)
         *
         * Some hardware abstraction layers use the CPU clock
         * in place of the real-time clock as a performance counter
         * reference.  This results in:
         *    - inconsistent results among the processors on
         *      multi-processor systems.
         *    - unpredictable changes in performance counter frequency
         *      on "gearshift" processors such as Transmeta and
         *      SpeedStep.
         * There seems to be no way to test whether the performance
         * counter is reliable, but a useful heuristic is that
         * if its frequency is 1.193182 MHz or 3.579545 MHz, it's
         * derived from a colorburst crystal and is therefore
         * the RTC rather than the TSC.  If it's anything else, we
         * presume that the performance counter is unreliable.
         */
        LARGE_INTEGER buf;

        freq = ( QueryPerformanceFrequency( &buf ) &&
				(buf.QuadPart == INT64_C(1193182) || buf.QuadPart == INT64_C(3579545) ) )
		? buf.QuadPart : 0;

#if defined( WIN32 )
        /* on windows 2000, XP and Vista detect if there are two
		 cores there - that makes QueryPerformanceFrequency in
		 any case not trustable?
		 (may also be true, for single cores with adaptive
		 CPU frequency and active power management?)
		 */
        HINSTANCE h_Kernel32 = LoadLibrary(_T("kernel32.dll"));
        if(h_Kernel32)
        {
            void WINAPI (*pf_GetSystemInfo)(LPSYSTEM_INFO);
            pf_GetSystemInfo = (void WINAPI (*)(LPSYSTEM_INFO))
			GetProcAddress(h_Kernel32, _T("GetSystemInfo"));
            if(pf_GetSystemInfo)
            {
				SYSTEM_INFO system_info;
				pf_GetSystemInfo(&system_info);
				if(system_info.dwNumberOfProcessors > 1)
					freq = 0;
            }
            FreeLibrary(h_Kernel32);
        }
#endif
    }

    if( freq != 0 )
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter (&counter);

        /* Convert to from (1/freq) to microsecond resolution */
        /* We need to split the division to avoid 63-bits overflow */
        lldiv_t d = lldiv (counter.QuadPart, freq);

        res = (d.quot * 1000000) + ((d.rem * 1000000) / freq);
    }
    else
    {
        /* Fallback on timeGetTime() which has a millisecond resolution
         * (actually, best case is about 5 ms resolution)
         * timeGetTime() only returns a DWORD thus will wrap after
         * about 49.7 days so we try to detect the wrapping. */

        static CRITICAL_SECTION date_lock;
        static mtime_t i_previous_time = INT64_C(-1);
        static int i_wrap_counts = -1;

        if( i_wrap_counts == -1 )
        {
            /* Initialization */
#if defined( WIN32 )
            i_previous_time = INT64_C(1000) * timeGetTime();
#else
            i_previous_time = INT64_C(1000) * GetTickCount();
#endif
            InitializeCriticalSection( &date_lock );
            i_wrap_counts = 0;
        }

        EnterCriticalSection( &date_lock );
#if defined( WIN32 )
        res = INT64_C(1000) *
		(i_wrap_counts * INT64_C(0x100000000) + timeGetTime());
#else
        res = INT64_C(1000) *
		(i_wrap_counts * INT64_C(0x100000000) + GetTickCount());
#endif
        if( i_previous_time > res )
        {
            /* Counter wrapped */
            i_wrap_counts++;
            res += INT64_C(0x100000000) * 1000;
        }
        i_previous_time = res;
        LeaveCriticalSection( &date_lock );
    }
#else
    struct timeval tv_date;

    /* gettimeofday() cannot fail given &tv_date is a valid address */
    (void)gettimeofday( &tv_date, NULL );
    res = (mtime_t) tv_date.tv_sec * 1000000 + (mtime_t) tv_date.tv_usec;
#endif

    return res;
}

#pragma mark Private

/*****************************************************************************
 * AudioStreamSupportsDigital: Check i_stream_id for digital stream support.
 *****************************************************************************/
int CoreAudioPlexSupport::AudioStreamSupportsDigital(AudioStreamID i_stream_id )

{
    OSStatus                    err = noErr;
    UInt32                      i_param_size = 0;
    AudioStreamBasicDescription *p_format_list = NULL;
    int                         i = 0, i_formats = 0;
    bool                  b_return = false;

    /* Retrieve all the stream formats supported by each output stream */
    err = AudioStreamGetPropertyInfo( i_stream_id, 0,
									 kAudioStreamPropertyPhysicalFormats,
									 &i_param_size, NULL );
    if( err != noErr )
    {
		CLog::Log(LOGDEBUG, "could not get number of streamformats: [%4.4s]", (char *)&err );
        return false;
    }

    i_formats = i_param_size / sizeof( AudioStreamBasicDescription );
    p_format_list = (AudioStreamBasicDescription *)malloc( i_param_size );
    if( p_format_list == NULL )
        return false;

    err = AudioStreamGetProperty( i_stream_id, 0,
								 kAudioStreamPropertyPhysicalFormats,
								 &i_param_size, p_format_list );
    if( err != noErr )
    {
		CLog::Log(LOGDEBUG, "could not get the list of streamformats: [%4.4s]", (char *)&err );
        free( p_format_list);
        p_format_list = NULL;
        return false;
    }

    for( i = 0; i < i_formats; i++ )
    {
		CLog::Log(LOGDEBUG, STREAM_FORMAT_MSG( "supported format: ", p_format_list[i] ) );

        if( p_format_list[i].mFormatID == 'IAC3' ||
		   p_format_list[i].mFormatID == kAudioFormat60958AC3 )
        {
            b_return = true;
        }
    }

    free( p_format_list );
    return b_return;
}

/*****************************************************************************
 * AudioDeviceHasOutput: Checks if the Device actually provides any outputs at all
 *****************************************************************************/
int CoreAudioPlexSupport::AudioDeviceHasOutput( AudioDeviceID i_dev_id )
{
    UInt32            dataSize;
    Boolean            isWritable;

    AudioDeviceGetPropertyInfo( i_dev_id, 0, FALSE, kAudioDevicePropertyStreams, &dataSize, &isWritable);
    if (dataSize == 0) return FALSE;

    return TRUE;
}

