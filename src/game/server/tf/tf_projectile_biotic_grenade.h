//========= OWTF2 ============================================================//
//
// Purpose: The Biotic Grenade (Combat Medic), tf_projectile_biotic_grenade. Server only:
//			clients see it as a plain animating model.
//
//	Thrown at tf_combatmed_grenade_speed with gravity. It doesn't use physics: every
//	tick it sweeps its movement and bursts on the first thing it touches (the world,
//	any player but the thrower, a building).
//
//	The burst reaches every living player within tf_combatmed_grenade_radius of it
//	(the nearest point of their box) with nothing solid in between, at full strength
//	(no falloff, like Ana's):
//	- Teammates, the thrower included: healed tf_combatmed_grenade_heal at once (never
//	  boosted, not even by an earlier grenade; no-heal blocks it), then heal boost
//	  (x tf_combatmed_grenade_heal_boost) for tf_combatmed_grenade_effect_time. A newer
//	  grenade's boost or no-heal replaces the old one (full duration again).
//	- Enemies: tf_combatmed_grenade_damage at once (no knockback), then no-heal for
//	  tf_combatmed_grenade_effect_time. ÜberCharged enemies are unaffected.
//	- Enemy buildings: the damage only.
//	The windows are tf_heal_modifiers.h; the tints are drawn by the YellowLevel
//	material proxy (c_tf_player.cpp).
//
//	PLACEHOLDER: the Grenade Launcher's grenade model and trail, Mad Milk's splash and
//	Jarate's break sound.
//
//=============================================================================//

#ifndef TF_PROJECTILE_BIOTIC_GRENADE_H
#define TF_PROJECTILE_BIOTIC_GRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

class CTFPlayer;

class CTFProjectile_BioticGrenade : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTFProjectile_BioticGrenade, CBaseAnimating );
	DECLARE_DATADESC();

	static CTFProjectile_BioticGrenade *Create( const Vector &vecOrigin, const QAngle &vecAngles, CTFPlayer *pOwner, CBaseEntity *pWeapon );

	virtual void	Precache( void ) OVERRIDE;
	virtual void	Spawn( void ) OVERRIDE;

private:
	void			MoveThink( void );
	void			Burst( const Vector &vecCenter );

	Vector			m_vecVelocity;
	float			m_flDieTime;
	EHANDLE			m_hWeapon;
};

#endif // TF_PROJECTILE_BIOTIC_GRENADE_H
