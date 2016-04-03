/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
// pm_math.c -- math primitives

#include "mathlib.h"
#include "const.h"
#include <math.h>

// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#ifdef MSC_VER
#pragma warning(disable : 4244)
#endif

vec3_t vec3_origin = {0,0,0};
int nanmask = 255<<23;

/*
=================
rsqrt
=================
*/
float rsqrt( float number )
{
	int	i;
	float	x, y;

	if( number == 0.0f )
		return 0.0f;

	x = number * 0.5f;
	i = *(int *)&number;	// evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);	// what the fuck?
	y = *(float *)&i;
	y = y * (1.5f - (x * y * y));	// first iteration

	return y;
}

float	anglemod(float a)
{
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}
#define RAD2DEG( x )	((float)(x) * (float)(180.f / M_PI))
#define DEG2RAD( x )	((float)(x) * (float)(M_PI / 180.f))

#ifdef VECTORIZE_SINCOS
// Test shown that this is not so effictively
#if defined(__SSE__) || defined(_M_IX86_FP)
#if defined(__SSE2__) || defined(_M_IX86_FP)
  #define USE_SSE2
 #endif
#include "sse_mathfun.h"
#endif


#if defined(__ARM_NEON__) || defined(__NEON__)
	#include "neon_mathfun.h"
#endif


void SinCosFastVector(float r1, float r2, float r3, float r4,
					  float *s0, float *s1, float *s2, float *s3,
					  float *c0, float *c1, float *c2, float *c3)
{
	v4sf rad_vector = {r1, r2, r3, r4};
	v4sf sin_vector, cos_vector;

	sincos_ps(rad_vector, &sin_vector, &cos_vector);

	*s0 = sin_vector[0];
	if(s1) *s1 = sin_vector[1];
	if(s2) *s2 = sin_vector[2];
	if(s3) *s3 = sin_vector[3];

	*c0 = cos_vector[0];
	if(s1) *c1 = cos_vector[1];
	if(s2) *c2 = cos_vector[2];
	if(s3) *c3 = cos_vector[3];
}
#endif

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		sr, sp, sy, cr, cp, cy;


#ifdef VECTORIZE_SINCOS
	SinCosFastVector( DEG2RAD(angles[YAW]),
					  DEG2RAD(angles[PITCH]),
					  DEG2RAD(angles[ROLL]), 0,
					  &sy, &sp, &sr, NULL,
					  &cy, &cp, &cr, NULL);
#else
	float		angle;
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);
#endif

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

void AngleVectorsTranspose (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		sr, sp, sy, cr, cp, cy;
	
#ifdef VECTORIZE_SINCOS
	SinCosFastVector( DEG2RAD(angles[YAW]),
					  DEG2RAD(angles[PITCH]),
					  DEG2RAD(angles[ROLL]), 0,
					  &sy, &sp, &sr, NULL,
					  &cy, &cp, &cr, NULL);
#else
	float		angle;
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);
#endif

	if (forward)
	{
		forward[0]	= cp*cy;
		forward[1]	= (sr*sp*cy+cr*-sy);
		forward[2]	= (cr*sp*cy+-sr*-sy);
	}
	if (right)
	{
		right[0]	= cp*sy;
		right[1]	= (sr*sp*sy+cr*cy);
		right[2]	= (cr*sp*sy+-sr*cy);
	}
	if (up)
	{
		up[0]		= -sp;
		up[1]		= sr*cp;
		up[2]		= cr*cp;
	}
}


void AngleMatrix (const vec3_t angles, float (*matrix)[4] )
{
	float		sr, sp, sy, cr, cp, cy;
	
#ifdef VECTORIZE_SINCOS
	SinCosFastVector( DEG2RAD(angles[YAW]),
					  DEG2RAD(angles[PITCH]),
					  DEG2RAD(angles[ROLL]), 0,
					  &sy, &sp, &sr, NULL,
					  &cy, &cp, &cr, NULL);
#else
	float		angle;
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);
#endif

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[1][0] = cp*sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[2][1] = sr*cp;
	matrix[0][2] = (cr*sp*cy+-sr*-sy);
	matrix[1][2] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0;
	matrix[1][3] = 0.0;
	matrix[2][3] = 0.0;
}

void AngleIMatrix (const vec3_t angles, float matrix[3][4] )
{
	float		sr, sp, sy, cr, cp, cy;
	
#ifdef VECTORIZE_SINCOS
	SinCosFastVector( DEG2RAD(angles[YAW]),
					  DEG2RAD(angles[PITCH]),
					  DEG2RAD(angles[ROLL]), 0,
					  &sy, &sp, &sr, NULL,
					  &cy, &cp, &cr, NULL);
#else
	float		angle;
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);
#endif

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy;
	matrix[0][1] = cp*sy;
	matrix[0][2] = -sp;
	matrix[1][0] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[1][2] = sr*cp;
	matrix[2][0] = (cr*sp*cy+-sr*-sy);
	matrix[2][1] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0;
	matrix[1][3] = 0.0;
	matrix[2][3] = 0.0;
}

void NormalizeAngles( float *angles )
{
	int i;
	// Normalize angles
	for ( i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0 )
		{
			angles[i] -= 360.0;
		}
		else if ( angles[i] < -180.0 )
		{
			angles[i] += 360.0;
		}
	}
}

/*
===================
InterpolateAngles

Interpolate Euler angles.
FIXME:  Use Quaternions to avoid discontinuities
Frac is 0.0 to 1.0 ( i.e., should probably be clamped, but doesn't have to be )
===================
*/
void InterpolateAngles( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;
	
	NormalizeAngles( start );
	NormalizeAngles( end );

	for ( i = 0 ; i < 3 ; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		output[i] = ang1 + d * frac;
	}

	NormalizeAngles( output );
}
 

/*
===================
AngleBetweenVectors

===================
*/
float AngleBetweenVectors( const vec3_t v1, const vec3_t v2 )
{
	float angle;
	float l1 = Length( v1 );
	float l2 = Length( v2 );

	if ( !l1 || !l2 )
		return 0.0f;

	angle = acos( DotProduct( v1, v2 ) ) / (l1*l2);
	angle = ( angle  * 180.0f ) / M_PI;

	return angle;
}


void VectorTransform (const vec3_t in1, float in2[3][4], vec3_t out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}


int VectorCompare (const vec3_t v1, const vec3_t v2)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (v1[i] != v2[i])
			return 0;
			
	return 1;
}

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

double sqrt(double x);

float Length(const vec3_t v)
{
	int		i;
	float	length = 0.0f;
		
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

float Distance(const vec3_t v1, const vec3_t v2)
{
	vec3_t d;
	VectorSubtract(v2,v1,d);
	return Length(d);
}

float VectorNormalize (vec3_t v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (length)
	{
		ilength = rsqrt( length );
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (const vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}


int Q_log2(int val)
{
	int answer=0;
	while (val>>=1)
		answer++;
	return answer;
}

void VectorMatrix( vec3_t forward, vec3_t right, vec3_t up)
{
	vec3_t tmp;

	if (forward[0] == 0 && forward[1] == 0)
	{
		right[0] = 1;	
		right[1] = 0; 
		right[2] = 0;
		up[0] = -forward[2]; 
		up[1] = 0; 
		up[2] = 0;
		return;
	}

	tmp[0] = 0; tmp[1] = 0; tmp[2] = 1.0;
	CrossProduct( forward, tmp, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
	VectorNormalize( up );
}


void VectorAngles( const vec3_t forward, vec3_t angles )
{
	float	tmp, yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt (forward[0]*forward[0] + forward[1]*forward[1]);
		pitch = (atan2(forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}
