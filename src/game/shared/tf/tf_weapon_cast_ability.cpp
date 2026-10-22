//========= OWTF2 ============================================================//
//
// Purpose: Base for ability weapons used by switching to them. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_cast_ability.h"
#include "tf_ultimate.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Weapon tables
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFCastAbilityWeapon, DT_TFCastAbilityWeapon )

BEGIN_NETWORK_TABLE( CTFCastAbilityWeapon, DT_TFCastAbilityWeapon )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flCastEndTime ) ),
	RecvPropTime( RECVINFO( m_flReadyTime ) ),
	RecvPropTime( RECVINFO( m_flReturnTime ) ),
#else
	SendPropTime( SENDINFO( m_flCastEndTime ) ),
	SendPropTime( SENDINFO( m_flReadyTime ) ),
	SendPropTime( SENDINFO( m_flReturnTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCastAbilityWeapon )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flCastEndTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flReadyTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flReturnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
CTFCastAbilityWeapon::CTFCastAbilityWeapon()
{
	m_flCastEndTime = 0.0f;
	m_flReadyTime = 0.0f;
	m_flReturnTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: On cooldown it can't be picked from the weapon slots or the mouse wheel.
//-----------------------------------------------------------------------------
bool CTFCastAbilityWeapon::CanBeSelected( void )
{
	if ( !IsReady() || Ultimate_IsCasting( GetTFPlayerOwner() ) || !CanUseNow() )
		return false;

	return BaseClass::CanBeSelected();
}

bool CTFCastAbilityWeapon::CanDeploy( void )
{
	// Not on cooldown, and not while casting the ultimate.
	if ( !IsReady() || Ultimate_IsCasting( GetTFPlayerOwner() ) || !CanUseNow() )
		return false;

	return BaseClass::CanDeploy();
}

//-----------------------------------------------------------------------------
// Purpose: Switching to the weapon uses the ability: start the cast.
//-----------------------------------------------------------------------------
bool CTFCastAbilityWeapon::Deploy( void )
{
	if ( !BaseClass::Deploy() )
		return false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();

	m_flCastEndTime = gpGlobals->curtime + GetCastTime();

	if ( IsInstantSwitch() )
	{
		// No draw time: the player isn't busy and the cast starts now.
		m_flNextPrimaryAttack = gpGlobals->curtime;
		m_flNextSecondaryAttack = gpGlobals->curtime;
		if ( pPlayer )
		{
			pPlayer->SetNextAttack( gpGlobals->curtime );
		}
	}

	const Activity actCast = GetCastActivity();
	if ( actCast != ACT_INVALID )
	{
		SendWeaponAnim( actCast );
	}

	OnCastStarted( pPlayer );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: The cast can't be cancelled by switching weapons.
//-----------------------------------------------------------------------------
bool CTFCastAbilityWeapon::CanHolster( void ) const
{
	if ( IsCasting() || IsRecovering() )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: A forced holster (death, class change) cancels the cast: no effect
//          and the cooldown doesn't start.
//-----------------------------------------------------------------------------
bool CTFCastAbilityWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flCastEndTime = 0.0f;
	m_flReturnTime = 0.0f;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: The deploy delay can run longer than the cast (the player is "busy"
//          until it ends), so check the cast from both frames.
//-----------------------------------------------------------------------------
void CTFCastAbilityWeapon::ItemPostFrame( void )
{
	CheckCast();

	BaseClass::ItemPostFrame();
}

void CTFCastAbilityWeapon::ItemBusyFrame( void )
{
	CheckCast();

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: End of the cast: the effect, the cooldown, back to the primary.
//-----------------------------------------------------------------------------
void CTFCastAbilityWeapon::CheckCast( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( IsCasting() && gpGlobals->curtime >= m_flCastEndTime )
	{
		m_flCastEndTime = 0.0f;
		m_flReadyTime = gpGlobals->curtime + GetCooldown();

		OnCastFinished( pPlayer );

		const float flRecovery = GetRecoveryTime();
		if ( flRecovery > 0.0f )
		{
			m_flReturnTime = gpGlobals->curtime + flRecovery;
			return;
		}

		SwitchToPrimary( pPlayer );
		return;
	}

	if ( IsRecovering() && gpGlobals->curtime >= m_flReturnTime )
	{
		m_flReturnTime = 0.0f;
		SwitchToPrimary( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Every cast returns to the primary, whatever was held before it.
//-----------------------------------------------------------------------------
void CTFCastAbilityWeapon::SwitchToPrimary( CTFPlayer *pPlayer )
{
	CBaseCombatWeapon *pPrimary = NULL;
	for ( int i = 0; i < pPlayer->WeaponCount(); i++ )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon( i );
		if ( pWeapon && pWeapon->GetSlot() == TF_WPN_TYPE_PRIMARY )
		{
			pPrimary = pWeapon;
			break;
		}
	}

	if ( pPrimary )
	{
		pPlayer->Weapon_Switch( pPrimary );
	}
	else
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 0..1 cooldown progress for the item effect meter.
//-----------------------------------------------------------------------------
float CTFCastAbilityWeapon::GetEffectBarProgress( void )
{
	const float flCooldown = GetCooldown();
	if ( flCooldown <= 0.0f || IsReady() )
		return 1.0f;

	return clamp( 1.0f - ( m_flReadyTime - gpGlobals->curtime ) / flCooldown, 0.0f, 1.0f );
}
