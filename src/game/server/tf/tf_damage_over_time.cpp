//========= OWTF2 ============================================================//
//
// Purpose: Damage spread over time. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_damage_over_time.h"
#include "tf_player.h"
#include "takedamageinfo.h"
#include "tf_heal_modifiers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_damage_over_time_interval( "tf_damage_over_time_interval", "0.1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Damage over time: seconds between hits. The total is the same at any interval.", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
struct DamageOverTimeEffect_t
{
	EHANDLE		hVictim;			// a player, or a building (damage only)
	EHANDLE		hAttacker;
	EHANDLE		hInflictor;
	EHANDLE		hWeapon;
	float		flDamagePerSecond;
	float		flDamageLeft;		// still to deal
	float		flDamageDue;		// accumulated since the last hit
	float		flLastUpdateTime;
	float		flNextHitTime;
	float		flEndTime;
	int			bitsDamageType;
	int			iCustomDamage;
	bool		bHeal;				// healing instead of damage; hAttacker is the healer
	float		flHealCarry;		// healing: the fraction of a point left over after the heal modifiers
};

//-----------------------------------------------------------------------------
// Purpose: Ticks every effect once per server frame.
//-----------------------------------------------------------------------------
class CDamageOverTimeSystem : public CAutoGameSystemPerFrame
{
public:
	CDamageOverTimeSystem() : CAutoGameSystemPerFrame( "CDamageOverTimeSystem" ) {}

	virtual void LevelShutdownPostEntity() OVERRIDE { m_Effects.Purge(); }
	virtual void FrameUpdatePostEntityThink() OVERRIDE;

	CUtlVector< DamageOverTimeEffect_t > m_Effects;

private:
	bool Update( DamageOverTimeEffect_t &effect );	// false = finished
};

static CDamageOverTimeSystem s_DamageOverTimeSystem;

//-----------------------------------------------------------------------------
void CDamageOverTimeSystem::FrameUpdatePostEntityThink()
{
	// Hits can kill and clear effects, so walk a copy of the list position by position.
	for ( int i = 0; i < m_Effects.Count(); )
	{
		DamageOverTimeEffect_t effect = m_Effects[i];
		const bool bKeep = Update( effect );

		// The list may have changed during the hit (a death clears the victim's effects).
		if ( i < m_Effects.Count() && m_Effects[i].hVictim == effect.hVictim && m_Effects[i].flEndTime == effect.flEndTime )
		{
			if ( bKeep )
			{
				m_Effects[i] = effect;
				i++;
			}
			else
			{
				m_Effects.Remove( i );
			}
		}
	}
}

//-----------------------------------------------------------------------------
bool CDamageOverTimeSystem::Update( DamageOverTimeEffect_t &effect )
{
	CBaseEntity *pVictim = effect.hVictim.Get();
	CBaseEntity *pAttacker = effect.hAttacker.Get();
	if ( !pVictim || !pAttacker )
		return false;

	// Players: alive. Buildings and the rest: still taking damage and not destroyed.
	if ( pVictim->IsPlayer() ? !pVictim->IsAlive() : ( pVictim->m_takedamage == DAMAGE_NO || pVictim->GetHealth() <= 0 ) )
		return false;

	const float flNow = MIN( gpGlobals->curtime, effect.flEndTime );
	effect.flDamageDue += effect.flDamagePerSecond * MAX( 0.0f, flNow - effect.flLastUpdateTime );
	effect.flLastUpdateTime = flNow;

	const bool bFinished = gpGlobals->curtime >= effect.flEndTime;
	if ( !bFinished && gpGlobals->curtime < effect.flNextHitTime )
		return true;

	// Whole points per hit; the last hit deals what's left.
	float flDamage = bFinished ? effect.flDamageLeft : floorf( MIN( effect.flDamageDue, effect.flDamageLeft ) );
	effect.flNextHitTime = gpGlobals->curtime + tf_damage_over_time_interval.GetFloat();

	if ( effect.bHeal && flDamage >= 1.0f )
	{
		effect.flDamageDue -= flDamage;
		effect.flDamageLeft -= flDamage;

		// Healing is only ever applied to players (HealOverTime_Apply).
		CTFPlayer *pPatient = ToTFPlayer( pVictim );
		if ( !pPatient )
			return false;

		// The heal modifiers apply here, keeping the fraction for the next hit (TakeHealth would
		// drop it); TakeHealth only adds the max health cap. Credit what was healed.
		effect.flHealCarry += flDamage * HealModifiers_GetIncomingScale( pPatient );
		const int nHeal = (int)effect.flHealCarry;
		effect.flHealCarry -= nHeal;
		const int nHealed = ( nHeal > 0 ) ? pPatient->TakeHealth( nHeal, DMG_GENERIC | DMG_IGNORE_DEBUFFS ) : 0;
		HealModifiers_CreditHealing( ToTFPlayer( pAttacker ), pPatient, nHealed );
	}
	else if ( !effect.bHeal && ( flDamage >= 1.0f || ( bFinished && flDamage > 0.0f ) ) )
	{
		effect.flDamageDue -= flDamage;
		effect.flDamageLeft -= flDamage;

		CTakeDamageInfo info( effect.hInflictor.Get() ? effect.hInflictor.Get() : pAttacker, pAttacker, effect.hWeapon.Get(),
							  vec3_origin, pVictim->WorldSpaceCenter(), flDamage, effect.bitsDamageType | DMG_PREVENT_PHYSICS_FORCE,
							  effect.iCustomDamage );
		pVictim->TakeDamage( info );
	}

	return !bFinished && effect.flDamageLeft > 0.0f;
}

