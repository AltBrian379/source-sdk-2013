//========= OWTF2 ============================================================//
//
// Purpose: Melee weapon whose swings alternate left and right. See the header.
//
//=============================================================================//

#include "cbase.h"
#include "tf_weapon_alternating_melee.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_obj.h"
#include "ilagcompensationmanager.h"
#include "debugoverlay_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
ConVar tf_alternating_swing_debug( "tf_alternating_swing_debug", "0", FCVAR_CHEAT,
	"Draw the sweep rays of alternating melee swings for a second (white: a ray or the space between rays, red: hit something)." );

// Largest angle between two rays before the space between them is filled with extra rays.
static const float ALTERNATING_SWING_FILL_STEP_DEGREES = 1.0f;
#endif

//-----------------------------------------------------------------------------
// Weapon tables. The swing state is predicted: the client picks the same side as the server.
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( TFAlternatingSwingMelee, DT_TFAlternatingSwingMelee )

BEGIN_NETWORK_TABLE( CTFAlternatingSwingMelee, DT_TFAlternatingSwingMelee )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bSwingRight ) ),
	RecvPropTime( RECVINFO( m_flSwingChainTime ) ),
	RecvPropBool( RECVINFO( m_bSwingBuffered ) ),
	RecvPropTime( RECVINFO( m_flSwingAnimStart ) ),
	RecvPropInt( RECVINFO( m_iSwingAnimPhase ) ),
#else
	SendPropBool( SENDINFO( m_bSwingRight ) ),
	SendPropTime( SENDINFO( m_flSwingChainTime ) ),
	SendPropBool( SENDINFO( m_bSwingBuffered ) ),
	SendPropTime( SENDINFO( m_flSwingAnimStart ) ),
	SendPropInt( SENDINFO( m_iSwingAnimPhase ), 3 ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFAlternatingSwingMelee )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bSwingRight, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSwingChainTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bSwingBuffered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSwingAnimStart, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iSwingAnimPhase, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
CTFAlternatingSwingMelee::CTFAlternatingSwingMelee()
{
	m_bSwingRight = false;
	m_flSwingChainTime = 0.0f;
	m_bSwingBuffered = false;
	m_flSwingAnimStart = 0.0f;
	m_iSwingAnimPhase = -1;

#ifdef GAME_DLL
	m_bSweepActive = false;
	m_bSweepRight = false;
	m_flSweepStartTime = 0.0f;
	m_flSweepDuration = 0.0f;
	m_iSweepNextRay = 0;
	m_bSweepHasLastRay = false;
	m_vecSweepLastStart.Init();
	m_vecSweepLastDir.Init();
	memset( &m_Sweep, 0, sizeof( m_Sweep ) );
	m_bSweepPushActive = false;
	m_vecSweepPush.Init();
#endif
}

void CTFAlternatingSwingMelee::ResetSwingChain( void )
{
	m_bSwingRight = false;
	m_flSwingChainTime = 0.0f;
	m_bSwingBuffered = false;
	m_iSwingAnimPhase = -1;

#ifdef GAME_DLL
	StopSweep();
#endif
}

void CTFAlternatingSwingMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();
	ResetSwingChain();
}

bool CTFAlternatingSwingMelee::Deploy( void )
{
	ResetSwingChain();
	return BaseClass::Deploy();
}

bool CTFAlternatingSwingMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	ResetSwingChain();
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Default animations: the stock melee swing for both sides. Weapons override this.
//-----------------------------------------------------------------------------
const AlternatingSwingAnims_t &CTFAlternatingSwingMelee::GetSwingAnims( void ) const
{
	static const AlternatingSwingAnims_t s_Anims =
	{
		ACT_VM_HITCENTER,
		{ NULL, NULL },
		{ 0.0f, 0.0f },
		{ 0.0f, 0.0f },
		{
			{ ACT_MP_ATTACK_STAND_MELEE,	ACT_MP_ATTACK_STAND_MELEE },
			{ ACT_MP_ATTACK_CROUCH_MELEE,	ACT_MP_ATTACK_CROUCH_MELEE },
			{ ACT_MP_ATTACK_SWIM_MELEE,		ACT_MP_ATTACK_SWIM_MELEE },
		},
	};
	return s_Anims;
}

