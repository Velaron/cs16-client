#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "vgui_parser.h"
DECLARE_MESSAGE( m_Money, Money )

int CHudMoney::Init( )
{
	HOOK_MESSAGE( Money );
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

	int r, g, b, a = max(MIN_ALPHA, m_fFade);

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	UnpackRGB(r,g,b, RGB_YELLOWISH);

	ScaleColors(r, g, b, a );

	int iDollarWidth = gHUD.GetSpriteRect(m_hSprite).right - gHUD.GetSpriteRect(m_hSprite).left;

	int x = ScreenWidth - iDollarWidth * 7;
	int y = MONEY_YPOS;

	SPR_Set(gHUD.GetSprite(m_hSprite), r, g, b);
	SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_hSprite));

	gHUD.DrawHudNumber2( x + iDollarWidth, y, false, 5, m_iMoneyCount, r, g, b );
	FillRGBA(x + iDollarWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, a );
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
	m_fFade = 200.0f; //!!!
	return 1;
}
