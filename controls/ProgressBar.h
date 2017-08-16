#ifndef CMENUPROGRESSBAR_H
#define CMENUPROGRESSBAR_H

#include "BaseItem.h"

class CMenuProgressBar : public CMenuBaseItem
{
public:
    CMenuProgressBar();
	void Draw( void );
	void LinkCvar( const char *cvName, float flMin, float flMax );
	void SetValue( float flValue );

private:
	float m_flMin, m_flMax, m_flValue;
	cvar_t *m_pCvar;
};

#endif // CMENUPROGRESSBAR_H
