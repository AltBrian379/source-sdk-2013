//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic's secondary, the Biotic Grenade (Ana-like). Plan: docs/combatmed-plan.txt.
//			Entity: tf_weapon_combatmed_grenade. Item: scripts/items/items_owtf2.txt (60003).
//
//			A cast ability weapon (tf_weapon_cast_ability.h) on key 2: the switch is
//			instant, the throw animation plays, and after tf_combatmed_grenade_cast_time
//			the grenade (tf_projectile_biotic_grenade, server/tf) is thrown, the cooldown
//			(tf_combatmed_grenade_cooldown) starts and the player is back on the rifle.
//			What the explosion does: tf_projectile_biotic_grenade.h.
//
//			PLACEHOLDER: Jarate model in hand, Medic's throwable viewmodel animation.
//
//=============================================================================//

#ifndef TF_WEAPON_COMBATMED_GRENADE_H
#define TF_WEAPON_COMBATMED_GRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_cast_ability.h"

#ifdef CLIENT_DLL
#define CTFCombatMedGrenade C_TFCombatMedGrenade
#endif

class CTFCombatMedGrenade : public CTFCastAbilityWeapon
{
public:
	DECLARE_CLASS( CTFCombatMedGrenade, CTFCastAbilityWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFCombatMedGrenade() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_COMBATMED_GRENADE; }
	virtual void	Precache( void ) OVERRIDE;

	virtual const char *GetEffectLabelText( void )		{ return "#OWTF2_BioticGrenade"; }

protected:
	virtual float	GetCastTime( void ) const OVERRIDE;
	virtual float	GetCooldown( void ) const OVERRIDE;
	virtual bool	IsInstantSwitch( void ) const OVERRIDE	{ return true; }
	virtual Activity GetCastActivity( void ) const OVERRIDE	{ return ACT_THROWABLE_VM_FIRE; }
	virtual void	OnCastStarted( CTFPlayer *pPlayer ) OVERRIDE;
	virtual void	OnCastFinished( CTFPlayer *pPlayer ) OVERRIDE;

private:
	CTFCombatMedGrenade( const CTFCombatMedGrenade & ) {}
};

#endif // TF_WEAPON_COMBATMED_GRENADE_H
