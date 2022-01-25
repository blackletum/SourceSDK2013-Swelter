#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_rpg.h"


#ifdef CLIENT_DLL
#include "c_baseplayer.h"
#include "model_types.h"
#include "beamdraw.h"
#include "fx_line.h"
#include "view.h"
#else
#include "basecombatcharacter.h"
#include "movie_explosion.h"
#include "soundent.h"
#include "player.h"
#include "rope.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "util.h"
#include "in_buttons.h"
#include "shake.h"
#include "te_effect_dispatch.h"
#include "triggers.h"
#include "smoke_trail.h"
#include "collisionutils.h"
#include "hl2_shareddefs.h"
#endif

#include "debugoverlay_shared.h"
#include "LaserSight.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"





LINK_ENTITY_TO_CLASS(env_lasersight, CLaserSight);

BEGIN_DATADESC(CLaserSight)
DEFINE_FIELD(m_vecSurfaceNormal, FIELD_VECTOR),
DEFINE_FIELD(m_hTargetEnt, FIELD_EHANDLE),
DEFINE_FIELD(m_bVisibleLaserDot, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsOn, FIELD_BOOLEAN),

//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),      // don't save - regenerated by constructor
END_DATADESC()


IMPLEMENT_NETWORKCLASS_ALIASED(LaserSight, DT_LaserSight)

BEGIN_NETWORK_TABLE(CLaserSight, DT_LaserSight)
END_NETWORK_TABLE()
//-----------------------------------------------------------------------------
// Finds missiles in cone
//-----------------------------------------------------------------------------
CBaseEntity *CreateLaserSight(const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot)
{
	return CLaserSight::Create(origin, pOwner, bVisibleDot);
}

void SetLaserSightTarget(CBaseEntity *pLaserDot, CBaseEntity *pTarget)
{
	CLaserSight *pDot = assert_cast< CLaserSight* >(pLaserDot);
	pDot->SetTargetEntity(pTarget);
}

void EnableLaserSight(CBaseEntity *pLaserDot, bool bEnable)
{
	CLaserSight *pDot = assert_cast< CLaserSight* >(pLaserDot);
	if (bEnable)
	{
		pDot->TurnOn();
	}
	else
	{
		pDot->TurnOff();
	}
}

CLaserSight::CLaserSight(void)
{
	m_hTargetEnt = NULL;
	m_bIsOn = true;
#ifndef CLIENT_DLL
	//      g_LaserSightList.Insert(this);
#endif
}

