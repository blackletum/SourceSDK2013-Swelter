//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef weapon_kulak2_H
#define weapon_kulak2_H

#include "basebludgeonweapon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef HL2MP
#error weapon_kulak2.h must not be included in hl2mp. The windows compiler will use the wrong class elsewhere if it is.
#endif

#define	CROWBAR_RANGE	85.0f
#define	CROWBAR_REFIRE	0.6f

//-----------------------------------------------------------------------------
// CWeaponkulak2
//-----------------------------------------------------------------------------

class CWeaponkulak2 : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponkulak2, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CWeaponkulak2();

	float		GetRange( void )		{	return	CROWBAR_RANGE;	}
	float		GetFireRate( void )		{	return	CROWBAR_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );

	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
	void		SecondaryAttack( void )	{	return;	}
	virtual bool	Deploy(void);

	// Animation event
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
};

#endif // weapon_kulak2_H
