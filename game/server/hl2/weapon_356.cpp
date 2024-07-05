//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// extern ConVar	sde_drop_mag;

// extern ConVar	sde_holster;

//-----------------------------------------------------------------------------
// CWeapon356
//-----------------------------------------------------------------------------

class CWeapon356 : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeapon356, CBaseHLCombatWeapon );
public:

	CWeapon356( void );

	void	PrimaryAttack( void );
	void	SecondaryAttack(void);
	void	HoldIronsight(void);
	void	FillDrum(void);
	virtual void	ItemPostFrame(void);
	bool	Deploy(void);
	bool	Reload(void);
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	Pump(void);
	float	WeaponAutoAimScale()	{ return 0.6f; }
	float	GetFireRate()			{ return 1.1f; }


	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
protected:
	int m_iRoundsLoadedInDrum;
private:
	bool	m_bNeedPump;
};

LINK_ENTITY_TO_CLASS( weapon_356, CWeapon356 );

PRECACHE_WEAPON_REGISTER( weapon_356 );

IMPLEMENT_SERVERCLASS_ST( CWeapon356, DT_Weapon356 )
END_SEND_TABLE()

BEGIN_DATADESC( CWeapon356 )

DEFINE_FIELD (m_iRoundsLoadedInDrum, FIELD_INTEGER),
DEFINE_FIELD(m_bNeedPump, FIELD_BOOLEAN),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon356::CWeapon356( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
	m_iRoundsLoadedInDrum = 6; //our revolver is found loaded, we're not EZ2 - HEVcrab
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon356::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBaseCombatCharacter *BCCOwner = GetOwner();
	CBasePlayer *pOwner = ToBasePlayer( BCCOwner );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				//regardless of sde_drop_mag value, eject only spent casings 
				//if (sde_drop_mag.GetInt())
				//{
					CEffectData data;
					// Eject (how many rounds were loaded previous time minus remaining ammo) spent casings (even for realistic mag drop, let the player keep rounds remaining in the drum) 
					for (int i = 0; i < m_iRoundsLoadedInDrum - m_iClip1; i++)
					{
						data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector(-4, 4);
						data.m_vAngles = QAngle(90, random->RandomInt(0, 360), 0);
						data.m_nEntIndex = entindex();

						DispatchEffect("ShellEject", data);
					}
				        m_iRoundsLoadedInDrum = MIN(GetWpnData().iMaxClip1, m_iClip1 + BCCOwner->GetAmmoCount(m_iPrimaryAmmoType));
				//}
				break;
			}
	}
}

bool CWeapon356::Deploy(void)
{

	DevMsg("SDE_SMG!_deploy\n");
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
		pPlayer->ShowCrosshair(true);
	DisplaySDEHudHint();

	bool return_value = BaseClass::Deploy();

	m_bForbidIronsight = true; // to suppress ironsight during deploy in case the weapon is empty and the player has ammo 
	// -> reload will be forced. Behavior of ironsightable weapons that don't bolt on deploy

	if (m_iClip1 || !pPlayer->GetAmmoCount(m_iPrimaryAmmoType))
	{
		pPlayer->SetNextAttack(gpGlobals->curtime + 1.0f); // this revolver's deploy animation looks bad first 1 second in ironsight,
														   // set the moment when calling ItemPostFrame() starts to enable ironsight
	}

	return return_value;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon356::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	
	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	WeaponSound( SINGLE ); //snd1
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration(); //was 0.75
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.1;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	
	
	pPlayer->SetTimeBase(2);
	pPlayer->FireBullets( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -2, 2 );
	angles.y += random->RandomInt( -2, 2 );
	angles.z = 0;

	pPlayer->SnapEyeAngles( angles );

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}

	if (m_iClip1 >= 1)
		m_bNeedPump = true;
}

void CWeapon356::HoldIronsight(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer->m_afButtonPressed & IN_IRONSIGHT)
	{
		EnableIronsights();
		pPlayer->ShowCrosshair(false);
	}
	if (pPlayer->m_afButtonReleased & IN_IRONSIGHT)
	{
		DisableIronsights();
		pPlayer->ShowCrosshair(true);
	}
}

void CWeapon356::Pump(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	m_bNeedPump = false;

	WeaponSound(SPECIAL1);

	// Finish reload animation
	SendWeaponAnim(ACT_SHOTGUN_PUMP);

	pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	//pOwner->m_flNextAttack = gpGlobals->curtime + 0.5;
	//m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;

}

bool CWeapon356::Reload(void)
{
	//m_bExpSighted.SdeDisableIrons();
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	bool fRet;

	if (m_iClip1 < 1)
	{
		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_NOBOLD);
	}
	else
	{
		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	}

	if (fRet)
	{
		m_bInReload = true;
		return fRet;
	}
	else
		return false;
}

void CWeapon356::FillDrum(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	// Add them to the clip
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		while (Clip1() < GetMaxClip1())
		{
			m_iClip1++;
			pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) == 0) break;
		}
	}
}

void CWeapon356::SecondaryAttack(void)
{
	//// Only the player fires this way so we can cast
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return;
	}

	ToggleIronsights();
	pOwner->ToggleCrosshair();
}

void CWeapon356::ItemPostFrame(void)
{

		CBasePlayer *pOwner = ToBasePlayer(GetOwner());
		if (!pOwner)
		{
			return;
		}

		// Allow  Ironsight immediately as ItemPostFrame() starts
		if (m_bForbidIronsight)
		{
			m_bForbidIronsight = false;
			if (!m_iClip1 && pOwner->GetAmmoCount(m_iPrimaryAmmoType))
				Reload();
		}

		if (m_bInReload && gpGlobals->curtime >= m_flNextPrimaryAttack)
		{
			FillDrum();
			m_bInReload = false;
		}

		if (!(m_bInReload || m_bForbidIronsight || GetActivity() == ACT_VM_HOLSTER))
		{
			// Allow  Ironsight
			HoldIronsight();
			if (m_iClip1 > 0)
			{
				if (m_bNeedPump && gpGlobals->curtime >= m_flNextPrimaryAttack)
				{
					Pump();
				}

				if ((pOwner->m_afButtonPressed & IN_ATTACK) && gpGlobals->curtime >= m_flNextPrimaryAttack)
				{
					PrimaryAttack();
				}

				if ((pOwner->m_afButtonPressed & IN_ATTACK2) && gpGlobals->curtime >= m_flNextSecondaryAttack)
					// toggle zoom on precision weapon like vanilla HL2 crossbow
				{
					SecondaryAttack();
				}

				if ((pOwner->m_afButtonPressed & IN_RELOAD) && gpGlobals->curtime >= m_flNextPrimaryAttack)
				{
					Reload();
				}
			}
			else if (pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{		
				Reload(); // empty drum but player has ammo
			}
			else
			{
				WeaponIdle();
			}
		}	
}
