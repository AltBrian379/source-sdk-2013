//========= OWTF2 ============================================================//
//
// Purpose: The Biotic Grenade's projectile and burst. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_projectile_biotic_grenade.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_heal_modifiers.h"
#include "particle_parse.h"
#include "tf_obj.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BIOTIC_GRENADE_MODEL		"models/weapons/w_models/w_grenade_grenadelauncher.mdl"	// PLACEHOLDER
#define BIOTIC_GRENADE_SOUND		"Jar.Explode"											// PLACEHOLDER
#define BIOTIC_GRENADE_LIFETIME		10.0f	// seconds before it's removed if it touches nothing
#define BIOTIC_GRENADE_HULL			11.0f	// half size: Ana's 0.275 m projectile radius at 39.37 HU/m (10.8)

//-----------------------------------------------------------------------------
// Tuning. Initial defaults are Ana's values scaled to TF2 (CLAUDE.md, "Stat scaling for ported heroes").
//-----------------------------------------------------------------------------
// Initial default: Ana's 30 m/s x 39.37 HU/m = 1181, rounded to 1184.
ConVar tf_combatmed_grenade_speed( "tf_combatmed_grenade_speed", "1184", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade throw speed (Overwatch's 30 m/s at 39.37 HU/m).", true, 1.0f, false, 0.0f );
// Initial default: TF2's gravity (1 = sv_gravity).
ConVar tf_combatmed_grenade_gravity( "tf_combatmed_grenade_gravity", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade gravity, as a multiple of sv_gravity.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 4 m x 39.37 HU/m = 157.5, rounded to 160.
ConVar tf_combatmed_grenade_radius( "tf_combatmed_grenade_radius", "160", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade burst radius (Overwatch's 4 m at 39.37 HU/m).", true, 0.0f, false, 0.0f );
// Initial default: Ana's 75 x 0.7 (175 / 250 HP) = 52.5, rounded down like the Biotic Rifle.
ConVar tf_combatmed_grenade_damage( "tf_combatmed_grenade_damage", "52", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade damage to enemies in the burst.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 75 x 0.7 = 52.5, rounded down.
ConVar tf_combatmed_grenade_heal( "tf_combatmed_grenade_heal", "52", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade healing to teammates in the burst (the thrower included).", true, 0.0f, false, 0.0f );
// Initial default: Ana's 3 s.
ConVar tf_combatmed_grenade_effect_time( "tf_combatmed_grenade_effect_time", "3", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: seconds of heal boost (teammates) and no-heal (enemies) after the burst.", true, 0.0f, false, 0.0f );
// Initial default: Ana's +50% healing received.
ConVar tf_combatmed_grenade_heal_boost( "tf_combatmed_grenade_heal_boost", "1.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Grenade heal boost, the multiplier on healing teammates receive.", true, 0.0f, false, 0.0f );

LINK_ENTITY_TO_CLASS( tf_projectile_biotic_grenade, CTFProjectile_BioticGrenade );
PRECACHE_REGISTER( tf_projectile_biotic_grenade );

BEGIN_DATADESC( CTFProjectile_BioticGrenade )
	DEFINE_THINKFUNC( MoveThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
CTFProjectile_BioticGrenade *CTFProjectile_BioticGrenade::Create( const Vector &vecOrigin, const QAngle &vecAngles, CTFPlayer *pOwner, CBaseEntity *pWeapon )
{
	if ( !pOwner )
		return NULL;

	CTFProjectile_BioticGrenade *pGrenade = static_cast< CTFProjectile_BioticGrenade * >( CBaseEntity::CreateNoSpawn( "tf_projectile_biotic_grenade", vecOrigin, vecAngles, pOwner ) );
	if ( !pGrenade )
		return NULL;

	pGrenade->SetOwnerEntity( pOwner );
	pGrenade->ChangeTeam( pOwner->GetTeamNumber() );
	pGrenade->m_hWeapon = pWeapon;

	Vector vecForward;
	AngleVectors( vecAngles, &vecForward );
	pGrenade->m_vecVelocity = vecForward * tf_combatmed_grenade_speed.GetFloat();

	DispatchSpawn( pGrenade );
	return pGrenade;
}

//-----------------------------------------------------------------------------
void CTFProjectile_BioticGrenade::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( BIOTIC_GRENADE_MODEL );
	PrecacheScriptSound( BIOTIC_GRENADE_SOUND );
	PrecacheParticleSystem( "pipebombtrail_red" );
	PrecacheParticleSystem( "pipebombtrail_blue" );
	PrecacheParticleSystem( "peejar_impact_milk" );
}

//-----------------------------------------------------------------------------
// Purpose: No physics: MoveThink moves it and finds what it touches.
//-----------------------------------------------------------------------------
void CTFProjectile_BioticGrenade::Spawn( void )
{
	Precache();
	SetModel( BIOTIC_GRENADE_MODEL );
	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	AddEffects( EF_NOSHADOW );

	DispatchParticleEffect( ( GetTeamNumber() == TF_TEAM_BLUE ) ? "pipebombtrail_blue" : "pipebombtrail_red", PATTACH_ABSORIGIN_FOLLOW, this );

	m_flDieTime = gpGlobals->curtime + BIOTIC_GRENADE_LIFETIME;
	SetThink( &CTFProjectile_BioticGrenade::MoveThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Gravity, then sweep this tick's movement; burst on the first touch.
//-----------------------------------------------------------------------------
void CTFProjectile_BioticGrenade::MoveThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner || gpGlobals->curtime >= m_flDieTime )
	{
		UTIL_Remove( this );
		return;
	}

	const float flDelta = gpGlobals->interval_per_tick;
	static ConVarRef sv_gravity( "sv_gravity" );
	m_vecVelocity.z -= sv_gravity.GetFloat() * tf_combatmed_grenade_gravity.GetFloat() * flDelta;

	const Vector vecStart = GetAbsOrigin();
	const Vector vecEnd = vecStart + m_vecVelocity * flDelta;
	const Vector vecHull( BIOTIC_GRENADE_HULL, BIOTIC_GRENADE_HULL, BIOTIC_GRENADE_HULL );

	trace_t tr;
	CTraceFilterSimple filter( pOwner, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecStart, vecEnd, -vecHull, vecHull, MASK_SOLID, &filter, &tr );

	if ( tr.fraction < 1.0f || tr.startsolid )
	{
		if ( tr.surface.flags & SURF_SKY )
		{
			UTIL_Remove( this );
			return;
		}

		Burst( tr.endpos );
		UTIL_Remove( this );
		return;
	}

	SetAbsOrigin( vecEnd );

	QAngle angles;
	VectorAngles( m_vecVelocity, angles );
	SetAbsAngles( angles );

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Heal and boost teammates, damage and block healing on enemies.
//-----------------------------------------------------------------------------
void CTFProjectile_BioticGrenade::Burst( const Vector &vecCenter )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	DispatchParticleEffect( "peejar_impact_milk", vecCenter, vec3_angle );
	EmitSound( BIOTIC_GRENADE_SOUND );

	const float flRadius = tf_combatmed_grenade_radius.GetFloat();
	const float flEffectTime = tf_combatmed_grenade_effect_time.GetFloat();
	CBaseEntity *pWeapon = m_hWeapon.Get();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
			continue;

		// In range: the nearest point of the player's box.
		Vector vecNearest;
		pPlayer->CollisionProp()->CalcNearestPoint( vecCenter, &vecNearest );
		if ( vecNearest.DistToSqr( vecCenter ) > flRadius * flRadius )
			continue;

		// Nothing solid in between (players don't block).
		trace_t tr;
		UTIL_TraceLine( vecCenter, pPlayer->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0f )
			continue;

		if ( pPlayer->GetTeamNumber() == pOwner->GetTeamNumber() )
		{
			// The burst's healing is never boosted, by this grenade or an earlier one (the boost
			// is a status effect, not a stack); no-heal still blocks it.
			if ( !HealModifiers_IsNoHeal( pPlayer ) )
			{
				const int nHealed = pPlayer->TakeHealth( tf_combatmed_grenade_heal.GetFloat(), DMG_GENERIC | DMG_IGNORE_DEBUFFS );
				HealModifiers_CreditHealing( pOwner, pPlayer, nHealed );
			}

			// A newer grenade's boost replaces the old one: full duration again.
			HealModifiers_ApplyHealBoost( pPlayer, flEffectTime, tf_combatmed_grenade_heal_boost.GetFloat() );
		}
		else
		{
			if ( pPlayer->m_Shared.IsInvulnerable() )
				continue;

			// No DMG_USEDISTANCEMOD and not radius damage: full damage anywhere in the burst, no knockback.
			CTakeDamageInfo info( this, pOwner, pWeapon, vec3_origin, vecCenter, tf_combatmed_grenade_damage.GetFloat(),
								  DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
			pPlayer->TakeDamage( info );

			if ( pPlayer->IsAlive() )
			{
				HealModifiers_ApplyNoHeal( pPlayer, flEffectTime );
			}
		}
	}

	// Enemy buildings in the burst take its damage, nothing else.
	for ( int i = 0; i < IBaseObjectAutoList::AutoList().Count(); i++ )
	{
		CBaseObject *pObject = static_cast< CBaseObject * >( IBaseObjectAutoList::AutoList()[i] );
		if ( !pObject || pObject->GetTeamNumber() == pOwner->GetTeamNumber() || pObject->m_takedamage == DAMAGE_NO )
			continue;

		Vector vecNearest;
		pObject->CollisionProp()->CalcNearestPoint( vecCenter, &vecNearest );
		if ( vecNearest.DistToSqr( vecCenter ) > flRadius * flRadius )
			continue;

		trace_t tr;
		UTIL_TraceLine( vecCenter, pObject->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0f )
			continue;

		CTakeDamageInfo info( this, pOwner, pWeapon, vec3_origin, vecCenter, tf_combatmed_grenade_damage.GetFloat(),
							  DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
		pObject->TakeDamage( info );
	}
}
