/*
BaseFontBackend.cpp - common font renderer backend code
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
#include "BaseFontBackend.h"
#include "FontManager.h"
#include <math.h>
#include "Utils.h"

#ifndef M_PI
# define M_PI		3.14159265358979323846	/* pi */
#endif

#ifndef M_E
# define M_E		2.7182818284590452354	/* e */
#endif

bool CBaseFont::GlyphLessFunc( const glyph_t &a, const glyph_t &b )
{
	return a.ch < b.ch;
}

CBaseFont::CBaseFont()
	: m_szName( ), m_iTall(), m_iWeight(), m_iFlags(),
	m_iHeight(), m_iMaxCharWidth(), m_iAscent(),
	m_iBlur(), m_fBrighten(),
	m_iEllipsisWide( 0 ),
	m_glyphs(0, 0, GlyphLessFunc)
{
}


/*
=========================
CBaseFont::GetTextureName

Mangle texture name, so using same font names with different attributes will not confuse engine or font renderer
=========================
+*/
void CBaseFont::GetTextureName(char *dst, size_t len) const
{
	char attribs[256];
	int i = 0;
	if( GetFlags() & FONT_ITALIC ) attribs[i++] = 'i'; // 1 parameter
	if( GetFlags() & FONT_UNDERLINE ) attribs[i++] = 'u'; // 1 parameter
	if( IsAdditive() ) attribs[i++] = 'a'; // 1
	if( m_iBlur )
	{
		int chars = snprintf( attribs + i, sizeof( attribs ) - 1 - i, "g%i%.2f", m_iBlur, m_fBrighten );
		i += chars;
	}
	if( m_iOutlineSize )
	{
		int chars = snprintf( attribs + i, sizeof( attribs ) - 1 - i, "o%i", m_iOutlineSize );
		i += chars;
	}
	if( m_iScanlineOffset )
	{
		int chars = snprintf( attribs + i, sizeof( attribs ) - 1 - i, "s%i%.2f", m_iScanlineOffset, m_fScanlineScale );
		i += chars;
	}
	attribs[i] = 0;

	if( i == 0 )
	{
		snprintf( dst, len - 1, "%s_%i_%i_font.bmp", GetName(), GetTall(), GetWeight() );
		dst[len - 1] = 0;
	}
	else
	{
		attribs[i] = 0;
		snprintf( dst, len - 1, "%s_%i_%i_%s_font.bmp", GetName(), GetTall(), GetWeight(), attribs );
		dst[len - 1] = 0;
	}
}

#define MAX_PAGE_SIZE 256

