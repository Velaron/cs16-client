//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "stdafx.h"
#include "perf_counter.h"
#ifdef _WIN32
#include <windows.h>
void CPerformanceCounter::InitializePerformanceCounter(void)
{
	LARGE_INTEGER PerformanceFreq;
	unsigned int lowpart, highpart;

	QueryPerformanceFrequency(&PerformanceFreq);

	lowpart = (unsigned int)PerformanceFreq.LowPart;
	highpart = (unsigned int)PerformanceFreq.HighPart;
	m_iLowShift = 0;

	while (highpart || (lowpart > 2000000.0))
	{
		m_iLowShift++;
		lowpart >>= 1;
		lowpart |= (highpart & 1) << 31;
		highpart >>= 1;
	}

	m_flPerfCounterFreq = 1.0 / (double)lowpart;
}

float CPerformanceCounter::GetCurTime(void)
{
	static int sametimecount;
	static unsigned int oldtime;
	static int first = 1;
	LARGE_INTEGER PerformanceCount;
	unsigned int temp, t2;
	double time;

	QueryPerformanceCounter(&PerformanceCount);

	temp = ((unsigned int)PerformanceCount.LowPart >> m_iLowShift) | ((unsigned int)PerformanceCount.HighPart << (32 - m_iLowShift));

	if (first)
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
		if ((temp <= oldtime) && ((oldtime - temp) < 0x10000000))
		{
			oldtime = temp;
		}
		else
		{
			t2 = temp - oldtime;
			time = (double)t2 * m_flPerfCounterFreq;
			oldtime = temp;
			m_flCurrentTime += time;

			if (m_flCurrentTime == m_flLastCurrentTime)
			{
				sametimecount++;

				if (sametimecount > 100000)
				{
					m_flCurrentTime += 1.0;
					sametimecount = 0;
				}
			}
			else
				sametimecount = 0;

			m_flLastCurrentTime = m_flCurrentTime;
		}
	}

	return m_flCurrentTime;
}
#else
#include <sys/time.h>
#include <unistd.h>
void CPerformanceCounter::InitializePerformanceCounter(void)
{
       struct timeval  tp;
       gettimeofday( &tp, NULL );
       secbase = tp.tv_sec;
}

float CPerformanceCounter::GetCurTime(void)
{
        struct timeval  tp;
        gettimeofday( &tp, NULL );

        if ( !secbase )
        {
                secbase = tp.tv_sec;
                return ( tp.tv_usec / 1000000.0 );
        }

	
	return (( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0 );

}
#endif
