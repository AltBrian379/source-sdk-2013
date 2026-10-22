//========= OWTF2 ============================================================//
//
// Purpose: Combat Medic's primary, the Biotic Rifle. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_combatmed_rifle.h"
#include "tf_projectile_biotic_round.h"
#include "tf_teammate_trace.h"
#include "in_buttons.h"
#include "tf_combatmed_abilities.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "prediction.h"
#include "c_baseviewmodel.h"
#else
#include "tf_player.h"
#include "ilagcompensationmanager.h"
#include "tf_damage_over_time.h"
#include "tf_fx.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so the client sees the server's values. Initial defaults are
// Ana's values scaled to TF2 (CLAUDE.md, "Stat scaling for ported heroes").
//-----------------------------------------------------------------------------
// Initial default: Ana's 75 x 0.7 (175 / 250 HP) = 52.5, rounded down to keep whole hits (4 shots kill 175, as 4 kill 250).
ConVar tf_combatmed_rifle_damage( "tf_combatmed_rifle_damage", "52", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Rifle damage per hit, dealt over tf_combatmed_rifle_effect_time.", true, 0.0f, false, 0.0f );
// Initial default: Ana's 75 x 0.7 = 52.5, rounded down like the damage.
ConVar tf_combatmed_rifle_heal( "tf_combatmed_rifle_heal", "52", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Rifle healing per hit on a hurt teammate, over tf_combatmed_rifle_effect_time.", true, 0.0f, false, 0.0f );
// Initial default: Ana's damage and healing land over 0.59 s.
ConVar tf_combatmed_rifle_effect_time( "tf_combatmed_rifle_effect_time", "0.59", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: seconds a Biotic Rifle hit's damage or healing is spread over.", true, 0.0f, false, 0.0f );
// Initial default: TF2's sniper rifle zoom (TF_WEAPON_ZOOM_FOV).
ConVar tf_combatmed_rifle_zoom_fov( "tf_combatmed_rifle_zoom_fov", "20", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Combat Medic: Biotic Rifle field of view while scoped.", true, 1.0f, true, 90.0f );

#define COMBATMED_RIFLE_SCOPED_RANGE	8192.0f
// Scoped shot tracer. PLACEHOLDER: the Hitman's Heatmaker's Focus beam.
#define COMBATMED_RIFLE_TRACER_RED		"dxhr_sniper_rail_red"
#define COMBATMED_RIFLE_TRACER_BLUE		"dxhr_sniper_rail_blue"

//-----------------------------------------------------------------------------
// Weapon tables
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFCombatMedRifle, DT_TFCombatMedRifle )

BEGIN_NETWORK_TABLE( CTFCombatMedRifle, DT_TFCombatMedRifle )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bScopeNeedsRelease ) ),
	RecvPropBool( RECVINFO( m_bScopeStoppedReload ) ),
#else
	SendPropBool( SENDINFO( m_bScopeNeedsRelease ) ),
	SendPropBool( SENDINFO( m_bScopeStoppedReload ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCombatMedRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bScopeNeedsRelease, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bScopeStoppedReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_combatmed_rifle, CTFCombatMedRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_combatmed_rifle );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFCombatMedRifle )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
CTFCombatMedRifle::CTFCombatMedRifle()
{
	m_bScopeNeedsRelease = false;
	m_bScopeStoppedReload = false;
}

void CTFCombatMedRifle::Precache( void )
{
	BaseClass::Precache();

#ifdef GAME_DLL
	UTIL_PrecacheOther( "tf_projectile_biotic_round" );
#endif
	PrecacheSound( COMBATMED_ULTIMATE_VOICE_LINE );	// Nano Boost's voice line
	PrecacheParticleSystem( COMBATMED_RIFLE_TRACER_RED );
	PrecacheParticleSystem( COMBATMED_RIFLE_TRACER_BLUE );
}

//-----------------------------------------------------------------------------
bool CTFCombatMedRifle::IsScoped( void ) const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	return pOwner && pOwner->m_Shared.InCond( TF_COND_ZOOMED );
}

//-----------------------------------------------------------------------------
// Purpose: Back from an ability (or any switch): the scope doesn't come back by
//          itself, MOUSE2 has to be pressed again.
//-----------------------------------------------------------------------------
bool CTFCombatMedRifle::Deploy( void )
{
	m_bScopeNeedsRelease = true;

	return BaseClass::Deploy();
}

bool CTFCombatMedRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	ZoomOut();
	m_bScopeNeedsRelease = true;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Hold MOUSE2 to scope. Pressing it during a reload interrupts the reload.