//-----------------------------------------------------------------------------
void DamageOverTime_Apply( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, CBaseEntity *pWeapon,
						   float flTotalDamage, float flDuration, int bitsDamageType, int iCustomDamage )
{
	if ( !pVictim || !pAttacker || flTotalDamage <= 0.0f )
		return;

	DamageOverTimeEffect_t effect;
	effect.hVictim = pVictim;
	effect.hAttacker = pAttacker;
	effect.hInflictor = pInflictor;
	effect.hWeapon = pWeapon;
	effect.flDamageLeft = flTotalDamage;
	effect.flDamageDue = 0.0f;
	effect.flLastUpdateTime = gpGlobals->curtime;
	effect.flNextHitTime = gpGlobals->curtime + tf_damage_over_time_interval.GetFloat();
	effect.bitsDamageType = bitsDamageType;
	effect.iCustomDamage = iCustomDamage;
	effect.bHeal = false;
	effect.flHealCarry = 0.0f;

	if ( flDuration <= 0.0f )
	{
		// No duration: all of it on the next tick.
		effect.flDamagePerSecond = 0.0f;
		effect.flEndTime = gpGlobals->curtime;
	}
	else
	{
		effect.flDamagePerSecond = flTotalDamage / flDuration;
		effect.flEndTime = gpGlobals->curtime + flDuration;
	}

	s_DamageOverTimeSystem.m_Effects.AddToTail( effect );
}

void HealOverTime_Apply( CTFPlayer *pTarget, CTFPlayer *pHealer, float flTotalHealing, float flDuration )
{
	if ( !pTarget || !pHealer || flTotalHealing <= 0.0f )
		return;

	// Same timing as damage over time; healing is whole points too.
	DamageOverTime_Apply( pTarget, pHealer, pHealer, NULL, flTotalHealing, flDuration, DMG_GENERIC, TF_DMG_CUSTOM_NONE );
	s_DamageOverTimeSystem.m_Effects.Tail().bHeal = true;
}

void DamageOverTime_Clear( CBaseEntity *pVictim )
{
	FOR_EACH_VEC_BACK( s_DamageOverTimeSystem.m_Effects, i )
	{
		if ( s_DamageOverTimeSystem.m_Effects[i].hVictim.Get() == pVictim )
		{
			s_DamageOverTimeSystem.m_Effects.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Testing aids. The target is the player with that entity index, or you.
//-----------------------------------------------------------------------------
static CTFPlayer *GetTestTarget( const CCommand &args, int iArg, CTFPlayer *pDefault )
{
	if ( args.ArgC() > iArg )
	{
		return ToTFPlayer( UTIL_PlayerByIndex( atoi( args[iArg] ) ) );
	}
	return pDefault;
}

CON_COMMAND_F( tf_damage_over_time_test, "Damage over time from you: tf_damage_over_time_test <damage> <seconds> [entindex, default you]", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || args.ArgC() < 3 )
	{
		Msg( "Usage: tf_damage_over_time_test <damage> <seconds> [entindex]\n" );
		return;
	}

	CTFPlayer *pVictim = GetTestTarget( args, 3, pPlayer );
	DamageOverTime_Apply( pVictim, pPlayer, pPlayer, pPlayer->GetActiveWeapon(), atof( args[1] ), atof( args[2] ), DMG_GENERIC, TF_DMG_CUSTOM_NONE );
}

// The per-application defense buff resistance (CTFPlayerShared::SetDefenseBuffResistance).
CON_COMMAND_F( tf_defensebuff_test, "Defense buff (no crit block) with a resistance: tf_defensebuff_test <resistance 0..1, -1 = TF2's> <seconds> [entindex, default you]", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer || args.ArgC() < 3 )
	{
		Msg( "Usage: tf_defensebuff_test <resistance> <seconds> [entindex]\n" );
		return;
	}

	CTFPlayer *pTarget = GetTestTarget( args, 3, pPlayer );
	if ( !pTarget )
		return;

	pTarget->m_Shared.AddCond( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, atof( args[2] ), pPlayer );
	pTarget->m_Shared.SetDefenseBuffResistance( atof( args[1] ) );
}
