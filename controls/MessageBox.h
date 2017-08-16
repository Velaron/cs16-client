#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "BaseWindow.h"

class CMenuMessageBox : public CMenuBaseWindow
{
public:
	CMenuMessageBox();

	void SetMessage( const char *sz );
private:
	void _Init();

	CMenuAction dlgMessage;
};

#endif // MESSAGEBOX_H
