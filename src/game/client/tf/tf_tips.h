//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF implementation of the IPresence interface
//
//=============================================================================

#ifndef TF_TIPS_H
#define TF_TIPS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

// OWTF2: MvM tips are stored as "Tip_MvM_<n>_*" for the 9 stock classes plus a general bucket at
// n = 10, which used to equal TF_LAST_NORMAL_CLASS. New classes shifted that, so name the bucket.
// GetRandomMvMTip reports this value in iClassUsed when it picks a general tip.
#define TF_MVM_TIP_GENERAL_BUCKET	( TF_CLASS_ENGINEER + 1 )


struct CaptainCanteenAsset_t
{
	char szImage[ MAX_PATH ];
	float flRarity;
};

//-----------------------------------------------------------------------------
// Purpose: helper class for TF tips
//-----------------------------------------------------------------------------
class CTFTips : public CAutoGameSystem
{
public:
	CTFTips();

	virtual bool Init();
	virtual char const *Name() { return "CTFTips"; }

	const wchar_t *GetRandomTip( int &iClassUsed ); // iClassUsed will be filled in with the class that was selected
	const wchar_t *GetNextClassTip( int iClass );
	const wchar_t *GetArenaTip( void );
	const wchar_t *GetAbuseReportTip( void );
	const wchar_t *GetRandomMvMTip( int &iClassUsed ); // iClassUsed will be filled in with the class that was selected
	void GetRandomCaptainCanteenImages( const char **ppchBody, const char **ppchMisc, const char **ppchHat );

private:
	const wchar_t *GetTip( int iClass, int iTip );
	const char *GetRandomCaptainCanteenAsset( CUtlVector< CaptainCanteenAsset_t > *pAssetBucket );

	int m_iTipCount[TF_LAST_NORMAL_CLASS+1];		// how many tips there are for each class
	int m_iTipCountAll;								// how many tips there are total
	int m_iCurrentClassTip;							// index of current per-class tip
	bool m_bInited;									// have we been initialized
	int	m_iArenaTipCount;
	int m_iMvMTipCount;

	CUtlVector< CaptainCanteenAsset_t > m_CaptainCanteenBody;
	CUtlVector< CaptainCanteenAsset_t > m_CaptainCanteenMisc;
	CUtlVector< CaptainCanteenAsset_t > m_CaptainCanteenHat;
};

extern CTFTips g_TFTips;
#endif // TF_TIPS_H
