/*
BackgroundBitmap.cpp -- background menu item
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "BackgroundBitmap.h"
#include "Utils.h"

bool CMenuBackgroundBitmap::s_bEnableLogoMovie = false;
Size CMenuBackgroundBitmap::s_BackgroundImageSize;
int CMenuBackgroundBitmap::s_iBackgroundCount = 0;
CMenuBackgroundBitmap::bimage_t CMenuBackgroundBitmap::s_Backgroudns[MAX_BACKGROUNDS];

CMenuBackgroundBitmap::CMenuBackgroundBitmap() : CMenuBitmap()
{
	szPic = 0;
	iFlags = QMF_INACTIVE;
	bForceWON = false;
	bForceColor = false;
	iColor = 0xFF505050;
}

void CMenuBackgroundBitmap::VidInit()
{
	if( m_pParent )
	{
		pos.x = pos.y = 0;
		size = m_pParent->size; // fill parent
		CalcPosition();
		CalcSizes();
	}
}

void CMenuBackgroundBitmap::DrawInGameBackground()
{
	UI_FillRect( m_scPos, m_scSize, uiColorBlack );
}

void CMenuBackgroundBitmap::DrawColor()
{
	UI_FillRect( m_scPos, m_scSize, iColor );
}

void CMenuBackgroundBitmap::DrawBackgroundLayout( Point p, float xScale, float yScale )
{
	// iterate and draw all the background pieces
	for (int i = 0; i < s_iBackgroundCount; i++)
	{
		bimage_t &bimage = s_Backgroudns[i];
		int dx = (int)ceil(bimage.coord.x * xScale);
		int dy = (int)ceil(bimage.coord.y * yScale);
		int dw = (int)ceil(bimage.size.w * xScale);
		int dt = (int)ceil(bimage.size.h * yScale);

		EngFuncs::PIC_Set( bimage.hImage, 255, 255, 255, 255 );
		EngFuncs::PIC_Draw( p.x + dx, p.y + dy, dw, dt );
	}
}

/*
=================
CMenuBackgroundBitmap::Draw
=================
*/
void CMenuBackgroundBitmap::Draw()
{
	if( bForceColor )
	{
		DrawColor();
		return;
	}

	if( EngFuncs::ClientInGame() )
	{
		if( EngFuncs::GetCvarFloat( "cl_background" ) )
		{
			return;
		}

		if( EngFuncs::GetCvarFloat( "ui_renderworld" ) )
		{
			DrawInGameBackground();
			return;
		}
	}

	if( s_iBackgroundCount == 0 )
	{
		DrawColor();
		return;
	}

	if( szPic )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szPic );
		return;
	}

	Point p;
	float xScale, yScale;

	// Disable parallax effect. It's just funny, but not really needed
#if 0
	float flParallaxScale = 0.02;
	p.x = (uiStatic.cursorX - ScreenWidth) * flParallaxScale;
	p.y = (uiStatic.cursorY - ScreenHeight) * flParallaxScale;

	// work out scaling factors
	xScale = ScreenWidth / s_BackgroundImageSize.w * (1 + flParallaxScale);
	yScale = xScale;
#else
	p.x = p.y = 0;

	// work out scaling factors
	xScale = ScreenWidth / s_BackgroundImageSize.w;
	yScale = xScale;
#endif

	DrawBackgroundLayout( p, xScale, yScale );
}

// fallback to old background loader, if new was failed(some game builds does not have this file)
bool CMenuBackgroundBitmap::LoadBackgroundImageOld( bool gamedirOnly )
{
	const int backgroundRows    = 3;
	const int backgroundColumns = 4;
	char filename[512];

	s_iBackgroundCount = 0;
	s_bEnableLogoMovie = false; // no logos for Steam background

	for( int y = 0; y < backgroundRows; y++ )
	{
		for( int x = 0; x < backgroundColumns; x++ )
		{
			sprintf(filename, "resource/background/800_%d_%c_loading.tga", y + 1, 'a' + x);
			if( !EngFuncs::FileExists( filename, gamedirOnly ) )
				return false;

			bimage_t &bimage = s_Backgroudns[s_iBackgroundCount++];


			bimage.hImage = EngFuncs::PIC_Load( filename, PIC_NOFLIP_TGA );

			if( !bimage.hImage )
				return false;

			bimage.size.w = EngFuncs::PIC_Width( bimage.hImage );
			bimage.size.h = EngFuncs::PIC_Height( bimage.hImage );

			if( x > 0 )
			{
				bimage_t &prevbimage = s_Backgroudns[y * backgroundRows + x - 1];
				bimage.coord.x = prevbimage.coord.x + prevbimage.size.w;
			}
			else bimage.coord.x = 0;

			if( y > 0 )
			{
				bimage_t &prevbimage = s_Backgroudns[(y - 1) * backgroundRows + x];
				bimage.coord.y = prevbimage.coord.y + prevbimage.size.h;
			}
			else bimage.coord.y = 0;
		}
	}

	return true;
}

