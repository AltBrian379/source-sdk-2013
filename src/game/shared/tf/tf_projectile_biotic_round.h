//========= OWTF2 ============================================================//
//
// Purpose: The Biotic Rifle's unscoped shot (Combat Medic), tf_projectile_biotic_round.
//
//	A straight, fast projectile (tf_combatmed_rifle_projectile_speed). The server
//	entity is invisible; clients draw a syringe temp entity with the Syringe Gun's
//	trail, like TF2's syringes.
//
//	It doesn't use physics: every tick it sweeps from its last position to the next
//	one with CTraceFilterSkipFullHealthTeammates, so it passes through the shooter and
//	teammates at full health, and stops at the first enemy, hurt teammate, building or
//	world surface. What a hit does is CTFCombatMedRifle::ApplyBioticHit.
//
//=============================================================================//

#ifndef TF_PROJECTILE_BIOTIC_ROUND_H
#define TF_PROJECTILE_BIOTIC_ROUND_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_projectile_base.h"

class CTFProjectile_BioticRound : public CTFBaseProjectile
{
public:
	DECLARE_CLASS( CTFProjectile_BioticRound, CTFBaseProjectile );

	static CTFProjectile_BioticRound *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner );

	virtual const char *GetProjectileModelName( void ) OVERRIDE;

#ifdef GAME_DLL
	DECLARE_DATADESC();

	virtual void	Spawn( void ) OVERRIDE;

private:
	void			MoveThink( void );

	float			m_flDieTime;
#endif
};

#endif // TF_PROJECTILE_BIOTIC_ROUND_H
