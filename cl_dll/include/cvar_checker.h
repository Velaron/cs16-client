#pragma once
#ifndef CVAR_CHECKER_H
#define CVAR_CHECKER_H

class CCVarCheckerPrivate;
class CCVarChecker
{
public:
	CCVarChecker();
	~CCVarChecker();

	void Init( void );

	// Must be run every frame
	void Run( float flTime );

	// Add to list
	void AddToCheckList( const char *szName, const char *szValue );
	void AddToCheckList( const char *szName, float flValue );

	void AddToCheckList( cvar_t *pCvar, const char *szValue );
	void AddToCheckList( cvar_t *pCvar, float flValue );

private:
	CCVarCheckerPrivate *p;
};

#endif // CVAR_CHECKER_H
