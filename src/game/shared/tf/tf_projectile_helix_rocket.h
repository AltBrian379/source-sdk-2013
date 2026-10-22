//========= OWTF2 ============================================================//
//
// Purpose: Veteran's helix rockets (alt-fire of tf_weapon_veteran_rifle).
//
//			PLACEHOLDER: a copy of the Engineer sentry's rocket. Same model and
//			zero-size hull (CTFProjectile_SentryRocket::Spawn), drawn at half size
//			(tf_veteran_helix_scale), the Direct Hit's speed (tf_veteran_helix_speed, 1980),
//			blast radius (146) and knockback (all from CTFBaseRocket, which the
//			sentry rocket uses unchanged), and the normal rocket trail. Damage is its
//			own: tf_veteran_helix_damage (60), set by the weapon, blast type. (The sentry
//			rocket's client class "disables" trails with CreateRocketTrails, which
//			overrides nothing, so sentry rockets get the normal trail too.) The trail is
//			a half-size copy of it, helix_rockettrail.
//
//=============================================================================//

#ifndef TF_PROJECTILE_HELIX_ROCKET_H
#define TF_PROJECTILE_HELIX_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_tf_projectile_rocket.h"
#define CTFProjectile_HelixRocket C_TFProjectile_HelixRocket	// c_tf_projectile_rocket.h maps CTFProjectile_Rocket
#else
#include "tf_projectile_rocket.h"
#endif

#define TF_HELIX_ROCKET_MODEL	"models/buildables/sentry3_rockets.mdl"
// The rocket trail at half size, to match the model (game/mod_tf/particles/helix_rockettrail.pcf).
#define TF_HELIX_ROCKET_TRAIL	"helix_rockettrail"

class CTFProjectile_HelixRocket : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFProjectile_HelixRocket, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	static CTFProjectile_HelixRocket *Create( CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner );

	virtual void	Spawn();
	virtual void	Precache();
	virtual int		GetWeaponID( void ) const	{ return TF_WEAPON_VETERAN_RIFLE; }

	// The weapon ID gives the rifle's bullet damage type; the rockets deal blast damage instead.
	virtual int		GetDamageType() OVERRIDE;
#else
	virtual const char *GetTrailParticleName( void ) OVERRIDE	{ return TF_HELIX_ROCKET_TRAIL; }
#endif
};

#endif // TF_PROJECTILE_HELIX_ROCKET_H
