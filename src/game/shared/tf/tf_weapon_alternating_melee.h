//========= OWTF2 ============================================================//
//
// Purpose: Melee weapon whose swings alternate left and right (shared hero system).
//			First used by the Crusader's hammer (tf_weapon_crusader_hammer).
//
//			Rule (Overwatch's Reinhardt): a swing goes to the left, unless it chains: it
//			starts on the very tick the previous swing allows the next one (the attack
//			button held, or a press buffered during the previous swing). A chained swing
//			goes the other way. A swing that starts even one tick later is a left swing again.
//
//			Buffer: pressing attack during a swing queues the next swing, which then starts
//			as soon as the swing allows it, even if the button was released.
//
//			Animations: per direction, a viewmodel sequence (by name) and third-person
//			gestures (by activity, standing / crouching / swimming), from GetSwingAnims().
//			Third person has its own left and right entries so a weapon with custom
//			animations can show both; until then both can be the same.
//			The overhead crit swing (ACT_VM_SWINGHARD) is never used for a normal swing.
//			Viewmodel timing (with a sweep): the sequence's playback rate changes per phase so
//			that its wind-up fills the cast, the frames where the blade crosses the screen
//			(flViewModelSweep*, as cycles) play exactly while the rays fire, and the rest fills
//			the recovery.
//
//			Sweep hit (optional, GetSweepParams): instead of TF2's one trace at the smack time,
//			after a cast delay, rays fire one at a time, evenly through the active time, across an arc in front of the
//			camera, in the direction the swing moves (a left swing sweeps from the right edge to
//			the left edge). Each ray is aimed from the camera when it fires, so turning while
//			swinging widens the arc. A ray stops at the world and props; every enemy player or
//			building whose hull it crosses is hit. The space between a ray and the previous one
//			is filled with extra rays (at most 1 degree apart), so anything between rays is hit
//			too. A target is hit at most once per swing. Server side, lag compensated.
//			Knockback: TF2's, from the damage (ApplyPushFromDamage), but in the direction the
//			swing moves (straight sideways from the camera), not away from the attacker
//			(AlternatingSwing_AdjustDamageDirection, from CTFPlayer::AdjustDamageDirection).
//
//=============================================================================//

#ifndef TF_WEAPON_ALTERNATING_MELEE_H
#define TF_WEAPON_ALTERNATING_MELEE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFAlternatingSwingMelee C_TFAlternatingSwingMelee
#endif

enum EAlternatingSwingSide
{
	ALTERNATING_SWING_LEFT = 0,
	ALTERNATING_SWING_RIGHT,

	ALTERNATING_SWING_SIDE_COUNT
};

enum EAlternatingSwingStance
{
	ALTERNATING_SWING_STAND = 0,
	ALTERNATING_SWING_CROUCH,
	ALTERNATING_SWING_SWIM,

	ALTERNATING_SWING_STANCE_COUNT
};

struct AlternatingSwingAnims_t
{
	// Viewmodel: the activity the swing is sent as (translated like any weapon activity, for
	// bookkeeping), then the sequence played on the viewmodel, by name. A missing name or
	// sequence leaves the activity's own pick.
	Activity	vmActivity;
	const char	*pszViewModelSequence[ ALTERNATING_SWING_SIDE_COUNT ];

	// Where the blade crosses the screen in each side's sequence, as cycles (0-1). With a sweep,
	// these frames are stretched over the sweep's active time. Start >= end: no stretching.
	float		flViewModelSweepStart[ ALTERNATING_SWING_SIDE_COUNT ];
	float		flViewModelSweepEnd[ ALTERNATING_SWING_SIDE_COUNT ];

	// Third person: gesture activity per stance and side (PLAYERANIMEVENT_CUSTOM_GESTURE).
	Activity	worldGesture[ ALTERNATING_SWING_STANCE_COUNT ][ ALTERNATING_SWING_SIDE_COUNT ];
};

