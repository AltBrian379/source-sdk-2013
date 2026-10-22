//========= OWTF2 ============================================================//
//
// Purpose: Ultimate ability charge system, shared by every hero class.
//
//	Per class (scripts/playerclasses/<class>.txt):
//		"ultimate_cost"		points needed to use the ultimate; 0 = class has no ultimate
//		"ultimate_duration"	seconds the charge drains over once used; 0 = spent instantly
//		"ultimate_cast_time"	seconds between using it and its effect starting (0 = immediate).
//						The ultimate counts as active during the cast, but doesn't drain.
//
//	Charging (server only; tuning in the tf_ultimate_* convars):
//		- passive, per second, while alive and the round is running with setup over
//		- per point of damage to enemy players (not buildings); damage that only removes
//		  overheal earns tf_ultimate_overheal_damage_scale of that
//		- per point of healing (TF2's healing accounting, CTFGameStats::Event_PlayerHealedOther)
//		Nothing charges past the cost, while the ultimate is active, from damage to
//		yourself or teammates, or from damage dealt by an ultimate.
//
//	Lifetime: kept through death; reset at every round start and on class change.
//	Dying ends an active ultimate. Being interrupted mid-cast (Sleep Dart) ends it too,
//	and the charge is lost.
//
//	Per-hero hooks (dispatched by class in tf_ultimate.cpp): can it be used now, cast
//	start (on use), cast end (effect starts), and end.
//
//	State: CTFPlayerShared::m_flUltimateCharge (points), m_bUltimateActive and
//	m_flUltimateCastEndTime, networked
//	to the owner. Input: +attack5 (IN_ATTACK5) uses the ultimate.
//
//=============================================================================//

#ifndef TF_ULTIMATE_H
#define TF_ULTIMATE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
class C_TFPlayer;
#define CTFPlayer C_TFPlayer
#else
class CTFPlayer;
class CBaseEntity;
class CTakeDamageInfo;
#endif

// Per-class values; 0 cost means the class has no ultimate.
float	Ultimate_GetCost( const CTFPlayer *pPlayer );
float	Ultimate_GetDuration( const CTFPlayer *pPlayer );
float	Ultimate_GetCastTime( const CTFPlayer *pPlayer );
bool	Ultimate_HasUltimate( const CTFPlayer *pPlayer );

// Charge as 0..1 of the cost (HUD).
float	Ultimate_GetChargeFraction( const CTFPlayer *pPlayer );
bool	Ultimate_IsReady( const CTFPlayer *pPlayer );
bool	Ultimate_IsActive( const CTFPlayer *pPlayer );
// Active and still casting (the hero is busy activating it).
bool	Ultimate_IsCasting( const CTFPlayer *pPlayer );
// Active and done casting: the effect is on.
bool	Ultimate_IsEffectActive( const CTFPlayer *pPlayer );

// Called every command from CTFPlayer::ItemPostFrame. Use is decided by the server.
void	Ultimate_PostFrame( CTFPlayer *pPlayer );

#ifdef GAME_DLL
// Called every tick for every player from CTFPlayerShared::ConditionGameRulesThink.
void	Ultimate_Think( CTFPlayer *pPlayer );

// Adds charge, honouring the cap and the "not while active" rule.
void	Ultimate_AddCharge( CTFPlayer *pPlayer, float flPoints );
void	Ultimate_SetCharge( CTFPlayer *pPlayer, float flPoints );
void	Ultimate_ResetCharge( CTFPlayer *pPlayer );

bool	Ultimate_TryActivate( CTFPlayer *pPlayer );
void	Ultimate_End( CTFPlayer *pPlayer );
// Ends an ultimate that is still casting; the charge is lost. True if there was one.
bool	Ultimate_InterruptCast( CTFPlayer *pPlayer );

// An ultimate's voice line: every player within flRadius of pPlayer hears it, through walls,
// at full volume like the announcer (no distance falloff), at iPitch percent (which also
// changes its speed). Returns how long it plays, in seconds.
float	Ultimate_PlayVoiceLine( CTFPlayer *pPlayer, const char *pszSound, float flRadius, int iPitch );

// Listeners, called where TF2 applies damage and accounts healing.
//	flDamage: health actually removed (no overkill); flOverhealDamage: the part of it that
//	only removed overheal.
void	Ultimate_OnDamageDealt( const CTakeDamageInfo &info, CBaseEntity *pVictim, float flDamage, float flOverhealDamage );
void	Ultimate_OnHealingDone( CTFPlayer *pHealer, float flAmount );

// Entities an ultimate creates (projectiles, areas...) should be marked so their damage
// and healing don't charge the ultimate after it has ended.
void	Ultimate_MarkEntity( CBaseEntity *pEntity );
bool	Ultimate_IsUltimateDamage( const CTakeDamageInfo &info, const CTFPlayer *pAttacker );
#endif

#endif // TF_ULTIMATE_H
