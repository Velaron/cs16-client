#ifndef EVENTSYSTEM_H
#define EVENTSYSTEM_H

// Use these macros to set EventCallback, if no function pointer is available
// SET_EVENT must be followed by event code and closed by END_EVENT
// SET_EVENT_THIS can be used for setting event member inside it's class
#if defined(MY_COMPILER_SUCKS)

// WARN: can't rely on "item" name, because it can be something like "ptr->member"
// So we use something more valid for struct name

#define PASTE(x,y) __##x##_##y
#define PASTE2(x,y) PASTE(x,y)
#define EVNAME(x) PASTE2(x, __LINE__)

#define SET_EVENT( item, callback ) typedef struct { static void callback( CMenuBaseItem *pSelf, void *pExtra )
#define END_EVENT( item, callback ) } EVNAME(callback); (item).callback = EVNAME(callback)::callback;

#define SET_EVENT_THIS( callback ) SET_EVENT( this, callback )
#define END_EVENT_THIS( callback ) } EVNAME(callback);; this->callback = EVNAME(callback)::callback;

#else

#define SET_EVENT( item, callback ) (item).callback = [](CMenuBaseItem *pSelf, void *pExtra)
#define END_EVENT( item, callback ) ;

#define SET_EVENT_THIS( callback ) SET_EVENT( (*this), callback )
#define END_EVENT_THIS( callback ) END_EVENT( (*this), callback )

#endif

#define DECLARE_NAMED_EVENT_TO_ITEM_METHOD( className, method, eventName ) \
	static void eventName##Cb( CMenuBaseItem *pSelf, void * ) \
	{\
		((className *)pSelf)->method();\
	}
#define DECLARE_NAMED_EVENT_TO_MENU_METHOD( className, method, eventName ) \
	static void eventName##Cb( CMenuBaseItem *pSelf, void * ) \
	{\
		((className *)pSelf->Parent())->method();\
	}

#define DECLARE_EVENT_TO_MENU_METHOD( className, method ) \
	DECLARE_NAMED_EVENT_TO_MENU_METHOD( className, method, method )

#define DECLARE_EVENT_TO_ITEM_METHOD( className, method ) \
	DECLARE_NAMED_EVENT_TO_ITEM_METHOD( className, method, method )

class CMenuBaseItem;
class CMenuItemsHolder;

enum menuEvent_e
{
	QM_GOTFOCUS = 1,
	QM_LOSTFOCUS,
	QM_ACTIVATED,
	QM_CHANGED,
	QM_PRESSED,
	QM_IMRESIZED
};

typedef void (*EventCallback)(CMenuBaseItem *, void *);
typedef void (*VoidCallback)(void);

class CEventCallback
{
public:
	CEventCallback() : pExtra( NULL ), callback( NULL ), szName( NULL ) {}
	CEventCallback( EventCallback cb, void *ex = NULL ) : pExtra( ex ), callback( cb ), szName( NULL ) {}

	void *pExtra;

	operator bool() { return callback != NULL; }
	operator EventCallback() { return callback; }

	void operator() ( CMenuBaseItem *pSelf ) { callback( pSelf, pExtra ); }

	EventCallback operator =( EventCallback cb ) { return callback = cb; }
	VoidCallback  operator =( VoidCallback cb )
	{
		pExtra = (void*)cb; // extradata can't be used anyway
		callback = VoidCallbackWrapperCb;
		return cb;
	}
	size_t operator =( size_t null )
	{
		callback = (EventCallback)null;
		return null;
	}
	void *operator =( void *null )
	{
		callback = (EventCallback)null;
		return null;
	}

	void SetCommand( int execute_now, const char *sz )
	{
		cmd.execute_now = execute_now;
		cmd.cmd = sz;

		pExtra = &cmd;
		callback = CmdCallbackWrapperCb;
	}

	static void NoopCb( CMenuBaseItem *, void * ) {}

private:
	struct CmdCallback
	{
		int execute_now;
		const char *cmd;
	} cmd;
	EventCallback callback;


	static void VoidCallbackWrapperCb( CMenuBaseItem *pSelf, void *pExtra )
	{
		((VoidCallback)pExtra)();
	}

	static void CmdCallbackWrapperCb( CMenuBaseItem *pSelf, void *pExtra )
	{
		CmdCallback *cmd = (CmdCallback*)pExtra;
		EngFuncs::ClientCmd( cmd->execute_now, cmd->cmd );
	}

	// to find event command by name(for items holder)
	const char *szName;

	friend class CMenuItemsHolder;
};


#endif // EVENTSYSTEM_H
