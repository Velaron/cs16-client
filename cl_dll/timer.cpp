#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

DECLARE_MESSAGE( m_Timer, RoundTime )
DECLARE_MESSAGE( m_Timer, ShowTimer )

int CHudTimer::Init()
{
	HOOK_MESSAGE( RoundTime );
	HOOK_MESSAGE( ShowTimer );
	m_iFlags = 0;
	m_bPanicColorChange = false;
	gHUD.AddHudElem(this);
	return 1;
}

int CHudTimer::VidInit()
{
	m_HUD_timer = gHUD.GetSpriteIndex( "stopwatch" );
	return 1;
}

int CHudTimer::Draw( float fTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) )
        return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;
	int r, g, b;
	// time must be positive
	int minutes = max( 0, (int)( m_iTime + m_fStartTime - gHUD.m_flTime ) / 60);
	int seconds = max( 0, (int)( m_iTime + m_fStartTime - gHUD.m_flTime ) - (minutes * 60));

	if( m_iTime < 20) )
	{
		UnpackRGB(r,g,b, RGB_YELLOWISH );
	}
	else
	{
		m_flPanicTime += gHUD.m_flTimeDelta;
		if( m_flPanicTime > ((float)seconds / 40.0f))
		{
			m_flPanicTime = 0;
			m_bPanicColorChange = !m_bPanicColorChange;
		}
		UnpackRGB( r, g, b, m_bPanicColorChange ? RGB_YELLOWISH : RGB_REDISH );

	}

    
    int iWatchWidth = gHUD.GetSpriteRect(m_HUD_timer).right - gHUD.GetSpriteRect(m_HUD_timer).left;
    
	int x = ScreenWidth/2;
	int y = ScreenHeight - 1.5 * gHUD.m_iFontHeight ;
    
    SPR_Set(gHUD.GetSprite(m_HUD_timer), r, g, b);
    SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_timer));
        
	x = gHUD.DrawHudNumber2( x + iWatchWidth, y, false, 2, minutes, r, g, b );
	FillRGBA(x + iWatchWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 100);
	FillRGBA(x + iWatchWidth / 4, y + gHUD.m_iFontHeight - gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 100);
	gHUD.DrawHudNumber2( x + iWatchWidth / 2, y, true, 2, seconds, r, g, b );
	return 1;
}

int CHudTimer::MsgFunc_RoundTime(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_iTime = READ_SHORT();
	m_fStartTime = gHUD.m_flTime;
	m_iFlags = HUD_ACTIVE;
	return 1;
}

int CHudTimer::MsgFunc_ShowTimer(const char *pszName, int iSize, void *pbuf)
{
	m_iFlags = HUD_ACTIVE;
	return 1;
}

DECLARE_MESSAGE( m_ProgressBar, BarTime )
DECLARE_MESSAGE( m_ProgressBar, BarTime2 )

int CHudProgressBar::Init()
{
	HOOK_MESSAGE( BarTime );
	HOOK_MESSAGE( BarTime2 );
	m_iFlags = 0;

	gHUD.AddHudElem(this);
	return 1;
}

int CHudProgressBar::VidInit()
{
	return 1;
}

int CHudProgressBar::Draw( float flTime )
{
	if( Percent >= 1.0f )
	{
		m_iFlags = 0;
		return 1;

	}
	Percent = ((flTime - StartTime) / Duration);

	gHUD.DrawDarkRectangle( ScreenWidth/4, ScreenHeight*2/3, ScreenWidth/2, ScreenHeight/30 );
	FillRGBA( ScreenWidth/4+2, ScreenHeight*2/3+2, Percent * (ScreenWidth/2-4), ScreenHeight/30-4, 255, 140, 0, 255 );

	return 1;
}

int CHudProgressBar::MsgFunc_BarTime(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	Duration = READ_SHORT();
	Percent = 0.0f;

	StartTime = gHUD.m_flTime;

	m_iFlags = HUD_ACTIVE;
	return 1;
}

int CHudProgressBar::MsgFunc_BarTime2(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	Duration = READ_SHORT();
	Percent = (float)READ_SHORT() / 100.0f;

	StartTime = gHUD.m_flTime;

	m_iFlags = HUD_ACTIVE;
	return 1;
}
