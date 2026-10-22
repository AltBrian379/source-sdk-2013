//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic's melee slot, the Sleep Dart (Ana-like). Plan: docs/combatmed-plan.txt.
//			Entity: tf_weapon_combatmed_sleepdart. Item: scripts/items/items_owtf2.txt (60004).
//
//			A cast ability weapon (tf_weapon_cast_ability.h). Used with +attack4 (SHIFT,
//			CombatMed_AbilityPostFrame) or key 3: instant switch, cast
//			tf_combatmed_sleepdart_cast_time, the dart (tf_projectile_sleep_dart, server/tf)
//			is fired, the cooldown (tf_combatmed_sleepdart_cooldown) starts, and after the
//			recovery (tf_combatmed_sleepdart_recovery_time) the player is back on the rifle.
//			It doesn't interrupt another cast ability: SHIFT does nothing until the
//			grenade's cast is over.
//
//			PLACEHOLDER: the Crusader's Crossbow model and its firing animation.
//
//=============================================================================//

#ifndef TF_WEAPON_COMBATMED_SLEEPDART_H
#define TF_WEAPON_COMBATMED_SLEEPDART_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_cast_ability.h"

#ifdef CLIENT_DLL
#define CTFCombatMedSleepDart C_TFCombatMedSleepDart
#endif

class CTFCombatMedSleepDart : public CTFCastAbilityWeapon
{
public:
	DECLARE_CLASS( CTFCombatMedSleepDart, CTFCastAbilityWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFCombatMedSleepDart() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_COMBATMED_SLEEPDART; }
	virtual void	Precache( void ) OVERRIDE;

	virtual const char *GetEffectLabelText( void )		{ return "#OWTF2_SleepDart"; }

protected:
	virtual float	GetCastTime( void ) const OVERRIDE;
	virtual float	GetCooldown( void ) const OVERRIDE;
	virtual float	GetRecoveryTime( void ) const OVERRIDE;
	virtual bool	IsInstantSwitch( void ) const OVERRIDE	{ return true; }
	virtual void	OnCastFinished( CTFPlayer *pPlayer ) OVERRIDE;

private:
	CTFCombatMedSleepDart( const CTFCombatMedSleepDart & ) {}
};

#endif // TF_WEAPON_COMBATMED_SLEEPDART_H