void CBaseFont::UploadGlyphsForRanges(charRange_t *range, int rangeSize)
{
	const int maxWidth = GetMaxCharWidth();
	const int height = GetHeight();
	const int tempSize = maxWidth * height * 4; // allocate temporary buffer for max possible glyph size
	const Point nullPt( 0, 0 );
	char name[256];

	CBMP bmp( MAX_PAGE_SIZE, MAX_PAGE_SIZE );
	byte *rgbdata = bmp.GetTextureData();
	bmp_t *hdr = bmp.GetBitmapHdr();

	Size tempDrawSize( maxWidth, height );
	byte *temp = new byte[tempSize];

	// abscissa atlas optimization
	CUtlVector<uint> lines;
	int line = 0;

	// texture is reversed by Y coordinates
	int xstart = 0, ystart = hdr->height-1;
	for( int iRange = 0; iRange < rangeSize; iRange++ )
	{
		for( int ch = range[iRange].chMin; ch <= range[iRange].chMax; ch++ )
		{
			// clear temporary buffer
			memset( temp, 0, tempSize );

			// draw it to temp buffer
			Size drawSize;
			GetCharRGBA( ch, nullPt, tempDrawSize, temp, drawSize );

			// see if we need to go down or create a new page
			if( xstart + drawSize.w > (int)hdr->width )
			{
				// update or push
				if( lines.IsValidIndex( line ) )
				{
					lines[line] = xstart;
					line++;
					// do we have next or don't have it yet?
					if( lines.IsValidIndex( line ) )
						xstart = lines[line];
					else
						xstart = 0;
				}
				else
				{
					lines.AddToTail(xstart);
					line++;
					// obviously we don't have next
					xstart = 0;

				}
				ystart -= height + 1; // HACKHACK: Add more space between rows, this removes ugly 1 height pixel rubbish

				// No free space now
				if( ystart - height - 1 <= 0 )
				{
					if( hdr->height <= (int)hdr->width ) // prioritize height grow
					{
						int oldheight = hdr->height - ystart;
						bmp.Increase( hdr->width, hdr->height * 2 );
						hdr = bmp.GetBitmapHdr();
						ystart = hdr->height - oldheight - 1;
					}
					else
					{
						bmp.Increase( hdr->width * 2, hdr->height );
						hdr = bmp.GetBitmapHdr();
						line = 0;
						xstart = lines[line];
						ystart = hdr->height - 1;
					}

					// update pointers
					rgbdata = bmp.GetTextureData();
				}
			}

			// set rgbdata rect
			wrect_t rect;
			rect.top    = hdr->height - ystart;
			rect.bottom = hdr->height - ystart + height;
			rect.left   = xstart;
			rect.right  = xstart + drawSize.w;

			// copy glyph to rgbdata

			for( int y = 0; y < height; y++ )
			{
				byte *dst = &rgbdata[(ystart - y) * hdr->width * 4];
				byte *src = &temp[y * maxWidth * 4];
				for( int x = 0; x < drawSize.w; x++ )
				{
					byte *xdst = &dst[ ( xstart + x ) * 4 ];
					byte *xsrc = &src[ x * 4 ];

					// copy 4 bytes: R, G, B and A
					memcpy( xdst, xsrc, 4 );
				}
			}

			// move xstart
			xstart += drawSize.w;

			glyph_t glyph;
			glyph.ch = ch;
			glyph.rect = rect;
			glyph.texture = 0; // will be acquired later

			m_glyphs.Insert( glyph );
		}
	}

	GetTextureName( name, sizeof( name ) );
	// bmp.Increase( hdr->width * 2, hdr->height );
	// bmp.Increase( hdr->width, hdr->height * 2 );
	HIMAGE hImage = EngFuncs::PIC_Load( name, bmp.GetBitmap(), bmp.GetBitmapHdr()->fileSize, 0 );
	Con_DPrintf( "Uploaded %s to %i\n", name, hImage );
	//delete[] bmp;
	delete[] temp;

	for( int i = m_glyphs.FirstInorder();; i = m_glyphs.NextInorder( i ) )
	{
		if( !m_glyphs[i].texture )
			m_glyphs[i].texture = hImage;
		if( i == m_glyphs.LastInorder() )
			break;
	}

	int dotWideA, dotWideB, dotWideC;
	GetCharABCWidths( '.', dotWideA, dotWideB, dotWideC );
	m_iEllipsisWide = ( dotWideA + dotWideB + dotWideC ) * 3;
}


CBaseFont::~CBaseFont()
{
	char name[256];
	GetTextureName( name, sizeof( name ) );
	EngFuncs::PIC_Free( name );
}

bool CBaseFont::IsEqualTo(const char *name, int tall, int weight, int blur, int flags)  const
{
	if( stricmp( name, m_szName ))
		return false;

	if( m_iTall != tall )
		return false;

	if( m_iWeight != weight )
		return false;

	if( m_iBlur != blur )
		return false;

	if( m_iFlags != flags )
		return false;

	return true;
}

