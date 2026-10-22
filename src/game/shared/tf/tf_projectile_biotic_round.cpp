//========= OWTF2 ============================================================//
//
// Purpose: The Biotic Rifle's unscoped shot. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_biotic_round.h"
#include "tf_teammate_trace.h"

#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_weapon_combatmed_rifle.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// PLACEHOLDER: the Syringe Gun's syringe (the crossbow bolt is too big and distracting).
#define BIOTIC_ROUND_MODEL				"models/weapons/w_models/w_syringe_proj.mdl"
#define BIOTIC_ROUND_DISPATCH_EFFECT	"ClientProjectile_BioticRound"
#define BIOTIC_ROUND_LIFETIME			5.0f	// seconds before it's removed if it hits nothing

// Initial default: Ana's 125 m/s x 39.37 HU/m = 4921, rounded to 4920.
ConVar tf_combatmed_rifle_projectile_speed( "tf_combatmed_rifle_projectile_speed", "4920", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Rifle unscoped projectile speed (Overwatch's 125 m/s at 39.37 HU/m).", true, 1.0f, false, 0.0f );

LINK_ENTITY_TO_CLASS( tf_projectile_biotic_round, CTFProjectile_BioticRound );
PRECACHE_REGISTER( tf_projectile_biotic_round );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_BioticRound )
	DEFINE_THINKFUNC( MoveThink ),
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
CTFProjectile_BioticRound *CTFProjectile_BioticRound::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	// On the client this only dispatches the visual (the server's copy of the effect
	// skips the shooter, who predicted it).
	return static_cast< CTFProjectile_BioticRound * >( CTFBaseProjectile::Create( "tf_projectile_biotic_round", vecOrigin, vecAngles, pOwner,
		tf_combatmed_rifle_projectile_speed.GetFloat(), modelinfo->GetModelIndex( BIOTIC_ROUND_MODEL ), BIOTIC_ROUND_DISPATCH_EFFECT, pOwner ) );
}

const char *CTFProjectile_BioticRound::GetProjectileModelName( void )
{
	return BIOTIC_ROUND_MODEL;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: No physics: MoveThink moves it and finds what it hits.
//-----------------------------------------------------------------------------
void CTFProjectile_BioticRound::Spawn( void )
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetTouch( NULL );

	m_flDieTime = gpGlobals->curtime + BIOTIC_ROUND_LIFETIME;
	SetThink( &CTFProjectile_BioticRound::MoveThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Sweep this tick's movement and stop at the first thing hit.
//-----------------------------------------------------------------------------
void CTFProjectile_BioticRound::MoveThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner || gpGlobals->curtime >= m_flDieTime )
	{
		UTIL_Remove( this );
		return;
	}

	const Vector vecStart = GetAbsOrigin();
	const Vector vecEnd = vecStart + GetAbsVelocity() * gpGlobals->interval_per_tick;

	trace_t tr;
	CTraceFilterSkipFullHealthTeammates filter( pOwner, COLLISION_GROUP_NONE, pOwner->GetTeamNumber() );
	UTIL_TraceHull( vecStart, vecEnd, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), MASK_SOLID, &filter, &tr );

	if ( tr.fraction < 1.0f || tr.startsolid )
	{
		if ( tr.m_pEnt && !tr.m_pEnt->IsWorld() && !( tr.surface.flags & SURF_SKY ) )
		{
			CTFCombatMedRifle::ApplyBioticHit( pOwner, tr.m_pEnt, GetLauncher() );
		}

		UTIL_Remove( this );
		return;
	}

	SetAbsOrigin( vecEnd );
	SetNextThink( gpGlobals->curtime );
}

#else // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: The visible syringe, with the Syringe Gun's team trail.
//-----------------------------------------------------------------------------
static void ClientsideProjectileBioticRoundCallback( const CEffectData &data )
{
	C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer * >( ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity ) );
	if ( !pPlayer )
		return;

	C_LocalTempEntity *pRound = ClientsideProjectileCallback( data, 0.0f );
	if ( pRound )
	{
		pRound->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
		pRound->AddParticleEffect( ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) ? "nailtrails_medic_blue" : "nailtrails_medic_red" );
		pRound->AddEffects( EF_NOSHADOW );
		pRound->flags |= FTENT_USEFASTCOLLISIONS;
	}
}

DECLARE_CLIENT_EFFECT( BIOTIC_ROUND_DISPATCH_EFFECT, ClientsideProjectileBioticRoundCallback );

#endif
