//========= OWTF2 ============================================================//
//
// Purpose: Veteran's primary weapon, the hero's heavy pulse rifle.
//			Entity: tf_weapon_veteran_rifle. Item: scripts/items/items_owtf2.txt.
//
//			Primary fire: PLACEHOLDER, exactly the SMG for now.
//			Alt-fire (+attack2): helix rockets (tf_projectile_helix_rocket) on a
//			cooldown, shown by the item effect meter like the Jarate/Mad Milk.
//
//=============================================================================//

#ifndef TF_WEAPON_VETERAN_RIFLE_H
#define TF_WEAPON_VETERAN_RIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_smg.h"

#ifdef CLIENT_DLL
#define CTFVeteranRifle C_TFVeteranRifle
#endif

class CTFVeteranRifle : public CTFSMG
{
public:
	DECLARE_CLASS( CTFVeteranRifle, CTFSMG );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFVeteranRifle();
	~CTFVeteranRifle() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_VETERAN_RIFLE; }
	virtual void	Precache( void ) OVERRIDE;

	// Helix rockets
	virtual void	SecondaryAttack( void ) OVERRIDE;
	bool			IsHelixReady( void ) const			{ return gpGlobals->curtime >= m_flHelixReadyTime; }
	void			ResetHelixCooldown( void )			{ m_flHelixReadyTime = 0.0f; }

	// Item effect meter (tf_hud_itemeffectmeter.cpp), same as the Jarate's.
	virtual float	GetEffectBarProgress( void ) OVERRIDE;
	float			GetProgress( void )					{ return GetEffectBarProgress(); }
	virtual const char *GetEffectLabelText( void )		{ return "#OWTF2_HelixRockets"; }

private:
	CNetworkVar( float, m_flHelixReadyTime );	// when the helix rockets can fire again

	CTFVeteranRifle( const CTFVeteranRifle & ) {}
};

#endif // TF_WEAPON_VETERAN_RIFLE_H
