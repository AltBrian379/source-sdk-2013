//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic's Sleep Dart. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_combatmed_sleepdart.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_projectile_sleep_dart.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so client prediction and the HUD use the server's values.
//-----------------------------------------------------------------------------
// Initial default: Ana's 0.24 s cast.
ConVar tf_combatmed_sleepdart_cast_time( "tf_combatmed_sleepdart_cast_time", "0.24", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Sleep Dart cast time in seconds, from switching to it until the dart is fired.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 0.75 s recovery.
ConVar tf_combatmed_sleepdart_recovery_time( "tf_combatmed_sleepdart_recovery_time", "0.75", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: seconds after the dart is fired before switching back to the rifle.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 14 s.
ConVar tf_combatmed_sleepdart_cooldown( "tf_combatmed_sleepdart_cooldown", "14", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Sleep Dart cooldown in seconds, from the shot.", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
// Weapon tables. The cast state is networked by CTFCastAbilityWeapon.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFCombatMedSleepDart, DT_TFCombatMedSleepDart )

BEGIN_NETWORK_TABLE( CTFCombatMedSleepDart, DT_TFCombatMedSleepDart )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCombatMedSleepDart )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_combatmed_sleepdart, CTFCombatMedSleepDart );
PRECACHE_WEAPON_REGISTER( tf_weapon_combatmed_sleepdart );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFCombatMedSleepDart )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
void CTFCombatMedSleepDart::Precache( void )
{
	BaseClass::Precache();

#ifdef GAME_DLL
	UTIL_PrecacheOther( "tf_projectile_sleep_dart" );
#endif
}

float CTFCombatMedSleepDart::GetCastTime( void ) const
{
	return tf_combatmed_sleepdart_cast_time.GetFloat();
}

float CTFCombatMedSleepDart::GetCooldown( void ) const
{
	return tf_combatmed_sleepdart_cooldown.GetFloat();
}

float CTFCombatMedSleepDart::GetRecoveryTime( void ) const
{
	return tf_combatmed_sleepdart_recovery_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: End of the cast: fire the dart along the aim.
//-----------------------------------------------------------------------------
void CTFCombatMedSleepDart::OnCastFinished( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	WeaponSound( SINGLE );	// the weapon script's single_shot: the Hitman's Heatmaker

#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	GetProjectileFireSetup( pPlayer, Vector( 16, 6, -8 ), &vecSrc, &angForward, false );

	CTFProjectile_SleepDart::Create( vecSrc, angForward, pPlayer, this );
#endif
}