//-----------------------------------------------------------------------------
// Purpose: Pick the side, then swing.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::PrimaryAttack( void )
{
	// The base class doesn't swing when it can't attack; don't change sides then.
	if ( !GetTFPlayerOwner() || !CanAttack() )
		return;

	// Chained: this swing starts on the first tick the last one allows it. Ticks land anywhere
	// in [chain time, chain time + one tick) on that first tick; a tick later is past it.
	const float flSinceChain = gpGlobals->curtime - m_flSwingChainTime;
	const bool bChained = m_flSwingChainTime > 0.0f && flSinceChain >= 0.0f && flSinceChain < gpGlobals->interval_per_tick;

	m_bSwingRight = bChained ? !m_bSwingRight : false;
	m_bSwingBuffered = false;

	BaseClass::PrimaryAttack();

	m_flSwingChainTime = m_flNextPrimaryAttack;

	// The viewmodel sequence started this frame (DoViewModelAnimation): time its phases.
	m_flSwingAnimStart = gpGlobals->curtime;
	m_iSwingAnimPhase = -1;
	UpdateSwingAnimRate();

#ifdef GAME_DLL
	StartSweep();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Buffer a press made during a swing; run the buffered swing when it's allowed.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::ItemPostFrame( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

#ifdef GAME_DLL
	// The swing in progress first: a new swing this frame starts its own sweep.
	UpdateSweep();
#endif

	// Checked before the base class: a press that starts a swing this frame isn't a buffer.
	if ( ( pOwner->m_afButtonPressed & IN_ATTACK ) && m_flSwingChainTime > gpGlobals->curtime )
	{
		m_bSwingBuffered = true;
	}

	UpdateSwingAnimRate();

	// Swings if the button is held and the weapon is ready (a held button chains on its own).
	BaseClass::ItemPostFrame();

	if ( m_bSwingBuffered && gpGlobals->curtime >= m_flNextPrimaryAttack && pOwner->GetActiveWeapon() == this )
	{
		// One try: if the swing can't happen now (stunned, ...), the buffer is spent.
		m_bSwingBuffered = false;
		PrimaryAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: First person: the side's sequence. Never the overhead crit swing.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::DoViewModelAnimation( void )
{
	const AlternatingSwingAnims_t &anims = GetSwingAnims();

	// Sets the weapon's activity (and plays one of the activity's sequences).
	SendWeaponAnim( anims.vmActivity );

	const char *pszSequence = anims.pszViewModelSequence[ GetSwingSide() ];
	if ( !pszSequence )
		return;

#ifdef CLIENT_DLL
	if ( !IsPredicted() )
		return;
#endif

	CTFPlayer *pOwner = GetTFPlayerOwner();
	CBaseViewModel *pViewModel = pOwner ? pOwner->GetViewModel( m_nViewModelIndex, false ) : NULL;
	if ( !pViewModel )
		return;

	const int iSequence = pViewModel->LookupSequence( pszSequence );
	if ( iSequence >= 0 )
	{
		pViewModel->SendViewModelMatchingSequence( iSequence );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stretch the swing's viewmodel sequence over the swing's phases: the wind-up over
//			the cast, the blade's crossing over the active time (when the rays fire), the rest
//			over the recovery. Only with a sweep and the side's crossing cycles. Predicted.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::UpdateSwingAnimRate( void )
{
	if ( m_flSwingAnimStart <= 0.0f || m_flSwingChainTime <= m_flSwingAnimStart )
		return;

	AlternatingSwingSweep_t sweep;
	if ( !GetSweepParams( sweep ) )
		return;

	const AlternatingSwingAnims_t &anims = GetSwingAnims();
	const int iSide = GetSwingSide();
	const float flCycleStart = anims.flViewModelSweepStart[ iSide ];
	const float flCycleEnd = anims.flViewModelSweepEnd[ iSide ];
	if ( !anims.pszViewModelSequence[ iSide ] || flCycleStart >= flCycleEnd )
		return;

	// The swing's phases.
	const float flSwing = m_flSwingChainTime - m_flSwingAnimStart;
	const float flCast = clamp( sweep.flDelay, 0.0f, flSwing );
	const float flActive = clamp( ( sweep.flDuration > 0.0f ) ? sweep.flDuration : flSwing - flCast, 0.0f, flSwing - flCast );
	const float flRecovery = flSwing - flCast - flActive;

	const float flElapsed = gpGlobals->curtime - m_flSwingAnimStart;
	int iPhase;
	if ( flElapsed < flCast )
		iPhase = 0;
	else if ( flElapsed < flCast + flActive )
		iPhase = 1;
	else if ( flElapsed < flSwing )
		iPhase = 2;
	else
		iPhase = 3;	// done: the next sequence (idle or the next swing) sets its own rate

	if ( iPhase == m_iSwingAnimPhase )
		return;
	m_iSwingAnimPhase = iPhase;
	if ( iPhase > 2 )
		return;

#ifdef CLIENT_DLL
	if ( !IsPredicted() )
		return;
#endif

	CTFPlayer *pOwner = GetTFPlayerOwner();
	CBaseViewModel *pViewModel = pOwner ? pOwner->GetViewModel( m_nViewModelIndex, false ) : NULL;
	if ( !pViewModel )
		return;

	const float flSequenceTime = pViewModel->SequenceDuration( pViewModel->GetSequence() );
	if ( flSequenceTime <= 0.0f )
		return;

	// Cycles to cover in this phase, over the phase's time.
	float flCycles, flTime;
	switch ( iPhase )
	{
	case 0:		flCycles = flCycleStart;			flTime = flCast;		break;
	case 1:		flCycles = flCycleEnd - flCycleStart;	flTime = flActive;		break;
	default:	flCycles = 1.0f - flCycleEnd;		flTime = flRecovery;	break;
	}
	if ( flTime <= 0.0f )
		return;

	// Clamped to the send table's range (CBaseViewModel's m_flPlaybackRate).
	pViewModel->SetPlaybackRate( clamp( flCycles * flSequenceTime / flTime, 0.0f, 12.0f ) );
}

//-----------------------------------------------------------------------------
// Purpose: Third person: the side's gesture for the player's stance.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	EAlternatingSwingStance eStance = ALTERNATING_SWING_STAND;
	if ( pPlayer->GetWaterLevel() >= WL_Waist )
	{
		eStance = ALTERNATING_SWING_SWIM;
	}
	else if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		eStance = ALTERNATING_SWING_CROUCH;
	}

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, GetSwingAnims().worldGesture[ eStance ][ GetSwingSide() ] );
}

//-----------------------------------------------------------------------------
// Purpose: TF2's single trace at the smack time, unless the weapon uses the sweep.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::Smack( void )
{
	AlternatingSwingSweep_t params;
	if ( GetSweepParams( params ) )
		return;

	BaseClass::Smack();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Trace filter for a sweep ray: the world and props stop it; players and
//			buildings don't (they're tested one by one against the ray).
//-----------------------------------------------------------------------------
class CTraceFilterAlternatingSwingWorld : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterAlternatingSwingWorld, CTraceFilterSimple );

	CTraceFilterAlternatingSwingWorld( const IHandleEntity *pPassEntity )
		: CTraceFilterSimple( pPassEntity, COLLISION_GROUP_NONE ) {}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask ) OVERRIDE
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity && ( pEntity->IsPlayer() || pEntity->IsBaseObject() ) )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}
};

