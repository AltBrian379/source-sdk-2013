//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic class-wide hooks. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_combatmed_abilities.h"
#include "tf_weapon_combatmed_rifle.h"
#include "tf_weapon_combatmed_grenade.h"
#include "tf_weapon_combatmed_sleepdart.h"
#include "in_buttons.h"
#include "tf_ultimate.h"
#include "tf_heal_modifiers.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "glow_outline_effect.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Initial default: Ana moves at 35% speed while scoped (a 65% penalty).
ConVar tf_combatmed_rifle_scoped_speed_mult( "tf_combatmed_rifle_scoped_speed_mult", "0.35", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: max speed multiplier while the Biotic Rifle is scoped.", true, 0.0f, true, 1.0f );
// Initial default: Ana's 8 s.
ConVar tf_combatmed_nano_duration( "tf_combatmed_nano_duration", "8", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Nano Boost duration in seconds.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 250 x 0.7 (175 / 250 HP) = 175.
ConVar tf_combatmed_nano_heal( "tf_combatmed_nano_heal", "175", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Nano Boost's instant healing on its target.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 40 m x 39.37 HU/m = 1575, rounded to 1576.
ConVar tf_combatmed_nano_range( "tf_combatmed_nano_range", "1576", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Nano Boost range (Overwatch's 40 m at 39.37 HU/m).", true, 0.0f, false, 0.0f );
// Initial default: PLACEHOLDER, how far from the crosshair a teammate can be (full angle, degrees).
ConVar tf_combatmed_nano_aim_cone( "tf_combatmed_nano_aim_cone", "20", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Nano Boost targeting cone, full angle in degrees, when no teammate is right under the crosshair.", true, 0.0f, true, 180.0f );
// Initial default: TF2's defense buff resistance, 35% (Ana's is 50%; the user chose TF2's).
ConVar tf_combatmed_nano_resistance( "tf_combatmed_nano_resistance", "0.35", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Nano Boost damage resistance (0..1), against every source.", true, 0.0f, true, 1.0f );
// Initial default: the Veteran's radius.
ConVar tf_combatmed_ultimate_voice_radius( "tf_combatmed_ultimate_voice_radius", "2048", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: players within this distance hear Nano Boost's voice line (through walls, no falloff).", true, 0.0f, false, 0.0f );
// Initial default: normal pitch.
ConVar tf_combatmed_ultimate_voice_pitch( "tf_combatmed_ultimate_voice_pitch", "100", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Nano Boost voice line pitch, percent.", true, 1.0f, true, 255.0f );
// Initial default: Ana's 5 s.
ConVar tf_combatmed_sleep_duration( "tf_combatmed_sleep_duration", "5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: seconds a Sleep Dart hit keeps the target asleep (unless damage wakes them).", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
float CombatMed_GetMaxSpeedMultiplier( const CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_COMBATMED ) )
		return 1.0f;

	const CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
	if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_COMBATMED_RIFLE && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return tf_combatmed_rifle_scoped_speed_mult.GetFloat();

	return 1.0f;
}

//-----------------------------------------------------------------------------
bool CombatMed_IsNanoBoosted( const CTFPlayer *pPlayer )
{
	return pPlayer && gpGlobals->curtime < pPlayer->m_Shared.GetNanoBoostEndTime();
}

//-----------------------------------------------------------------------------
void CombatMed_ResetAbilities( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	CTFCombatMedGrenade *pGrenade = dynamic_cast< CTFCombatMedGrenade * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_COMBATMED_GRENADE ) );
	if ( pGrenade )
	{
		pGrenade->ResetCooldown();
	}

	CTFCombatMedSleepDart *pDart = dynamic_cast< CTFCombatMedSleepDart * >( pPlayer->Weapon_OwnsThisID( TF_WEAPON_COMBATMED_SLEEPDART ) );
	if ( pDart )
	{
		pDart->ResetCooldown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: SHIFT uses the Sleep Dart: the same as switching to it with key 3. The
//          weapon's own rules apply (cooldown, the ultimate's cast, and the current
//          weapon has to allow it: a grenade cast isn't interrupted).
//-----------------------------------------------------------------------------
void CombatMed_AbilityPostFrame( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_COMBATMED ) || !pPlayer->IsAlive() )
		return;

	if ( !( pPlayer->m_afButtonPressed & IN_ATTACK4 ) )
		return;

	CBaseCombatWeapon *pDart = pPlayer->Weapon_OwnsThisID( TF_WEAPON_COMBATMED_SLEEPDART );
	if ( pDart && pPlayer->GetActiveWeapon() != pDart && pPlayer->Weapon_CanSwitchTo( pDart ) )
	{
		pPlayer->Weapon_Switch( pDart );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Nano Boost target: the teammate right under the crosshair, else the one
//          closest to it inside the cone. Living, in range, in line of sight.
//-----------------------------------------------------------------------------
CTFPlayer *CombatMed_FindNanoTarget( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	const float flRange = tf_combatmed_nano_range.GetFloat();
	const Vector vecEye = pPlayer->EyePosition();
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );

	// Right under the crosshair.
	trace_t tr;
	UTIL_TraceLine( vecEye, vecEye + vecForward * flRange, MASK_SOLID | CONTENTS_HITBOX, pPlayer, COLLISION_GROUP_NONE, &tr );
	CTFPlayer *pAimed = ToTFPlayer( tr.m_pEnt );
	if ( pAimed && pAimed->IsAlive() && pAimed->GetTeamNumber() == pPlayer->GetTeamNumber() )
		return pAimed;

	// Otherwise the closest to the crosshair inside the cone.
	const float flCosCone = cosf( DEG2RAD( tf_combatmed_nano_aim_cone.GetFloat() * 0.5f ) );
	CTFPlayer *pBest = NULL;
	float flBestDot = flCosCone;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pOther = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pOther || pOther == pPlayer || !pOther->IsAlive() || pOther->GetTeamNumber() != pPlayer->GetTeamNumber() )
			continue;

		Vector vecTo = pOther->WorldSpaceCenter() - vecEye;
		const float flDist = VectorNormalize( vecTo );
		if ( flDist > flRange )
			continue;

		const float flDot = DotProduct( vecTo, vecForward );
		if ( flDot < flBestDot )
			continue;

		// Line of sight: nothing solid but players in between.
		UTIL_TraceLine( vecEye, pOther->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0f )
			continue;

		pBest = pOther;
		flBestDot = flDot;
	}

	return pBest;
}

#ifdef GAME_DLL
// Nano Boost targets waiting to say their Battle Cry.
struct NanoBattleCry_t
{
	CHandle< CTFPlayer >	hTarget;
	float					flTime;
};
static CUtlVector< NanoBattleCry_t > s_NanoBattleCries;

//-----------------------------------------------------------------------------
void CombatMed_PutToSleep( CTFPlayer *pVictim, CTFPlayer *pAttacker )
{
	if ( !pVictim || !pVictim->IsAlive() || pVictim->m_Shared.IsInvulnerable() )
		return;

	const float flDuration = tf_combatmed_sleep_duration.GetFloat();
	if ( flDuration <= 0.0f )
		return;

	// TF2's full stun: no moving, no attacking. The stun is on at once, so nothing
	// can wake the target between the hit and the sleep. TF2 lengthens control stuns
	// for its get-up animation; CombatMed_SleepThink ends the sleep at flDuration.
	pVictim->m_Shared.StunPlayer( flDuration, 1.0f, TF_STUN_MOVEMENT | TF_STUN_CONTROLS, pAttacker );
	pVictim->m_Shared.SetSleepEndTime( gpGlobals->curtime + flDuration );

	// Sleep interrupts any hero's ultimate cast; its charge is lost.
	Ultimate_InterruptCast( pVictim );
}

bool CombatMed_IsAsleep( const CTFPlayer *pPlayer )
{
	return pPlayer && gpGlobals->curtime < pPlayer->m_Shared.GetSleepEndTime() && pPlayer->m_Shared.InCond( TF_COND_STUNNED );
}

static void WakeUp( CTFPlayer *pPlayer )
{
	pPlayer->m_Shared.SetSleepEndTime( 0.0f );
	if ( pPlayer->m_Shared.InCond( TF_COND_STUNNED ) )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_STUNNED );
	}
}

void CombatMed_OnDamageTaken( CTFPlayer *pPlayer )
{
	if ( CombatMed_IsAsleep( pPlayer ) )
	{
		WakeUp( pPlayer );
	}
}

void CombatMed_SleepThink( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// A Nano Boost target's Battle Cry, once the Combat Medic's line is over.
	FOR_EACH_VEC_BACK( s_NanoBattleCries, i )
	{
		CTFPlayer *pTarget = s_NanoBattleCries[i].hTarget.Get();
		if ( !pTarget )
		{
			s_NanoBattleCries.Remove( i );
			continue;
		}

		if ( pTarget == pPlayer && gpGlobals->curtime >= s_NanoBattleCries[i].flTime )
		{
			s_NanoBattleCries.Remove( i );
			if ( pPlayer->IsAlive() && TFGameRules() )
			{
				TFGameRules()->VoiceCommand( pPlayer, 2, 1 );
			}
		}
	}

	const float flEndTime = pPlayer->m_Shared.GetSleepEndTime();
	if ( flEndTime > 0.0f && gpGlobals->curtime >= flEndTime )
	{
		WakeUp( pPlayer );
	}
}
//-----------------------------------------------------------------------------
// Purpose: No target, no ultimate: the charge is kept.
//-----------------------------------------------------------------------------
bool CombatMed_CanUseUltimate( CTFPlayer *pPlayer )
{
	return CombatMed_FindNanoTarget( pPlayer ) != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Nano Boost (instant): heal the target, then minicrits and resistance.
//-----------------------------------------------------------------------------
void CombatMed_OnUltimateCastEnd( CTFPlayer *pPlayer )
{
	CTFPlayer *pTarget = CombatMed_FindNanoTarget( pPlayer );
	if ( !pTarget )
		return;

	// The ultimate's own healing earns no ultimate charge (it's spent by now; put it back to 0).
	const int nHealed = pTarget->TakeHealth( tf_combatmed_nano_heal.GetFloat(), DMG_GENERIC );
	HealModifiers_CreditHealing( pPlayer, pTarget, nHealed );
	Ultimate_SetCharge( pPlayer, 0.0f );

	const float flDuration = tf_combatmed_nano_duration.GetFloat();
	pTarget->m_Shared.SetNanoBoostEndTime( gpGlobals->curtime + flDuration );
	pTarget->m_Shared.AddCond( TF_COND_ENERGY_BUFF, flDuration, pPlayer );
	pTarget->m_Shared.AddCond( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, flDuration, pPlayer );
	pTarget->m_Shared.SetDefenseBuffResistance( tf_combatmed_nano_resistance.GetFloat() );
	pTarget->TeamFortress_SetSpeed();

	// PLACEHOLDER voice line, heard like the Veteran's; when it's over, the target answers
	// with the Battle Cry voice command (voice menu 3, item 2: C then 2).
	const float flLineTime = Ultimate_PlayVoiceLine( pPlayer, COMBATMED_ULTIMATE_VOICE_LINE, tf_combatmed_ultimate_voice_radius.GetFloat(),
													 tf_combatmed_ultimate_voice_pitch.GetInt() );
	NanoBattleCry_t battleCry;
	battleCry.hTarget = pTarget;
	battleCry.flTime = gpGlobals->curtime + flLineTime;
	s_NanoBattleCries.AddToTail( battleCry );
}
#endif

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: While the local Combat Medic's Nano Boost is ready, outline (yellow) the
//          teammate Q would give it to.
//-----------------------------------------------------------------------------
class CCombatMedNanoTargetOutline : public CAutoGameSystemPerFrame
{
public:
	CCombatMedNanoTargetOutline() : CAutoGameSystemPerFrame( "CCombatMedNanoTargetOutline" ), m_pGlow( NULL ) {}

	virtual void LevelShutdownPreEntity() OVERRIDE
	{
		delete m_pGlow;
		m_pGlow = NULL;
	}

	virtual void Update( float frametime ) OVERRIDE
	{
		C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
		C_TFPlayer *pTarget = NULL;
		if ( pLocal && pLocal->IsAlive() && pLocal->IsPlayerClass( TF_CLASS_COMBATMED ) && Ultimate_IsReady( pLocal ) )
		{
			pTarget = CombatMed_FindNanoTarget( pLocal );
		}

		if ( !pTarget )
		{
			if ( m_pGlow )
			{
				m_pGlow->SetEntity( NULL );
			}
			return;
		}

		if ( !m_pGlow )
		{
			m_pGlow = new CGlowObject( pTarget, Vector( 1.0f, 0.85f, 0.1f ), 1.0f, false, true );
		}
		else
		{
			m_pGlow->SetEntity( pTarget );
		}
	}

private:
	CGlowObject *m_pGlow;
};

static CCombatMedNanoTargetOutline s_CombatMedNanoTargetOutline;
#endif
