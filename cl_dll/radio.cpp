
#include "hud.h"
#include "parsemsg.h"
#include "cl_util.h"
#include <string.h>

DECLARE_MESSAGE( m_Radio, SendAudio )

int CHudRadio::Init( )
{
	HOOK_MESSAGE( SendAudio );
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	m_enableRadio = false;
	return 1;
}

int CHudRadio::Draw(float flTime)
{
	if( !m_enableRadio ) return 1;

	if( m_sentence[0] == '%' )
		PlaySound( &m_sentence[1], 100.0 );
	else
		PlaySound( m_sentence, 100.0 );

	m_enableRadio = false;

	return 1;
}

int CHudRadio::VidInit()
{
	return 1;
}

int CHudRadio::MsgFunc_SendAudio(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_bFirst = READ_BYTE( );
	strcpy( m_sentence, READ_STRING( ));
	m_sThird = READ_SHORT( );
	m_enableRadio = true;

	return 1;
}

void Broadcast( const char *sentence )
{
	if( sentence[0] == '%' )
		PlaySound( (char*)&sentence[1], 100.0 );
	else
		PlaySound( (char*)sentence, 100.0 );
}
