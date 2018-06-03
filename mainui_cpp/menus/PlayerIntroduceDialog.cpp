/*
Copyright (C) 2017 a1batross.
PlayerIntroduceDialog.cpp -- dialog intended to let player introduce themselves: enter nickname

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

#include "BaseWindow.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "MessageBox.h"
#include "Field.h"
#include "PlayerIntroduceDialog.h"

static class CMenuPlayerIntroduceDialog : public CMenuBaseWindow
{
public:
	CMenuPlayerIntroduceDialog() : CMenuBaseWindow( "PlayerIntroduceDialog" ), msgBox( true )
	{
	}

	void WriteOrDiscard();
	virtual void _Init();
	virtual void _VidInit();
	virtual const char *Key( int key, int down );

private:
	CMenuAction dlgMessage;
	CMenuField name;
	CMenuPicButton ok;
	CMenuYesNoMessageBox msgBox;
} uiIntroduceDialog;

void CMenuPlayerIntroduceDialog::WriteOrDiscard()
{
	if( !UI::Names::CheckIsNameValid( name.GetBuffer() ) )
	{
		msgBox.Show();
	}
	else
	{
		name.WriteCvar();
		SaveAndPopMenu();
	}
}

const char *CMenuPlayerIntroduceDialog::Key( int key, int down )
{
	if( down && UI::Key::IsEscape( key ) )
	{
		return uiSoundNull; // handled
	}

	if( down && UI::Key::IsEnter( key ) && ItemAtCursor() == &name )
	{
		WriteOrDiscard();
	}

	return CMenuBaseWindow::Key( key, down );
}

void CMenuPlayerIntroduceDialog::_Init()
{
	iFlags |= QMF_DIALOG;

	background.bForceColor = true;
	background.iColor = uiPromptBgColor;

	dlgMessage.eTextAlignment = QM_TOP; // center
	dlgMessage.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	dlgMessage.SetCharSize( QM_DEFAULTFONT );
	dlgMessage.SetRect( 0, 24, 640, 128 );
	dlgMessage.szName = "Please, introduce yourself";

	name.bAllowColorstrings = true;
	name.szName = "Enter player name:";
	name.SetRect( 188, 140, 270, 32 );
	name.LinkCvar( "name" );
	name.iMaxLength = MAX_SCOREBOARDNAME;

	msgBox.SetMessage( "Please, choose another player name" );
	msgBox.Link( this );

	ok.szName = "Ok";
	ok.SetRect( 298, 204, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
	ok.SetPicture( PC_OK );
	ok.onActivated = VoidCb( &CMenuPlayerIntroduceDialog::WriteOrDiscard );

	AddItem( background );
	AddItem( dlgMessage );
	AddItem( name );
	AddItem( ok );
}

void CMenuPlayerIntroduceDialog::_VidInit( void )
{
	SetRect( DLG_X + 192, 256, 640, 256 );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;
}

void UI_PlayerIntroduceDialog_Precache()
{

}

void UI_PlayerIntroduceDialog_Show()
{
	uiIntroduceDialog.Show();
}
