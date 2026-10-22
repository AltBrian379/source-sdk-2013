//========= OWTF2 ============================================================//
//
// Purpose: Base for ability weapons that are used by switching to them:
//			switch in -> cast -> effect -> cooldown -> switch back to the primary.
//			Veteran's Field Rations, Combat Medic's Biotic Grenade and Sleep Dart.
//
//			- Switching to the weapon starts the cast (GetCastTime). The player can't
//			  switch away during it. A forced holster (death, class change) cancels it:
//			  no effect, no cooldown.
//			- When the cast ends, OnCastFinished runs (client and server; spawn entities
//			  on the server) and the cooldown starts (GetCooldown). After an optional
//			  recovery (GetRecoveryTime, still on the weapon, still can't switch away)
//			  the player switches back to the primary.
//			- It can't be selected while on cooldown or while the ultimate is casting.
//			- Optional: an instant switch (no draw time) and an activity played when
//			  the cast starts (a throw, for example).
//			- The cooldown is shown by the item effect meter like the Jarate's.
//
//=============================================================================//

#ifndef TF_WEAPON_CAST_ABILITY_H
#define TF_WEAPON_CAST_ABILITY_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#define CTFCastAbilityWeapon C_TFCastAbilityWeapon
#endif

class CTFCastAbilityWeapon : public CTFWeaponBase
{
public:
	DECLARE_CLASS( CTFCastAbilityWeapon, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFCastAbilityWeapon();

	// Selecting the weapon is the ability's input.
	virtual bool	CanBeSelected( void ) OVERRIDE;
	virtual bool	CanDeploy( void ) OVERRIDE;
	virtual bool	Deploy( void ) OVERRIDE;
	virtual bool	CanHolster( void ) const OVERRIDE;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;
	virtual void	ItemBusyFrame( void ) OVERRIDE;
	virtual void	PrimaryAttack( void ) OVERRIDE {}
	virtual void	SecondaryAttack( void ) OVERRIDE {}

	bool			IsReady( void ) const				{ return gpGlobals->curtime >= m_flReadyTime; }
	bool			IsCasting( void ) const				{ return m_flCastEndTime > 0.0f; }
	bool			IsRecovering( void ) const			{ return m_flReturnTime > 0.0f; }
	void			ResetCooldown( void )				{ m_flReadyTime = 0.0f; m_flCastEndTime = 0.0f; m_flReturnTime = 0.0f; }

	// Item effect meter (tf_hud_itemeffectmeter.cpp), same as the Jarate's.
	virtual float	GetEffectBarProgress( void ) OVERRIDE;
	float			GetProgress( void )					{ return GetEffectBarProgress(); }

protected:
	// Per ability. Not pure virtual: the networked base class needs a client factory.
	virtual float	GetCastTime( void ) const			{ return 0.0f; }
	virtual float	GetCooldown( void ) const			{ return 0.0f; }
	virtual float	GetRecoveryTime( void ) const		{ return 0.0f; }	// after the effect, before switching back
	virtual bool	CanUseNow( void )					{ return true; }	// extra per-ability conditions
	virtual bool	IsInstantSwitch( void ) const		{ return false; }	// skip the draw time
	virtual Activity GetCastActivity( void ) const		{ return ACT_INVALID; }	// played when the cast starts
	virtual void	OnCastStarted( CTFPlayer *pPlayer )	{}
	virtual void	OnCastFinished( CTFPlayer *pPlayer ) {}	// the effect; spawn entities on the server

private:
	void			CheckCast( void );
	void			SwitchToPrimary( CTFPlayer *pPlayer );

	CNetworkVar( float, m_flCastEndTime );	// when the cast ends; 0 = not casting
	CNetworkVar( float, m_flReadyTime );	// when the ability can be used again
	CNetworkVar( float, m_flReturnTime );	// after the effect: when to switch back; 0 = not recovering

	CTFCastAbilityWeapon( const CTFCastAbilityWeapon & ) {}
};

#endif // TF_WEAPON_CAST_ABILITY_H
