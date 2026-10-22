//========= OWTF2 ============================================================//
//
// Purpose: Traces for shots that heal teammates (Combat Medic's Biotic Rifle).
//
//	TF2's bullet traces already stop at teammates (friendly fire only blocks the
//	damage). A healing shot should pass through teammates at full health, since
//	there's nothing to heal, and hit hurt ones.
//
//	Lag compensation: TF2 only rewinds teammates for the Medigun and melee
//	(CTFPlayer::WantsLagCompensationOnEntity). Weapons whose shots hit teammates
//	return true from CTFWeaponBase::HitsTeammates() to get them rewound too.
//
//=============================================================================//

#ifndef TF_TEAMMATE_TRACE_H
#define TF_TEAMMATE_TRACE_H
#ifdef _WIN32
#pragma once
#endif

// True for a living player of iTeam below max health (overheal counts as full).
inline bool OWTF2_IsHurtTeammate( CBaseEntity *pEntity, int iTeam )
{
	return pEntity && pEntity->IsPlayer() && pEntity->IsAlive() && pEntity->GetTeamNumber() == iTeam &&
		pEntity->GetHealth() < pEntity->GetMaxHealth();
}

//-----------------------------------------------------------------------------
// Purpose: Like CTraceFilterSimple, but passes through players of iTeam at full health.
//-----------------------------------------------------------------------------
class CTraceFilterSkipFullHealthTeammates : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterSkipFullHealthTeammates, CTraceFilterSimple );

	CTraceFilterSkipFullHealthTeammates( const IHandleEntity *pPassEntity, int collisionGroup, int iTeam )
		: CTraceFilterSimple( pPassEntity, collisionGroup ), m_iTeam( iTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask ) OVERRIDE
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity && pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iTeam && !OWTF2_IsHurtTeammate( pEntity, m_iTeam ) )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:
	int m_iTeam;
};

#endif // TF_TEAMMATE_TRACE_H
