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

#define BUYING_EVERYONE 0
#define BUYING_ONLY_CT 1
#define BUYING_ONLY_T 2
#define BUYING_NO_ONE 3

class CMapInfo : public CPointEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);

public:
	int m_iBuyingStatus;
	int m_flBombRadius;
};