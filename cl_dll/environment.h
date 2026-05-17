#pragma once
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "wrect.h"
#include "cl_dll.h"
#include "com_model.h"

#include <vector>

class CPartRainDrop;
class CPartWind;

struct ParticleParams
{
	float minSize;
	float maxSize;
	Vector color;
	byte brightness;
	byte renderMode;
	short lightFlag;

	float GetSize() const;
};

struct WeatherData
{
	WeatherData();

	int entIndex;
	int flags;

	short intensity;
	float updatePeriod;
	short distance;

	short minHeight;
	short maxHeight;

	Vector location;

	Vector absmin;
	Vector absmax;

	float updateTime;

	bool IsLocalizedPoint() const;
	bool IsVolume() const;
	bool IsGlobal() const;
	bool AllowIndoors() const;
	bool DistanceIsRadius() const;
	bool NeedsPVSCheck() const;
	float RandomHeight() const;
	bool CanGoThroughBrushEntities() const;
	bool ShouldTraceWater() const;

	Vector GetWeatherOrigin(const Vector& globalWeatherOrigin) const;
	void GetBoundingBox(const Vector& weatherOrigin, Vector& mins, Vector& maxs) const;
	Vector GetRandomOrigin(const Vector& weatherOrigin) const;
};

struct RainData : public WeatherData
{
	RainData();

	ParticleParams raindropParticleParams;
	float raindropStretchY;
	float raindropMinSpeed;
	float raindropMaxSpeed;
	float raindropLife;

	float GetRaindropFallingSpeed() const;

	ParticleParams windParticleParams;
	float windpuffLife;

	ParticleParams splashParticleParams;
	ParticleParams rippleParticleParams;

	bool RaindropsAffectedByWind() const;
	bool WindPuffsAllowed() const;
	bool SplashesAllowed() const;
	bool RipplesAllowed() const;

	model_t* rainSprite;
	model_t* windPuffSprite;
	model_t* splashSprite;
	model_t* rippleSprite;
};

struct SnowData : public WeatherData
{
	SnowData();

	ParticleParams snowflakeParticleParams;
	byte snowflakeInitialBrightness;
	float snowflakeMinSpeed;
	float snowflakeMaxSpeed;
	float snowflakeLife;

	float GetSnowflakeFallingSpeed() const;

	bool SnowflakesAffectedByWind() const;

	model_t* snowSprite;
};

class CEnvironment
{
public:
	CEnvironment() = default;

	float GetOldTime() const { return m_flOldTime; }

	void Initialize();
	void Reset();
	void Clear();
	void Update();
	void RestoreWeather();

	int MsgFunc_Rain(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_Snow(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_WeatherPos(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ReceiveW(const char *pszName, int iSize, void *pbuf);

private:
	void RemoveRain(int entIndex);
	void AddRain(const RainData& rainData);
	void RemoveSnow(int entIndex);
	void AddSnow(const SnowData& snowData);

	bool ShouldUpdateWind() const;
	void UpdateWind();
	void UpdateRain(const RainData& rainData);
	void UpdateSnow(const SnowData& snowData);

	CPartRainDrop* CreateRaindrop(const Vector& vecOrigin, const RainData& rainData);
	CPartWind* CreateWindParticle(const Vector& vecOrigin, const RainData& rainData);
	void CreateSnowFlake(const Vector& vecOrigin, const SnowData& snowData);

	model_t* LoadSprite(const char* spriteName);

	int m_iSavedWeatherType = 0;
private:
	Vector m_vecWeatherOrigin;

	model_t* m_pRainSprite = nullptr;
	model_t* m_pGasPuffSprite = nullptr;
	model_t* m_pRainSplash = nullptr;
	model_t* m_pRipple = nullptr;
	model_t* m_pSnowSprite = nullptr;

	Vector m_vecWind;

	Vector m_vecDesiredWindDirection;

	float m_flDesiredWindSpeed;
	float m_flNextWindChangeTime;

	float m_flOldTime;

	float m_flIdealYaw;
	float m_flWeatherValue;

	std::vector<RainData> m_rains;
	std::vector<SnowData> m_snows;

private:
	CEnvironment( const CEnvironment& ) = delete;
	CEnvironment& operator=( const CEnvironment& ) = delete;
};

extern CEnvironment g_Environment;

#endif
