//========= OWTF2 ============================================================//
//
// Purpose: Merges the mod's item definitions into TF2's item schema.
//
//			TF2's scripts/items/items_game.txt is signature-checked, so the mod
//			can't override it. Instead, after it is parsed, the sections in
//			scripts/items/items_owtf2.txt (same layout: "items", "prefabs", ...)
//			are merged into it. A key that already exists is replaced.
//
//			Mod item definition indices start at 60000 (TF2's are below 32000).
//
//=============================================================================//

#ifndef TF_OWTF2_ITEMS_H
#define TF_OWTF2_ITEMS_H
#ifdef _WIN32
#pragma once
#endif

class KeyValues;

#define OWTF2_ITEMS_FILE				"scripts/items/items_owtf2.txt"
#define OWTF2_ITEMDEF_VETERAN_RIFLE		60000
#define OWTF2_ITEMDEF_VETERAN_FIELD_RATIONS	60001

// Called by CEconItemSchema::BInitTextBuffer with the parsed items_game.txt.
void OWTF2_MergeModItemSchema( KeyValues *pSchemaKV );

#endif // TF_OWTF2_ITEMS_H
