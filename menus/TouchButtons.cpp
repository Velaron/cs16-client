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
#include "mobility_int.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Slider.h"
#include "Field.h"
#include "Action.h"
#include "ScrollList.h"
#include "CheckBox.h"
#include "YesNoMessageBox.h"

#define ART_BANNER	  	"gfx/shell/head_touch_buttons"

class CMenuTouchButtons : public CMenuFramework
{
public:
	CMenuTouchButtons() : CMenuFramework( "CMenuTouchButtons" ) { }

	void AddButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags );
	void GetButtonList();
	bool gettingList;

private:
	void _Init();
	void _VidInit();
public:
	void DeleteButton();
	void ResetButtons();
	void UpdateFields();

	DECLARE_EVENT_TO_MENU_METHOD( CMenuTouchButtons, ResetButtons )
	DECLARE_EVENT_TO_MENU_METHOD( CMenuTouchButtons, DeleteButton )
	DECLARE_EVENT_TO_MENU_METHOD( CMenuTouchButtons, UpdateFields )
	static void ExitMenuCb(CMenuBaseItem *pSelf, void *pExtra);
	// Use an event system here!
	// Don't make this static method cry.
	static void FileDialogCallback( bool success );


	char		bNames[UI_MAXGAMES][95];
	char		bTextures[UI_MAXGAMES][95];
	char		bCommands[UI_MAXGAMES][95];
	unsigned char bColors[UI_MAXGAMES][4];
	int        bFlags[UI_MAXGAMES];

	HIMAGE textureid;
	char selectedName[256];
	int curflags;

    char		*bNamesPtr[UI_MAXGAMES];
	
	CMenuPicButton	done;
	CMenuPicButton	cancel;

	CMenuSlider	red;
	CMenuSlider	green;
	CMenuSlider	blue;
	CMenuSlider	alpha;
	CMenuCheckBox	hide;
	CMenuCheckBox	sp;
	CMenuCheckBox	mp;
	CMenuCheckBox	lock;
	CMenuCheckBox	additive;
	CMenuCheckBox	precision;
	CMenuPicButton	reset;
	CMenuPicButton	remove;
	CMenuPicButton	save;
	CMenuPicButton	select;
	CMenuPicButton	editor;
	CMenuField	command;
	CMenuField	texture;
	CMenuField	name;
	class CMenuColor : public CMenuAction
	{
	public:
		virtual void Draw();
	} color;
	class CMenuButtonPreview : public CMenuAction
	{
	public:
		virtual void Draw();
		HIMAGE textureId;
	} preview;
	CMenuScrollList buttonList;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
	bool initialized;

};

static CMenuTouchButtons uiTouchButtons;


void CMenuTouchButtons::AddButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	if( !gettingList )
		return;

	int i = buttonList.iNumItems++;

	strcpy( bNames[i], name );
	bNamesPtr[i] = uiTouchButtons.bNames[i];
	strcpy( bTextures[i], texture );
	strcpy( bCommands[i], command );
	memcpy( bColors[i], color, 4 );
	bFlags[i] = flags;
}

// Engine callback
extern "C" EXPORT void AddTouchButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	uiTouchButtons.AddButtonToList( name, texture, command, color, flags );
}


void CMenuTouchButtons::GetButtonList()
{
	if( !initialized )
		return;

	buttonList.iNumItems = 0;

	EngFuncs::ClientCmd( TRUE, "" ); // perform Cbuf_Execute()

	gettingList = true;
	EngFuncs::ClientCmd( TRUE, "touch_list\n" );
	gettingList = false;

	int i = buttonList.iNumItems;

	if( buttonList.charSize.h )
	{
		buttonList.iNumRows = (buttonList.size.h / buttonList.charSize.h ) - 2;
		if( buttonList.iNumRows > buttonList.iNumItems )
			buttonList.iNumRows = i;
	}

	for ( ; i < UI_MAXGAMES; i++ )
		bNamesPtr[i] = NULL;

	buttonList.pszItemNames = (const char **)bNamesPtr;

	UpdateFields();
}

void CMenuTouchButtons::CMenuColor::Draw()
{
	CMenuTouchButtons *parent = (CMenuTouchButtons *)Parent();

	uint color = ((uint)parent->blue.GetCurrentValue()) |
				(((uint)parent->green.GetCurrentValue()) << 8) |
				(((uint)parent->red.GetCurrentValue()) << 16) |
				(((uint)parent->alpha.GetCurrentValue()) << 24);

	UI_FillRect( m_scPos, m_scSize, color );
}