void CBaseFont::DebugDraw()
{
	HIMAGE hImage;
	char name[256];

	{
		GetTextureName( name, sizeof( name ) );

		hImage = EngFuncs::PIC_Load( name );
		int w, h;
		w = EngFuncs::PIC_Width( hImage );
		h = EngFuncs::PIC_Height( hImage );
		int x = 0;
		EngFuncs::PIC_Set( hImage, 255, 255, 255 );
		if( IsAdditive() )
			EngFuncs::PIC_DrawAdditive(  Point(x, 0), Size( w, h ) );
		else
			EngFuncs::PIC_DrawTrans( Point(x, 0), Size( w, h ) );

		for( int i = m_glyphs.FirstInorder();; i = m_glyphs.NextInorder( i ) )
		{
			if( m_glyphs[i].texture == hImage )
			{
				Point pt;
				Size sz;
				pt.x = x + m_glyphs[i].rect.left;
				pt.y = m_glyphs[i].rect.top;

				sz.w = m_glyphs[i].rect.right - m_glyphs[i].rect.left;
				sz.h = m_glyphs[i].rect.bottom - pt.y;

				UI_DrawRectangleExt( pt, sz, PackRGBA( 255, 0, 0, 255 ), 1 );

				int a, b, c;
				GetCharABCWidths( m_glyphs[i].ch, a, b, c );

				pt.x -= a;
				sz.w += c + a;

				UI_DrawRectangleExt( pt, sz, PackRGBA( 0, 255, 0, 255 ), 1, QM_LEFT | QM_RIGHT );

				int ascender = GetAscent();

				pt.y += ascender;
				UI_DrawRectangleExt( pt, sz, PackRGBA( 0, 0, 255, 255 ), 1, QM_TOP );
			}

			if( i == m_glyphs.LastInorder() )
				break;
		}
	}

}

void CBaseFont::ApplyBlur(Size rgbaSz, byte *rgba)
{
	if( !m_iBlur )
		return;

	const int size = rgbaSz.w * rgbaSz.h * 4;
	byte *src = new byte[size];
	double sigma2;
	memcpy( src, rgba, size );

	sigma2 = 0.5 * m_iBlur;
	sigma2 *= sigma2;
	float * distribution = new float[m_iBlur * 2 + 1];
	for( int x = 0; x <= m_iBlur * 2; x++ )
	{
		int val = x - m_iBlur;
		distribution[x] = (float)(1.0f / sqrt(2 * 3.14 * sigma2)) * pow(2.7, -1 * (val * val) / (2 * sigma2));

		// brightening factor
		distribution[x] *= m_fBrighten;
	}


	for( int y = 0; y < rgbaSz.h; y++ )
	{
		for( int x = 0; x < rgbaSz.w; x++ )
		{
			GetBlurValueForPixel( distribution, src, Point(x, y), rgbaSz, rgba );

			rgba += 4;
		}
	}

	delete[] distribution;
	delete[] src;
}

void CBaseFont::GetBlurValueForPixel(float *distribution, byte *src, Point srcPt, Size srcSz, byte *dest)
{
	float accum = 0.0f;
	bool additive = IsAdditive();

	// scan the positive x direction
	int maxX = Q_min( srcPt.x + m_iBlur, srcSz.w );
	int minX = Q_max( srcPt.x - m_iBlur, 0 );
	for( int x = minX; x < maxX; x++ )
	{
		int maxY = Q_min( srcPt.y + m_iBlur, srcSz.h );
		int minY = Q_max( srcPt.y - m_iBlur, 0);
		for (int y = minY; y < maxY; y++)
		{
			byte *srcPos = src + ((x + (y * srcSz.w)) * 4);

			// muliply by the value matrix
			float weight = distribution[(x - srcPt.x) + m_iBlur];
			float weight2 = distribution[(y - srcPt.y) + m_iBlur];
			accum += ( additive ? srcPos[0] : srcPos[3] ) * (weight * weight2);
		}
	}

	// all the values are the same for fonts, just use the calculated alpha
	if( !additive )
	{
		dest[0] = dest[1] = dest[2] = 255;
		dest[3] = Q_min( (int)(accum + 0.5f), 255);
	}
	else
	{
		dest[0] = dest[1] = dest[2] = Q_min( (int)(accum + 0.5f), 255 );
		dest[3] = 255;
	}
}