CLaserSight::~CLaserSight(void)
{
#ifndef CLIENT_DLL
	//      g_LaserSightList.Remove(this);
#endif
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : &origin -
// Output : CLaserDot
//-----------------------------------------------------------------------------
CLaserSight *CLaserSight::Create(const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot)
{
#ifndef CLIENT_DLL
	CLaserSight *pLaserDot = (CLaserSight *)CBaseEntity::Create("env_lasersight", origin, QAngle(0, 0, 0));

	if (pLaserDot == NULL)
		return NULL;

	pLaserDot->m_bVisibleLaserDot = bVisibleDot;
	pLaserDot->SetMoveType(MOVETYPE_NONE);
	pLaserDot->AddSolidFlags(FSOLID_NOT_SOLID);
	pLaserDot->AddEffects(EF_NOSHADOW);
	UTIL_SetSize(pLaserDot, -Vector(4, 4, 4), Vector(4, 4, 4));

	pLaserDot->SetOwnerEntity(pOwner);

	pLaserDot->AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

	if (!bVisibleDot)
	{
		pLaserDot->MakeInvisible();
	}

	return pLaserDot;
#else
	return NULL;
#endif
}

void CLaserSight::SetLaserPosition(const Vector &origin, const Vector &normal)
{
	SetAbsOrigin(origin);
	m_vecSurfaceNormal = normal;
}

Vector CLaserSight::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLaserSight::TurnOn(void)
{
	m_bIsOn = true;
	if (m_bVisibleLaserDot)
	{
		//BaseClass::TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLaserSight::TurnOff(void)
{
	m_bIsOn = false;
	if (m_bVisibleLaserDot)
	{
		//BaseClass::TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLaserSight::MakeInvisible(void)
{
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Draw our sprite
//-----------------------------------------------------------------------------
int CLaserSight::DrawModel(int flags)
{
	color32 color = { 255, 255, 255, 255 };
	Vector  vecAttachment, vecDir;
	QAngle  angles;

	float   scale;
	Vector  endPos;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer(GetOwnerEntity());

	if (pOwner != NULL && pOwner->IsDormant() == false)
	{
		// Always draw the dot in front of our faces when in first-person
		if (pOwner->IsLocalPlayer())
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();
		}
		else
		{
			// Take the eye position and direction
			vecAttachment = pOwner->EyePosition();

			QAngle angles = pOwner->GetAnimEyeAngles();
			AngleVectors(angles, &vecDir);
		}

		trace_t tr;
		UTIL_TraceLine(vecAttachment, vecAttachment + (vecDir * MAX_TRACE_LENGTH), MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

		// Backup off the hit plane
		endPos = tr.endpos + (tr.plane.normal * 4.0f);
	}
	else
	{
		// Just use our position if we can't predict it otherwise
		endPos = GetAbsOrigin();
	}

	// Randomly flutter
	scale = 16.0f + random->RandomFloat(-4.0f, 4.0f);

	// Draw our laser dot in space
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind(m_hSpriteMaterial, this);
	DrawSprite(endPos, scale, scale, color);

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Setup our sprite reference
//-----------------------------------------------------------------------------
void CLaserSight::OnDataChanged(DataUpdateType_t updateType)
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		m_hSpriteMaterial.Init(RPG_LASER_SPRITE, TEXTURE_GROUP_CLIENT_EFFECTS);
	}
}

#endif  //CLIENT_DLL


void ILaserSight::CreateLaserPointer(void)
{
#ifndef CLIENT_DLL
	if (m_hLaserDot != NULL)
		return;
	if (_weap == NULL) return;


	CBasePlayer *pOwner = ToBasePlayer(_weap->GetOwner());
	//      if (pOwner == NULL) pOwner = _c;
	if (pOwner == NULL)
		return;

	m_hLaserDot = CLaserSight::Create(_weap->GetAbsOrigin(), pOwner);
	//m_hLaserDot->TurnOff();

	UpdateLaserPosition();
#endif
}

void ILaserSight::UpdateLaserPosition(Vector vecMuzzlePos, Vector vecEndPos)
{
#ifndef CLIENT_DLL
	if (vecMuzzlePos == vec3_origin || vecEndPos == vec3_origin)
	{
		CBasePlayer *pPlayer = ToBasePlayer(_weap->GetOwner());
		if (!pPlayer)
			return;

		vecMuzzlePos = pPlayer->Weapon_ShootPosition();
		Vector  forward;
		pPlayer->EyeVectors(&forward);
		vecEndPos = vecMuzzlePos + (forward * MAX_TRACE_LENGTH);
	}

	//Move the laser dot, if active
	trace_t tr;

	// Trace out for the endpoint
	UTIL_TraceLine(vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), _weap->GetOwner(), COLLISION_GROUP_NONE, &tr);

	// Move the laser sprite
	if (m_hLaserDot != NULL)
	{
		Vector  laserPos = tr.endpos;
		m_hLaserDot->SetLaserPosition(laserPos, tr.plane.normal);

		if (tr.DidHitNonWorldEntity())
		{
			CBaseEntity *pHit = tr.m_pEnt;

			if ((pHit != NULL) && (pHit->m_takedamage))
			{
				m_hLaserDot->SetTargetEntity(pHit);
			}
			else
			{
				m_hLaserDot->SetTargetEntity(NULL);
			}
		}
		else
		{
			m_hLaserDot->SetTargetEntity(NULL);
		}
	}
#endif
}

Vector ILaserSight::GetLaserPosition(void)
{
#ifndef CLIENT_DLL
	CreateLaserPointer();

	if (m_hLaserDot != NULL)
		return m_hLaserDot->GetAbsOrigin();

	//FIXME: The laser dot sprite is not active, this code should not be allowed!
	assert(0);
#endif
	return vec3_origin;
}

ILaserSight::~ILaserSight()
{
#ifndef CLIENT_DLL
	if (m_hLaserDot != NULL)
	{
		UTIL_Remove(m_hLaserDot);
		m_hLaserDot = NULL;
	}
#endif
}

ILaserSight::ILaserSight(CBaseHLCombatWeapon * weap)
{
	_weap = weap;

	_weap->PrecacheModel("sprites/redglow1.vmt");
	_weap->PrecacheModel(RPG_LASER_SPRITE);
	_weap->PrecacheModel(RPG_BEAM_SPRITE);
	_weap->PrecacheModel(RPG_BEAM_SPRITE_NOZ);
}