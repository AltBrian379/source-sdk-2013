//========= OWTF2 ============================================================//
//
// Purpose: Veteran's helix rockets. Placeholder copy of the sentry rocket, at the
//			Direct Hit's speed and half the size.
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_helix_rocket.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Initial default: Overwatch's 50 m/s x 39.37 HU/m = 1968.5, rounded to 1968.
ConVar tf_veteran_helix_speed( "tf_veteran_helix_speed", "1968", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rocket speed (Overwatch's 50 m/s at 39.37 HU/m).", true, 1.0f, false, 0.0f );
ConVar tf_veteran_helix_scale( "tf_veteran_helix_scale", "0.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rocket model scale (visual only; the rocket's hull is a point either way).", true, 0.01f, false, 0.0f );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_HelixRocket, DT_TFProjectile_HelixRocket )

BEGIN_NETWORK_TABLE( CTFProjectile_HelixRocket, DT_TFProjectile_HelixRocket )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_helix_rocket, CTFProjectile_HelixRocket );
PRECACHE_REGISTER( tf_projectile_helix_rocket );

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
CTFProjectile_HelixRocket *CTFProjectile_HelixRocket::Create( CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CTFProjectile_HelixRocket *pRocket = static_cast< CTFProjectile_HelixRocket * >(
		CTFBaseRocket::Create( pLauncher, "tf_projectile_helix_rocket", vecOrigin, vecAngles, pOwner ) );

	if ( pRocket )
	{
		pRocket->SetScorer( pOwner );

		// CTFBaseRocket::Create launched it at 1100 (plus speed attributes); use our own speed.
		Vector vecVelocity = pRocket->GetAbsVelocity();
		if ( VectorNormalize( vecVelocity ) > 0.0f )
		{
			vecVelocity *= tf_veteran_helix_speed.GetFloat();
			pRocket->SetAbsVelocity( vecVelocity );
			pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );
		}
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
void CTFProjectile_HelixRocket::Precache()
{
	PrecacheModel( TF_HELIX_ROCKET_MODEL );
	PrecacheParticleSystem( TF_HELIX_ROCKET_TRAIL );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Same as CTFProjectile_SentryRocket::Spawn, plus the model scale.
//-----------------------------------------------------------------------------
void CTFProjectile_HelixRocket::Spawn()
{
	BaseClass::Spawn();

	SetModel( TF_HELIX_ROCKET_MODEL );

	UTIL_SetSize( this, vec3_origin, vec3_origin );

	ResetSequence( LookupSequence( "idle" ) );

	SetModelScale( tf_veteran_helix_scale.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: Explosive damage. CTFBaseRocket looks the damage type up by weapon ID,
//          which is the rifle's (DMG_BULLET | DMG_USEDISTANCEMOD). Swap the bullet
//          flag for blast; keep the distance modifier and the crit flag as they are.
//-----------------------------------------------------------------------------
int CTFProjectile_HelixRocket::GetDamageType()
{
	return ( BaseClass::GetDamageType() & ~DMG_BULLET ) | DMG_BLAST;
}

#endif // GAME_DLL
