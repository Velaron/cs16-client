#pragma once
#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include "BaseWindow.h"

class CMenuFramework : public CMenuBaseWindow
{
public:
	CMenuFramework( const char *name = "Unnamed Framework" );

	void Show();
	void Init() FINAL;
	void VidInit() FINAL;
	void Hide();
	bool IsVisible();
	bool IsRoot() { return true; }

	bool DrawAnimation(EAnimation anim);

	CMenuBannerBitmap banner;
};

#endif // FRAMEWORK_H
