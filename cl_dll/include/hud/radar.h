/*
radar.h
Copyright (C) 2015 a1batross
*/
#pragma once
#ifndef RADAR_H
#define RADAR_H

class CClientSprite;

class CHudRadar: public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw( float flTime );
	virtual void Reset();
	virtual void Shutdown();

	int MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf);

	void UserCmd_ShowRadar();
	void UserCmd_HideRadar();
	CClientSprite m_hRadar;
	CClientSprite m_hRadarOpaque;

	int MsgFunc_BombDrop(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_BombPickup(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf);
private:

	cvar_t *cl_radartype;

	int InitBuiltinTextures();
	void DrawPlayerLocation();
	void DrawRadarDot(int x, int y, int size, int r, int g, int b, int a);
	void DrawCross(int x, int y, int size, int r, int g, int b, int a );
	void DrawT( int x, int y, int size, int r, int g, int b, int a );
	void DrawFlippedT( int x, int y, int size, int r, int g, int b, int a );
	Vector WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles );

	bool bUseRenderAPI, bTexturesInitialized;
	int hDot, hCross, hT, hFlippedT;
};

#endif // RADAR_H
