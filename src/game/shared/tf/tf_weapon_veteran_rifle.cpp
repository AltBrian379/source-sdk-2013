//========= OWTF2 ============================================================//
//
// Purpose: Veteran's primary weapon. Primary fire: SMG placeholder.
//			Alt-fire: helix rockets.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_veteran_rifle.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_projectile_helix_rocket.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so client prediction and the HUD use the server's values.
// PLACEHOLDER values copied from the sentry rocket (damage) and the source hero (cooldown).
//-----------------------------------------------------------------------------
ConVar tf_veteran_helix_cooldown( "tf_veteran_helix_cooldown", "6", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rockets cooldown in seconds.", true, 0.0f, false, 0.0f );
ConVar tf_veteran_helix_damage( "tf_veteran_helix_damage", "100", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rockets damage (the sentry rocket's is 100)." );

//-----------------------------------------------------------------------------
// Weapon tables
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVeteranRifle, DT_TFVeteranRifle )

BEGIN_NETWORK_TABLE( CTFVeteranRifle, DT_TFVeteranRifle )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flHelixReadyTime ) ),
#else
	SendPropTime( SENDINFO( m_flHelixReadyTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVeteranRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flHelixReadyTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_veteran_rifle, CTFVeteranRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_veteran_rifle );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFVeteranRifle )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
CTFVeteranRifle::CTFVeteranRifle()
{
	m_flHelixReadyTime = 0.0f;
}

//-----------------------------------------------------------------------------
void CTFVeteranRifle::Precache( void )
{
	BaseClass::Precache();
	PrecacheScriptSound( "Building_Sentrygun.FireRocket" );
}

//-----------------------------------------------------------------------------
// Purpose: Helix rockets. Fired like the Rocket Launcher (CTFWeaponBaseGun::FireRocket):
//			the rocket starts beside the gun and GetProjectileFireSetup aims it at
//			whatever the crosshair is on, so it converges on the crosshair target
//			with a small correction that depends on the target's distance.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer || !pPlayer->IsAlive() || !CanAttack() || !IsHelixReady() )
		return;

	m_flHelixReadyTime = gpGlobals->curtime + tf_veteran_helix_cooldown.GetFloat();

	// Placeholder sound: the sentry's rocket launch.
	CPASAttenuationFilter filter( pPlayer, "Building_Sentrygun.FireRocket" );
	filter.UsePredictionRules();
	EmitSound( filter, pPlayer->entindex(), "Building_Sentrygun.FireRocket" );

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

#ifdef GAME_DLL
	// Same firing offset and setup as the Rocket Launcher.
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	Vector vecSrc;
	QAngle angForward;
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	// Don't start inside a wall.
	trace_t trace;
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( pPlayer->EyePosition(), vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_HelixRocket *pRocket = CTFProjectile_HelixRocket::Create( this, trace.endpos, angForward, pPlayer );
	if ( pRocket )
	{
		pRocket->SetCritical( false );	// sentry rockets don't crit
		pRocket->SetDamage( tf_veteran_helix_damage.GetFloat() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 0..1 cooldown progress for the item effect meter.
//-----------------------------------------------------------------------------
float CTFVeteranRifle::GetEffectBarProgress( void )
{
	const float flCooldown = tf_veteran_helix_cooldown.GetFloat();
	if ( flCooldown <= 0.0f || IsHelixReady() )
		return 1.0f;

	return clamp( 1.0f - ( m_flHelixReadyTime - gpGlobals->curtime ) / flCooldown, 0.0f, 1.0f );
}
