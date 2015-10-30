#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

DECLARE_MESSAGE( m_Timer, RoundTime )
DECLARE_MESSAGE( m_Timer, KillTimer )
DECLARE_MESSAGE( m_Timer, ShowTimer )

int CHudTimer::Init()
{
	HOOK_MESSAGE( RoundTime );
	HOOK_MESSAGE( KillTimer );
	HOOK_MESSAGE( ShowTimer );
	ShowTimer = false;
	m_iFlags = HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
}

int CHudTimer::VidInit()
{
	ShowTimer = false;
	m_HUD_timer = gHUD.GetSpriteIndex( "stopwatch" );
	return 1;
}

int CHudTimer::Draw( float fTime )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) || ( !ShowTimer ) )
        return 1;
	int r, g, b;
    UnpackRGB(r,g,b, RGB_YELLOWISH);
    
    int iWatchWidth = gHUD.GetSpriteRect(m_HUD_timer).right - gHUD.GetSpriteRect(m_HUD_timer).left;
    
	int x = ScreenWidth/2;
    int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
    
    SPR_Set(gHUD.GetSprite(m_HUD_timer), r, g, b);
    SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_timer));
    
    int minutes = (int)( Time + StartTime - gHUD.m_flTime ) / 60;
    int seconds = (int)( Time + StartTime - gHUD.m_flTime ) - (minutes * 60);

	if( minutes < 0 )
		minutes = 0;
	if( seconds < 0 )
		seconds = 0;
    
	x = gHUD.DrawHudNumber2( x + iWatchWidth * 2, y, true, 2, minutes, r, g, b );
    FillRGBA(x + iWatchWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 220);
    FillRGBA(x + iWatchWidth / 4, y + gHUD.m_iFontHeight - gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 220);
    gHUD.DrawHudNumber2( x + iWatchWidth / 2, y, true, 2, seconds, r, g, b );
	return 1;
}

int CHudTimer::MsgFunc_RoundTime(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	Time = READ_SHORT();
	StartTime = gHUD.m_flTime;
	ShowTimer = true;
	return 1;
}

int CHudTimer::MsgFunc_ShowTimer(const char *pszName, int iSize, void *pbuf)
{
	ShowTimer = true;
	return 1;
}

int CHudTimer::MsgFunc_KillTimer(const char *pszName, int iSize, void *pbuf)
{
	ShowTimer = false;
	return 1;
}

