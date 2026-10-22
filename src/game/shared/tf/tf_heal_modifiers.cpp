//========= OWTF2 ============================================================//
//
// Purpose: Incoming-healing windows (no-heal, heal boost). See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_heal_modifiers.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
bool HealModifiers_IsNoHeal( const CTFPlayer *pPlayer )
{
	return pPlayer && gpGlobals->curtime < pPlayer->m_Shared.GetNoHealEndTime();
}

bool HealModifiers_IsHealBoosted( const CTFPlayer *pPlayer )
{
	return pPlayer && gpGlobals->curtime < pPlayer->m_Shared.GetHealBoostEndTime();
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
float HealModifiers_GetIncomingScale( const CTFPlayer *pPlayer )
{
	if ( HealModifiers_IsNoHeal( pPlayer ) )
		return 0.0f;

	if ( HealModifiers_IsHealBoosted( pPlayer ) )
		return pPlayer->m_Shared.GetHealBoostScale();

	return 1.0f;
}

//-----------------------------------------------------------------------------
void HealModifiers_ApplyNoHeal( CTFPlayer *pPlayer, float flDuration )
{
	if ( !pPlayer || flDuration <= 0.0f )
		return;

	const float flEndTime = gpGlobals->curtime + flDuration;
	if ( flEndTime > pPlayer->m_Shared.GetNoHealEndTime() )
	{
		pPlayer->m_Shared.SetNoHealEndTime( flEndTime );
	}
}

void HealModifiers_ApplyHealBoost( CTFPlayer *pPlayer, float flDuration, float flScale )
{
	if ( !pPlayer || flDuration <= 0.0f )
		return;

	const float flEndTime = gpGlobals->curtime + flDuration;
	if ( flEndTime > pPlayer->m_Shared.GetHealBoostEndTime() )
	{
		pPlayer->m_Shared.SetHealBoost( flEndTime, flScale );
	}
}

void HealModifiers_Clear( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	pPlayer->m_Shared.SetNoHealEndTime( 0.0f );
	pPlayer->m_Shared.SetHealBoost( 0.0f, 1.0f );
}

//-----------------------------------------------------------------------------
void HealModifiers_CreditHealing( CTFPlayer *pHealer, CTFPlayer *pPatient, int nHealed )
{
	if ( !pHealer || !pPatient || nHealed <= 0 )
		return;

	CTF_GameStats.Event_PlayerHealedOther( pHealer, nHealed );

	IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
	if ( event )
	{
		event->SetInt( "priority", 1 );	// HLTV event priority
		event->SetInt( "patient", pPatient->GetUserID() );
		event->SetInt( "healer", pHealer->GetUserID() );
		event->SetInt( "amount", nHealed );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Testing aid.
//-----------------------------------------------------------------------------
CON_COMMAND_F( tf_heal_modifier_test, "Applies a healing window to you: tf_heal_modifier_test noheal <seconds> | boost <seconds> <scale> | clear", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || args.ArgC() < 2 )
	{
		Msg( "Usage: tf_heal_modifier_test noheal <seconds> | boost <seconds> <scale> | clear\n" );
		return;
	}

	if ( FStrEq( args[1], "noheal" ) && args.ArgC() >= 3 )
	{
		HealModifiers_ApplyNoHeal( pPlayer, atof( args[2] ) );
	}
	else if ( FStrEq( args[1], "boost" ) && args.ArgC() >= 4 )
	{
		HealModifiers_ApplyHealBoost( pPlayer, atof( args[2] ), atof( args[3] ) );
	}
	else if ( FStrEq( args[1], "clear" ) )
	{
		HealModifiers_Clear( pPlayer );
	}
	else
	{
		Msg( "Usage: tf_heal_modifier_test noheal <seconds> | boost <seconds> <scale> | clear\n" );
	}
}

#endif // GAME_DLL
