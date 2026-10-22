//========= OWTF2 ============================================================//
//
// Purpose: Merges the mod's item definitions into TF2's item schema.
//
//=============================================================================//

#include "cbase.h"
#include "tf_owtf2_items.h"
#include "filesystem.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void OWTF2_MergeModItemSchema( KeyValues *pSchemaKV )
{
	if ( !pSchemaKV || !g_pFullFileSystem )
		return;

	KeyValues *pModKV = new KeyValues( "items_owtf2" );
	KeyValues::AutoDelete autoDelete( pModKV );

	if ( !pModKV->LoadFromFile( g_pFullFileSystem, OWTF2_ITEMS_FILE, "GAME" ) )
	{
		Warning( "OWTF2: couldn't load %s; mod items will be missing.\n", OWTF2_ITEMS_FILE );
		return;
	}

	int nMerged = 0;
	FOR_EACH_TRUE_SUBKEY( pModKV, pModSection )
	{
		KeyValues *pSchemaSection = pSchemaKV->FindKey( pModSection->GetName(), true );

		FOR_EACH_SUBKEY( pModSection, pModEntry )
		{
			KeyValues *pExisting = pSchemaSection->FindKey( pModEntry->GetName() );
			if ( pExisting )
			{
				pSchemaSection->RemoveSubKey( pExisting );
				pExisting->deleteThis();
			}

			pSchemaSection->AddSubKey( pModEntry->MakeCopy() );
			nMerged++;
		}
	}

	DevMsg( "OWTF2: merged %d entries from %s into the item schema.\n", nMerged, OWTF2_ITEMS_FILE );
}