//-----------------------------------------------------------------------------
// Purpose: A swing started: begin its sweep (the first ray fires now).
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::StartSweep( void )
{
	StopSweep();

	AlternatingSwingSweep_t params;
	if ( !GetSweepParams( params ) || params.nRays <= 0 )
		return;

	m_Sweep = params;
	m_bSweepRight = m_bSwingRight;

	// The rays start after the cast.
	m_flSweepStartTime = gpGlobals->curtime + Max( params.flDelay, 0.0f );
	m_flSweepDuration = ( params.flDuration > 0.0f ) ? params.flDuration : Max( m_flNextPrimaryAttack - m_flSweepStartTime, 0.0f );
	m_iSweepNextRay = 0;
	m_bSweepHasLastRay = false;
	m_SweepHits.Purge();
	m_bSweepActive = true;

	UpdateSweep();
}

//-----------------------------------------------------------------------------
// Purpose: Fire the rays that are due. Ray i fires at start (after the cast) + i * duration / rays.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::UpdateSweep( void )
{
	if ( !m_bSweepActive )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner || !pOwner->IsAlive() )
	{
		StopSweep();
		return;
	}

	const int nRays = m_Sweep.nRays;
	const float flRayInterval = m_flSweepDuration / nRays;
	const float flHalfArc = DEG2RAD( m_Sweep.flArcDegrees ) * 0.5f;

	bool bLagCompensated = false;
	CUtlVector< CBaseEntity * > candidates;

	while ( m_iSweepNextRay < nRays && gpGlobals->curtime >= m_flSweepStartTime + flRayInterval * m_iSweepNextRay )
	{
		if ( !bLagCompensated )
		{
			// Other players back to where this player saw them.
			lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
			bLagCompensated = true;

			// Enemies close enough to be reached this frame.
			const Vector vecEye = pOwner->Weapon_ShootPosition();
			const float flReach = m_Sweep.flRange + 128.0f;

			CUtlVector< CTFPlayer * > enemies;
			CollectPlayers( &enemies, GetEnemyTeam( pOwner->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );
			FOR_EACH_VEC( enemies, i )
			{
				if ( enemies[i]->WorldSpaceCenter().DistToSqr( vecEye ) <= flReach * flReach )
				{
					candidates.AddToTail( enemies[i] );
				}
			}

			for ( int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
			{
				CBaseObject *pObject = static_cast< CBaseObject * >( IBaseObjectAutoList::AutoList()[i] );
				if ( !pObject || pObject->GetTeamNumber() == pOwner->GetTeamNumber() || pObject->IsPlacing() || pObject->IsCarried() )
					continue;
				if ( pObject->WorldSpaceCenter().DistToSqr( vecEye ) <= flReach * flReach )
				{
					candidates.AddToTail( pObject );
				}
			}
		}

		// Across the arc in the direction the swing moves: a left swing (the blade moving left)
		// goes from the right edge to the left edge. Positive angle = left of the crosshair.
		const float flFraction = ( nRays > 1 ) ? (float)m_iSweepNextRay / (float)( nRays - 1 ) : 0.5f;
		float flAngle = -flHalfArc + flFraction * 2.0f * flHalfArc;
		if ( m_bSweepRight )
		{
			flAngle = -flAngle;
		}

		// Relative to the camera now.
		Vector vecForward, vecRight;
		AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, NULL );
		Vector vecDir = vecForward * cosf( flAngle ) - vecRight * sinf( flAngle );
		vecDir.NormalizeInPlace();
		const Vector vecStart = pOwner->Weapon_ShootPosition();

		// The knockback's direction: straight to the side the swing moves toward (the camera's
		// left for a left swing), horizontal; no push toward or away from the attacker.
		Vector vecPush = m_bSweepRight ? vecRight : -vecRight;
		vecPush.z = 0.0f;
		vecPush.NormalizeInPlace();

		// The space since the previous ray.
		if ( m_bSweepHasLastRay )
		{
			const float flGap = RAD2DEG( acosf( clamp( DotProduct( m_vecSweepLastDir, vecDir ), -1.0f, 1.0f ) ) );
			const int nSteps = Min( (int)ceilf( flGap / ALTERNATING_SWING_FILL_STEP_DEGREES ), 180 );
			for ( int iStep = 1; iStep < nSteps; ++iStep )
			{
				const float t = (float)iStep / (float)nSteps;
				Vector vecFillDir = Lerp( t, m_vecSweepLastDir, vecDir );
				if ( vecFillDir.NormalizeInPlace() < 0.001f )
					continue;
				CastSweepRay( pOwner, Lerp( t, m_vecSweepLastStart, vecStart ), vecFillDir, vecPush, candidates );
			}
		}

		CastSweepRay( pOwner, vecStart, vecDir, vecPush, candidates );

		m_vecSweepLastStart = vecStart;
		m_vecSweepLastDir = vecDir;
		m_bSweepHasLastRay = true;
		++m_iSweepNextRay;
	}

	if ( bLagCompensated )
	{
		lagcompensation->FinishLagCompensation( pOwner );
	}

	if ( m_iSweepNextRay >= nRays )
	{
		StopSweep();
	}
}

