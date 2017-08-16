#pragma once
#ifndef BASEWINDOW_H
#define BASEWINDOW_H

#include "ItemsHolder.h"
#include "BackgroundBitmap.h"

// Base class for windows.
// Should be used for message boxes, dialogs, root menus(e.g. frameworks)
class CMenuBaseWindow : public CMenuItemsHolder
{
public:
	CMenuBaseWindow( const char *name = "Unnamed Window" );

	// Overloaded functions
	// Window visibility is switched through window stack
	virtual void Hide();
	virtual void Show();
	virtual bool IsVisible();

	virtual const char *Key( int key, int down );
	virtual void Draw();

	enum EAnimation
	{
		ANIM_IN = 0,
		ANIM_OUT
	};

	// Override this method to draw custom animations
	// For example, during transitions
	// Return false when animation is still going
	// Otherwise return true, so window will be marked as "no animation"
	// and this method will not be called anymore(until next menu transition)
	virtual bool DrawAnimation( EAnimation anim );

	// Check current window is a root
	virtual bool IsRoot() { return false; }

	// Hide current window and save changes
	virtual void SaveAndPopMenu();

	// Events library
	DECLARE_EVENT_TO_MENU_METHOD( CMenuBaseWindow, SaveAndPopMenu );

	bool bAllowDrag;
	CMenuBackgroundBitmap background;
protected:

private:
	friend void UI_DrawMouseCursor( void ); // HACKHACK: Cursor should be set by menu item
	friend void UI_UpdateMenu( float flTime );

	virtual bool IsAbsolutePositioned( void ) { return true; }

	bool bInTransition;
	bool m_bHolding;
	Point m_bHoldOffset;


	void PushMenu();
	void PopMenu();
};

#endif // BASEWINDOW_H
