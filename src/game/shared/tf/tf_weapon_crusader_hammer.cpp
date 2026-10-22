//========= OWTF2 ============================================================//
//
// Purpose: Crusader's hammer. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_crusader_hammer.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning: the swing's sweep (see tf_weapon_alternating_melee.h). Replicated so both sides agree.
//-----------------------------------------------------------------------------
// Initial default: the user's design (not an Overwatch value), 10 rays.
ConVar tf_crusader_hammer_rays( "tf_crusader_hammer_rays", "10", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Crusader: rays per hammer swing, fired one at a time across the arc.", true, 1.0f, true, 64.0f );
// Initial default: the user's design, 65 degrees in front of the player.
ConVar tf_crusader_hammer_arc( "tf_crusader_hammer_arc", "65", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Crusader: full width of the hammer swing's arc, in degrees, centered on the crosshair.", true, 0.0f, true, 360.0f );
// Initial default: Reinhardt's 5 m reach; 5 * 39.37 = 197 HU, rounded to 200.
ConVar tf_crusader_hammer_range( "tf_crusader_hammer_range", "200", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Crusader: hammer swing range from the eye, in HU.", true, 0.0f, false, 0.0f );
// A swing (the user's Overwatch timings): cast 0.4 s (no hitbox), active 0.21 s (the rays), recovery (the rest of
// the swing's 1.05 s, TimeFireDelay in tf_weapon_crusader_hammer.txt: 0.44 s).
// Initial default: Reinhardt's 0.4 s cast.
ConVar tf_crusader_hammer_cast_time( "tf_crusader_hammer_cast_time", "0.4", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Crusader: seconds from the start of a hammer swing to its first ray (no hitbox before).", true, 0.0f, false, 0.0f );
// Initial default: Reinhardt's 0.21 s attack (the hitbox's active time).
ConVar tf_crusader_hammer_sweep_time( "tf_crusader_hammer_sweep_time", "0.21", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Crusader: seconds the hammer's rays take to cross the arc, after the cast; 0 = the rest of the swing.", true, 0.0f, false, 0.0f );

//-----------------------------------------------------------------------------
// Weapon tables. The swing state is networked by CTFAlternatingSwingMelee.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFCrusaderHammer, DT_TFCrusaderHammer )

BEGIN_NETWORK_TABLE( CTFCrusaderHammer, DT_TFCrusaderHammer )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCrusaderHammer )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_crusader_hammer, CTFCrusaderHammer );
PRECACHE_WEAPON_REGISTER( tf_weapon_crusader_hammer );

//-----------------------------------------------------------------------------
// Purpose: Eyelander swings in first person; the bottle's swing in third person (no
//			right-hand version exists yet, so both sides use it).
//-----------------------------------------------------------------------------
const AlternatingSwingAnims_t &CTFCrusaderHammer::GetSwingAnims( void ) const
{
	static const AlternatingSwingAnims_t s_Anims =
	{
		ACT_VM_HITCENTER,	// translated to ACT_VM_HITCENTER_SPECIAL below
		// Left, right (the direction the blade moves). Checked in slow motion, frame by frame:
		// cm_swing_a winds up to the right, then the blade crosses from right to left at about
		// 0.19-0.33 s of its 1.0 s; cm_swing_b raises the arm to the upper left, then the blade
		// crosses from left to right at about 0.23-0.37 s.
		{ "cm_swing_a", "cm_swing_b" },
		{ 0.19f, 0.23f },	// crossing start (cycle), left, right
		{ 0.33f, 0.37f },	// crossing end (cycle)
		{
			{ ACT_MP_ATTACK_STAND_MELEE,	ACT_MP_ATTACK_STAND_MELEE },
			{ ACT_MP_ATTACK_CROUCH_MELEE,	ACT_MP_ATTACK_CROUCH_MELEE },
			{ ACT_MP_ATTACK_SWIM_MELEE,		ACT_MP_ATTACK_SWIM_MELEE },
		},
	};
	return s_Anims;
}

//-----------------------------------------------------------------------------
// Purpose: The swing hits with a sweep of rays instead of TF2's single trace.
//-----------------------------------------------------------------------------
bool CTFCrusaderHammer::GetSweepParams( AlternatingSwingSweep_t &params ) const
{
	params.nRays = tf_crusader_hammer_rays.GetInt();
	params.flArcDegrees = tf_crusader_hammer_arc.GetFloat();
	params.flRange = tf_crusader_hammer_range.GetFloat();
	params.flDelay = tf_crusader_hammer_cast_time.GetFloat();
	params.flDuration = tf_crusader_hammer_sweep_time.GetFloat();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Same mapping as CTFSword uses on the Demoman (tf_weapon_sword.cpp).
//-----------------------------------------------------------------------------
Activity CTFCrusaderHammer::TranslateViewmodelHandActivityInternal( Activity actBase )
{
	switch ( actBase )
	{
	case ACT_VM_DRAW:				actBase = ACT_VM_DRAW_SPECIAL; break;
	case ACT_VM_HOLSTER:			actBase = ACT_VM_HOLSTER_SPECIAL; break;
	case ACT_VM_IDLE:				actBase = ACT_VM_IDLE_SPECIAL; break;
	case ACT_VM_PULLBACK:			actBase = ACT_VM_PULLBACK_SPECIAL; break;
	case ACT_VM_PRIMARYATTACK:		actBase = ACT_VM_PRIMARYATTACK_SPECIAL; break;
	case ACT_VM_SECONDARYATTACK:	actBase = ACT_VM_PRIMARYATTACK_SPECIAL; break;
	case ACT_VM_HITCENTER:			actBase = ACT_VM_HITCENTER_SPECIAL; break;
	case ACT_VM_SWINGHARD:			actBase = ACT_VM_SWINGHARD_SPECIAL; break;
	case ACT_VM_IDLE_TO_LOWERED:	actBase = ACT_VM_IDLE_TO_LOWERED_SPECIAL; break;
	case ACT_VM_IDLE_LOWERED:		actBase = ACT_VM_IDLE_LOWERED_SPECIAL; break;
	case ACT_VM_LOWERED_TO_IDLE:	actBase = ACT_VM_LOWERED_TO_IDLE_SPECIAL; break;
	default:						break;
	}

	return BaseClass::TranslateViewmodelHandActivityInternal( actBase );
}
