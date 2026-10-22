//========= OWTF2 ============================================================//
//
// Purpose: Incoming-healing windows on a player, for any hero to apply.
//
//	No-heal: the player can't be healed at all.
//	Heal boost: incoming healing is multiplied.
//	If both are on, no-heal wins.
//
//	Applied to every way TF2 heals a player: CTFPlayer::TakeHealth (health packs,
//	crossbow bolts, on-hit heals, burst heals), the healer-list tick in
//	CTFPlayerShared::Heal (Medigun, dispenser, healing fields) and class health
//	regeneration. Healing credit (ultimate charge, scores) is the healing actually done.
//
//	State: CTFPlayerShared::m_flNoHealEndTime / m_flHealBoostEndTime (networked to
//	everyone, for tints), m_flHealBoostScale (server only). Cleared on spawn.
//
//=============================================================================//

#ifndef TF_HEAL_MODIFIERS_H
#define TF_HEAL_MODIFIERS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
class C_TFPlayer;
#define CTFPlayer C_TFPlayer
#else
class CTFPlayer;
#endif

bool	HealModifiers_IsNoHeal( const CTFPlayer *pPlayer );
bool	HealModifiers_IsHealBoosted( const CTFPlayer *pPlayer );

#ifdef GAME_DLL
// Multiplier on healing the player receives right now: 0 under no-heal, the boost scale
// while boosted, else 1.
float	HealModifiers_GetIncomingScale( const CTFPlayer *pPlayer );

// A new window replaces a shorter one; it never shortens one already running.
void	HealModifiers_ApplyNoHeal( CTFPlayer *pPlayer, float flDuration );
void	HealModifiers_ApplyHealBoost( CTFPlayer *pPlayer, float flDuration, float flScale );
void	HealModifiers_Clear( CTFPlayer *pPlayer );

// Healing one of the mod's heroes did: credits the healer (CTF_GameStats.Event_PlayerHealedOther:
// scores, ultimate charge) and fires player_healed (the heal numbers). nHealed is what
// TakeHealth returned.
void	HealModifiers_CreditHealing( CTFPlayer *pHealer, CTFPlayer *pPatient, int nHealed );
#endif

#endif // TF_HEAL_MODIFIERS_H
