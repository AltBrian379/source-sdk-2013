//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic's primary, the Biotic Rifle (Ana-like). Plan: docs/combatmed-plan.txt.
//			Entity: tf_weapon_combatmed_rifle. Item: scripts/items/items_owtf2.txt (60002).
//
//			Unscoped: fires tf_projectile_biotic_round (tf_projectile_biotic_round.h).
//			Scoped (hold +attack2): hitscan, lag compensated, teammates included. Tracer:
//			the Hitman's Heatmaker's Focus beam (dxhr_sniper_rail_red / _blue).
//			Both: pass through the shooter and teammates at full health. An enemy hit
//			(player or building) takes tf_combatmed_rifle_damage over
//			tf_combatmed_rifle_effect_time; a hurt
//			teammate is healed tf_combatmed_rifle_heal over the same time (no overheal).
//			Flat: no distance modifier, no headshots, no random crits.
//			Stats in the weapon script (scripts/tf_weapon_combatmed_rifle.txt): 15-round
//			clip, 0.8 s between shots, 1.5 s reload (reserve: AmmoMax in combatmed.txt).
//			Scoped: FOV tf_combatmed_rifle_zoom_fov, max speed x
//			tf_combatmed_rifle_scoped_speed_mult (tf_combatmed_abilities.cpp). Reloading
//			and switching away unscope; MOUSE2 must be released and pressed again to
//			scope after either. Scoping (a new MOUSE2 press) interrupts a reload; with an empty
//			clip the rifle then reloads once unscoped.
//			PLACEHOLDER: the Crusader's Crossbow's model and animations, the Sydney
//			Sleeper's fire sound.
//
//=============================================================================//

#ifndef TF_WEAPON_COMBATMED_RIFLE_H
#define TF_WEAPON_COMBATMED_RIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFCombatMedRifle C_TFCombatMedRifle
#endif

class CTFCombatMedRifle : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS( CTFCombatMedRifle, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFCombatMedRifle();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_COMBATMED_RIFLE; }
	virtual void	Precache( void ) OVERRIDE;

	// Shots hit (and heal) teammates: lag compensate them (tf_teammate_trace.h).
	virtual bool	HitsTeammates( void ) const OVERRIDE	{ return true; }

	// No random crits, like TF2's sniper rifles.
	virtual bool	CalcIsAttackCriticalHelper( void ) OVERRIDE				{ return false; }
	virtual bool	CalcIsAttackCriticalHelperNoCrits( void ) OVERRIDE		{ return false; }

	virtual bool	Deploy( void ) OVERRIDE;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;
	virtual void	ItemBusyFrame( void ) OVERRIDE;
	virtual void	SecondaryAttack( void ) OVERRIDE	{}	// scoping is handled in ItemPostFrame
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer ) OVERRIDE;
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity ) OVERRIDE;
	virtual void	ZoomIn( void ) OVERRIDE;
	virtual void	ZoomOut( void ) OVERRIDE;

	bool			IsScoped( void ) const;

#ifdef GAME_DLL
	// What a shot does to what it hit (projectile or scoped). pWeapon is the inflictor.
	static void		ApplyBioticHit( CTFPlayer *pOwner, CBaseEntity *pTarget, CBaseEntity *pWeapon );
#endif

private:
	void			FireScopedShot( CTFPlayer *pPlayer );
	void			UpdateScope( CTFPlayer *pPlayer );

	CNetworkVar( bool, m_bScopeNeedsRelease );	// MOUSE2 must be let go before scoping again
	CNetworkVar( bool, m_bScopeStoppedReload );	// scoping interrupted a reload: an empty clip waits until unscoped

	CTFCombatMedRifle( const CTFCombatMedRifle & ) {}
};

#endif // TF_WEAPON_COMBATMED_RIFLE_H
