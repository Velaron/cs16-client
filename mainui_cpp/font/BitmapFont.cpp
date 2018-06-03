/*
BitmapFont.cpp - bitmap font backend
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

#include "BaseMenu.h"
#include "BaseFontBackend.h"
#include "menufont.h"
#include "BitmapFont.h"

CBitmapFont::CBitmapFont() : CBaseFont(), hImage( 0 ) { }
CBitmapFont::~CBitmapFont() { }

bool CBitmapFont::Create(const char *name, int tall, int weight, int blur, float brighten, int outlineSize, int scanlineOffset, float scanlineScale, int flags)
{
	Q_strncpy( m_szName, name, sizeof( m_szName ) );
	m_iHeight = m_iTall = tall;
	m_iWeight = weight;
	m_iFlags = flags;

	m_iBlur = blur;
	m_fBrighten = brighten;

	m_iOutlineSize = outlineSize;

	m_iScanlineOffset = scanlineOffset;
	m_fScanlineScale = scanlineScale;
	m_iAscent = 0;
	m_iMaxCharWidth = 0;

	hImage = EngFuncs::PIC_Load( "#XASH_SYSTEMFONT_001.bmp", menufont_bmp, sizeof( menufont_bmp ), 0 );
	iImageWidth = EngFuncs::PIC_Width( hImage );
	iImageHeight = EngFuncs::PIC_Height( hImage );
	int a, c;
	GetCharABCWidths( '.', a, m_iEllipsisWide, c );
	m_iEllipsisWide *= 3;

	return hImage != 0;
}

void CBitmapFont::GetCharRGBA(int ch, Point pt, Size sz, byte *rgba, Size &drawSize)
{
	// stub!
	Con_DPrintf( "CBitmapFont::GetCharRGBA\n" );
}

void CBitmapFont::GetCharABCWidths(int ch, int &a, int &b, int &c)
{
	a = c = 0;
	if( hImage )
		b = m_iHeight/2;
	else  b = 0;
}

bool CBitmapFont::HasChar(int ch) const
{
	if( ch >= 33 || ch <= 126 // ascii
		|| ch >= 0x0400 || ch <= 0x045F ) // cyrillic
		return true;
	return false;
}

bool CBitmapFont::IsValid() const
{
	// stub!
	Con_DPrintf( "CBitmapFont::IsValid\n" );
	return true;
}

void CBitmapFont::UploadGlyphsForRanges(charRange_t *range, int rangeSize)
{
	// stub!
	Con_DPrintf( "CBitmapFont::UploadGlyphsForRanges\n" );
}

int table_cp1251[64] = {
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
	0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x007F, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
	0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
	0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457
};

int CBitmapFont::DrawCharacter(int ch, Point pt, Size sz, const int color)
{
	// let's say we have twice lower width from height
	// cp1251 now
	if( ch >= 0x0410 && ch <= 0x042F )
		ch = ch - 0x410 + 0xC0;
	if( ch >= 0x0430 && ch <= 0x044F )
		ch = ch - 0x430 + 0xE0;
	else
	{
		int i;
		for( i = 0; i < 64; i++ )
			if( table_cp1251[i] == ch )
				ch = i + 0x80;
	}

	// Draw character doesn't works with alpha override
	// EngFuncs::DrawCharacter( pt.x, pt.y, sz.h / 2, sz.h, ch, (int)iColor, hImage );

	EngFuncs::PIC_Set( hImage, Red( color ), Green( color ), Blue( color ), Alpha( color ));

	float	row, col, size;
	col = (ch & 15) * 0.0625f + (0.5f / 256.0f);
	row = (ch >> 4) * 0.0625f + (0.5f / 256.0f);
	size = 0.0625f - (1.0f / 256.0f);

	wrect_t rc;
	int w, h;
	w = iImageWidth;
	h = iImageHeight;

	rc.top    = h * row;
	rc.left   = w * col;
	rc.bottom = rc.top + h * size;
	rc.right  = rc.left + w * size;

	EngFuncs::PIC_DrawTrans( pt.x, pt.y, sz.h / 2, sz.h, &rc );

	return sz.h / 2;
}