void CMenuTouchButtons::CMenuButtonPreview::Draw()
{
	CMenuTouchButtons *parent = (CMenuTouchButtons *)Parent();

	UI_FillRect( m_scPos.x - 2, m_scPos.y - 2, m_scSize.w + 4, m_scSize.h + 4, 0xFFC0C0C0 );
	UI_FillRect( m_scPos, m_scSize, 0xFF808080 );

	EngFuncs::PIC_Set( textureId,
		(int)parent->red.GetCurrentValue(),
		(int)parent->green.GetCurrentValue(),
		(int)parent->blue.GetCurrentValue(),
		(int)parent->alpha.GetCurrentValue() );
	if( parent->additive.bChecked )
	{
		EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
	}
	else
	{
		EngFuncs::PIC_DrawTrans( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
	}
}

void CMenuTouchButtons::DeleteButton()
{
	char command[256];
	snprintf(command, 256, "touch_removebutton \"%s\"\n", selectedName );
	EngFuncs::ClientCmd(1, command);
	GetButtonList();
}

void CMenuTouchButtons::ResetButtons()
{
	EngFuncs::ClientCmd( 0, "touch_removeall\n" );
	EngFuncs::ClientCmd( 1, "touch_loaddefaults\n" );
	GetButtonList();
}

void CMenuTouchButtons::UpdateFields( )
{
	int i = buttonList.iCurItem;

	strcpy( selectedName, bNames[i]);
	red.SetCurrentValue( bColors[i][0] );
	green.SetCurrentValue( bColors[i][1] );
	blue.SetCurrentValue( bColors[i][2] );
	alpha.SetCurrentValue( bColors[i][3] );

	curflags = bFlags[i];
	mp.bChecked = !!( curflags & TOUCH_FL_MP );
	sp.bChecked = !!( curflags & TOUCH_FL_SP );
	lock.bChecked = !!( curflags & TOUCH_FL_NOEDIT );
	hide.bChecked = !!( curflags & TOUCH_FL_HIDE );
	additive.bChecked = !!( curflags & TOUCH_FL_DRAW_ADDITIVE );
	precision.bChecked = !!( curflags & TOUCH_FL_PRECISION );

	const char *buf = texture.GetBuffer();
	if( buf[0] && buf[0] != '#' )
		preview.textureId = EngFuncs::PIC_Load(buf);
	else
		preview.textureId = 0;

	name.Clear();
	texture.SetBuffer( bTextures[i] );
	command.SetBuffer( bCommands[i] );
}

void CMenuTouchButtons::FileDialogCallback( bool success )
{
	if( success )
	{
		uiTouchButtons.texture.SetBuffer( uiFileDialogGlobal.result );
		uiTouchButtons.textureid = EngFuncs::PIC_Load(uiTouchButtons.texture.GetBuffer());
	}
}

void CMenuTouchButtons::ExitMenuCb(CMenuBaseItem *pSelf, void *pExtra)
{
	const char *cmd = (const char *)pExtra;

	EngFuncs::ClientCmd( 0, cmd );
	pSelf->Parent()->Hide();
}

/*
=================
UI_TouchButtons_Init
=================
*/
void CMenuTouchButtons::_Init( void )
{
	initialized = true;

	banner.SetPicture(ART_BANNER);

	done.SetNameAndStatus( "Done", "Save changes and go back to the Touch Menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = ExitMenuCb;
	done.onActivated.pExtra = (void*)"touch_writeconfig\n";

	cancel.SetNameAndStatus( "Cancel", "Discard changes and go back to the Touch Menu" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onActivated = ExitMenuCb;
	cancel.onActivated.pExtra = (void*)"touch_loadconfig\n";

	red.eFocusAnimation = QM_PULSEIFFOCUS;
	red.SetNameAndStatus( "Red:", "Texture red channel" );
	red.Setup( 0, 255, 1 );

	green.eFocusAnimation = QM_PULSEIFFOCUS;
	green.SetNameAndStatus( "Green:", "Texture green channel" );
	green.Setup( 0, 255, 1 );

	blue.eFocusAnimation = QM_PULSEIFFOCUS;
	blue.SetNameAndStatus( "Blue:", "Texture blue channel" );
	blue.Setup( 0, 255, 1 );

	alpha.eFocusAnimation = QM_PULSEIFFOCUS;
	alpha.SetNameAndStatus( "Alpha:", "Texture alpha channel" );
	alpha.Setup( 0, 255, 1 );

	hide.SetNameAndStatus( "Hide", "Show/hide button" );
	hide.iMask = TOUCH_FL_HIDE;
	hide.onChanged.pExtra = &curflags;
	hide.onChanged = CMenuCheckBox::BitMaskCb;

	additive.SetNameAndStatus( "Additive", "Set button additive draw mode" );
	additive.iMask = TOUCH_FL_DRAW_ADDITIVE;
	additive.onChanged.pExtra = &curflags;
	additive.onChanged = CMenuCheckBox::BitMaskCb;

	mp.SetNameAndStatus( "MP", "Show button only in multiplayer" );
	SET_EVENT( mp, onChanged )
	{
		CMenuCheckBox *self = (CMenuCheckBox*)pSelf;
		CMenuTouchButtons *parent = (CMenuTouchButtons*)pSelf->Parent();
		if( self->bChecked )
		{
			parent->curflags |= TOUCH_FL_MP;
			parent->curflags &= ~TOUCH_FL_SP;
			parent->sp.bChecked = false;
		}
		else
		{
			parent->curflags &= ~TOUCH_FL_MP;
		}
	}
	END_EVENT( mp, onChanged )

	sp.SetNameAndStatus( "SP", "Show button only in singleplayer" );
	SET_EVENT( sp, onChanged )
	{
		CMenuCheckBox *self = (CMenuCheckBox*)pSelf;
		CMenuTouchButtons *parent = (CMenuTouchButtons*)pSelf->Parent();
		if( self->bChecked )
		{
			parent->curflags |= TOUCH_FL_SP;
			parent->curflags &= ~TOUCH_FL_MP;
			parent->mp.bChecked = false;
		}
		else
		{
			parent->curflags &= ~TOUCH_FL_SP;
		}
	}
	END_EVENT( sp, onChanged )

	lock.SetNameAndStatus( "Lock", "Lock button editing" );
	lock.iMask = TOUCH_FL_NOEDIT;
	lock.onChanged.pExtra = &curflags;
	lock.onChanged = CMenuCheckBox::BitMaskCb;

	precision.SetNameAndStatus( "Look precision", "Increase look precision" );
	precision.iMask = TOUCH_FL_PRECISION;
	precision.onChanged.pExtra = &curflags;
	precision.onChanged = CMenuCheckBox::BitMaskCb;

	save.SetNameAndStatus( "Save", "Save as new button" );
	save.SetPicture("gfx/shell/btn_touch_save");
	SET_EVENT( save, onActivated )
	{
		char command[256];
		CMenuTouchButtons *parent = (CMenuTouchButtons*)pSelf->Parent();

		if( parent->name.GetBuffer()[0] )
		{
			snprintf( command, 256, "touch_addbutton \"%s\" \"%s\" \"%s\"\n",
				parent->name.GetBuffer(),
				parent->texture.GetBuffer(),
				parent->command.GetBuffer() );
			EngFuncs::ClientCmd(0, command);

		}
		else
		{
			snprintf( command, 256, "touch_settexture \"%s\" \"%s\"\n", parent->selectedName, parent->texture.GetBuffer() );
			EngFuncs::ClientCmd(0, command);
			snprintf( command, 256, "touch_setcommand \"%s\" \"%s\"\n", parent->selectedName, parent->command.GetBuffer() );
			EngFuncs::ClientCmd(0, command);
		}

		snprintf( command, 256, "touch_setflags \"%s\" %i\n", parent->name.GetBuffer(), parent->curflags );
		EngFuncs::ClientCmd(0, command);

		snprintf( command, 256, "touch_setcolor \"%s\" %u %u %u %u\n", parent->name.GetBuffer(),
			(uint)parent->red.GetCurrentValue(),
			(uint)parent->green.GetCurrentValue(),
			(uint)parent->blue.GetCurrentValue(),
			(uint)parent->alpha.GetCurrentValue() );
		EngFuncs::ClientCmd(1, command);

		if( parent->name.GetBuffer()[0] )
		{
			parent->name.Clear();
		}
		parent->GetButtonList();
	}
	END_EVENT( save, onActivated )

	editor.SetNameAndStatus( "Editor", "Open interactive editor" );
	editor.SetPicture("gfx/shell/btn_touch_editor");
	editor.onActivated = UI_TouchEdit_Menu;

	select.SetNameAndStatus( "Select", "Select texture from list" );
	select.SetPicture("gfx/shell/btn_touch_select");
	SET_EVENT( select, onActivated )
	{
		// TODO: Remove uiFileDialogGlobal
		// TODO: Make uiFileDialog menu globally known
		// TODO: Make FileDialogCallback as event
		uiFileDialogGlobal.npatterns = 7;
		strcpy( uiFileDialogGlobal.patterns[0], "touch/*.tga");
		strcpy( uiFileDialogGlobal.patterns[1], "touch_default/*.tga");
		strcpy( uiFileDialogGlobal.patterns[2], "gfx/touch/*");
		strcpy( uiFileDialogGlobal.patterns[3], "gfx/vgui/*");
		strcpy( uiFileDialogGlobal.patterns[4], "gfx/shell/*");
		strcpy( uiFileDialogGlobal.patterns[5], "*.tga");
		uiFileDialogGlobal.preview = true;
		uiFileDialogGlobal.valid = true;
		uiFileDialogGlobal.callback = CMenuTouchButtons::FileDialogCallback;
		UI_FileDialog_Menu();
	}
	END_EVENT( select, onActivated )

	name.szName = "New Button:";
	name.iMaxLength = 255;

	command.szName = "Command:";
	command.iMaxLength = 255;

	texture.szName = "Texture:";
	texture.iMaxLength = 255;
	SET_EVENT( texture, onChanged )
	{
		CMenuField *self = (CMenuField*)pSelf;

		const char *buf = self->GetBuffer();
		if( buf[0] && buf[0] != '#' )
			((CMenuTouchButtons*)self->Parent())->preview.textureId = EngFuncs::PIC_Load( buf );
		else
			((CMenuTouchButtons*)self->Parent())->preview.textureId = 0;
	}
	END_EVENT( texture, onChanged )

	msgBox.SetPositiveButton( "Ok", PC_OK );
	msgBox.SetNegativeButton( "Cancel", PC_CANCEL );

	reset.SetNameAndStatus( "Reset", "Reset touch to default state" );
	reset.SetPicture( "gfx/shell/btn_touch_reset" );
	SET_EVENT( reset, onActivated )
	{
		CMenuTouchButtons *parent = (CMenuTouchButtons*)pSelf->Parent();
		parent->msgBox.SetMessage( "Reset all buttons?" );
		parent->msgBox.onPositive = CMenuTouchButtons::ResetButtonsCb;
		parent->msgBox.Show();
	}
	END_EVENT( reset, onActivated )

	remove.SetNameAndStatus( "Delete", "Delete selected button" );
	remove.SetPicture( PC_DELETE );
	SET_EVENT( remove, onActivated )
	{
		CMenuTouchButtons *parent = (CMenuTouchButtons*)pSelf->Parent();
		parent->msgBox.SetMessage( "Delete selected button?" );
		parent->msgBox.onPositive = CMenuTouchButtons::DeleteButtonCb;
		parent->msgBox.Show();
	}
	END_EVENT( remove, onActivated )

	buttonList.pszItemNames = (const char **)bNamesPtr;
	buttonList.onChanged = UpdateFieldsCb;
	msgBox.Link( this );

	AddItem( background );
	AddItem( remove );
	AddItem( reset );
	AddItem( done );
	AddItem( cancel );
	AddItem( red );
	AddItem( green );
	AddItem( blue );
	AddItem( alpha );
	AddItem( hide );
	AddItem( additive );
	AddItem( precision );
	AddItem( sp );
	AddItem( mp );
	AddItem( lock );

	AddItem( buttonList );

	AddItem( save );
	AddItem( select );
	AddItem( editor );

	AddItem( banner );
	AddItem( color );
	AddItem( preview );
	AddItem( command );
	AddItem( texture );
	AddItem( name );

	GetButtonList();
}

void CMenuTouchButtons::_VidInit()
{
	int sliders_x = uiStatic.width - 344;
	int fields_w = 205 + uiStatic.width - 1024;
	if( fields_w < 205 ) fields_w = 205;

	banner.SetCoord( 72, 0 );
	done.SetCoord( 72, 550 );
	cancel.SetCoord( 72, 600 );

	red.SetCoord( sliders_x, 150 );
	green.SetCoord( sliders_x, 210 );
	blue.SetCoord( sliders_x, 270 );
	alpha.SetCoord( sliders_x, 330 );

	additive.SetCoord( 650, 470 );

	mp.SetCoord( 400, 420 );
	sp.SetCoord( 160 - 72 + 400, 420 );
	lock.SetCoord( 256 - 72 + 400, 420 );
	hide.SetCoord( 384 - 72 + 400, 420 );

	precision.SetCoord( 400, 470 );
	buttonList.SetRect( 72, 150, 300, 370 );

	save.SetRect( 384 - 42 + 320, 550, 130, 50 );
	editor.SetRect( 384 - 42 + 320, 600, 130, 50 );
	select.SetRect( 400 + fields_w - 95, 300, 150,50 );

	name.SetRect( 400, 550, 205, 32 );
	command.SetRect( 400, 150, fields_w, 32 );
	texture.SetRect( 400, 250, fields_w, 32 );
	color.SetRect( sliders_x + 120, 360, 70, 50 );
	preview.SetRect( 400, 300, 70, 70 );

	reset.SetRect( 384 - 72 + 480, 600, 130, 50 );
	remove.SetRect( 384 - 72 + 480, 550, 130, 50 );
}

/*
=================
UI_TouchButtons_Precache
=================
*/
void UI_TouchButtons_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
	uiTouchButtons.gettingList = false; // prevent filling list before init
}

/*
=================
UI_TouchButtons_Menu
=================
*/
void UI_TouchButtons_Menu( void )
{
    UI_TouchButtons_Precache();

	uiTouchButtons.Show();
}

void UI_TouchButtons_GetButtonList()
{
	uiTouchButtons.GetButtonList();
}
