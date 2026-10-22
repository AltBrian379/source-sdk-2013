//========= OWTF2 ============================================================//
//
// Purpose: Veteran class abilities (shared client/server, predicted).
//
//			Sprint: +attack4 toggles a movement speed bonus.
//			Helix rockets: +attack2, implemented as the rifle's alt-fire
//			(tf_weapon_veteran_rifle.cpp, tf_projectile_helix_rocket.cpp).
//			Healing field: switching to the secondary, the Field Rations, casts it
//			(tf_weapon_veteran_field_rations.cpp, tf_veteran_healing_field.cpp).
//			Ultimate: charge and use are the shared system in tf_ultimate.cpp. The
//			Veteran's: a cast (class "ultimate_cast_time") during which the rifle,
//			abilities and weapon switching are blocked, then the rifle's ultimate
//			state (full clip, fast reload, auto-aim) until it drains
//			(tf_weapon_veteran_rifle.cpp). Hooks: Veteran_OnUltimate*.
//
//			Ability state lives in CTFPlayerShared (networked and predicted);
//			the logic lives here so the hero's systems stay out of Valve's files.
//
//=============================================================================//

#ifndef TF_VETERAN_ABILITIES_H
#define TF_VETERAN_ABILITIES_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
class C_TFPlayer;
#define CTFPlayer C_TFPlayer
#else
class CTFPlayer;
#endif

class CMoveData;

// Called every command from CTFPlayer::ItemPostFrame, on the server and in client prediction.
void Veteran_AbilityPostFrame( CTFPlayer *pPlayer );

// Called by CTFGameMovement::PlayerMove after the move, on the server and in client prediction.
// Ends Sprint when the player stops moving forward.
void Veteran_SprintPostMove( CTFPlayer *pPlayer, const CMoveData *pMove );

// Turns Sprint off, if on. Firing the primary weapon and using any ability call this.
void Veteran_StopSprint( CTFPlayer *pPlayer );

// Multiplier applied to the player's max speed by CTFPlayer::TeamFortress_CalculateMaxSpeed.
float Veteran_GetMaxSpeedMultiplier( const CTFPlayer *pPlayer );

// Clears all ability state (spawn, death, class change).
void Veteran_ResetAbilities( CTFPlayer *pPlayer );

#ifdef GAME_DLL
// Ultimate hooks, called by tf_ultimate.cpp: can it be used now, cast start (on use),
// cast end (the rifle's ultimate state starts), end.
bool Veteran_CanUseUltimate( CTFPlayer *pPlayer );
void Veteran_OnUltimateStart( CTFPlayer *pPlayer );
void Veteran_OnUltimateCastEnd( CTFPlayer *pPlayer );
void Veteran_OnUltimateEnd( CTFPlayer *pPlayer );
#endif

#endif // TF_VETERAN_ABILITIES_H
