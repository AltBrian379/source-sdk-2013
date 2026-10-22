//========= OWTF2 ============================================================//
//
// Purpose: The Sleep Dart's projectile. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_sleep_dart.h"
#include "tf_player.h"
#include "tf_combatmed_abilities.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SLEEP_DART_MODEL		"models/weapons/w_models/w_syringe_proj.mdl"	// PLACEHOLDER: the crossbow bolt is this model at 3x
#define SLEEP_DART_MODEL_SCALE	3.0f
#define SLEEP_DART_HULL			8.0f	// half size: Ana's 0.2 m projectile radius at 39.37 HU/m (7.9)

//-----------------------------------------------------------------------------
// Tuning. Initial defaults are Ana's values scaled to TF2 (CLAUDE.md, "Stat scaling for ported heroes").
//-----------------------------------------------------------------------------
// Initial default: Ana's 60 m/s x 39.37 HU/m = 2362, rounded to 2360.
ConVar tf_combatmed_sleepdart_speed( "tf_combatmed_sleepdart_speed", "2360", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Sleep Dart speed (Overwatch's 60 m/s at 39.37 HU/m).", true, 1.0f, false, 0.0f );
// Initial default: Ana's 200 m x 39.37 HU/m = 7874, rounded to 7872.
ConVar tf_combatmed_sleepdart_range( "tf_combatmed_sleepdart_range", "7872", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: distance the Sleep Dart flies before it's removed (Overwatch's 200 m).", true, 1.0f, false, 0.0f );
// Initial default: Ana's 5 x 0.7 (175 / 250 HP) = 3.5, rounded down like the other damage.
ConVar tf_combatmed_sleepdart_damage( "tf_combatmed_sleepdart_damage", "3", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Sleep Dart damage, dealt before the target falls asleep.", true, 0.0f, false, 0.0f );

LINK_ENTITY_TO_CLASS( tf_projectile_sleep_dart, CTFProjectile_SleepDart );
PRECACHE_REGISTER( tf_projectile_sleep_dart );

BEGIN_DATADESC( CTFProjectile_SleepDart )
	DEFINE_THINKFUNC( MoveThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Passes through the shooter and all of their teammates.
//-----------------------------------------------------------------------------
class CTraceFilterSkipTeammates : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterSkipTeammates, CTraceFilterSimple );

	CTraceFilterSkipTeammates( const IHandleEntity *pPassEntity, int iTeam )
		: CTraceFilterSimple( pPassEntity, COLLISION_GROUP_NONE ), m_iTeam( iTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask ) OVERRIDE
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity && pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iTeam )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:
	int m_iTeam;
};

//-----------------------------------------------------------------------------
CTFProjectile_SleepDart *CTFProjectile_SleepDart::Create( const Vector &vecOrigin, const QAngle &vecAngles, CTFPlayer *pOwner, CBaseEntity *pWeapon )
{
	if ( !pOwner )
		return NULL;

	CTFProjectile_SleepDart *pDart = static_cast< CTFProjectile_SleepDart * >( CBaseEntity::CreateNoSpawn( "tf_projectile_sleep_dart", vecOrigin, vecAngles, pOwner ) );
	if ( !pDart )
		return NULL;

	pDart->SetOwnerEntity( pOwner );
	pDart->ChangeTeam( pOwner->GetTeamNumber() );
	pDart->m_hWeapon = pWeapon;

	Vector vecForward;
	AngleVectors( vecAngles, &vecForward );
	const float flSpeed = tf_combatmed_sleepdart_speed.GetFloat();
	pDart->m_vecVelocity = vecForward * flSpeed;

	DispatchSpawn( pDart );
	pDart->m_flDieTime = gpGlobals->curtime + tf_combatmed_sleepdart_range.GetFloat() / flSpeed;
	return pDart;
}

//-----------------------------------------------------------------------------
void CTFProjectile_SleepDart::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( SLEEP_DART_MODEL );
	PrecacheParticleSystem( "healshot_trail_red" );
	PrecacheParticleSystem( "healshot_trail_blue" );
}

//-----------------------------------------------------------------------------
// Purpose: No physics: MoveThink moves it and finds what it hits.
//-----------------------------------------------------------------------------
void CTFProjectile_SleepDart::Spawn( void )
{
	Precache();
	SetModel( SLEEP_DART_MODEL );
	SetModelScale( SLEEP_DART_MODEL_SCALE );
	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	AddEffects( EF_NOSHADOW );

	DispatchParticleEffect( ( GetTeamNumber() == TF_TEAM_BLUE ) ? "healshot_trail_blue" : "healshot_trail_red", PATTACH_ABSORIGIN_FOLLOW, this );

	SetThink( &CTFProjectile_SleepDart::MoveThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Sweep this tick's movement and stop at the first thing hit.
//-----------------------------------------------------------------------------
void CTFProjectile_SleepDart::MoveThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner || gpGlobals->curtime >= m_flDieTime )
	{
		UTIL_Remove( this );
		return;
	}

	const Vector vecStart = GetAbsOrigin();
	const Vector vecEnd = vecStart + m_vecVelocity * gpGlobals->interval_per_tick;
	const Vector vecHull( SLEEP_DART_HULL, SLEEP_DART_HULL, SLEEP_DART_HULL );

	trace_t tr;
	CTraceFilterSkipTeammates filter( pOwner, pOwner->GetTeamNumber() );
	UTIL_TraceHull( vecStart, vecEnd, -vecHull, vecHull, MASK_SOLID, &filter, &tr );

	if ( tr.fraction < 1.0f || tr.startsolid )
	{
		CTFPlayer *pVictim = ToTFPlayer( tr.m_pEnt );
		if ( pVictim && pVictim->IsAlive() && pVictim->GetTeamNumber() != pOwner->GetTeamNumber() && !pVictim->m_Shared.IsInvulnerable() )
		{
			CTakeDamageInfo info( this, pOwner, m_hWeapon.Get(), vec3_origin, tr.endpos, tf_combatmed_sleepdart_damage.GetFloat(),
								  DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
			pVictim->TakeDamage( info );

			if ( pVictim->IsAlive() )
			{
				CombatMed_PutToSleep( pVictim, pOwner );
			}
		}

		UTIL_Remove( this );
		return;
	}

	SetAbsOrigin( vecEnd );

	QAngle angles;
	VectorAngles( m_vecVelocity, angles );
	SetAbsAngles( angles );

	SetNextThink( gpGlobals->curtime );
}
