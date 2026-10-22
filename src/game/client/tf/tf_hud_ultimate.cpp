//========= OWTF2 ============================================================//
//
// Purpose: Ultimate charge meter, for any class with an ultimate (tf_ultimate.h).
//			A copy of the Medic's Ubercharge meter (tf_hud_mediccharge.cpp).
//			Layout: resource/UI/HudUltimate.res.
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "tf_ultimate.h"
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/AnimationController.h>
#include "tf_imagepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CHudUltimateMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudUltimateMeter, EditablePanel );

public:
	CHudUltimateMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	ContinuousProgressBar	*m_pChargeMeter;

	bool	m_bCharged;
	float	m_flLastChargeValue;
};

DECLARE_HUDELEMENT( CHudUltimateMeter );

//-----------------------------------------------------------------------------
CHudUltimateMeter::CHudUltimateMeter( const char *pElementName )
	: CHudElement( pElementName ), BaseClass( NULL, "HudUltimate" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bCharged = false;
	m_flLastChargeValue = -1;

	SetDialogVariable( "charge", 0 );

	RegisterForRenderGroup( "inspect_panel" );
}

//-----------------------------------------------------------------------------
void CHudUltimateMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// The .res also positions this panel itself (its "HudUltimate" block),
	// so TF2's hudlayout.res doesn't need overriding.
	LoadControlSettings( "resource/UI/HudUltimate.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	// Re-evaluate the charged animation after a scheme reload.
	m_flLastChargeValue = -1;
	m_bCharged = false;
}

//-----------------------------------------------------------------------------
bool CHudUltimateMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsAlive() || !Ultimate_HasUltimate( pPlayer ) )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
void CHudUltimateMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !Ultimate_HasUltimate( pPlayer ) )
		return;

	const float flCharge = Ultimate_GetChargeFraction( pPlayer );
	const bool bActive = Ultimate_IsActive( pPlayer );
	if ( flCharge == m_flLastChargeValue )
		return;

	if ( m_pChargeMeter )
	{
		m_pChargeMeter->SetProgress( flCharge );
	}
	SetDialogVariable( "charge", (int)( flCharge * 100 ) );

	// Like the Ubercharge meter: pulse once full, and keep pulsing while the
	// ultimate drains. TF2's events animate the children named ChargeLabel and
	// ChargeMeter, which HudUltimate.res keeps.
	if ( !m_bCharged && flCharge >= 1.0f )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicCharged" );
		m_bCharged = true;
	}
	else if ( m_bCharged && flCharge < 1.0f && !bActive )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudMedicChargedStop" );
		m_bCharged = false;
	}

	m_flLastChargeValue = flCharge;
}
