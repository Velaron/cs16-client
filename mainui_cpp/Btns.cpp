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


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "BtnsBMPTable.h"

#define ART_BUTTONS_MAIN		"gfx/shell/btns_main.bmp"	// we support bmp only

/*
=================
UI_LoadBmpButtons
=================
*/
void UI_LoadBmpButtons( void )
{
	memset( uiStatic.buttonsPics, 0, sizeof( uiStatic.buttonsPics ));

	int bmp_len_holder;
	byte *bmp_buffer = (byte*)EngFuncs::COM_LoadFile( ART_BUTTONS_MAIN, &bmp_len_holder );

	if( !bmp_buffer || !bmp_len_holder )
	{
		Con_Printf( "UI_LoadBmpButtons: btns_main.bmp not found\n" );
		return;
	}

	bmp_t bhdr;
	memcpy( &bhdr, bmp_buffer, sizeof( bmp_t ));

	int pallete_sz = bhdr.bitmapDataOffset - sizeof( bmp_t );

	uiStatic.buttons_height = ( bhdr.bitsPerPixel == 4 ) ? 80 : 78; // bugstompers issues
	uiStatic.buttons_width = bhdr.width - 3; // make some offset

	int stride = bhdr.width * bhdr.bitsPerPixel / 8;
	int cutted_img_sz = ((stride + 3 ) & ~3) * uiStatic.buttons_height;
	int CuttedBmpSize = sizeof( bmp_t ) + pallete_sz + cutted_img_sz;
	byte *img_data = &bmp_buffer[bmp_len_holder-cutted_img_sz];

	if ( bhdr.bitsPerPixel <= 8 )
	{
		byte* pallete=&bmp_buffer[sizeof( bmp_t )];
		byte* firstpixel_col=&pallete[img_data[0]*4];
		firstpixel_col[0]=firstpixel_col[1]=firstpixel_col[2]=0;
	}

	// determine buttons count by image height...
	// int EngFuncs::PIC_count = ( pInfoHdr->biHeight == 5538 ) ? PC_BUTTONCOUNT
	int pic_count = ( bhdr.height / 78 );

	bhdr.height = 78;     //uiStatic.buttons_height;
	bhdr.fileSize = CuttedBmpSize;
	bhdr.bitmapDataSize = CuttedBmpSize - bhdr.bitmapDataOffset;

	char fname[256];
	byte *raw_img_buff = (byte *)MALLOC( sizeof( bmp_t ) + pallete_sz + cutted_img_sz );

	for( int i = 0; i < pic_count; i++ )
	{
		int offset = 0;
		sprintf( fname, "#btns_%d.bmp", i );

		memcpy( raw_img_buff, bmp_buffer, offset);

		memcpy( &raw_img_buff[offset], &bhdr, sizeof( bmp_t ));
		offset += sizeof( bmp_t );

		if( bhdr.bitsPerPixel <= 8 )
		{
			memcpy( &raw_img_buff[offset], &bmp_buffer[offset], pallete_sz );
			offset += pallete_sz;
		}

		memcpy( &raw_img_buff[offset], img_data, cutted_img_sz );

		// upload image into viedo memory
		uiStatic.buttonsPics[i] = EngFuncs::PIC_Load( fname, raw_img_buff, CuttedBmpSize );

		img_data -= cutted_img_sz;
	}

	FREE( raw_img_buff );
	EngFuncs::COM_FreeFile( bmp_buffer );
}