//-----------------------------------------------------------------------------
void CTFCombatMedRifle::UpdateScope( CTFPlayer *pPlayer )
{
	const bool bHeld = ( pPlayer->m_nButtons & IN_ATTACK2 ) != 0;
	if ( !bHeld )
	{
		m_bScopeNeedsRelease = false;
	}

	if ( bHeld && !m_bScopeNeedsRelease && m_bInReload )
	{
		// Scoping wins over the reload (like the Veteran's helix rockets).
		AbortReload();
		m_bInReload = false;
		m_flNextPrimaryAttack = Max( (float)gpGlobals->curtime, (float)m_flReloadPriorNextFire );
		pPlayer->m_flNextAttack = gpGlobals->curtime;
		m_bScopeStoppedReload = true;
	}

	if ( IsScoped() )
	{
		if ( !bHeld || m_bInReload )
		{
			ZoomOut();
		}
	}
	else if ( bHeld && !m_bScopeNeedsRelease && !m_bInReload )
	{
		ZoomIn();
	}
}

void CTFCombatMedRifle::ItemPostFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		UpdateScope( pPlayer );

		// An empty clip reloads even while MOUSE2 is held: the base class only reloads by
		// itself with no fire button down, and MOUSE2 is the scope. After scoping stopped
		// a reload, it waits until the player unscopes.
		if ( m_iClip1 == 0 && !m_bInReload && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 &&
			 m_flNextPrimaryAttack <= gpGlobals->curtime && !( m_bScopeStoppedReload && IsScoped() ) )
		{
			Reload();
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: A reload only gets busy frames: scoping is checked here too.
//-----------------------------------------------------------------------------
void CTFCombatMedRifle::ItemBusyFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		UpdateScope( pPlayer );
	}

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: CTFWeaponBaseGun::ZoomIn with our own field of view.
//-----------------------------------------------------------------------------
void CTFCombatMedRifle::ZoomIn( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	pPlayer->SetFOV( pPlayer, tf_combatmed_rifle_zoom_fov.GetFloat(), 0.1f );
	pPlayer->m_Shared.AddCond( TF_COND_ZOOMED );

#ifdef CLIENT_DLL
	// Hides the viewmodel while scoped.
	pPlayer->UpdateVisibility();
#endif

	// The scoped speed (CombatMed_GetMaxSpeedMultiplier) applies now, not at the next speed update.
	pPlayer->TeamFortress_SetSpeed();
}

void CTFCombatMedRifle::ZoomOut( void )
{
	BaseClass::ZoomOut();
	m_bScopeStoppedReload = false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called by CTFWeaponBaseGun::PrimaryAttack once a shot goes off.
//          Replaces CTFWeaponBaseGun::FireProjectile, so it also does the base's
//          per-shot bookkeeping after the shot (ammo, fire effects, punch).
//-----------------------------------------------------------------------------
CBaseEntity *CTFCombatMedRifle::FireProjectile( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	CTFProjectile_BioticRound *pRound = NULL;
	if ( IsScoped() )
	{
		FireScopedShot( pPlayer );
	}
	else
	{
		// Aimed like the Syringe Gun (same offset), without its spread.
		Vector vecSrc;
		QAngle angForward;
		GetProjectileFireSetup( pPlayer, Vector( 16, 6, -8 ), &vecSrc, &angForward, false );

		pRound = CTFProjectile_BioticRound::Create( vecSrc, angForward, pPlayer );
#ifdef GAME_DLL
		if ( pRound )
		{
			pRound->SetLauncher( this );
			pRound->SetWeaponID( GetWeaponID() );
		}
#endif
	}

	RemoveProjectileAmmo( pPlayer );
	m_flLastFireTime = gpGlobals->curtime;
	m_iConsecutiveShots++;
	DoFireEffects();
	UpdatePunchAngles( pPlayer );

	return pRound;
}

//-----------------------------------------------------------------------------
// Purpose: Scoped: a hitscan shot from the eye, lag compensated (teammates too).
//-----------------------------------------------------------------------------
void CTFCombatMedRifle::FireScopedShot( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	const Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward );
	const Vector vecEnd = vecSrc + vecForward * COMBATMED_RIFLE_SCOPED_RANGE;

#ifdef GAME_DLL
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	trace_t tr;
	CTraceFilterSkipFullHealthTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID | CONTENTS_HITBOX, &filter, &tr );

#ifdef GAME_DLL
	if ( tr.m_pEnt && !tr.m_pEnt->IsWorld() )
	{
		ApplyBioticHit( pPlayer, tr.m_pEnt, this );
	}

	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	// Effects: the shooter predicts them, the server sends them to everyone else.
	if ( tr.m_pEnt && tr.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() && !( tr.surface.flags & SURF_SKY ) )
	{
		UTIL_ImpactTrace( &tr, DMG_BULLET );
	}

	// Tracer: the Hitman's Heatmaker's (its Focus railgun beam), team colored, from the same
	// start point TF2 uses for it (CTFPlayer::GetHorriblyHackedRailgunPosition, which is private).
	Vector vecBeamForward, vecBeamRight, vecBeamUp;
	AngleVectors( pPlayer->EyeAngles(), &vecBeamForward, &vecBeamRight, &vecBeamUp );
