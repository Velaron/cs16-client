#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "Action.h"
#include "ItemsHolder.h"
#include "MessageBox.h"

CMenuMessageBox::CMenuMessageBox() : CMenuBaseWindow()
{
	iFlags |= QMF_INACTIVE;
}

void CMenuMessageBox::_Init()
{
	background.bForceColor = true;
	background.iColor = uiPromptBgColor;

	dlgMessage.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	dlgMessage.SetCoord( 128, 44 );

	AddItem( background );
	AddItem( dlgMessage );
}

void CMenuMessageBox::SetMessage( const char *sz )
{
	dlgMessage.szName = sz;
}
