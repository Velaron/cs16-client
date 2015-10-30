#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
DECLARE_MESSAGE( m_Money, Money )

int CHudMoney::Init( )
{
	HOOK_MESSAGE( Money );
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	return 1;
}

int CHudMoney::Draw(float flTime)
{
	if(( gHUD.m_iHideHUDDisplay & ( HIDEHUD_HEALTH ) ))
		return 1;
	int r, g, b;
	UnpackRGB(r,g,b, RGB_YELLOWISH);

	int iDollarWidth = gHUD.GetSpriteRect(m_hSprite).right - gHUD.GetSpriteRect(m_hSprite).left;

	int x = ScreenWidth - iDollarWidth * 7;
	int y = ScreenHeight - 3 * gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	SPR_Set(gHUD.GetSprite(m_hSprite), r, g, b);
	SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_hSprite));

	gHUD.DrawHudNumber2( x + iDollarWidth, y, false, 5, m_iMoneyCount, r, g, b );
	FillRGBA(x + iDollarWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 128 );
	return 1;
}

int CHudMoney::VidInit()
{
	m_hSprite = gHUD.GetSpriteIndex( "dollar" );

	return 1;
}

int CHudMoney::MsgFunc_Money(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_iMoneyCount = READ_LONG();
	return 1;
}