#ifdef CLIENT_DLL
	if ( TeamFortress_ShouldFlipClientViewModel() )
	{
		vecBeamRight *= -1;
	}
#endif
	const Vector vecBeamStart = vecSrc + vecBeamForward * 60.9f + vecBeamRight * 13.1f - vecBeamUp * 15.1f;
	const char *pszBeam = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) ? COMBATMED_RIFLE_TRACER_BLUE : COMBATMED_RIFLE_TRACER_RED;
#ifdef GAME_DLL
	CBroadcastNonOwnerRecipientFilter beamFilter( pPlayer );
	te_tf_particle_effects_control_point_t controlPoint = { PATTACH_WORLDORIGIN, tr.endpos };
	TE_TFParticleEffectComplex( beamFilter, 0.0f, pszBeam, vecBeamStart, vec3_angle, NULL, &controlPoint, this, PATTACH_CUSTOMORIGIN );
#else
	if ( prediction->IsFirstTimePredicted() )
	{
		CSmartPtr< CNewParticleEffect > pEffect = ParticleProp()->Create( pszBeam, PATTACH_CUSTOMORIGIN, 0 );
		if ( pEffect.IsValid() && pEffect->IsValid() )
		{
			pEffect->SetSortOrigin( vecBeamStart );
			pEffect->SetControlPoint( 0, vecBeamStart );
			pEffect->SetControlPoint( 1, tr.endpos );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Fixed reload time from the weapon script ("TimeReload") instead of the
//          reload animation's length, like the Veteran's rifle; the animation is
//          stretched to fit. Reloading unscopes.
//-----------------------------------------------------------------------------
bool CTFCombatMedRifle::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	const bool bWasReloading = m_bInReload;

	if ( !BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity ) )
		return false;

	if ( bWasReloading || !m_bInReload )
		return true;

	ZoomOut();
	m_bScopeNeedsRelease = true;

	const float flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload;
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( flReloadTime > 0.0f && pPlayer )
	{
		// The base class just started the reload animation and timed the reload to end
		// 0.2 s before it does.
		const float flAnimReloadTime = SequenceDuration() - 0.2f;

		// SetReloadTimer would overwrite the saved next fire time (the point from which
		// firing may cancel the reload); keep the base class's.
		const float flPriorNextFire = m_flReloadPriorNextFire;
		SetReloadTimer( flReloadTime );
		m_flReloadPriorNextFire = flPriorNextFire;

		m_flNextPrimaryAttack = Max( (float)pPlayer->m_flNextAttack, flPriorNextFire );

		if ( flAnimReloadTime > 0.0f )
		{
			const float flStretch = flAnimReloadTime / flReloadTime;
			for ( int i = 0; i < MAX_VIEWMODELS; i++ )
			{
				CBaseViewModel *pViewModel = pPlayer->GetViewModel( i );
				if ( pViewModel )
				{
					pViewModel->SetPlaybackRate( pViewModel->GetPlaybackRate() * flStretch );
				}
			}
		}
	}

	return true;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Enemy player or building: damage over time. Hurt teammate: healing over
//          time. Teammates at full health never get here (the traces skip them).
//-----------------------------------------------------------------------------
void CTFCombatMedRifle::ApplyBioticHit( CTFPlayer *pOwner, CBaseEntity *pTarget, CBaseEntity *pWeapon )
{
	if ( !pOwner || !pTarget )
		return;

	CBaseEntity *pInflictor = pWeapon ? pWeapon : pOwner;
	const float flEffectTime = tf_combatmed_rifle_effect_time.GetFloat();

	CTFPlayer *pTFTarget = ToTFPlayer( pTarget );
	if ( pTFTarget )
	{
		if ( !pTFTarget->IsAlive() || pTFTarget == pOwner )
			return;

		if ( pTFTarget->GetTeamNumber() == pOwner->GetTeamNumber() )
		{
			if ( OWTF2_IsHurtTeammate( pTFTarget, pOwner->GetTeamNumber() ) )
			{
				HealOverTime_Apply( pTFTarget, pOwner, tf_combatmed_rifle_heal.GetFloat(), flEffectTime );
			}
			return;
		}

		// DMG_BULLET without DMG_USEDISTANCEMOD: flat, no falloff or close-range bonus.
		DamageOverTime_Apply( pTFTarget, pOwner, pInflictor, pWeapon, tf_combatmed_rifle_damage.GetFloat(), flEffectTime, DMG_BULLET, TF_DMG_CUSTOM_NONE );
		return;
	}

	// Enemy buildings (and anything else that takes damage): the same damage over time.
	if ( pTarget->GetTeamNumber() != pOwner->GetTeamNumber() && pTarget->m_takedamage != DAMAGE_NO )
	{
		DamageOverTime_Apply( pTarget, pOwner, pInflictor, pWeapon, tf_combatmed_rifle_damage.GetFloat(), flEffectTime, DMG_BULLET, TF_DMG_CUSTOM_NONE );
	}
}
#endif
