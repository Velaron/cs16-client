#include "BaseWindow.h"
#include "CheckBox.h"
#include "ConnectionWarning.h"
#include "PicButton.h"
#include "Action.h"

enum EPresets { EPRESET_NORMAL = 0, EPRESET_DSL, EPRESET_SLOW, EPRESET_LAST };

static class CMenuConnectionWarning : public CMenuBaseWindow
{
public:
	CMenuConnectionWarning() : CMenuBaseWindow( "ConnectionWarning" )
	{

	}
	virtual void _Init();
	virtual void _VidInit();
	virtual const char *Key( int key, int down );

	void WriteSettings(const EPresets preset );

	CMenuPicButton done;
private:
	CMenuPicButton options;
	CMenuCheckBox normal, dsl, slowest;
	CMenuAction title, message;
} uiConnectionWarning;

const char *CMenuConnectionWarning::Key( int key, int down )
{
	if( down && UI::Key::IsEscape( key ) )
	{
		return uiSoundNull; // handled
	}

	return CMenuBaseWindow::Key( key, down );
}

void CMenuConnectionWarning::_Init()
{
	iFlags |= QMF_DIALOG;

	background.bForceColor = true;
	background.iColor = uiPromptBgColor;

	normal.szName = "Normal internet connection";
	normal.SetCoord( 20, 140 );
	SET_EVENT( normal.onChanged,
		((CMenuConnectionWarning*)pSelf->Parent())->WriteSettings( EPRESET_NORMAL ) );

	dsl.szName = "DSL or PPTP with limited packet size";
	dsl.SetCoord( 20, 200 );
	SET_EVENT( dsl.onChanged,
		((CMenuConnectionWarning*)pSelf->Parent())->WriteSettings( EPRESET_DSL ) );

	slowest.szName = "Slow connection mode (64kbps)";
	slowest.SetCoord( 20, 260 );
	SET_EVENT( slowest.onChanged,
		((CMenuConnectionWarning*)pSelf->Parent())->WriteSettings( EPRESET_SLOW ) );

	done.SetPicture( PC_DONE );
	done.szName = "Done";
	done.SetGrayed( true );
	done.SetRect( 410, 320, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
	done.onActivated = VoidCb( &CMenuConnectionWarning::Hide );
	done.bEnableTransitions = false;

	options.SetPicture( PC_ADV_OPT );
	options.szName = "Adv Options";
	SET_EVENT_MULTI( options.onActivated,
	{
		UI_GameOptions_Menu();
		uiConnectionWarning.done.SetGrayed( false );
	});
	options.SetRect( 154, 320, UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );
	options.bEnableTransitions = false;

	title.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	title.eTextAlignment = QM_CENTER;
	title.szName = "Connection problem";
	title.SetRect( 0, 16, 640, 20 );

	message.iFlags = QMF_INACTIVE;
	message.szName = "Too many lost packets while connecting!\nPlease select network settings";
	message.SetRect( 20, 60, 600, 32 );

	AddItem( background );
	AddItem( done );
	AddItem( options );
	AddItem( normal );
	AddItem( dsl );
	AddItem( slowest );
	AddItem( title );
	AddItem( message );
}

void CMenuConnectionWarning::_VidInit()
{
	SetRect( DLG_X + 192, 192, 640, 384 );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;
}

void CMenuConnectionWarning::WriteSettings( const EPresets preset)
{
	const struct
	{
		float cl_maxpacket;
		float cl_maxpayload;
		float cl_cmdrate;
		float cl_updaterate;
		float rate;
	} presets[EPRESET_LAST] =
	{
	{ 1400, 0,    30, 60, 25000 },
	{ 1200, 1000, 30, 60, 25000 },
	{ 900,  700,  25, 30, 7500 }
	};

	EngFuncs::CvarSetValue("cl_maxpacket",  presets[preset].cl_maxpacket );
	EngFuncs::CvarSetValue("cl_maxpayload", presets[preset].cl_maxpayload );
	EngFuncs::CvarSetValue("cl_cmdrate",    presets[preset].cl_cmdrate );
	EngFuncs::CvarSetValue("cl_updaterate", presets[preset].cl_updaterate );
	EngFuncs::CvarSetValue("rate",          presets[preset].rate );

	normal.bChecked  = preset == EPRESET_NORMAL;
	dsl.bChecked     = preset == EPRESET_DSL;
	slowest.bChecked = preset == EPRESET_SLOW;

	done.SetGrayed( false );
}

void UI_ConnectionWarning_f()
{
	if( !UI_IsVisible() )
		UI_Main_Menu();
	uiConnectionWarning.Show();
}
