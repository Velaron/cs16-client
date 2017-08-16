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
#include "Action.h"
#include "ScrollList.h"
#include "PicButton.h"

#define ART_BANNER	  	"gfx/shell/head_touchoptions"
#define ART_GAMMA		"gfx/shell/gamma"

uiFileDialogGlobal_t	uiFileDialogGlobal;

class CMenuFileDialog : public CMenuFramework
{
public:
	CMenuFileDialog() : CMenuFramework("CMenuFileDialog") { }

private:
	virtual void _Init( void );
	virtual void _VidInit( void );
	static void SaveAndPopCb( CMenuBaseItem *pSelf, void *pExtra );
	static void UpdateExtra( CMenuBaseItem *pSelf, void *pExtra );
	void GetFileList( void );
	char		filePath[UI_MAXGAMES][95];
	char		*filePathPtr[UI_MAXGAMES];
	
	CMenuPicButton	done;
	CMenuPicButton	cancel;

	CMenuScrollList fileList;

public:
	class CPreview : public CMenuAction
	{
		public:
		virtual void Draw();
		HIMAGE image;
	} preview;
};

static CMenuFileDialog uiFileDialog;


void CMenuFileDialog::CPreview::Draw()
{
	UI_FillRect( m_scPos.x - 2, m_scPos.y - 2,  m_scSize.w + 4, m_scSize.h + 4, 0xFFC0C0C0 );
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, 0xFF808080 );
	EngFuncs::PIC_Set( image, 255, 255, 255, 255 );
	EngFuncs::PIC_DrawTrans( m_scPos, m_scSize );
}

void CMenuFileDialog::GetFileList( void )
{
	char	**filenames;
	int	i = 0, numFiles, j, k;


	for( k = 0; k < uiFileDialogGlobal.npatterns; k++)
	{
		filenames = EngFuncs::GetFilesList( uiFileDialogGlobal.patterns[k], &numFiles, TRUE );
		for ( j = 0; j < numFiles; i++, j++ )
		{
			if( i >= UI_MAXGAMES ) break;
			strcpy( filePath[i],filenames[j] );
			filePathPtr[i] = filePath[i];
		}
	}
	fileList.iNumItems = i;

	if( fileList.charSize.h )
	{
		fileList.iNumRows = (fileList.size.h / fileList.charSize.h) - 2;
		if( fileList.iNumRows > fileList.iNumItems )
			fileList.iNumRows = i;
	}

	for ( ; i < UI_MAXGAMES; i++ )
		filePathPtr[i] = NULL;


	fileList.pszItemNames = (const char **)filePathPtr;
	preview.image = EngFuncs::PIC_Load( filePath[ fileList.iCurItem ] );
}

void CMenuFileDialog::SaveAndPopCb(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuFileDialog *parent = (CMenuFileDialog*)pSelf->Parent();
	const char *fileName = (const char *)pExtra;

	strncpy( uiFileDialogGlobal.result, fileName, 256 );
	uiFileDialogGlobal.result[255] = 0;
	uiFileDialogGlobal.valid = false;
	uiFileDialogGlobal.callback( fileName[0] != 0 );

	parent->Hide();
}

void CMenuFileDialog::UpdateExtra(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuFileDialog *parent = (CMenuFileDialog*)pSelf->Parent();
	const char *fileName = parent->filePath[parent->fileList.iCurItem];

	parent->done.onActivated.pExtra = (void*)fileName;

	if( uiFileDialogGlobal.preview )
		parent->preview.image = EngFuncs::PIC_Load( fileName );
}

/*
=================
UI_FileDialog_Init
=================
*/
void CMenuFileDialog::_Init( void )
{
	// memset( &uiFileDialog, 0, sizeof( uiFileDialog_t ));

	// banner.SetPicture( ART_BANNER );

	done.SetNameAndStatus( "Done", "Use selected file" );
	done.SetPicture( PC_DONE );
	done.onActivated = SaveAndPopCb;

	cancel.SetNameAndStatus( "Cancel", "Cancel file selection" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onActivated = SaveAndPopCb;
	cancel.onActivated.pExtra = (void*)"";

	fileList.iFlags |= QMF_DROPSHADOW;
	fileList.onChanged = UpdateExtra;

	//preview.iFlags |= QMF_INACTIVE;

	GetFileList();

	AddItem( background );
	//AddItem( banner );
	AddItem( done );
	AddItem( cancel );
	AddItem( preview );
	AddItem( fileList );
}

void CMenuFileDialog::_VidInit()
{
	done.SetCoord( 72, 150 );
	cancel.SetCoord( 72, 210 );
	fileList.SetRect( 340, 150, 600, 500 );
	preview.SetRect( 72, 300, 196, 196 );

	preview.SetVisibility( uiFileDialogGlobal.preview );
}

/*
=================
UI_FileDialog_Precache
=================
*/
void UI_FileDialog_Precache( void )
{
	//EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_FileDialog_Menu
=================
*/
void UI_FileDialog_Menu( void )
{
	UI_FileDialog_Precache();

	uiFileDialog.Show();
}
