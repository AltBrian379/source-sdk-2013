//========= OWTF2 ============================================================//
//
// Purpose: Veteran's primary weapon: hitscan auto rifle with recoil climb.
//			Alt-fire: helix rockets.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_veteran_rifle.h"
#include "in_buttons.h"
#include "tf_veteran_abilities.h"
#include "tf_ultimate.h"
#include "tf_fx_shared.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "prediction.h"
#include "usercmd.h"
#else
#include "tf_player.h"
#include "tf_projectile_helix_rocket.h"
#include "ilagcompensationmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning. Replicated so client prediction and the HUD use the server's values.
// PLACEHOLDER cooldown from the source hero.
//-----------------------------------------------------------------------------
ConVar tf_veteran_helix_cooldown( "tf_veteran_helix_cooldown", "6", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rockets cooldown in seconds.", true, 0.0f, false, 0.0f );
ConVar tf_veteran_helix_cast_time( "tf_veteran_helix_cast_time", "0.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rockets cast time in seconds: after a launch the rifle can't fire or reload for this long.", true, 0.0f, false, 0.0f );
ConVar tf_veteran_helix_damage( "tf_veteran_helix_damage", "85", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rockets base damage (before distance and blast falloff)." );
ConVar tf_veteran_helix_sound_volume( "tf_veteran_helix_sound_volume", "0.75", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: helix rockets launch sound volume, as a fraction of the sound's script volume.", true, 0.0f, true, 1.0f );

ConVar tf_veteran_rifle_recoil_climb( "tf_veteran_rifle_recoil_climb", "0.392", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle recoil, degrees the view climbs per shot (from tf_veteran_rifle_recoil_start_shot on).", true, 0.0f, false, 0.0f );
ConVar tf_veteran_rifle_recoil_start_shot( "tf_veteran_rifle_recoil_start_shot", "6", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle recoil, the first shot of a burst that climbs (6 = shots 1-5 don't).", true, 1.0f, false, 0.0f );
ConVar tf_veteran_rifle_recoil_recovery_rate( "tf_veteran_rifle_recoil_recovery_rate", "15", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle recoil, degrees per second the view recovers after firing stops.", true, 0.0f, false, 0.0f );
ConVar tf_veteran_rifle_recoil_recovery_delay( "tf_veteran_rifle_recoil_recovery_delay", "0.15", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle recoil, seconds after the last shot before recovery starts. Keep it above the time between shots (1/9 s), or recovery runs mid-burst.", true, 0.0f, false, 0.0f );
ConVar tf_veteran_rifle_falloff_start( "tf_veteran_rifle_falloff_start", "512", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle damage falloff starts at this distance (full damage here; TF2's close-range bonus below it).", true, 1.0f, false, 0.0f );
ConVar tf_veteran_rifle_falloff_end( "tf_veteran_rifle_falloff_end", "640", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle damage falloff reaches its minimum (TF2's -50%) at this distance.", true, 1.0f, false, 0.0f );

ConVar tf_veteran_ultimate_reload_time( "tf_veteran_ultimate_reload_time", "0.5", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: rifle reload time in seconds while the ultimate is on.", true, 0.01f, false, 0.0f );
ConVar tf_veteran_ultimate_aim_cone( "tf_veteran_ultimate_aim_cone", "45", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: ultimate auto-aim cone, full angle in degrees around the crosshair (45 = 22.5 each side).", true, 0.0f, true, 180.0f );
ConVar tf_veteran_ultimate_aim_range( "tf_veteran_ultimate_aim_range", "2048", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Veteran: ultimate auto-aim range.", true, 0.0f, false, 0.0f );

#ifdef CLIENT_DLL
static void VeteranRifleRecoil_OnShot( bool bClimb );
#endif

//-----------------------------------------------------------------------------
// Weapon tables
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFVeteranRifle, DT_TFVeteranRifle )

BEGIN_NETWORK_TABLE( CTFVeteranRifle, DT_TFVeteranRifle )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flHelixReadyTime ) ),
	RecvPropInt( RECVINFO( m_iBurstShots ) ),
	RecvPropTime( RECVINFO( m_flHelixCastEndTime ) ),
#else
	SendPropTime( SENDINFO( m_flHelixReadyTime ) ),
	SendPropInt( SENDINFO( m_iBurstShots ), 8, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flHelixCastEndTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFVeteranRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flHelixReadyTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iBurstShots, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flHelixCastEndTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_veteran_rifle, CTFVeteranRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_veteran_rifle );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFVeteranRifle )
END_DATADESC()
#endif

//-----------------------------------------------------------------------------
CTFVeteranRifle::CTFVeteranRifle()
{
	m_flHelixReadyTime = 0.0f;
	m_iBurstShots = 0;
	m_flHelixCastEndTime = 0.0f;
}

//-----------------------------------------------------------------------------
void CTFVeteranRifle::Precache( void )
{
	BaseClass::Precache();
	PrecacheScriptSound( "Building_Sentrygun.FireRocket" );
	PrecacheSound( VETERAN_ULTIMATE_VOICE_LINE );
}

//-----------------------------------------------------------------------------
// Purpose: A burst starts over whenever the weapon comes out or goes away.
//-----------------------------------------------------------------------------
bool CTFVeteranRifle::Deploy( void )
{
	m_iBurstShots = 0;
	return BaseClass::Deploy();
}

bool CTFVeteranRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_iBurstShots = 0;
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: A burst lasts while the trigger is held.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::ItemPostFrame( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// Casting the ultimate: no firing, alt-fire or reloading. A reload in progress
	// is dropped (the cast refills the clip anyway).
	if ( pPlayer && Ultimate_IsCasting( pPlayer ) )
	{
		if ( m_bInReload )
		{
			AbortReload();
			m_bInReload = false;
		}
		m_iBurstShots = 0;
		return;
	}

	// Helix cast: no firing or reloading (a reload in progress was cancelled by the launch).
	if ( IsHelixCasting() )
	{
		m_iBurstShots = 0;
		return;
	}

	BaseClass::ItemPostFrame();

	if ( pPlayer && !( pPlayer->m_nButtons & IN_ATTACK ) )
	{
		m_iBurstShots = 0;
	}
}

void CTFVeteranRifle::ItemBusyFrame( void )
{
	// The busy frame is where firing cancels a reload; not during the ultimate's cast.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && Ultimate_IsCasting( pPlayer ) )
		return;

	// While reloading the weapon only gets busy frames. Helix rockets interrupt the
	// reload: drop it and launch (the cast then blocks firing and reloading).
	if ( pPlayer && m_bInReload && ( pPlayer->m_nButtons & IN_ATTACK2 ) && CanFireHelix( pPlayer ) )
	{
		AbortReload();
		m_bInReload = false;
		pPlayer->m_flNextAttack = gpGlobals->curtime;
		m_flNextPrimaryAttack = Max( (float)gpGlobals->curtime, (float)m_flReloadPriorNextFire );
		SecondaryAttack();
		return;
	}

	BaseClass::ItemBusyFrame();
}

bool CTFVeteranRifle::CanHolster( void ) const
{
	// No switching weapons during the ultimate's cast.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && Ultimate_IsCasting( pPlayer ) )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Exact fire rate. CTFWeaponBaseGun schedules the next shot at
//          "now + fire delay", and "now" is always a whole server tick, so each
//          shot loses up to a tick (1/9 s is 7.4 ticks, rounded up to 8: 8.3
//          shots/s). While the trigger stays held, schedule from the previous
//          shot's due time instead, so the rate averages exactly 1 / TimeFireDelay.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::PrimaryAttack( void )
{
	// The base ItemPostFrame can call this in the same frame as a helix launch.
	if ( IsHelixCasting() )
		return;

	const float flPrevNextAttack = m_flNextPrimaryAttack;
	const int iPrevBurstShots = m_iBurstShots;

	BaseClass::PrimaryAttack();

	const bool bFired = ( m_iBurstShots != iPrevBurstShots );
	const bool bOnTime = ( gpGlobals->curtime - flPrevNextAttack ) < TICK_INTERVAL;
	if ( bFired && iPrevBurstShots > 0 && bOnTime )
	{
		const float flFireDelay = m_flNextPrimaryAttack - gpGlobals->curtime;
		m_flNextPrimaryAttack = flPrevNextAttack + flFireDelay;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Primary fire. Called only when a shot actually goes off; firing ends
//          Sprint and counts toward recoil.
//-----------------------------------------------------------------------------
CBaseEntity *CTFVeteranRifle::FireProjectile( CTFPlayer *pPlayer )
{
	Veteran_StopSprint( pPlayer );

	m_iBurstShots = Min( m_iBurstShots + 1, 255 );

#ifdef CLIENT_DLL
	// The view belongs to the local player's client: kick it once per real shot,
	// not again when prediction replays the command.
	if ( pPlayer && pPlayer == C_TFPlayer::GetLocalTFPlayer() && prediction->IsFirstTimePredicted() )
	{
		VeteranRifleRecoil_OnShot( m_iBurstShots >= tf_veteran_rifle_recoil_start_shot.GetInt() );
	}
#endif

	return BaseClass::FireProjectile( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Fixed reload time from the weapon script ("TimeReload") instead of
//          the reload animation's length. SetReloadTimer applies reload-speed
//          attributes on top; the animation is stretched to fit.
//-----------------------------------------------------------------------------
bool CTFVeteranRifle::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	const bool bWasReloading = m_bInReload;

	if ( !BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity ) )
		return false;

	if ( bWasReloading || !m_bInReload )
		return true;

	m_iBurstShots = 0;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	const float flReloadTime = ( pOwner && Ultimate_IsEffectActive( pOwner ) ) ?
		tf_veteran_ultimate_reload_time.GetFloat() :
		GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload;
	if ( flReloadTime > 0.0f )
	{
		// The base class just started the reload animation and timed the reload
		// to end 0.2 s before it does.
		const float flAnimReloadTime = SequenceDuration() - 0.2f;

		// SetReloadTimer saves the next fire time as the point from which firing may
		// cancel the reload (CTFWeaponBase::ItemBusyFrame). The base class already
		// saved the real one; calling it again would save the first reload's end
		// instead and make the reload uncancellable, so put it back.
		const float flPriorNextFire = m_flReloadPriorNextFire;

		SetReloadTimer( flReloadTime );

		m_flReloadPriorNextFire = flPriorNextFire;

		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			m_flNextPrimaryAttack = Max( (float)pPlayer->m_flNextAttack, flPriorNextFire );
		}
		if ( pPlayer && flAnimReloadTime > 0.0f )
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

//-----------------------------------------------------------------------------
// Purpose: The ultimate's cast ends with a full clip. Free: the reserve isn't used.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::Ultimate_RefillClip( void )
{
	if ( m_bInReload )
	{
		AbortReload();
		m_bInReload = false;
	}
	m_iClip1 = GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: Primary fire. During the ultimate's effect, bullets are aimed at the
//          nearest enemy player the auto-aim can see (FindAutoAimTarget) instead of
//          along the crosshair. The aimed angles go to FX_FireBullets, so the damage
//          trace, the shooter's predicted tracers and the tracers other players see
//          all follow them.
//
//          On the server the target is picked with the other players moved back to
//          where the shooter saw them (lag compensation), then FX_FireBullets rewinds
//          them again, to the same spots, for the actual shot. Lag compensation can't
//          be nested, so the two happen one after the other.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::FireBullet( CTFPlayer *pPlayer )
{
	if ( !Ultimate_IsEffectActive( pPlayer ) )
	{
		BaseClass::FireBullet( pPlayer );
		return;
	}

	const Vector vecSrc = pPlayer->Weapon_ShootPosition();
	QAngle angShoot = pPlayer->EyeAngles() + pPlayer->GetPunchAngle();

	Vector vecForward;
	AngleVectors( angShoot, &vecForward );

#ifdef GAME_DLL
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	Vector vecAimPoint;
	if ( FindAutoAimTarget( pPlayer, vecSrc, vecForward, &vecAimPoint ) )
	{
		VectorAngles( vecAimPoint - vecSrc, angShoot );
	}

#ifdef GAME_DLL
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	PlayWeaponShootSound();

	FX_FireBullets(
		this,
		pPlayer->entindex(),
		vecSrc,
		angShoot,
		GetWeaponID(),
		m_iWeaponMode,
		CBaseEntity::GetPredictionRandomSeed( UseServerRandomSeed() ) & 255,
		GetWeaponSpread(),
		GetProjectileDamage(),
		IsCurrentAttackACrit() );
}

//-----------------------------------------------------------------------------
// Purpose: The ultimate's target: the nearest living enemy player that's within
//          tf_veteran_ultimate_aim_range, inside the tf_veteran_ultimate_aim_cone
//          around the crosshair, and in line of sight. Players only, never buildings.
//          Skips the same spies a sentry gun does (CObjectSentrygun::ValidTargetPlayer):
//          mostly invisible, disguised as our team, or disguised as a dispenser.
//          Line of sight uses the sentry's mask (grates block it). Each player is tried
//          at the body's center, then the head, then the legs; the first visible point
//          inside the cone is where the bullet goes.
//-----------------------------------------------------------------------------
CTFPlayer *CTFVeteranRifle::FindAutoAimTarget( CTFPlayer *pOwner, const Vector &vecSrc, const Vector &vecForward, Vector *pvecAimPoint )
{
	if ( !pOwner )
		return NULL;

	const float flRange = tf_veteran_ultimate_aim_range.GetFloat();
	const float flCosHalfCone = cos( DEG2RAD( tf_veteran_ultimate_aim_cone.GetFloat() * 0.5f ) );
	const int iTeam = pOwner->GetTeamNumber();

	CTFPlayer *pBest = NULL;
	float flBestDistSqr = flRange * flRange;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTarget = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTarget || pTarget == pOwner || !pTarget->IsAlive() )
			continue;

#ifdef CLIENT_DLL
		if ( pTarget->IsDormant() )
			continue;
#endif

		if ( pTarget->GetTeamNumber() < FIRST_GAME_TEAM || pTarget->GetTeamNumber() == iTeam )
			continue;

		// The sentry gun's spy rules.
		if ( pTarget->m_Shared.GetPercentInvisible() > 0.5f )
			continue;
		if ( pTarget->m_Shared.InCond( TF_COND_DISGUISED ) && pTarget->m_Shared.GetDisguiseTeam() == iTeam )
			continue;
		if ( pTarget->m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) )
			continue;

		const float flDistSqr = ( pTarget->WorldSpaceCenter() - vecSrc ).LengthSqr();
		if ( flDistSqr > flBestDistSqr )
			continue;

		const Vector vecPoints[3] =
		{
			pTarget->WorldSpaceCenter(),
			pTarget->EyePosition(),
			pTarget->GetAbsOrigin() + Vector( 0.0f, 0.0f, 16.0f ),
		};

		for ( int iPoint = 0; iPoint < ARRAYSIZE( vecPoints ); iPoint++ )
		{
			Vector vecToPoint = vecPoints[iPoint] - vecSrc;
			const float flDist = VectorNormalize( vecToPoint );
			if ( flDist <= 0.0f || flDist > flRange || DotProduct( vecToPoint, vecForward ) < flCosHalfCone )
				continue;

			trace_t tr;
			CTraceFilterSimple filter( pOwner, COLLISION_GROUP_NONE );
			UTIL_TraceLine( vecSrc, vecPoints[iPoint], MASK_SHOT | CONTENTS_GRATE, &filter, &tr );
			if ( tr.fraction < 1.0f && tr.m_pEnt != pTarget )
				continue;

			pBest = pTarget;
			flBestDistSqr = flDistSqr;
			if ( pvecAimPoint )
			{
				*pvecAimPoint = vecPoints[iPoint];
			}
			break;
		}
	}

	return pBest;
}

//-----------------------------------------------------------------------------
// Purpose: TF2's distance modifier works on a "center" value: 1 at point-blank
//          (+50%), 0.5 at 512 units (base damage), 0 at 1024 (-50%). The rifle
//          keeps the close-range half, scaled to end at falloff_start, and drops
//          to the minimum between falloff_start and falloff_end.
//-----------------------------------------------------------------------------
float CTFVeteranRifle::GetDistanceModCenter( float flDistance )
{
	const float flStart = tf_veteran_rifle_falloff_start.GetFloat();
	const float flEnd = Max( tf_veteran_rifle_falloff_end.GetFloat(), flStart + 1.0f );

	if ( flDistance <= flStart )
		return RemapValClamped( flDistance, 0.0f, flStart, 1.0f, 0.5f );

	return RemapValClamped( flDistance, flStart, flEnd, 0.5f, 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Helix rockets. Fired like the Rocket Launcher (CTFWeaponBaseGun::FireRocket):
//			the rocket starts beside the gun and GetProjectileFireSetup aims it at
//			whatever the crosshair is on, so it converges on the crosshair target
//			with a small correction that depends on the target's distance.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !CanFireHelix( pPlayer ) )
		return;

	m_flHelixReadyTime = gpGlobals->curtime + tf_veteran_helix_cooldown.GetFloat();

	// Cast time: no firing or reloading until it ends (ItemPostFrame). Pushing the next
	// primary attack out also stops the base ItemPostFrame from firing or starting a
	// reload later in this same frame.
	m_flHelixCastEndTime = gpGlobals->curtime + tf_veteran_helix_cast_time.GetFloat();
	m_flNextPrimaryAttack = Max( (float)m_flNextPrimaryAttack, (float)m_flHelixCastEndTime );
	m_iBurstShots = 0;

	// Using an ability ends Sprint.
	Veteran_StopSprint( pPlayer );

	// Placeholder sound: the sentry's rocket launch, at tf_veteran_helix_sound_volume of its script volume.
	CPASAttenuationFilter filter( pPlayer, "Building_Sentrygun.FireRocket" );
	filter.UsePredictionRules();
	CSoundParameters params;
	if ( GetParametersForSound( "Building_Sentrygun.FireRocket", params, NULL ) )
	{
		EmitSound_t ep( params );
		ep.m_flVolume = params.volume * tf_veteran_helix_sound_volume.GetFloat();
		EmitSound( filter, pPlayer->entindex(), ep );
	}

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

#ifdef GAME_DLL
	// Same firing offset and setup as the Rocket Launcher.
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}

	Vector vecSrc;
	QAngle angForward;
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

	// Don't start inside a wall.
	trace_t trace;
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( pPlayer->EyePosition(), vecSrc, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_HelixRocket *pRocket = CTFProjectile_HelixRocket::Create( this, trace.endpos, angForward, pPlayer );
	if ( pRocket )
	{
		pRocket->SetCritical( false );	// sentry rockets don't crit
		pRocket->SetDamage( tf_veteran_helix_damage.GetFloat() );
	}
#endif
}

//-----------------------------------------------------------------------------
bool CTFVeteranRifle::CanFireHelix( CTFPlayer *pPlayer )
{
	return pPlayer && pPlayer->IsAlive() && CanAttack() && IsHelixReady();
}

//-----------------------------------------------------------------------------
// Purpose: Activating the ultimate cancels the helix cast: the rifle can fire and
//          reload again as soon as the ultimate allows it.
//-----------------------------------------------------------------------------
void CTFVeteranRifle::CancelHelixCast( void )
{
	if ( !IsHelixCasting() )
		return;

	m_flHelixCastEndTime = 0.0f;
	m_flNextPrimaryAttack = Min( (float)m_flNextPrimaryAttack, (float)gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 0..1 cooldown progress for the item effect meter.
//-----------------------------------------------------------------------------
float CTFVeteranRifle::GetEffectBarProgress( void )
{
	const float flCooldown = tf_veteran_helix_cooldown.GetFloat();
	if ( flCooldown <= 0.0f || IsHelixReady() )
		return 1.0f;

	return clamp( 1.0f - ( m_flHelixReadyTime - gpGlobals->curtime ) / flCooldown, 0.0f, 1.0f );
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Recoil (client only)
//
// The view angles are the client's: they come from its input and the server
// fires along whatever angles arrive with each command. So recoil is applied
// here, to the local player's view, and the server needs nothing: later shots
// simply fire along the raised view.
//
//   - Each climbing shot adds tf_veteran_rifle_recoil_climb degrees to a pending
//     kick; the next CreateMove raises the view by it.
//   - s_flRecoilOffset is how far above the player's own aim the view sits
//     because of recoil. Pulling the mouse down pays it off; moving it up or
//     sideways doesn't.
//   - Once no shot has been fired for tf_veteran_rifle_recoil_recovery_delay,
//     the remaining offset recovers at tf_veteran_rifle_recoil_recovery_rate,
//     but only while there's no vertical mouse input. Horizontal input doesn't
//     pause it.
//-----------------------------------------------------------------------------
static float s_flRecoilPending = 0.0f;	// degrees still to climb
static float s_flRecoilOffset = 0.0f;	// degrees above the player's own aim
static double s_flLastShotTime = 0.0;
static double s_flLastUpdateTime = 0.0;
static float s_flLastPitch = 0.0f;		// pitch we left the view at last time
static bool s_bHaveLastPitch = false;

// A pitch change this big between two samples isn't the mouse (teleport, spawn,
// server-set angles): drop the recoil instead of reading it as input.
#define VETERAN_RECOIL_SNAP_DEGREES		30.0f

static void VeteranRifleRecoil_Reset( void )
{
	s_flRecoilPending = 0.0f;
	s_flRecoilOffset = 0.0f;
	s_bHaveLastPitch = false;
}

static void VeteranRifleRecoil_OnShot( bool bClimb )
{
	s_flLastShotTime = Plat_FloatTime();

	if ( bClimb )
	{
		s_flRecoilPending += tf_veteran_rifle_recoil_climb.GetFloat();
	}
}

void VeteranRifleRecoil_CreateMove( C_TFPlayer *pPlayer, CUserCmd *pCmd, bool bActive )
{
	const double flNow = Plat_FloatTime();
	const float flDeltaTime = clamp( (float)( flNow - s_flLastUpdateTime ), 0.0f, 0.25f );
	s_flLastUpdateTime = flNow;

	if ( !pPlayer || !pPlayer->IsAlive() || !pPlayer->IsPlayerClass( TF_CLASS_VETERAN ) )
	{
		VeteranRifleRecoil_Reset();
		return;
	}

	if ( !bActive )
	{
		// Something else sets the angles this time; pick up from wherever they end up.
		s_bHaveLastPitch = false;
		return;
	}

	float flPitch = pCmd->viewangles[PITCH];

	// The player's own vertical input since last time (positive = down).
	float flInputDelta = s_bHaveLastPitch ? ( flPitch - s_flLastPitch ) : 0.0f;
	if ( fabs( flInputDelta ) > VETERAN_RECOIL_SNAP_DEGREES )
	{
		s_flRecoilPending = 0.0f;
		s_flRecoilOffset = 0.0f;
		flInputDelta = 0.0f;
	}

	const bool bVerticalInput = ( flInputDelta != 0.0f );

	// Pulling down compensates.
	if ( flInputDelta > 0.0f )
	{
		s_flRecoilOffset = Max( 0.0f, s_flRecoilOffset - flInputDelta );
	}

	if ( s_flRecoilPending > 0.0f )
	{
		// Climb (pitch goes negative when looking up).
		flPitch -= s_flRecoilPending;
		s_flRecoilOffset += s_flRecoilPending;
		s_flRecoilPending = 0.0f;
	}
	else if ( s_flRecoilOffset > 0.0f && !bVerticalInput &&
			  flNow - s_flLastShotTime >= tf_veteran_rifle_recoil_recovery_delay.GetFloat() )
	{
		// Recover toward where the player was aiming.
		const float flStep = Min( s_flRecoilOffset, tf_veteran_rifle_recoil_recovery_rate.GetFloat() * flDeltaTime );
		flPitch += flStep;
		s_flRecoilOffset -= flStep;
	}

	flPitch = clamp( flPitch, -89.0f, 89.0f );

	pCmd->viewangles[PITCH] = flPitch;
	s_flLastPitch = flPitch;
	s_bHaveLastPitch = true;
}

#endif // CLIENT_DLL
