//===== Copyright ?1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//===========================================================================//

class CPerformanceCounter
{
public:
	void InitializePerformanceCounter(void);
	float GetCurTime(void);

private:
#ifdef _WIN32
	int m_iLowShift;
	float m_flPerfCounterFreq;
	float m_flCurrentTime;
	float m_flLastCurrentTime;
#else
	int      secbase;
#endif
};
