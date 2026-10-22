//========= OWTF2 ============================================================//
//
// Purpose: Veteran's healing field, placed by the Field Rations
//			(tf_weapon_veteran_field_rations.cpp). Entity: tf_veteran_healing_field.
//
//			Sits on the ground with no collision and heals teammates inside its area
//			for tf_veteran_field_duration seconds.
//
//			PLACEHOLDERS:
//			- Model: the small health pack.
//			- Area: twice the dispenser's (a box of +-tf_veteran_field_radius, 128, around the
//			  field's center, which touches the player's bounding box; teammates need
//			  line of sight to the field, like the dispenser's MASK_DISPENSER check).
//			- Heal rate: the level 2 dispenser's (15 HP/s, constant rate, no overheal).
//			- Effects: the Quick-Fix ÜberCharge ground ring (medic_megaheal_red/blue)
//			  shows the area; dispenser heal beams (dispenser_heal_red/blue) go to
//			  each player being healed.
//
//=============================================================================//

#ifndef TF_VETERAN_HEALING_FIELD_H
#define TF_VETERAN_HEALING_FIELD_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_baseanimating.h"
#define CTFVeteranHealingField C_TFVeteranHealingField
class C_TFPlayer;
#define CTFPlayer C_TFPlayer
#else
#include "baseanimating.h"
class CTFPlayer;
#endif

#define TF_VETERAN_HEALING_FIELD_MODEL	"models/items/medkit_small.mdl"

// One bit per player slot (entity index 1..MAX_PLAYERS): who the field is healing.
#define TF_VETERAN_HEALING_FIELD_BIT_WORDS	( ( MAX_PLAYERS + 31 ) / 32 )

class CTFVeteranHealingField : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTFVeteranHealingField, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CTFVeteranHealingField();

	virtual void	Precache( void ) OVERRIDE;
	virtual void	UpdateOnRemove( void ) OVERRIDE;

	bool			IsHealingPlayer( int iPlayerIndex ) const;

#ifdef GAME_DLL
	DECLARE_DATADESC();

	// Places a field on the ground in front of the player.
	static CTFVeteranHealingField *Create( CTFPlayer *pOwner );

	virtual void	Spawn( void ) OVERRIDE;

	void			HealThink( void );

private:
	bool			CouldHealTarget( CTFPlayer *pPlayer );
	void			StartHealing( CTFPlayer *pPlayer );
	void			StopHealing( CTFPlayer *pPlayer );
	void			StopHealingAll( void );
	void			SetHealingPlayer( int iPlayerIndex, bool bHealing );

	float			m_flDieTime;
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;

private:
	void			UpdateEffects( void );

	CNewParticleEffect	*m_pAreaEffect;
	CNewParticleEffect	*m_pBeamEffects[MAX_PLAYERS + 1];	// indexed by player entity index
#endif

private:
	CNetworkArray( unsigned int, m_nHealedPlayerBits, TF_VETERAN_HEALING_FIELD_BIT_WORDS );
};

#endif // TF_VETERAN_HEALING_FIELD_H
