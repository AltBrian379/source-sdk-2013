//========= OWTF2 ============================================================//
//
// Purpose: Veteran class abilities (shared client/server, predicted).
//
//=============================================================================//

#include "cbase.h"
#include "tf_veteran_abilities.h"
#include "in_buttons.h"
#include "igamemovement.h"
#include "tf_weapon_veteran_rifle.h"
#include "tf_weapon_veteran_field_rations.h"
#include "tf_ultimate.h"

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
ConVar tf_veteran_ultimate_voice_radius( "tf_veteran_ultimate_voice_radius", "2048", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: players within this distance hear the ultimate's voice line (through walls, no falloff).", true, 0.0f, false, 0.0f );
ConVar tf_veteran_ultimate_voice_pitch( "tf_veteran_ultimate_voice_pitch", "85", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: ultimate voice line pitch, percent (85 = 85% speed, lower pitch).", true, 1.0f, true, 255.0f );

ConVar tf_veteran_sprint_speed_mult( "tf_veteran_sprint_speed_mult", "1.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: max speed multiplier while Sprint is on (1.5 = +50%).", true, 0.1f, true, 10.0f );
ConVar tf_veteran_sprint_blocked_speed( "tf_veteran_sprint_blocked_speed", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: Sprint ends when the player is on the ground, holding forward, and moving forward (along the view yaw) slower than this many units/s, e.g. walking into a wall.", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
// Purpose: Per-command ability input. Runs on the server and in client prediction.
//-----------------------------------------------------------------------------
void Veteran_AbilityPostFrame( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_VETERAN ) || !pPlayer->IsAlive() )
		return;

	// Sprint: +attack4 toggles it on and off. Not while casting the ultimate.
	if ( ( pPlayer->m_afButtonPressed & IN_ATTACK4 ) && !Ultimate_IsCasting( pPlayer ) )
	{
		pPlayer->m_Shared.SetVeteranSprinting( !pPlayer->m_Shared.IsVeteranSprinting() );
		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ends Sprint when the player stops moving forward:
//          - no forward input (released W, or W+S), on the ground or in the air;
//          - on the ground, holding forward, but the move left no forward velocity
//            (blocked head-on by a wall or object). Sliding along a wall at an
//            angle still moves forward, so it keeps Sprint. In the air only the
//            input counts.
//-----------------------------------------------------------------------------
void Veteran_SprintPostMove( CTFPlayer *pPlayer, const CMoveData *pMove )
{
	if ( !pPlayer || !pMove || !pPlayer->IsPlayerClass( TF_CLASS_VETERAN ) )
		return;

	if ( !pPlayer->m_Shared.IsVeteranSprinting() )
		return;

	bool bStop = ( pMove->m_flForwardMove <= 0.0f );

	if ( !bStop && pPlayer->GetGroundEntity() )
	{
		Vector vecForward;
		AngleVectors( QAngle( 0.0f, pMove->m_vecViewAngles[YAW], 0.0f ), &vecForward );

		Vector vecVelocity = pMove->m_vecVelocity;
		vecVelocity.z = 0.0f;

		bStop = ( DotProduct( vecVelocity, vecForward ) < tf_veteran_sprint_blocked_speed.GetFloat() );
	}

	if ( bStop )
	{
		Veteran_StopSprint( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turns Sprint off. Also called when the Veteran fires the primary
//          weapon or uses an ability (helix rockets, ultimate).
//-----------------------------------------------------------------------------
void Veteran_StopSprint( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->m_Shared.IsVeteranSprinting() )
		return;

	pPlayer->m_Shared.SetVeteranSprinting( false );

	// Max speed is cached on the player; drop the Sprint bonus now.
	pPlayer->TeamFortress_SetSpeed();
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
// Purpose: Veteran ultimate. The charge, drain and input are handled by
//          tf_ultimate.cpp. The blocking during the cast and the rifle's ultimate
//          state (fast reload, auto-aim) are checked where they apply, through
//          Ultimate_IsCasting / Ultimate_IsEffectActive.
//-----------------------------------------------------------------------------
bool Veteran_CanUseUltimate( CTFPlayer *pPlayer )
{
	// Not in the middle of placing a healing field.
	CTFVeteranFieldRations *pFieldRations = dynamic_cast< CTFVeteranFieldRations * >( pPlayer->GetActiveTFWeapon() );
	return !( pFieldRations && pFieldRations->IsCasting() );
}

void Veteran_OnUltimateStart( CTFPlayer *pPlayer )
{
	// Using an ability ends Sprint.
	Veteran_StopSprint( pPlayer );

	// It also cancels the helix rockets' cast.
	CTFVeteranRifle *pRifle = dynamic_cast< CTFVeteranRifle * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_VETERAN_RIFLE ) );
	if ( pRifle )
	{
		pRifle->CancelHelixCast();
	}

	// PLACEHOLDER voice line: every player within tf_veteran_ultimate_voice_radius hears
	// it, through walls, at full volume like the announcer (no distance falloff), and
	// pitched down (which also slows it).
	Ultimate_PlayVoiceLine( pPlayer, VETERAN_ULTIMATE_VOICE_LINE, tf_veteran_ultimate_voice_radius.GetFloat(),
							tf_veteran_ultimate_voice_pitch.GetInt() );
}

void Veteran_OnUltimateCastEnd( CTFPlayer *pPlayer )
{
	// The rifle comes out of the cast fully loaded (free: the reserve isn't touched).
	CTFVeteranRifle *pRifle = dynamic_cast< CTFVeteranRifle * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_VETERAN_RIFLE ) );
	if ( pRifle )
	{
		pRifle->Ultimate_RefillClip();
	}
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

	// So is the healing field.
	CTFVeteranFieldRations *pFieldRations = dynamic_cast< CTFVeteranFieldRations * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_VETERAN_FIELD_RATIONS ) );
	if ( pFieldRations )
	{
		pFieldRations->ResetCooldown();
	}

	Veteran_StopSprint( pPlayer );
}
