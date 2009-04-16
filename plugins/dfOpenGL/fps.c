#include <sys/time.h>
#include <unistd.h>
#include "gpu_i.h"

static const double NTSC = 100000000/5994;
static const double PAL = 100000000/5000;

static double lastvsync = 0;
#define FRAMESAMPLES 10

double GetTime()	//in microseconds
{
 struct timeval tv;
 gettimeofday(&tv, 0);                                 // well, maybe there are better ways
 return (double)tv.tv_sec * 1000000 + tv.tv_usec;      // to do that, but at least it works
}

void waitforrealtime()
{
	double currenttime,tickstogo;
	double target;

	currenttime = GetTime();

	if (currenttime < lastvsync + 1000000)
		target = lastvsync + (psxDisp.pal ? PAL : NTSC);
	else
		target = currenttime;

	lastvsync = target;

	while (currenttime < target)
	{
		tickstogo = target - currenttime;
		if (tickstogo >= 500.0f)
		{
			usleep((useconds_t)tickstogo-200);	//usleep in microseconds
		}
		currenttime = GetTime();
	}
}
