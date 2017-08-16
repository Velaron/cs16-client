#include "extdll_menu.h"
#include "BaseMenu.h"
#include "ProgressBar.h"

CMenuProgressBar::CMenuProgressBar() : CMenuBaseItem()
{
	m_flMin = 0.0f;
	m_flMax = 100.0f;
	m_flValue = 0.0f;
	m_pCvar	= NULL;
}

void CMenuProgressBar::LinkCvar( const char *cvName, float flMin, float flMax )
{
	m_pCvar = EngFuncs::CvarRegister( cvName, "0", 0 );

	m_flMax = flMax;
	m_flMin = flMin;
}

void CMenuProgressBar::SetValue( float flValue )
{
	if( flValue > 1.0f ) flValue = 1;
	if( flValue < 0.0f ) flValue = 0;
	m_flValue = flValue;
	m_pCvar = NULL;
}

void CMenuProgressBar::Draw( void )
{
	float flProgress;

	if( m_pCvar )
	{
		flProgress = bound( m_flMin, m_pCvar->value, m_flMax );
		flProgress = ( flProgress - m_flMin ) / ( m_flMax - m_flMin );
	}
	else
		flProgress = m_flValue;

	// draw the background
	UI_FillRect( m_scPos, m_scSize, uiInputBgColor );

	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w * flProgress, m_scSize.h, uiPromptTextColor );

	// draw the rectangle
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}