struct AlternatingSwingSweep_t
{
	int		nRays;			// rays per swing, evenly spread across the arc
	float	flArcDegrees;	// full width of the arc, centered on the crosshair
	float	flRange;		// ray length from the eye, in HU
	float	flDelay;		// cast: seconds from the swing's start to the first ray
	float	flDuration;		// active: seconds the rays take (ray i at delay + i * duration / rays); <= 0: the rest of the swing
};

class CTFAlternatingSwingMelee : public CTFWeaponBaseMelee
{
public:
	DECLARE_CLASS( CTFAlternatingSwingMelee, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFAlternatingSwingMelee();

	virtual void	WeaponReset( void ) OVERRIDE;
	virtual bool	Deploy( void ) OVERRIDE;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo ) OVERRIDE;
	virtual void	PrimaryAttack( void ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;

	virtual void	DoViewModelAnimation( void ) OVERRIDE;
	virtual void	SendPlayerAnimEvent( CTFPlayer *pPlayer ) OVERRIDE;
	virtual void	Smack( void ) OVERRIDE;

#ifdef GAME_DLL
	// While a sweep hit is being applied: the direction the swing pushes it (horizontal).
	bool			GetSweepPushDirection( Vector &vecPush ) const	{ vecPush = m_vecSweepPush; return m_bSweepPushActive; }
#endif

	// The side of the swing in progress (or the last one).
	EAlternatingSwingSide GetSwingSide( void ) const	{ return m_bSwingRight ? ALTERNATING_SWING_RIGHT : ALTERNATING_SWING_LEFT; }

protected:
	// Default: the stock melee swing on both sides (a networked class can't be abstract).
	virtual const AlternatingSwingAnims_t &GetSwingAnims( void ) const;

	// Sweep hit: return true and fill the parameters to use it instead of TF2's single trace.
	virtual bool	GetSweepParams( AlternatingSwingSweep_t &params ) const	{ return false; }

	// Forget the chain and the buffer: the next swing is a left swing.
	void			ResetSwingChain( void );

	// Viewmodel playback rate for the swing's current phase (cast, active, recovery).
	void			UpdateSwingAnimRate( void );

private:
	CNetworkVar( bool, m_bSwingRight );			// side of the current / last swing
	CNetworkVar( float, m_flSwingChainTime );	// when the last swing allows the next one (0 = no chain)
	CNetworkVar( bool, m_bSwingBuffered );		// attack pressed during the swing: swing again when allowed
	CNetworkVar( float, m_flSwingAnimStart );	// when the current swing's viewmodel sequence started
	CNetworkVar( int, m_iSwingAnimPhase );		// its timing phase: 0 cast, 1 active, 2 recovery, -1 none

#ifdef GAME_DLL
	void			StartSweep( void );
	void			StopSweep( void )	{ m_bSweepActive = false; }
	void			UpdateSweep( void );
	void			CastSweepRay( CTFPlayer *pOwner, const Vector &vecStart, const Vector &vecDir, const Vector &vecPush, CUtlVector< CBaseEntity * > &candidates );

	bool						m_bSweepActive;
	bool						m_bSweepRight;		// the swing's side when the sweep started
	AlternatingSwingSweep_t		m_Sweep;
	float						m_flSweepStartTime;
	float						m_flSweepDuration;
	int							m_iSweepNextRay;
	bool						m_bSweepHasLastRay;
	Vector						m_vecSweepLastStart;
	Vector						m_vecSweepLastDir;
	CUtlVector< EHANDLE >		m_SweepHits;		// hit this swing
	bool						m_bSweepPushActive;	// a sweep hit is being applied
	Vector						m_vecSweepPush;		// its push direction
#endif

	CTFAlternatingSwingMelee( const CTFAlternatingSwingMelee & ) {}
};

#ifdef GAME_DLL
// From CTFPlayer::AdjustDamageDirection: if the damage is an alternating melee sweep hit, point
// the push direction (dir points from the victim toward the push's source) along the swing.
void AlternatingSwing_AdjustDamageDirection( const CTakeDamageInfo &info, Vector &dir );
#endif

#endif // TF_WEAPON_ALTERNATING_MELEE_H