//-----------------------------------------------------------------------------
// Purpose: One ray: stopped by the world and props; hits every candidate whose hull it
//			crosses that this swing hasn't hit yet.
//-----------------------------------------------------------------------------
void CTFAlternatingSwingMelee::CastSweepRay( CTFPlayer *pOwner, const Vector &vecStart, const Vector &vecDir, const Vector &vecPush, CUtlVector< CBaseEntity * > &candidates )
{
	trace_t trWorld;
	CTraceFilterAlternatingSwingWorld filter( pOwner );
	UTIL_TraceLine( vecStart, vecStart + vecDir * m_Sweep.flRange, MASK_SOLID, &filter, &trWorld );

	Ray_t ray;
	ray.Init( vecStart, trWorld.endpos );

	bool bHit = false;
	FOR_EACH_VEC( candidates, i )
	{
		CBaseEntity *pTarget = candidates[i];
		if ( !pTarget || m_SweepHits.HasElement( pTarget ) )
			continue;

		trace_t tr;
		enginetrace->ClipRayToEntity( ray, MASK_SOLID, pTarget, &tr );
		if ( tr.fraction >= 1.0f && !tr.startsolid )
			continue;

		tr.m_pEnt = pTarget;
		m_SweepHits.AddToTail( pTarget );
		bHit = true;

		// The damage (and its push, AlternatingSwing_AdjustDamageDirection) happens in here.
		m_vecSweepPush = vecPush;
		m_bSweepPushActive = true;
		OnSwingHit( tr );
		m_bSweepPushActive = false;

		// OnSwingHit's hit sound is predicted (it leaves the owner out), but the sweep runs on
		// the server only: play it to the owner too.
		const char *pszHitSound = GetShootSound( MELEE_HIT );
		if ( pszHitSound && pszHitSound[0] )
		{
			CSingleUserRecipientFilter ownerFilter( pOwner );
			EmitSound( ownerFilter, pOwner->entindex(), pszHitSound );
		}
	}

	if ( tf_alternating_swing_debug.GetBool() )
	{
		if ( bHit )
			NDebugOverlay::Line( vecStart, trWorld.endpos, 255, 0, 0, true, 1.0f );
		else
			NDebugOverlay::Line( vecStart, trWorld.endpos, 255, 255, 255, true, 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Knockback along the swing. ApplyPushFromDamage pushes along -dir, with TF2's
//			damage-based strength.
//-----------------------------------------------------------------------------
void AlternatingSwing_AdjustDamageDirection( const CTakeDamageInfo &info, Vector &dir )
{
	CTFAlternatingSwingMelee *pWeapon = dynamic_cast< CTFAlternatingSwingMelee * >( info.GetWeapon() );
	if ( !pWeapon )
		return;

	Vector vecPush;
	if ( pWeapon->GetSweepPushDirection( vecPush ) )
	{
		dir = -vecPush;
	}
}
#endif // GAME_DLL
