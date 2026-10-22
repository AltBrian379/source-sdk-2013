//========= OWTF2 ============================================================//
//
// Purpose: Veteran's healing field. See tf_veteran_healing_field.h.
//
//=============================================================================//

#include "cbase.h"
#include "tf_veteran_healing_field.h"
#include "collisionutils.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so the values show up (and can be tuned) on clients too.
// PLACEHOLDER values: twice the dispenser's area, and its level 2 heal rate.
//-----------------------------------------------------------------------------
ConVar tf_veteran_field_duration( "tf_veteran_field_duration", "5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: healing field lifetime in seconds.", true, 0.0f, false, 0.0f );
ConVar tf_veteran_field_heal_rate( "tf_veteran_field_heal_rate", "15", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: healing field heal rate in HP per second (the level 2 dispenser's is 15).", true, 0.0f, false, 0.0f );
ConVar tf_veteran_field_radius( "tf_veteran_field_radius", "128", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: healing field area, a box of +-this many units around the field's center (twice the dispenser's 64).", true, 0.0f, false, 0.0f );
ConVar tf_veteran_field_center_height( "tf_veteran_field_center_height", "32", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: height of the healing field area's center above the ground (about a dispenser's center)." );
ConVar tf_veteran_field_offset( "tf_veteran_field_offset", "16", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: the healing field is placed this many units in front of the player's feet." );

// Same as the dispenser's line-of-sight mask: everything except grating.
#define MASK_VETERAN_HEALING_FIELD	( MASK_BLOCKLOS | CONTENTS_WINDOW )

// How often the server rechecks who is inside (the dispenser's rate).
#define TF_VETERAN_HEALING_FIELD_THINK	0.1f

// The ground search starts this far above the feet: the player's step height.
#define TF_VETERAN_HEALING_FIELD_STEP	18.0f

//-----------------------------------------------------------------------------
// Network tables
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVeteranHealingField, DT_TFVeteranHealingField )

BEGIN_NETWORK_TABLE( CTFVeteranHealingField, DT_TFVeteranHealingField )
#ifdef CLIENT_DLL
	RecvPropArray3( RECVINFO_ARRAY( m_nHealedPlayerBits ), RecvPropInt( RECVINFO( m_nHealedPlayerBits[0] ) ) ),
#else
	SendPropArray3( SENDINFO_ARRAY3( m_nHealedPlayerBits ), SendPropInt( SENDINFO_ARRAY( m_nHealedPlayerBits ), 32, SPROP_UNSIGNED ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_veteran_healing_field, CTFVeteranHealingField );
PRECACHE_REGISTER( tf_veteran_healing_field );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFVeteranHealingField )
	DEFINE_THINKFUNC( HealThink ),
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
CTFVeteranHealingField::CTFVeteranHealingField()
{
	for ( int i = 0; i < TF_VETERAN_HEALING_FIELD_BIT_WORDS; i++ )
	{
		m_nHealedPlayerBits.Set( i, 0 );
	}

#ifdef GAME_DLL
	m_flDieTime = 0.0f;
#else
	m_pAreaEffect = NULL;
	V_memset( m_pBeamEffects, 0, sizeof( m_pBeamEffects ) );
#endif
}

//-----------------------------------------------------------------------------
void CTFVeteranHealingField::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( TF_VETERAN_HEALING_FIELD_MODEL );
	PrecacheParticleSystem( "medic_megaheal_red" );
	PrecacheParticleSystem( "medic_megaheal_blue" );
	PrecacheParticleSystem( "dispenser_heal_red" );
	PrecacheParticleSystem( "dispenser_heal_blue" );
}

//-----------------------------------------------------------------------------
bool CTFVeteranHealingField::IsHealingPlayer( int iPlayerIndex ) const
{
	if ( iPlayerIndex < 1 || iPlayerIndex > MAX_PLAYERS )
		return false;

	const int iBit = iPlayerIndex - 1;
	return ( m_nHealedPlayerBits[iBit / 32] & ( 1u << ( iBit % 32 ) ) ) != 0;
}

//-----------------------------------------------------------------------------
void CTFVeteranHealingField::UpdateOnRemove( void )
{
#ifdef GAME_DLL
	// Players keep a handle to every healer; take ours off them first.
	StopHealingAll();
#else
	// The particle property stops our effects when the entity goes away.
	m_pAreaEffect = NULL;
	V_memset( m_pBeamEffects, 0, sizeof( m_pBeamEffects ) );
#endif

	BaseClass::UpdateOnRemove();
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Places the field tf_veteran_field_offset units in front of the
//          player's feet, on the ground. With no ground there (the player is
//          in the air or at a ledge), it drops to the first ground below.
//-----------------------------------------------------------------------------
CTFVeteranHealingField *CTFVeteranHealingField::Create( CTFPlayer *pOwner )
{
	if ( !pOwner )
		return NULL;

	Vector vecForward;
	AngleVectors( QAngle( 0.0f, pOwner->EyeAngles()[YAW], 0.0f ), &vecForward );

	// The point is inside the player's own hull (16 < half the hull width), so it
	// can't be inside a wall. Start a step above the feet so it also finds the
	// ground on slopes and stairs, then trace straight down.
	const Vector vecPoint = pOwner->GetAbsOrigin() + vecForward * tf_veteran_field_offset.GetFloat();
	const Vector vecStart = vecPoint + Vector( 0.0f, 0.0f, TF_VETERAN_HEALING_FIELD_STEP );
	const Vector vecEnd = vecPoint - Vector( 0.0f, 0.0f, MAX_TRACE_LENGTH );

	trace_t tr;
	CTraceFilterNoNPCsOrPlayer filter( pOwner, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, &filter, &tr );

	// Nothing below (outside the world): use the player's feet.
	const Vector vecOrigin = ( tr.fraction < 1.0f && !tr.startsolid ) ? tr.endpos : pOwner->GetAbsOrigin();

	CTFVeteranHealingField *pField = static_cast< CTFVeteranHealingField * >(
		CBaseEntity::CreateNoSpawn( "tf_veteran_healing_field", vecOrigin, QAngle( 0.0f, pOwner->EyeAngles()[YAW], 0.0f ), pOwner ) );
	if ( !pField )
		return NULL;

	pField->ChangeTeam( pOwner->GetTeamNumber() );
	DispatchSpawn( pField );

	return pField;
}

//-----------------------------------------------------------------------------
void CTFVeteranHealingField::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetModel( TF_VETERAN_HEALING_FIELD_MODEL );

	// No collision: players, bullets and projectiles pass through it.
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	m_flDieTime = gpGlobals->curtime + tf_veteran_field_duration.GetFloat();

	SetThink( &CTFVeteranHealingField::HealThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Like CObjectDispenser::DispenseThink: start and stop healing the
//          players inside the area, then disappear when the time is up.
//-----------------------------------------------------------------------------
void CTFVeteranHealingField::HealThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner || gpGlobals->curtime >= m_flDieTime )
	{
		// The heal is credited to the owner, so the field ends if they leave.
		UTIL_Remove( this );
		return;
	}

	const float flRadius = tf_veteran_field_radius.GetFloat();
	const Vector vecCenter = GetAbsOrigin() + Vector( 0.0f, 0.0f, tf_veteran_field_center_height.GetFloat() );
	const Vector vecAreaMins = vecCenter - Vector( flRadius, flRadius, flRadius );
	const Vector vecAreaMaxs = vecCenter + Vector( flRadius, flRadius, flRadius );

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
		{
			SetHealingPlayer( i, false );
			continue;
		}

		// The dispenser's trigger counts a player as inside when their bounding box touches it.
		Vector vecPlayerMins, vecPlayerMaxs;
		pPlayer->CollisionProp()->WorldSpaceAABB( &vecPlayerMins, &vecPlayerMaxs );
		const bool bInside = IsBoxIntersectingBox( vecAreaMins, vecAreaMaxs, vecPlayerMins, vecPlayerMaxs );

		const bool bShouldHeal = bInside && CouldHealTarget( pPlayer );
		if ( bShouldHeal && !IsHealingPlayer( i ) )
		{
			StartHealing( pPlayer );
		}
		else if ( !bShouldHeal && IsHealingPlayer( i ) )
		{
			StopHealing( pPlayer );
		}
	}

	SetNextThink( gpGlobals->curtime + TF_VETERAN_HEALING_FIELD_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: Same rules as CObjectDispenser::CouldHealTarget: living teammates
//          (and enemy spies disguised as one) in line of sight.
//-----------------------------------------------------------------------------
bool CTFVeteranHealingField::CouldHealTarget( CTFPlayer *pPlayer )
{
	if ( !pPlayer->IsAlive() || pPlayer->IsInPurgatory() )
		return false;

	int iPlayerTeam = pPlayer->GetTeamNumber();
	if ( iPlayerTeam != GetTeamNumber() && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iPlayerTeam = pPlayer->m_Shared.GetDisguiseTeam();
	}

	if ( iPlayerTeam != GetTeamNumber() )
		return false;

	// Line of sight from just above the health pack to the player's center.
	trace_t tr;
	CTraceFilterNoNPCsOrPlayer filter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( GetAbsOrigin() + Vector( 0.0f, 0.0f, 8.0f ), pPlayer->WorldSpaceCenter(), MASK_VETERAN_HEALING_FIELD, &filter, &tr );

	return tr.fraction >= 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Heal at a constant rate like a dispenser (no overheal), credited to
//          the Veteran.
//-----------------------------------------------------------------------------
void CTFVeteranHealingField::StartHealing( CTFPlayer *pPlayer )
{
	pPlayer->m_Shared.Heal( this, tf_veteran_field_heal_rate.GetFloat(), 1.0f, 1.0f, true, ToTFPlayer( GetOwnerEntity() ) );
	SetHealingPlayer( pPlayer->entindex(), true );
}

void CTFVeteranHealingField::StopHealing( CTFPlayer *pPlayer )
{
	pPlayer->m_Shared.StopHealing( this );
	SetHealingPlayer( pPlayer->entindex(), false );
}

void CTFVeteranHealingField::StopHealingAll( void )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		if ( !IsHealingPlayer( i ) )
			continue;

		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->m_Shared.StopHealing( this );
		}
		SetHealingPlayer( i, false );
	}
}

