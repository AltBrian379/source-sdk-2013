//========= OWTF2 ============================================================//
//
// Purpose: Veteran's helix rockets. Placeholder copy of the sentry rocket.
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_helix_rocket.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
void CTFProjectile_HelixRocket::Precache()
{
	PrecacheModel( TF_HELIX_ROCKET_MODEL );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Same as CTFProjectile_SentryRocket::Spawn.
//-----------------------------------------------------------------------------
void CTFProjectile_HelixRocket::Spawn()
{
	BaseClass::Spawn();

	SetModel( TF_HELIX_ROCKET_MODEL );

	UTIL_SetSize( this, vec3_origin, vec3_origin );

	ResetSequence( LookupSequence( "idle" ) );
}

#endif // GAME_DLL
