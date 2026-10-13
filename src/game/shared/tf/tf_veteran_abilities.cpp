//========= OWTF2 ============================================================//
//
// Purpose: Veteran class abilities (shared client/server, predicted).
//
//=============================================================================//

#include "cbase.h"
#include "tf_veteran_abilities.h"
#include "in_buttons.h"
#include "tf_weapon_veteran_rifle.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so client prediction uses the server's value.
//-----------------------------------------------------------------------------
ConVar tf_veteran_sprint_speed_mult( "tf_veteran_sprint_speed_mult", "1.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: max speed multiplier while Sprint is on (1.5 = +50%).", true, 0.1f, true, 10.0f );

//-----------------------------------------------------------------------------
// Purpose: Per-command ability input. Runs on the server and in client prediction.
//-----------------------------------------------------------------------------
void Veteran_AbilityPostFrame( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_VETERAN ) || !pPlayer->IsAlive() )
		return;

	// Sprint: +attack4 toggles it on and off.
	if ( pPlayer->m_afButtonPressed & IN_ATTACK4 )
	{
		pPlayer->m_Shared.SetVeteranSprinting( !pPlayer->m_Shared.IsVeteranSprinting() );
		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Speed multiplier from active abilities.
//-----------------------------------------------------------------------------
float Veteran_GetMaxSpeedMultiplier( const CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_VETERAN ) )
		return 1.0f;

	if ( pPlayer->m_Shared.IsVeteranSprinting() )
		return tf_veteran_sprint_speed_mult.GetFloat();

	return 1.0f;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Veteran ultimate effect. The charge, drain and input are handled by
//          tf_ultimate.cpp; only the effect itself belongs here. Not implemented yet.
//-----------------------------------------------------------------------------
void Veteran_OnUltimateStart( CTFPlayer *pPlayer )
{
}

void Veteran_OnUltimateEnd( CTFPlayer *pPlayer )
{
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Clear ability state.
//-----------------------------------------------------------------------------
void Veteran_ResetAbilities( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Ultimate charge is handled by tf_ultimate.cpp (it survives death).

	// Helix rockets are ready again on every spawn.
	CTFVeteranRifle *pRifle = dynamic_cast< CTFVeteranRifle * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_VETERAN_RIFLE ) );
	if ( pRifle )
	{
		pRifle->ResetHelixCooldown();
	}

	if ( pPlayer->m_Shared.IsVeteranSprinting() )
	{
		pPlayer->m_Shared.SetVeteranSprinting( false );

		// Max speed is cached on the player; drop the Sprint bonus now rather than
		// whenever something else next recalculates it.
		pPlayer->TeamFortress_SetSpeed();
	}
}
