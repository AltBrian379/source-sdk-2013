//========= OWTF2 ============================================================//
//
// Purpose: The Sleep Dart (Combat Medic), tf_projectile_sleep_dart. Server only: clients
//			see it as a plain animating model.
//
//	Flies straight at tf_combatmed_sleepdart_speed for tf_combatmed_sleepdart_range.
//	It doesn't use physics: every tick it sweeps its movement. It passes through the
//	shooter and every teammate, and stops at the first enemy player, building or
//	world surface.
//
//	An enemy player it hits takes tf_combatmed_sleepdart_damage and falls asleep
//	(CombatMed_PutToSleep, tf_combatmed_abilities.h). ÜberCharge blocks both.
//
//	PLACEHOLDER: the Crusader's Crossbow bolt (the syringe model at 3x) and its trail.
//
//=============================================================================//

#ifndef TF_PROJECTILE_SLEEP_DART_H
#define TF_PROJECTILE_SLEEP_DART_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

class CTFPlayer;

class CTFProjectile_SleepDart : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTFProjectile_SleepDart, CBaseAnimating );
	DECLARE_DATADESC();

	static CTFProjectile_SleepDart *Create( const Vector &vecOrigin, const QAngle &vecAngles, CTFPlayer *pOwner, CBaseEntity *pWeapon );

	virtual void	Precache( void ) OVERRIDE;
	virtual void	Spawn( void ) OVERRIDE;

private:
	void			MoveThink( void );

	Vector			m_vecVelocity;
	float			m_flDieTime;
	EHANDLE			m_hWeapon;
};

#endif // TF_PROJECTILE_SLEEP_DART_H
