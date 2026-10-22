//========= OWTF2 ============================================================//
//
// Purpose: Veteran's primary weapon, the hero's heavy pulse rifle.
//			Entity: tf_weapon_veteran_rifle. Item: scripts/items/items_owtf2.txt.
//
//			Primary fire: hitscan auto rifle. Stats in the weapon script
//			(scripts/tf_weapon_veteran_rifle.txt): no spread, 30-round clip,
//			9 shots/s, 1.5 s reload. Headshots minicrit (tf_gamerules.cpp).
//			Damage falloff: TF2's close-range bonus up to tf_veteran_rifle_falloff_start,
//			then down to the minimum at tf_veteran_rifle_falloff_end.
//			Recoil: from shot tf_veteran_rifle_recoil_start_shot of a burst on, each
//			shot climbs the view (client side, applied in C_TFPlayer::CreateMove);
//			the player pulls down to compensate, and leftover recoil recovers
//			after firing stops.
//			Ultimate (tf_ultimate.cpp, Veteran_OnUltimate*): during the cast the rifle
//			can't fire, reload or be put away; when it ends the clip refills, and while
//			the effect lasts the reload takes tf_veteran_ultimate_reload_time and bullets
//			aim themselves at the nearest visible enemy player in a cone
//			(tf_veteran_ultimate_aim_cone / _range). Helix rockets aren't aimed.
//			Alt-fire (+attack2): helix rockets (tf_projectile_helix_rocket) on a
//			cooldown, shown by the item effect meter like the Jarate/Mad Milk.
//			Model, sounds and animations: PLACEHOLDER, the SMG's.
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

// PLACEHOLDER: the Veteran's ultimate voice line (Veteran_OnUltimateStart), precached by the rifle.
#define VETERAN_ULTIMATE_VOICE_LINE		"vo/sniper_battlecry05.mp3"

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

	// Always usable: the helix rockets need no ammo, so an empty rifle stays out
	// (no auto-switch to the Field Rations, which would cast it) and can be switched back to.
	virtual bool	HasAnyAmmo( void ) OVERRIDE			{ return true; }

	virtual bool	Deploy( void ) OVERRIDE;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;
	virtual void	ItemBusyFrame( void ) OVERRIDE;
	virtual bool	CanHolster( void ) const OVERRIDE;

	// Primary fire
	virtual void	PrimaryAttack( void ) OVERRIDE;
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer ) OVERRIDE;
	virtual void	FireBullet( CTFPlayer *pPlayer ) OVERRIDE;
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity ) OVERRIDE;

	// Damage falloff, used by CTFGameRules::ApplyOnDamageModifyRules for the rifle's bullets.
	// Returns TF2's distance-mod "center" (1 = +50% damage, 0.5 = base, 0 = -50%).
	static float	GetDistanceModCenter( float flDistance );

	// Ultimate
	void			Ultimate_RefillClip( void );
	// Nearest enemy player the ultimate's auto-aim can lock onto from vecSrc; fills the point to aim at.
	static CTFPlayer *FindAutoAimTarget( CTFPlayer *pOwner, const Vector &vecSrc, const Vector &vecForward, Vector *pvecAimPoint );

	// Helix rockets
	virtual void	SecondaryAttack( void ) OVERRIDE;
	bool			IsHelixReady( void ) const			{ return gpGlobals->curtime >= m_flHelixReadyTime; }
	void			ResetHelixCooldown( void )			{ m_flHelixReadyTime = 0.0f; }
	// Cast time after a helix launch: no firing or reloading until it ends.
	bool			IsHelixCasting( void ) const		{ return gpGlobals->curtime < m_flHelixCastEndTime; }
	void			CancelHelixCast( void );

	// Item effect meter (tf_hud_itemeffectmeter.cpp), same as the Jarate's.
	virtual float	GetEffectBarProgress( void ) OVERRIDE;
	float			GetProgress( void )					{ return GetEffectBarProgress(); }
	virtual const char *GetEffectLabelText( void )		{ return "#OWTF2_HelixRockets"; }

private:
	CNetworkVar( float, m_flHelixReadyTime );	// when the helix rockets can fire again
	CNetworkVar( int, m_iBurstShots );			// shots fired since the trigger was pressed
	CNetworkVar( float, m_flHelixCastEndTime );	// when the helix cast ends (firing and reloading allowed again)

	bool			CanFireHelix( CTFPlayer *pPlayer );

	CTFVeteranRifle( const CTFVeteranRifle & ) {}
};

#ifdef CLIENT_DLL
class C_TFPlayer;
class CUserCmd;

// Recoil, client only. Called from C_TFPlayer::CreateMove (also for the extra
// per-frame mouse samples). bActive is false while something else owns the view
// angles (taunts, karts, frozen input).
void VeteranRifleRecoil_CreateMove( C_TFPlayer *pPlayer, CUserCmd *pCmd, bool bActive );
#endif

#endif // TF_WEAPON_VETERAN_RIFLE_H