void CBaseFont::ApplyOutline(Point pt, Size rgbaSz, byte *rgba)
{
	if( !m_iOutlineSize )
		return;

	int x, y;

	for( y = pt.x; y < rgbaSz.h; y++ )
	{
		for( x = pt.y; x < rgbaSz.w; x++ )
		{
			byte *src = &rgba[(x + (y * rgbaSz.w)) * 4];

			if( src[3] != 0 )
				continue;

			int shadowX, shadowY;

			for( shadowX = -m_iOutlineSize; shadowX <= m_iOutlineSize; shadowX++ )
			{
				for( shadowY = -m_iOutlineSize; shadowY <= m_iOutlineSize; shadowY++ )
				{
					if( !shadowX && !shadowY )
						continue;

					int testX = shadowX + x, testY = shadowY + y;
					if( testX < 0 || testX >= rgbaSz.w ||
						testY < 0 || testY >= rgbaSz.h )
						continue;

					byte *test = &rgba[(testX + (testY * rgbaSz.w)) * 4];
					if( test[0] == 0 || test[1] == 0 || test[3] == 0 )
						continue;

					src[0] = src[1] = src[2] = 0;
					src[3] = -1;
				}
			}
		}
	}
}

void CBaseFont::ApplyScanline(Size rgbaSz, byte *rgba)
{
	if( m_iScanlineOffset < 2 )
		return;

	for( int y = 0; y < rgbaSz.h; y++ )
	{
		if( y % m_iScanlineOffset == 0 )
			continue;

		byte *src = &rgba[(y * rgbaSz.w) * 4];
		for( int x = 0; x < rgbaSz.w; x++, src += 4 )
		{
			src[0] *= m_fScanlineScale;
			src[1] *= m_fScanlineScale;
			src[2] *= m_fScanlineScale;
		}
	}
}

void CBaseFont::ApplyStrikeout(Size rgbaSz, byte *rgba)
{
	if( !(m_iFlags & FONT_STRIKEOUT) )
		return;

	const int y = rgbaSz.h * 0.5f;

	byte *src = &rgba[(y*rgbaSz.w) * 4];

	for( int x = 0; x < rgbaSz.w; x++, src += 4 )
	{
		src[0] = src[1] = src[2] = 127;
		src[3] = 255;
	}
}

int CBaseFont::DrawCharacter(int ch, Point pt, Size sz, const int color)
{
	Size charSize;
	int a, b, c, width;

#ifdef SCALE_FONTS
	float factor = (float)sz.h / (float)GetTall();
#endif

	GetCharABCWidths( ch, a, b, c );
	width = a + b + c;

	// skip whitespace
	if( ch == ' ' )
	{
#ifdef SCALE_FONTS
		if( sz.h > 0 )
		{
			return width * factor + 0.5f;
		}
		else
#endif
		{
			return width;
		}
	}

	CBaseFont::glyph_t find( ch );
	int idx = m_glyphs.Find( find );

	if( m_glyphs.IsValidIndex( idx ) )
	{
		CBaseFont::glyph_t &glyph = m_glyphs[idx];

		int r, g, b, alpha;

		UnpackRGBA(r, g, b, alpha, color );

#ifdef SCALE_FONTS	// Scale font
		if( sz.h > 0 )
		{
			charSize.w = (glyph.rect.right - glyph.rect.left) * factor + 0.5f;
			charSize.h = GetHeight() * factor + 0.5f;
		}
		else
#endif
		{
			charSize.w = glyph.rect.right - glyph.rect.left;
			charSize.h = GetHeight();
		}

		pt.x += a;

		EngFuncs::PIC_Set( glyph.texture, r, g, b, alpha );
		if( IsAdditive() )
			EngFuncs::PIC_DrawAdditive( pt, charSize, &glyph.rect );
		else
			EngFuncs::PIC_DrawTrans( pt, charSize, &glyph.rect );
	}

#ifdef SCALE_FONTS
	if( sz.h > 0 )
	{
		return width * factor + 0.5f;
	}
#endif
	return width;
}
