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
#include "Bitmap.h"
#include "PicButton.h"
#include "Slider.h"
#include "CheckBox.h"

#define ART_BANNER	  	"gfx/shell/head_vidoptions"
#define ART_GAMMA		"gfx/shell/gamma"

class CMenuVidOptions : public CMenuFramework
{
private:
	void _Init();
	void _VidInit();

public:
	CMenuVidOptions() : CMenuFramework( "CMenuVidOptions" ) { }
	void SaveAndPopMenu();
	void GammaUpdate();
	void GammaGet();

	int		outlineWidth;

	class CMenuVidPreview : public CMenuBitmap
	{
		virtual void Draw();
	} testImage;

	CMenuPicButton	done;

	CMenuSlider	screenSize;
	CMenuSlider	gammaIntensity;
	CMenuSlider	glareReduction;
	CMenuCheckBox	fastSky;
	CMenuCheckBox	hiTextures;
	CMenuCheckBox   vbo;
	CMenuCheckBox   bump;

	HIMAGE		hTestImage;
} uiVidOptions;


/*
=================
CMenuVidOptions::GammaUpdate
=================
*/
void CMenuVidOptions::GammaUpdate( void )
{
	float val = RemapVal( uiVidOptions.gammaIntensity.GetCurrentValue(), 0.0, 1.0, 1.8, 7.0 );
	EngFuncs::CvarSetValue( "gamma", val );
	EngFuncs::ProcessImage( uiVidOptions.hTestImage, val );
}

void CMenuVidOptions::GammaGet( void )
{
	float val = EngFuncs::GetCvarFloat( "gamma" );

	uiVidOptions.gammaIntensity.SetCurrentValue( RemapVal( val, 1.8f, 7.0f, 0.0f, 1.0f ) );
	EngFuncs::ProcessImage( uiVidOptions.hTestImage, val );

	uiVidOptions.gammaIntensity.SetOriginalValue( val );
}

