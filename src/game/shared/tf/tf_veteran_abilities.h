//========= OWTF2 ============================================================//
//
// Purpose: Veteran class abilities (shared client/server, predicted).
//
//			Sprint: +attack4 toggles a movement speed bonus.
//			Helix rockets: +attack2, implemented as the rifle's alt-fire
//			(tf_weapon_veteran_rifle.cpp, tf_projectile_helix_rocket.cpp).
//			Ultimate: charge and use are the shared system in tf_ultimate.cpp; the
//			Veteran's effect goes in Veteran_OnUltimateStart/End.
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

// Called every command from CTFPlayer::ItemPostFrame, on the server and in client prediction.
void Veteran_AbilityPostFrame( CTFPlayer *pPlayer );

// Multiplier applied to the player's max speed by CTFPlayer::TeamFortress_CalculateMaxSpeed.
float Veteran_GetMaxSpeedMultiplier( const CTFPlayer *pPlayer );

// Clears all ability state (spawn, death, class change).
void Veteran_ResetAbilities( CTFPlayer *pPlayer );

#ifdef GAME_DLL
// Ultimate effect, called by tf_ultimate.cpp.
void Veteran_OnUltimateStart( CTFPlayer *pPlayer );
void Veteran_OnUltimateEnd( CTFPlayer *pPlayer );
#endif

#endif // TF_VETERAN_ABILITIES_H
