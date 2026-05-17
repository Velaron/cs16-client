#pragma once
#ifndef PARTICLEMAN_H
#define PARTICLEMAN_H

#include "CBaseParticle.h"

class IParticleMan
{
public:
	virtual ~IParticleMan() {}
	virtual void SetUp(cl_enginefunc_t* pEnginefuncs) = 0;
	virtual void Update() = 0;
	virtual void SetVariables(float flGravity, Vector vViewAngles) = 0;
	virtual void ResetParticles() = 0;
	virtual void ApplyForce(Vector vOrigin, Vector vDirection, float flRadius, float flStrength, float flDuration) = 0;

	//Use this if you want to create a new particle without any overloaded functions, Think, Touch, etc.
	//Just call this function, set the particle's behavior and let it rip.
	virtual CBaseParticle* CreateParticle(Vector org, Vector normal, model_s* sprite, float size, float brightness, const char* classname) = 0;

	virtual void SetRender(int iRender) = 0;
};

extern IParticleMan* g_pParticleMan;
#endif