//-----------------------------------------------------------------------------
void CTFVeteranHealingField::SetHealingPlayer( int iPlayerIndex, bool bHealing )
{
	if ( iPlayerIndex < 1 || iPlayerIndex > MAX_PLAYERS )
		return;

	const int iBit = iPlayerIndex - 1;
	unsigned int nWord = m_nHealedPlayerBits[iBit / 32];
	if ( bHealing )
	{
		nWord |= ( 1u << ( iBit % 32 ) );
	}
	else
	{
		nWord &= ~( 1u << ( iBit % 32 ) );
	}
	m_nHealedPlayerBits.Set( iBit / 32, nWord );
}

#else // CLIENT_DLL

//-----------------------------------------------------------------------------
void CTFVeteranHealingField::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// The Quick-Fix ÜberCharge ring, on the floor, shows the area.
		const char *pszAreaEffect = ( GetTeamNumber() == TF_TEAM_RED ) ? "medic_megaheal_red" : "medic_megaheal_blue";
		m_pAreaEffect = ParticleProp()->Create( pszAreaEffect, PATTACH_ABSORIGIN_FOLLOW );
	}

	UpdateEffects();
}

//-----------------------------------------------------------------------------
// Purpose: A dispenser heal beam from the health pack to each player being healed.
//-----------------------------------------------------------------------------
void CTFVeteranHealingField::UpdateEffects( void )
{
	const char *pszBeamEffect = ( GetTeamNumber() == TF_TEAM_RED ) ? "dispenser_heal_red" : "dispenser_heal_blue";

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		// Like the dispenser: no beam to stealthed spies.
		bool bWantBeam = IsHealingPlayer( i ) && pPlayer &&
			!pPlayer->m_Shared.IsStealthed() && !pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK );

		if ( bWantBeam && !m_pBeamEffects[i] )
		{
			// Same control points as the dispenser's beam: start here, end at the player's chest.
			CNewParticleEffect *pEffect = ParticleProp()->Create( pszBeamEffect, PATTACH_ABSORIGIN_FOLLOW, 0, Vector( 0.0f, 0.0f, 8.0f ) );
			if ( pEffect )
			{
				ParticleProp()->AddControlPoint( pEffect, 1, pPlayer, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0.0f, 0.0f, 50.0f ) );
			}
			m_pBeamEffects[i] = pEffect;
		}
		else if ( !bWantBeam && m_pBeamEffects[i] )
		{
			ParticleProp()->StopEmission( m_pBeamEffects[i] );
			m_pBeamEffects[i] = NULL;
		}
	}
}

#endif // CLIENT_DLL
