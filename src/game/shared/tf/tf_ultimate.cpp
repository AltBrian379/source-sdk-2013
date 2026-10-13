//========= OWTF2 ============================================================//
//
// Purpose: Ultimate ability charge system, shared by every hero class.
//			See tf_ultimate.h for the rules.
//
//=============================================================================//

#include "cbase.h"
#include "tf_ultimate.h"
#include "tf_classdata.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "tf_veteran_abilities.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "GameEventListener.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so the client sees the server's values.
//-----------------------------------------------------------------------------
ConVar tf_ultimate_charge_per_second( "tf_ultimate_charge_per_second", "5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Ultimate: passive charge per second, once the round is running and setup is over." );
ConVar tf_ultimate_charge_per_damage( "tf_ultimate_charge_per_damage", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Ultimate: charge per point of damage dealt to enemies." );
ConVar tf_ultimate_charge_per_heal( "tf_ultimate_charge_per_heal", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Ultimate: charge per point of healing done." );
ConVar tf_ultimate_overheal_damage_scale( "tf_ultimate_overheal_damage_scale", "0.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Ultimate: multiplier on charge for damage that only removes a target's overheal." );

//-----------------------------------------------------------------------------
// Per-class values
//-----------------------------------------------------------------------------
static const TFPlayerClassData_t *GetClassData( const CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->GetPlayerClass() )
		return NULL;

	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( iClass <= TF_CLASS_UNDEFINED || iClass >= TF_CLASS_COUNT_ALL )
		return NULL;

	return GetPlayerClassData( iClass );
}

float Ultimate_GetCost( const CTFPlayer *pPlayer )
{
	const TFPlayerClassData_t *pData = GetClassData( pPlayer );
	return pData ? pData->m_flUltimateCost : 0.0f;
}

float Ultimate_GetDuration( const CTFPlayer *pPlayer )
{
	const TFPlayerClassData_t *pData = GetClassData( pPlayer );
	return pData ? pData->m_flUltimateDuration : 0.0f;
}

bool Ultimate_HasUltimate( const CTFPlayer *pPlayer )
{
	return Ultimate_GetCost( pPlayer ) > 0.0f;
}

float Ultimate_GetChargeFraction( const CTFPlayer *pPlayer )
{
	const float flCost = Ultimate_GetCost( pPlayer );
	if ( flCost <= 0.0f )
		return 0.0f;

	return clamp( pPlayer->m_Shared.GetUltimateCharge() / flCost, 0.0f, 1.0f );
}

bool Ultimate_IsReady( const CTFPlayer *pPlayer )
{
	return Ultimate_HasUltimate( pPlayer ) && !Ultimate_IsActive( pPlayer ) &&
		pPlayer->m_Shared.GetUltimateCharge() >= Ultimate_GetCost( pPlayer );
}

bool Ultimate_IsActive( const CTFPlayer *pPlayer )
{
	return pPlayer && pPlayer->m_Shared.IsUltimateActive();
}

//-----------------------------------------------------------------------------
// Purpose: Input. The server decides; the client sees the result over the network.
//-----------------------------------------------------------------------------
void Ultimate_PostFrame( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	if ( pPlayer && ( pPlayer->m_afButtonPressed & IN_ATTACK5 ) )
	{
		Ultimate_TryActivate( pPlayer );
	}
#endif
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Per-hero start and end of the ultimate's effect.
//-----------------------------------------------------------------------------
static void OnUltimateStarted( CTFPlayer *pPlayer )
{
	switch ( pPlayer->GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_VETERAN:	Veteran_OnUltimateStart( pPlayer );	break;
	default:				break;
	}
}

static void OnUltimateEnded( CTFPlayer *pPlayer )
{
	switch ( pPlayer->GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_VETERAN:	Veteran_OnUltimateEnd( pPlayer );	break;
	default:				break;
	}
}

//-----------------------------------------------------------------------------
void Ultimate_SetCharge( CTFPlayer *pPlayer, float flPoints )
{
	if ( !pPlayer )
		return;

	pPlayer->m_Shared.SetUltimateCharge( clamp( flPoints, 0.0f, Ultimate_GetCost( pPlayer ) ) );
}

void Ultimate_AddCharge( CTFPlayer *pPlayer, float flPoints )
{
	if ( !pPlayer || flPoints <= 0.0f || !Ultimate_HasUltimate( pPlayer ) || Ultimate_IsActive( pPlayer ) )
		return;

	// Once full, nothing adds more.
	const float flCost = Ultimate_GetCost( pPlayer );
	const float flCharge = pPlayer->m_Shared.GetUltimateCharge();
	if ( flCharge >= flCost )
		return;

	pPlayer->m_Shared.SetUltimateCharge( MIN( flCharge + flPoints, flCost ) );
}

void Ultimate_ResetCharge( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( Ultimate_IsActive( pPlayer ) )
	{
		Ultimate_End( pPlayer );
	}
	pPlayer->m_Shared.SetUltimateCharge( 0.0f );
}

//-----------------------------------------------------------------------------
bool Ultimate_TryActivate( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !Ultimate_IsReady( pPlayer ) )
		return false;

	if ( Ultimate_GetDuration( pPlayer ) > 0.0f )
	{
		// Drains over the duration in Ultimate_Think.
		pPlayer->m_Shared.SetUltimateActive( true );
		OnUltimateStarted( pPlayer );
	}
	else
	{
		// Instant: spent at once.
		pPlayer->m_Shared.SetUltimateCharge( 0.0f );
		OnUltimateStarted( pPlayer );
		OnUltimateEnded( pPlayer );
	}
	return true;
}

void Ultimate_End( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !Ultimate_IsActive( pPlayer ) )
		return;

	pPlayer->m_Shared.SetUltimateActive( false );
	pPlayer->m_Shared.SetUltimateCharge( 0.0f );
	OnUltimateEnded( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: True when the round has started and any setup (doors) is over.
//-----------------------------------------------------------------------------
static bool IsRoundLive()
{
	CTFGameRules *pRules = TFGameRules();
	return pRules && pRules->State_Get() == GR_STATE_RND_RUNNING &&
		!pRules->InSetup() && !pRules->IsInWaitingForPlayers();
}

//-----------------------------------------------------------------------------
void Ultimate_Think( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	CTFPlayerShared &shared = pPlayer->m_Shared;

	const float flDelta = ( shared.m_flUltimateLastThink > 0.0f ) ? ( gpGlobals->curtime - shared.m_flUltimateLastThink ) : 0.0f;
	shared.m_flUltimateLastThink = gpGlobals->curtime;

	// Charge belongs to the class that earned it.
	const int iClass = pPlayer->GetPlayerClass() ? pPlayer->GetPlayerClass()->GetClassIndex() : TF_CLASS_UNDEFINED;
	if ( iClass != shared.m_iUltimateChargeClass )
	{
		if ( shared.m_iUltimateChargeClass != TF_CLASS_UNDEFINED )
		{
			shared.SetUltimateActive( false );
			shared.SetUltimateCharge( 0.0f );
		}
		shared.m_iUltimateChargeClass = iClass;
	}

	if ( !Ultimate_HasUltimate( pPlayer ) || flDelta <= 0.0f )
		return;

	if ( Ultimate_IsActive( pPlayer ) )
	{
		// Dying ends it; otherwise drain over the duration.
		const float flDuration = Ultimate_GetDuration( pPlayer );
		if ( !pPlayer->IsAlive() || flDuration <= 0.0f )
		{
			Ultimate_End( pPlayer );
			return;
		}

		const float flCharge = shared.GetUltimateCharge() - ( Ultimate_GetCost( pPlayer ) / flDuration ) * flDelta;
		if ( flCharge <= 0.0f )
		{
			Ultimate_End( pPlayer );
		}
		else
		{
			shared.SetUltimateCharge( flCharge );
		}
		return;
	}

	if ( pPlayer->IsAlive() && IsRoundLive() )
	{
		Ultimate_AddCharge( pPlayer, tf_ultimate_charge_per_second.GetFloat() * flDelta );
	}
}

//-----------------------------------------------------------------------------
// Entities created by ultimates. Handles go null when the entity is deleted.
static CUtlVector< EHANDLE > s_UltimateEntities;

static bool IsMarkedUltimateEntity( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	FOR_EACH_VEC( s_UltimateEntities, i )
	{
		if ( s_UltimateEntities[i].Get() == pEntity )
			return true;
	}
	return false;
}

void Ultimate_MarkEntity( CBaseEntity *pEntity )
{
	if ( !pEntity || IsMarkedUltimateEntity( pEntity ) )
		return;

	// Drop handles to entities that no longer exist.
	FOR_EACH_VEC_BACK( s_UltimateEntities, i )
	{
		if ( !s_UltimateEntities[i].Get() )
		{
			s_UltimateEntities.FastRemove( i );
		}
	}

	s_UltimateEntities.AddToTail( pEntity );
}

bool Ultimate_IsUltimateDamage( const CTakeDamageInfo &info, const CTFPlayer *pAttacker )
{
	// Anything done while the ultimate runs counts as the ultimate's.
	if ( Ultimate_IsActive( pAttacker ) )
		return true;

	// Lingering ultimate effects (projectiles, areas) are marked when created.
	return IsMarkedUltimateEntity( info.GetInflictor() ) || IsMarkedUltimateEntity( info.GetWeapon() );
}

//-----------------------------------------------------------------------------
void Ultimate_OnDamageDealt( const CTakeDamageInfo &info, CBaseEntity *pVictim, float flDamage, float flOverhealDamage )
{
	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pAttacker || !pVictim || flDamage <= 0.0f || !Ultimate_HasUltimate( pAttacker ) )
		return;

	// No charge from hurting yourself or your team.
	if ( pVictim == pAttacker || pVictim->GetTeamNumber() == pAttacker->GetTeamNumber() )
		return;

	if ( Ultimate_IsUltimateDamage( info, pAttacker ) )
		return;

	flOverhealDamage = clamp( flOverhealDamage, 0.0f, flDamage );
	const float flPoints = ( flDamage - flOverhealDamage ) + flOverhealDamage * tf_ultimate_overheal_damage_scale.GetFloat();

	Ultimate_AddCharge( pAttacker, flPoints * tf_ultimate_charge_per_damage.GetFloat() );
}

void Ultimate_OnHealingDone( CTFPlayer *pHealer, float flAmount )
{
	if ( !pHealer || flAmount <= 0.0f || Ultimate_IsActive( pHealer ) )
		return;

	Ultimate_AddCharge( pHealer, flAmount * tf_ultimate_charge_per_heal.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: Ultimate charge doesn't carry over between rounds.
//-----------------------------------------------------------------------------
class CUltimateRoundListener : public CAutoGameSystem, public CGameEventListener
{
public:
	CUltimateRoundListener() : CAutoGameSystem( "CUltimateRoundListener" ) {}

	virtual bool Init()
	{
		ListenForGameEvent( "teamplay_round_start" );
		return true;
	}

	virtual void FireGameEvent( IGameEvent *pEvent )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			Ultimate_ResetCharge( ToTFPlayer( UTIL_PlayerByIndex( i ) ) );
		}
	}
};
static CUltimateRoundListener s_UltimateRoundListener;

//-----------------------------------------------------------------------------
// Purpose: Testing aid.
//-----------------------------------------------------------------------------
CON_COMMAND_F( tf_ultimate_charge_set, "Ultimate: set your ultimate charge in points (capped at your class's cost).", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || args.ArgC() < 2 )
	{
		Msg( "Usage: tf_ultimate_charge_set <points>\n" );
		return;
	}

	if ( Ultimate_IsActive( pPlayer ) )
	{
		Ultimate_End( pPlayer );
	}
	Ultimate_SetCharge( pPlayer, atof( args[1] ) );
}

#endif // GAME_DLL
