/*
radar.h
Copyright (C) 2015 a1batross
*/
#pragma once
#ifndef RADAR_H
#define RADAR_H

class CHudRadar: public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw( float flTime );
	virtual void Reset();

	int MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf);

	void UserCmd_ShowRadar();
	void UserCmd_HideRadar();
	wrect_t m_hrad;
	wrect_t m_hradopaque;

private:
	HSPRITE m_hRadar;
	HSPRITE m_hRadaropaque;

	cvar_t *cl_radartype;

	void DrawPlayerLocation();
	void DrawRadarDot(int x, int y, int size, int r, int g, int b, int a);
	void DrawCross(int x, int y, int size, int r, int g, int b, int a );
	Vector2D WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles );

};

#endif // RADAR_H