bool CMenuBackgroundBitmap::LoadBackgroundImageNew( bool gamedirOnly )
{
	char *afile = NULL, *pfile;
	char token[4096];

	bool loaded = false;

	s_iBackgroundCount = 0;
	s_bEnableLogoMovie = false; // no logos for Steam background

	if( !EngFuncs::FileExists("resource/BackgroundLayout.txt", gamedirOnly ) )
		return CMenuBackgroundBitmap::LoadBackgroundImageOld( gamedirOnly ); // fallback to hardcoded values

	afile = (char*)EngFuncs::COM_LoadFile( "resource/BackgroundLayout.txt" );

	pfile = afile;

	pfile = EngFuncs::COM_ParseFile( pfile, token );
	if( !pfile || strcmp( token, "resolution" )) // resolution at first!
		goto freefile;

	pfile = EngFuncs::COM_ParseFile( pfile, token );
	if( !pfile ) goto freefile;

	s_BackgroundImageSize.w = atoi( token );

	pfile = EngFuncs::COM_ParseFile( pfile, token );
	if( !pfile ) goto freefile;

	s_BackgroundImageSize.h = atoi( token );

	// Now read all tiled background list
	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )))
	{
		bimage_t img;

		img.hImage = EngFuncs::PIC_Load( token, PIC_NOFLIP_TGA );

		if( !img.hImage ) goto freefile;

		// ignore "scaled" attribute. What does it mean?
		pfile = EngFuncs::COM_ParseFile( pfile, token );
		if( !pfile ) goto freefile;

		pfile = EngFuncs::COM_ParseFile( pfile, token );
		if( !pfile ) goto freefile;
		img.coord.x = atoi( token );

		pfile = EngFuncs::COM_ParseFile( pfile, token );
		if( !pfile ) goto freefile;
		img.coord.y = atoi( token );

		img.size.w = EngFuncs::PIC_Width( img.hImage );
		img.size.h = EngFuncs::PIC_Height( img.hImage );

		s_Backgroudns[s_iBackgroundCount] = img;
		s_iBackgroundCount++;
	}

	loaded = true;

freefile:
	EngFuncs::COM_FreeFile( afile );
	return loaded;
}

bool CMenuBackgroundBitmap::CheckBackgroundSplash( bool gamedirOnly )
{
	s_iBackgroundCount = 0; // not a steam background
	s_bEnableLogoMovie = false;

	if( EngFuncs::FileExists( ART_BACKGROUND, gamedirOnly ))
	{
		s_Backgroudns[0].hImage = EngFuncs::PIC_Load( ART_BACKGROUND );

		if( !s_Backgroudns[0].hImage )
			return false;

		s_Backgroudns[0].coord.x = s_Backgroudns[0].coord.y = 0;
		s_Backgroudns[0].size.w = EngFuncs::PIC_Width( s_Backgroudns[0].hImage );
		s_Backgroudns[0].size.h = EngFuncs::PIC_Height( s_Backgroudns[0].hImage );
		s_BackgroundImageSize = s_Backgroudns[0].size;
		s_iBackgroundCount++;

		if( gamedirOnly )
		{
			// if we doesn't have logo.avi in gamedir we don't want to draw it
			s_bEnableLogoMovie = EngFuncs::FileExists( "media/logo.avi", TRUE );
		}

		return true;
	}

	return false;
}

void CMenuBackgroundBitmap::LoadBackground()
{
	bool haveBg;

	// try to load backgrounds from mod
	haveBg = CMenuBackgroundBitmap::LoadBackgroundImageNew( false ); // gameui background layouts are inherited
	if( haveBg ) return;

	haveBg = CMenuBackgroundBitmap::CheckBackgroundSplash( true );
	if( haveBg ) return;
}
