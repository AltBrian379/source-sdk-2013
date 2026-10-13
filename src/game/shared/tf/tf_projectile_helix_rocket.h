//========= OWTF2 ============================================================//
//
// Purpose: Veteran's helix rockets (alt-fire of tf_weapon_veteran_rifle).
//
//			PLACEHOLDER: a copy of the Engineer sentry's rocket. Same model and
//			zero-size hull (CTFProjectile_SentryRocket::Spawn), same speed (1100),
//			blast radius (146) and knockback (all from CTFBaseRocket, which the
//			sentry rocket uses unchanged), same damage (100, set by the weapon
//			through tf_veteran_helix_damage), and the normal rocket trail. (The sentry
//			rocket's client class "disables" trails with CreateRocketTrails, which
//			overrides nothing, so sentry rockets get the normal trail too.)
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
#endif
};

#endif // TF_PROJECTILE_HELIX_ROCKET_H
