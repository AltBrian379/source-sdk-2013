//========= OWTF2 ============================================================//
//
// Purpose: Damage (and healing) spread over time, for any hero to apply (Combat Medic's Biotic
//			Rifle deals its damage this way). TF2's bleed has fixed ticks and doesn't fit.
//
//	Each application is its own effect: the total damage is dealt evenly over the
//	duration, in hits every tf_damage_over_time_interval seconds (and the rest at the
//	end), through the normal damage path (CTakeDamageInfo, ApplyOnDamageModifyRules),
//	credited to the attacker, inflictor and weapon given. No knockback.
//	An effect stops when the victim dies (a building: is destroyed) or the attacker is gone.
//
//	Healing over time works the same way: whole points every interval through
//	CTFPlayer::TakeHealth (heal modifiers apply, no overheal), credited to the healer
//	(CTF_GameStats.Event_PlayerHealedOther: scores and ultimate charge) with a
//	player_healed event.
//
//=============================================================================//

#ifndef TF_DAMAGE_OVER_TIME_H
#define TF_DAMAGE_OVER_TIME_H
#ifdef _WIN32
#pragma once
#endif

class CTFPlayer;
class CBaseEntity;

// pVictim: a player, or a building or anything else that takes damage.
void	DamageOverTime_Apply( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, CBaseEntity *pWeapon,
							  float flTotalDamage, float flDuration, int bitsDamageType, int iCustomDamage );
void	HealOverTime_Apply( CTFPlayer *pTarget, CTFPlayer *pHealer, float flTotalHealing, float flDuration );
// Removes damage and healing over time on the entity.
void	DamageOverTime_Clear( CBaseEntity *pVictim );

#endif // TF_DAMAGE_OVER_TIME_H
