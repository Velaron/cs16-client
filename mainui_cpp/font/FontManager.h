/*
FontManager.h - font manager
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
#pragma once
#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include "BaseMenu.h"
#include "utl/utlvector.h"

class CBaseFont;

enum EFontFlags
{
	FONT_NONE      = 0,
	FONT_ITALIC    = BIT( 0 ),
	FONT_UNDERLINE = BIT( 1 ),
	FONT_STRIKEOUT = BIT( 2 ),
	FONT_ADDITIVE  = BIT( 3 )
};

/*
 * Font manager is used for creating and operating with fonts
 **/
class CFontManager
{
public:
	CFontManager();
	~CFontManager();

	void VidInit();

	void DeleteAllFonts();

	void DeleteFont( HFont hFont );

	HFont GetFontByName( const char *name );
	void  GetCharABCWide( HFont font, int ch, int &a, int &b, int &c );
	int   GetFontTall( HFont font );
	int   GetFontAscent( HFont font );
	int   GetCharacterWidth( HFont font, int ch );
	bool  GetFontUnderlined( HFont font );

	void  GetTextSize( HFont font, const char *text, int *wide, int *tall = NULL, int size = -1 );
	int   GetTextHeight( HFont font, const char *text, int size = -1 );

	int   GetTextWide( HFont font, const char *text, int size = -1 );

	int   CutText(HFont fontHandle, const char *text, int height, int visibleSize , int &width);

	int GetTextWideScaled(HFont font, const char *text, const int height, int size = -1 );

	int DrawCharacter(HFont font, int ch, Point pt, Size sz, const int color );

	void DebugDraw( HFont font );
	CBaseFont *GetIFontFromHandle( HFont font );

	int GetEllipsisWide( HFont font ); // cached wide of "..."

	int GetCharacterWidthScaled(HFont font, int ch, int charH );
private:
	void UploadTextureForFont(CBaseFont *font );

	CUtlVector<CBaseFont*> m_Fonts;

	friend class CFontBuilder;
};

class CFontBuilder
{
public:
	CFontBuilder( const char *name, int tall, int weight )
	{
		m_szName = name;
		m_iTall = tall;
		m_iWeight = weight;

		m_iFlags = FONT_NONE;
		m_iBlur = m_iScanlineOffset = m_iOutlineSize = 0;
		m_hForceHandle = -1;
	}

	CFontBuilder &SetBlurParams( int blur, float brighten = 1.0f )
	{
		m_iBlur = blur;
		m_fBrighten = brighten;
		return *this;
	}

	CFontBuilder &SetOutlineSize( int outlineSize = 1 )
	{
		m_iOutlineSize = outlineSize;
		return *this;
	}

	CFontBuilder &SetScanlineParams( int offset = 2, float scale = 0.7f )
	{
		m_iScanlineOffset = offset;
		m_fScanlineScale = scale;
		return *this;
	}

	CFontBuilder &SetFlags( int flags )
	{
		m_iFlags = flags;
		return *this;
	}

	HFont Create();

private:
	CFontBuilder &SetHandleNum( HFont num )
	{
		m_hForceHandle = num;
		return *this;
	}

	const char *m_szName;
	int m_iTall, m_iWeight, m_iFlags;
	int m_iBlur;
	float m_fBrighten;

	int m_iOutlineSize;
	int m_iPreferredType;

	int m_iScanlineOffset;
	float m_fScanlineScale;
	HFont m_hForceHandle;
	friend class CFontManager;
};

extern CFontManager g_FontMgr;

#endif // FONTMANAGER_H
