/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "Slider.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"

#define ART_BANNER			"gfx/shell/head_audio"

class CMenuAudio : public CMenuFramework
{
private:
	virtual void _Init();
	virtual void _VidInit();
	void GetConfig();
	void SaveAndPopMenu();

	CMenuPicButton	done;

public:
	CMenuAudio() : CMenuFramework("CMenuAudio") { }

	CMenuSlider	soundVolume;
	CMenuSlider	musicVolume;
	CMenuSlider	suitVolume;
	CMenuSlider	vibration;
	CMenuCheckBox lerping;
	CMenuCheckBox noDSP;
	CMenuCheckBox muteFocusLost;
	CMenuCheckBox vibrationEnable;
	CMenuCheckBox reverseChannels;
};

static CMenuAudio		uiAudio;

/*
=================
CMenuAudio::GetConfig
=================
*/
void CMenuAudio::GetConfig( void )
{
	soundVolume.LinkCvar( "volume" );
	musicVolume.LinkCvar( "musicvolume" );
	suitVolume.LinkCvar( "suitvolume" );
	vibration.LinkCvar( "vibration_length" );

	lerping.LinkCvar( "s_lerping" );
	noDSP.LinkCvar( "dsp_off" );
	muteFocusLost.LinkCvar( "snd_mute_losefocus" );
	vibrationEnable.LinkCvar( "vibration_enable" );
	reverseChannels.LinkCvar( "s_reverse_channels" );
}

/*
=================
CMenuAudio::SetConfig
=================
*/
void CMenuAudio::SaveAndPopMenu( void )
{
	soundVolume.WriteCvar();
	musicVolume.WriteCvar();
	suitVolume.WriteCvar();
	vibration.WriteCvar();
	lerping.WriteCvar();
	noDSP.WriteCvar();
	muteFocusLost.WriteCvar();
	vibrationEnable.WriteCvar();
	reverseChannels.WriteCvar();

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuAudio::Init
=================
*/
void CMenuAudio::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	done.SetNameAndStatus( "Done", "Go back to the Configuration Menu");
	done.SetPicture( PC_DONE );
	done.onActivated = SaveAndPopMenuCb;

	soundVolume.SetNameAndStatus( "Game sound volume", "Set master volume level" );
	soundVolume.Setup( 0.0, 1.0, 0.05f );
	soundVolume.onChanged = CMenuEditable::WriteCvarCb;

	musicVolume.SetNameAndStatus( "Game music volume", "Set background music volume level" );
	musicVolume.Setup( 0.0, 1.0, 0.05f );
	musicVolume.onChanged = CMenuEditable::WriteCvarCb;

	suitVolume.SetNameAndStatus( "Suit volume", "Set suit volume level" );
	suitVolume.Setup( 0.0, 1.0, 0.05f );
	suitVolume.onChanged = CMenuEditable::WriteCvarCb;

	lerping.SetNameAndStatus( "Enable sound interpolation", "Enable/disable interpolation on sound output" );
	lerping.onChanged = CMenuEditable::WriteCvarCb;

	noDSP.SetNameAndStatus( "Disable DSP effects", "Disable sound processing (like echo, flanger, etc)" );
	noDSP.onChanged = CMenuEditable::WriteCvarCb;

	muteFocusLost.SetNameAndStatus( "Mute when inactive", "Disable sound when game goes into background" );
	muteFocusLost.onChanged = CMenuEditable::WriteCvarCb;

	vibrationEnable.SetNameAndStatus( "Enable vibration", "In-game vibration(when player injured, etc)");
	SET_EVENT( vibrationEnable, onChanged )
	{
		CMenuCheckBox *cb = (CMenuCheckBox *)pSelf;
		CMenuAudio *audio = (CMenuAudio *)pSelf->Parent();
		if( cb->bChecked )
		{
			audio->vibration.iFlags |= QMF_GRAYED|QMF_INACTIVE;
		}
		else
		{
			audio->vibration.iFlags &= ~(QMF_GRAYED|QMF_INACTIVE);
		}
	}
	END_EVENT( vibrationEnable, onChanged )

	vibration.SetNameAndStatus( "Vibration", "Default vibration length" );
	vibration.Setup( 0.0f, 5.0f, 0.05f );
	SET_EVENT( vibration, onChanged )
	{
		static float oldValue;
		float newValue = ((CMenuEditable*)pSelf)->CvarValue();
		if( oldValue != newValue )
		{
			char cmd[64];
			snprintf( cmd, 64, "vibrate %f", newValue );
			EngFuncs::ClientCmd( FALSE, cmd );
			CMenuEditable::WriteCvarCb( pSelf, pExtra );
			oldValue = newValue;
		}
	}
	END_EVENT( vibration, onChanged )

	reverseChannels.SetNameAndStatus( "Reverse audio channels", "Use it when you can't swap your headphones' speakers" );
	reverseChannels.onChanged = CMenuEditable::WriteCvarCb;

	GetConfig();

	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( soundVolume );
	AddItem( musicVolume );
	AddItem( suitVolume );
	AddItem( lerping );
	AddItem( noDSP );
	AddItem( muteFocusLost );
	AddItem( reverseChannels );
	AddItem( vibrationEnable );
	AddItem( vibration );
}

void CMenuAudio::_VidInit( )
{
	done.SetCoord( 72, 230 );

	soundVolume.SetCoord( 320, 280 );
	musicVolume.SetCoord( 320, 340 );
	suitVolume.SetCoord( 320, 400 );
	lerping.SetCoord( 320, 470 );
	noDSP.SetCoord( 320, 520 );
	muteFocusLost.SetCoord( 320, 570 );
	reverseChannels.SetCoord( 320, 620 );

	vibrationEnable.SetCoord( 700, 470 );
	vibration.SetCoord( 700, 570 );
}

/*
=================
UI_Audio_Precache
=================
*/
void UI_Audio_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_Audio_Menu
=================
*/
void UI_Audio_Menu( void )
{
	UI_Audio_Precache();

	uiAudio.Show();
}
