//========= OWTF2 ============================================================//
//
// Purpose: Crusader's hammer (Reinhardt-like). Plan: docs/crusader-plan.txt.
//			Entity: tf_weapon_crusader_hammer. Item: scripts/items/items_owtf2.txt (60005).
//
//			Swings alternate left and right (CTFAlternatingSwingMelee). Animations, per the
//			plan's "Animations" section: the Eyelander's left and right swings in first
//			person (the swords' *_SPECIAL viewmodel set, c_demo_arms), the bottle's swing in
//			third person for both sides (the swords' own swing is an overhead strike, kept
//			for Fire Strike), over the two-handed sword pose (anim_slot item1).
//
//			Hit: a sweep of rays across an arc in front of the camera, through the swing
//			(tf_crusader_hammer_rays / _arc / _range / _sweep_time; see the alternating melee).
//
//			PLACEHOLDER: the Eyelander's model and sounds, damage and swing time.
//
//=============================================================================//

#ifndef TF_WEAPON_CRUSADER_HAMMER_H
#define TF_WEAPON_CRUSADER_HAMMER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_alternating_melee.h"

#ifdef CLIENT_DLL
#define CTFCrusaderHammer C_TFCrusaderHammer
#endif

class CTFCrusaderHammer : public CTFAlternatingSwingMelee
{
public:
	DECLARE_CLASS( CTFCrusaderHammer, CTFAlternatingSwingMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFCrusaderHammer() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_CRUSADER_HAMMER; }

	// The swords' viewmodel set (cm_*), like CTFSword on the Demoman.
	virtual Activity	TranslateViewmodelHandActivityInternal( Activity actBase ) OVERRIDE;

protected:
	virtual const AlternatingSwingAnims_t &GetSwingAnims( void ) const OVERRIDE;
	virtual bool	GetSweepParams( AlternatingSwingSweep_t &params ) const OVERRIDE;

private:
	CTFCrusaderHammer( const CTFCrusaderHammer & ) {}
};

#endif // TF_WEAPON_CRUSADER_HAMMER_H
