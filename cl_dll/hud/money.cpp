#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "vgui_parser.h"
DECLARE_MESSAGE( m_Money, Money )
DECLARE_MESSAGE( m_Money, BlinkAcct )

int CHudMoney::Init( )
{
	HOOK_MESSAGE( Money );
	HOOK_MESSAGE( BlinkAcct );
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	m_fFade = 0;
	return 1;
}

int CHudMoney::Draw(float flTime)
{
	if(( gHUD.m_iHideHUDDisplay & ( HIDEHUD_HEALTH ) ))
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	int r, g, b, alphaBalance;
	m_fFade -= gHUD.m_flTimeDelta;
	if( m_fFade < 0)
	{
		m_fFade = 0.0f;
		m_iDelta = 0;
	}
	float interpolate = ( 5 - m_fFade ) / 5;

	int iDollarWidth = gHUD.GetSpriteRect(m_HUD_dollar).right - gHUD.GetSpriteRect(m_HUD_dollar).left;

	int x = ScreenWidth - iDollarWidth * 7;
	int y = MONEY_YPOS;

	if( m_iBlinkAmt )
	{
		m_fBlinkTime += gHUD.m_flTimeDelta;
		UnpackRGB( r, g, b, m_fBlinkTime > 0.5f? RGB_REDISH : RGB_YELLOWISH );

		if( m_fBlinkTime > 1.0f )
		{
			m_fBlinkTime = 0.0f;
			--m_iBlinkAmt;
		}
	}
	else
	{
		if( m_iDelta != 0 )
		{
			if( m_iDelta > 0 )
			{
				r = interpolate * ((RGB_YELLOWISH & 0xFF0000) >> 16);
				g = (RGB_GREENISH & 0xFF00) >> 8;
				b = (RGB_GREENISH & 0xFF);
			}
			else if( m_iDelta < 0)
			{
				r = (RGB_REDISH & 0xFF0000) >> 16;
				g = ((RGB_REDISH & 0xFF00) >> 8) + interpolate * (((RGB_YELLOWISH & 0xFF00) >> 8) - ((RGB_REDISH & 0xFF00) >> 8));
				b = (RGB_REDISH & 0xFF) - interpolate * (RGB_REDISH & 0xFF);
			}
		}
		else UnpackRGB(r, g, b, RGB_YELLOWISH );
	}

	alphaBalance = 128 - interpolate * (128 - MIN_ALPHA);

	ScaleColors( r, g, b, alphaBalance );

	SPR_Set(gHUD.GetSprite(m_HUD_dollar), r, g, b);
	SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_dollar));

	gHUD.DrawHudNumber2( x + iDollarWidth, y, false, 5, m_iMoneyCount, r, g, b );
	FillRGBA(x + iDollarWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, alphaBalance );
	return 1;
}

int CHudMoney::VidInit()
{
	m_HUD_dollar = gHUD.GetSpriteIndex( "dollar" );
	m_HUD_minus = gHUD.GetSpriteIndex( "minus" );
	m_HUD_plus = gHUD.GetSpriteIndex( "plus" );

	return 1;
}

int CHudMoney::MsgFunc_Money(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	int iOldCount = m_iMoneyCount;
	m_iMoneyCount = READ_LONG();
	m_iDelta = m_iMoneyCount - iOldCount;
	m_fFade = 5.0f; //fade for 5 seconds
	return 1;
}

int CHudMoney::MsgFunc_BlinkAcct(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iBlinkAmt = READ_BYTE();
	m_fBlinkTime = 0;
	return 1;
}
