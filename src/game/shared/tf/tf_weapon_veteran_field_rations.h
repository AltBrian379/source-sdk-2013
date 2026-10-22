//========= OWTF2 ============================================================//
//
// Purpose: Veteran's secondary, the Field Rations (healing field ability).
//			Entity: tf_weapon_veteran_field_rations. Item: scripts/items/items_owtf2.txt.
//
//			A cast ability weapon (tf_weapon_cast_ability.h): switching to it casts
//			(tf_veteran_field_cast_time), then places the healing field
//			(tf_veteran_healing_field.cpp), starts the cooldown and switches back to
//			the primary.
//
//			PLACEHOLDER: Jarate model and animations.
//
//=============================================================================//

#ifndef TF_WEAPON_VETERAN_FIELD_RATIONS_H
#define TF_WEAPON_VETERAN_FIELD_RATIONS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_cast_ability.h"

#ifdef CLIENT_DLL
#define CTFVeteranFieldRations C_TFVeteranFieldRations
#endif

class CTFVeteranFieldRations : public CTFCastAbilityWeapon
{
public:
	DECLARE_CLASS( CTFVeteranFieldRations, CTFCastAbilityWeapon );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFVeteranFieldRations() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_VETERAN_FIELD_RATIONS; }
	virtual void	Precache( void ) OVERRIDE;

	virtual const char *GetEffectLabelText( void )		{ return "#OWTF2_FieldRations"; }

protected:
	virtual float	GetCastTime( void ) const OVERRIDE;
	virtual float	GetCooldown( void ) const OVERRIDE;
	virtual void	OnCastStarted( CTFPlayer *pPlayer ) OVERRIDE;
	virtual void	OnCastFinished( CTFPlayer *pPlayer ) OVERRIDE;

private:
	CTFVeteranFieldRations( const CTFVeteranFieldRations & ) {}
};

#endif // TF_WEAPON_VETERAN_FIELD_RATIONS_H