void CMenuVidOptions::SaveAndPopMenu( void )
{
	screenSize.WriteCvar();
	glareReduction.WriteCvar();
	fastSky.WriteCvar();
	hiTextures.WriteCvar();
	vbo.WriteCvar();
	bump.WriteCvar();
	// gamma is already written

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuVidOptions::Ownerdraw
=================
*/
void CMenuVidOptions::CMenuVidPreview::Draw( )
{
	int		color = 0xFFFF0000; // 255, 0, 0, 255
	int		viewport[4];
	int		viewsize, size, sb_lines;

	viewsize = EngFuncs::GetCvarFloat( "viewsize" );

	if( viewsize >= 120 )
		sb_lines = 0;	// no status bar at all
	else if( viewsize >= 110 )
		sb_lines = 24;	// no inventory
	else sb_lines = 48;

	size = Q_min( viewsize, 100 );

	viewport[2] = m_scSize.w * size / 100;
	viewport[3] = m_scSize.h * size / 100;

	if( viewport[3] > m_scSize.h - sb_lines )
		viewport[3] = m_scSize.h - sb_lines;
	if( viewport[3] > m_scSize.h )
		viewport[3] = m_scSize.h;

	viewport[2] &= ~7;
	viewport[3] &= ~1;

	viewport[0] = (m_scSize.w - viewport[2]) / 2;
	viewport[1] = (m_scSize.h - sb_lines - viewport[3]) / 2;

	UI_DrawPic( m_scPos.x + viewport[0], m_scPos.y + viewport[1], viewport[2], viewport[3], uiColorWhite, szPic );
	UI_DrawRectangleExt( m_scPos, m_scSize, color, ((CMenuVidOptions*)Parent())->outlineWidth );
}

/*
=================
CMenuVidOptions::Init
=================
*/
void CMenuVidOptions::_Init( void )
{
#ifdef PIC_KEEP_RGBDATA
	hTestImage = EngFuncs::PIC_Load( ART_GAMMA, PIC_KEEP_RGBDATA );
#else
	hTestImage = EngFuncs::PIC_Load( ART_GAMMA, PIC_KEEP_SOURCE );
#endif

	banner.SetPicture(ART_BANNER);

	testImage.iFlags = QMF_INACTIVE;
	testImage.SetRect( 390, 225, 480, 450 );
	testImage.SetPicture( ART_GAMMA );

	done.SetNameAndStatus( "Done", "Go back to the Video Menu" );
	done.SetCoord( 72, 435 );
	done.SetPicture( PC_DONE );
	done.onActivated = VoidCb( &CMenuVidOptions::SaveAndPopMenu );

	screenSize.SetNameAndStatus( "Screen size",  "Set the screen size" );
	screenSize.SetCoord( 72, 280 );
	screenSize.Setup( 30, 120, 10 );
	screenSize.LinkCvar( "viewsize" );
	screenSize.onChanged = CMenuEditable::WriteCvarCb;

	gammaIntensity.SetNameAndStatus( "Gamma", "Set gamma value (0.5 - 2.3)" );
	gammaIntensity.SetCoord( 72, 340 );
	gammaIntensity.Setup( 0.0, 1.0, 0.025 );
	gammaIntensity.onChanged = VoidCb( &CMenuVidOptions::GammaUpdate );
	gammaIntensity.onCvarGet = VoidCb( &CMenuVidOptions::GammaGet );
	gammaIntensity.LinkCvar( "gamma" );

	glareReduction.SetCoord( 72, 400 );
	if( UI_IsXashFWGS() )
	{
		glareReduction.SetNameAndStatus( "Glare reduction", "Set glare reduction level" );
		glareReduction.Setup( 100, 300, 15 );
		glareReduction.LinkCvar( "r_flaresize" );
	}
	else
	{
		glareReduction.SetNameAndStatus( "Brightness", "Set brightness level" );
		glareReduction.Setup( 0, 3, 0.1 );
		glareReduction.LinkCvar( "brightness" );
	}

	bump.SetNameAndStatus( "Bump-mapping", "Enable bump mapping" );
	bump.SetCoord( 72, 515 );
	bump.LinkCvar( "r_bump" );
	if( !EngFuncs::GetCvarFloat( "r_vbo" ) )
		bump.SetGrayed( true );

	vbo.SetNameAndStatus( "Use VBO", "Use new world renderer. Faster, but rarely glitchy" );
	vbo.SetCoord( 72, 565 );
	vbo.LinkCvar( "r_vbo" );
	vbo.onChanged = CMenuCheckBox::BitMaskCb;
	vbo.onChanged.pExtra = &bump.iFlags;
	vbo.bInvertMask = true;
	vbo.iMask = QMF_GRAYED;

	fastSky.SetNameAndStatus( "Draw simple sky", "enable/disable fast sky rendering (for old computers)" );
	fastSky.SetCoord( 72, 615 );
	fastSky.LinkCvar( "r_fastsky" );

	hiTextures.SetNameAndStatus( "Allow materials", "let engine replace 8-bit textures with full color hi-res prototypes (if present)" );
	hiTextures.SetCoord( 72, 665 );
	hiTextures.LinkCvar( "host_allow_materials" );

	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( screenSize );
	AddItem( gammaIntensity );
	AddItem( glareReduction );
	AddItem( bump );
	AddItem( vbo );
	AddItem( fastSky );
	AddItem( hiTextures );
	AddItem( testImage );
}

void CMenuVidOptions::_VidInit()
{
	outlineWidth = 2;
	UI_ScaleCoords( NULL, NULL, &outlineWidth, NULL );
}

/*
=================
CMenuVidOptions::Precache
=================
*/
void UI_VidOptions_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
CMenuVidOptions::Menu
=================
*/
void UI_VidOptions_Menu( void )
{
	UI_VidOptions_Precache();
	uiVidOptions.Show();
}
