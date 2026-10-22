//========= OWTF2 ============================================================//
//
// Purpose: Veteran's secondary, the Field Rations (healing field ability).
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_veteran_field_rations.h"
#include "tf_veteran_abilities.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_veteran_healing_field.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so client prediction and the HUD use the server's values.
//-----------------------------------------------------------------------------
ConVar tf_veteran_field_cast_time( "tf_veteran_field_cast_time", "0.6", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: Field Rations cast time in seconds (from switching to it until the field is placed).", true, 0.0f, false, 0.0f );
ConVar tf_veteran_field_cooldown( "tf_veteran_field_cooldown", "15", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: Field Rations cooldown in seconds, from when the field is placed.", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
// Weapon tables. The cast state is networked by CTFCastAbilityWeapon.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVeteranFieldRations, DT_TFVeteranFieldRations )

BEGIN_NETWORK_TABLE( CTFVeteranFieldRations, DT_TFVeteranFieldRations )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVeteranFieldRations )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_veteran_field_rations, CTFVeteranFieldRations );
PRECACHE_WEAPON_REGISTER( tf_weapon_veteran_field_rations );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFVeteranFieldRations )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
void CTFVeteranFieldRations::Precache( void )
{
	BaseClass::Precache();

#ifdef GAME_DLL
	UTIL_PrecacheOther( "tf_veteran_healing_field" );
#endif
}

//-----------------------------------------------------------------------------
float CTFVeteranFieldRations::GetCastTime( void ) const
{
	return tf_veteran_field_cast_time.GetFloat();
}

float CTFVeteranFieldRations::GetCooldown( void ) const
{
	return tf_veteran_field_cooldown.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Using an ability ends Sprint.
//-----------------------------------------------------------------------------
void CTFVeteranFieldRations::OnCastStarted( CTFPlayer *pPlayer )
{
	Veteran_StopSprint( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: End of the cast: place the field.
//-----------------------------------------------------------------------------
void CTFVeteranFieldRations::OnCastFinished( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	CTFVeteranHealingField::Create( pPlayer );
#endif
}
