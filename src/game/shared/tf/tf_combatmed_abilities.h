//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic (TF_CLASS_COMBATMED) class-wide hooks. Plan: docs/combatmed-plan.txt.
//
//	Biotic Rifle (tf_weapon_combatmed_rifle.cpp): while scoped, max speed is multiplied
//	by tf_combatmed_rifle_scoped_speed_mult.
//	Sleep Dart (tf_weapon_combatmed_sleepdart.cpp): +attack4 (SHIFT) switches to it, like
//	key 3. A hit puts the target to sleep: TF2's full stun (TF_STUN_BOTH, its stars
//	and animation as PLACEHOLDER visuals) for tf_combatmed_sleep_duration, and an
//	ultimate the target is still casting is interrupted (its charge is lost). Any
//	damage that lands wakes the sleeper (damage over time too); healing doesn't.
//	Nano Boost (the ultimate, tf_ultimate.cpp; instant): needs a living teammate under
//	the crosshair or near it (tf_combatmed_nano_aim_cone) within tf_combatmed_nano_range,
//	in line of sight; without one the ultimate isn't used and keeps its charge. The
//	target is healed tf_combatmed_nano_heal, then for tf_combatmed_nano_duration gets
//	Crit-a-Cola's minicrits (TF_COND_ENERGY_BUFF, without the Heavy's speed bonus) and
//	the Battalion's Backup's resistance without its crit immunity
//	(TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK at tf_combatmed_nano_resistance). The Combat
//	Medic's voice line plays like the Veteran's (Ultimate_PlayVoiceLine); when it ends
//	the target says the Battle Cry voice command. While the ultimate is ready, the client
//	outlines the teammate it would go to (yellow).
//
//=============================================================================//

#ifndef TF_COMBATMED_ABILITIES_H
#define TF_COMBATMED_ABILITIES_H
#ifdef _WIN32
#pragma once
#endif

// PLACEHOLDER: Nano Boost's voice line (precached by the Biotic Rifle).
#define COMBATMED_ULTIMATE_VOICE_LINE	"vo/medic_battlecry01.mp3"

#ifdef CLIENT_DLL
class C_TFPlayer;
#define CTFPlayer C_TFPlayer
#else
class CTFPlayer;
#endif

// Multiplier on max speed (CTFPlayer::TeamFortress_CalculateMaxSpeed).
float	CombatMed_GetMaxSpeedMultiplier( const CTFPlayer *pPlayer );

// The player is under a Combat Medic's Nano Boost (client and server).
bool	CombatMed_IsNanoBoosted( const CTFPlayer *pPlayer );

// Every spawn: abilities are ready again (CTFPlayerShared::Spawn).
void	CombatMed_ResetAbilities( CTFPlayer *pPlayer );

// Ability input, every command (CTFPlayer::ItemPostFrame). Predicted.
void	CombatMed_AbilityPostFrame( CTFPlayer *pPlayer );

#ifdef GAME_DLL
// Sleep Dart hit (tf_projectile_sleep_dart.cpp). Does nothing to an ÜberCharged player.
void	CombatMed_PutToSleep( CTFPlayer *pVictim, CTFPlayer *pAttacker );
bool	CombatMed_IsAsleep( const CTFPlayer *pPlayer );
// Health was just taken from the player (CTFPlayer::OnTakeDamage_Alive): wakes a sleeper.
void	CombatMed_OnDamageTaken( CTFPlayer *pPlayer );
// Every player, every tick (CTFPlayerShared::ConditionGameRulesThink): ends the sleep on time.
void	CombatMed_SleepThink( CTFPlayer *pPlayer );

// Ultimate hooks (tf_ultimate.cpp).
bool	CombatMed_CanUseUltimate( CTFPlayer *pPlayer );
void	CombatMed_OnUltimateCastEnd( CTFPlayer *pPlayer );
#endif

// The teammate Nano Boost would go to now, or NULL. Client and server: the client
// outlines it (yellow) while the ultimate is ready.
CTFPlayer *CombatMed_FindNanoTarget( CTFPlayer *pPlayer );

#endif // TF_COMBATMED_ABILITIES_H
