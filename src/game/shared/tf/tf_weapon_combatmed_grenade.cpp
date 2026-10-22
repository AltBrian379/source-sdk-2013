//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic's secondary, the Biotic Grenade. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_combatmed_grenade.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_projectile_biotic_grenade.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so client prediction and the HUD use the server's values.
//-----------------------------------------------------------------------------
// Initial default: PLACEHOLDER, a short delay before the grenade leaves the hand (tune to the throw animation).
ConVar tf_combatmed_grenade_cast_time( "tf_combatmed_grenade_cast_time", "0.25", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade cast time in seconds, from switching to it until the grenade is thrown.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 14 s (12 s in 6v6).
ConVar tf_combatmed_grenade_cooldown( "tf_combatmed_grenade_cooldown", "14", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade cooldown in seconds, from the throw.", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
// Weapon tables. The cast state is networked by CTFCastAbilityWeapon.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFCombatMedGrenade, DT_TFCombatMedGrenade )

BEGIN_NETWORK_TABLE( CTFCombatMedGrenade, DT_TFCombatMedGrenade )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCombatMedGrenade )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_combatmed_grenade, CTFCombatMedGrenade );
PRECACHE_WEAPON_REGISTER( tf_weapon_combatmed_grenade );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFCombatMedGrenade )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
void CTFCombatMedGrenade::Precache( void )
{
	BaseClass::Precache();

#ifdef GAME_DLL
	UTIL_PrecacheOther( "tf_projectile_biotic_grenade" );
#endif
}

float CTFCombatMedGrenade::GetCastTime( void ) const
{
	return tf_combatmed_grenade_cast_time.GetFloat();
}

float CTFCombatMedGrenade::GetCooldown( void ) const
{
	return tf_combatmed_grenade_cooldown.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Third-person throw: every class's throwable gesture (ACT_MP_THROW, the
//          overhand throw of the throwables and spells), played like the spellbook's.
//-----------------------------------------------------------------------------
void CTFCombatMedGrenade::OnCastStarted( CTFPlayer *pPlayer )
{
	if ( pPlayer )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_MP_THROW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: End of the cast: throw the grenade along the aim.
//-----------------------------------------------------------------------------
void CTFCombatMedGrenade::OnCastFinished( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	Vector vecSrc;
	QAngle angForward;
	GetProjectileFireSetup( pPlayer, Vector( 16, 8, -6 ), &vecSrc, &angForward, false );

	CTFProjectile_BioticGrenade::Create( vecSrc, angForward, pPlayer, this );
#endif
}
