# OWTF2 — Overwatch hero mod for TF2

A Team Fortress 2 mod built on Valve's Source SDK 2013 (TF2 branch). First milestone:
port the gameplay of one Overwatch hero into TF2 as a new playable class.

## Current status
- Hero: **Soldier: 76** (gameplay only). In code and in game it's the original-named
  class "Veteran" (`TF_CLASS_VETERAN` = 10, inserted before Civilian).
- Phase: the C++ class exists and is selectable. It uses the Sniper model and stats and has
  no weapons or abilities yet. See `docs/veteran-class-changes.txt`.

## Plan
Build the hero directly in C++ in this repo, on top of the existing Veteran class:
weapons, client-predicted abilities and a custom HUD. The VScript prototyping phase
was dropped: don't build a VScript version of the hero. All hero work goes into
the C++ class.

## Testing in game
VScript is the test harness. A temporary `game/mod_tf/scripts/vscripts/mapspawn.nut`
drives a test run:
- Launch: `game/mod_tf_win64.exe -game mod_tf -novid -windowed -w 1280 -h 720 -condebug -nojoy -dev +map itemtest`.
  `-dev` is needed or `+map` is ignored, and `wait` is disabled in cfg files.
- Drive the test from a think function (`AddThinkToEnt`) with timed steps.
  `SendToConsole` runs client commands through the real input path (`jointeam`,
  `joinclass`, `+attack4`). `SendToServerConsole` sets server convars.
- Log results with `printl` and read `game/mod_tf/console.log`. `jpeg` takes
  screenshots into `game/mod_tf/screenshots/`.
- Delete the test script, screenshots and `console.log` afterwards.

The main objective is functionality. Polish comes last. Order:
1. Abilities: sprint, helix rockets, healing field, then the ultimate.
2. Weapons: primary hitscan auto rifle.
3. Details: cooldown HUD, original model, art, sounds and class-menu assets.

Stats stay as they are (Sniper values in `veteran.txt`). Don't change them unless asked.

Ability conventions:
- Logic goes in `src/game/shared/tf/tf_veteran_abilities.cpp/.h` (shared and predicted).
  State goes in `CTFPlayerShared` as networked, predicted vars (`DT_TFPlayerSharedLocal`
  plus a `DEFINE_PRED_FIELD`).
- Every tunable number is a replicated convar named `tf_veteran_*`
  (`FCVAR_REPLICATED | FCVAR_NOTIFY`), so it can be tuned without a rebuild.
- Input: `+attack4` (`IN_ATTACK4`, bit 26) is the class ability key, bound to SHIFT
  by default. Add another `+attackN` only when a new key is needed.
- HUD must match stock TF2 elements: cooldown meters look like the Mad Milk and
  Jarate meters. The ultimate bar mirrors the Ubercharge meter, including its
  effects when fully charged.

Done: Sprint (toggle on `+attack4`, `tf_veteran_sprint_speed_mult` = 1.5).
Done: primary weapon `tf_weapon_veteran_rifle` (`CTFVeteranRifle`, `TF_WEAPON_VETERAN_RIFLE`),
item 60000. Placeholder: it subclasses the SMG and has no mechanics of its own yet.
Done: ultimate system (reusable by any class), `src/game/shared/tf/tf_ultimate.cpp/.h`.
- Per class: `"ultimate_cost"` (0 = no ultimate) and `"ultimate_duration"` (seconds to drain; 0 = instant)
  in `scripts/playerclasses/<class>.txt`. Veteran: 2310 and 6, approximately the source hero's values.
- Charge: passive `tf_ultimate_charge_per_second` (5) once the round is running and setup is over;
  damage to enemy players and buildings (`tf_ultimate_charge_per_damage`; overheal portion
  x `tf_ultimate_overheal_damage_scale` 0.5); healing (`tf_ultimate_charge_per_heal`, through
  `CTFGameStats::Event_PlayerHealedOther`). Capped at the cost. Nothing builds while active, from
  self or team damage, or from damage by an ultimate (active, or an entity passed to `Ultimate_MarkEntity`).
- Kept through death; reset on round start (`teamplay_round_start` listener) and class change.
  Dying ends an active ultimate.
- Use: `+attack5` (`IN_ATTACK5`, bit 27), default key Q (replaces TF2's default Q = lastinv).
  Per-hero effect hooks: `Veteran_OnUltimateStart/End` (empty for now), dispatched in `tf_ultimate.cpp`.
- HUD: `CHudUltimateMeter` (`client/tf/tf_hud_ultimate.cpp`, `resource/ui/HudUltimate.res`), shown for any
  class with an ultimate. It sits in the Medic meter's spot (bottom right, r138/r69). While it shows, the ammo counter
  (`CTFHudWeaponAmmo`, `tf_hud_ammostatus.cpp`) moves up to sit on top of it. It pulses like
  Ubercharge when full and while active.
- Self-healing rules are deferred: they'll be set per hero and per ability.
- Testing: `tf_ultimate_charge_set <points>` (cheat).
Done: helix rockets, the rifle's alt-fire (`+attack2`, MOUSE2). `CTFVeteranRifle::SecondaryAttack` fires
`tf_projectile_helix_rocket` (`tf_projectile_helix_rocket.cpp/.h`, a copy of the sentry rocket: model,
zero-size hull, speed 1100, radius 146, base damage 100). It's aimed like the Rocket Launcher
(`GetProjectileFireSetup`, same 23.5/12/-3 offset). The cooldown `tf_veteran_helix_cooldown` (6 s) is shown by
the item effect meter (the Jarate's meter, label "HELIX", recharge beep). Damage is `tf_veteran_helix_damage`.

## Known issues to revisit (numbers and interactions; functionality comes first)
- HUD overlap: the HELIX cooldown meter (TF2's item effect meter spot, r162/r52) is drawn over the left
  edge of the ultimate meter (now in the Medic meter's spot).
- Helix damage follows TF2's normal distance modifier from the shooter (+-50% around 512 units:
  about 150 point-blank, 50 far away). Sentry rockets measure from the Engineer, and the Rocket
  Launcher gets half the close-range bonus. Decide the rule, or turn it off (no DMG_USEDISTANCEMOD).
- Helix rockets damage and push the Veteran like soldier rockets (a point-blank test hit took 125 to 118).
  Decide self-damage and self-knockback.
- Placeholder sound (Building_Sentrygun.FireRocket) and the third-person attack gesture.

## Weapons and items
- Weapon code: `src/game/shared/tf/tf_weapon_<name>.cpp/.h`. New weapon IDs go at the end
  of `ETFWeaponType` (before `TF_WEAPON_COUNT`), with matching rows in `g_aWeaponNames` and
  `g_aWeaponDamageTypes` in `tf_shareddefs.cpp`.
- Weapon scripts: `game/mod_tf/scripts/tf_weapon_<name>.txt`. A plain `.txt` is read
  before an encrypted `.ctx`.
- Item definitions: `game/mod_tf/scripts/items/items_owtf2.txt`, using defindex 60000 and up.
  TF2's `items_game.txt` is signature-checked, so the mod can't override it.
  `OWTF2_MergeModItemSchema` (`tf_owtf2_items.cpp`) merges the mod file into it at load time.
  Don't use TF2 prefabs: they carry their own `used_by_classes`.
- A class's default weapon is the `"baseitem" "1"` item whose `used_by_classes` and
  `item_slot` match. The Veteran has a single weapon, so its loadout page shows only the
  primary slot (`g_LoadoutPanelPositioning_Veteran`, `class_loadout_panel.cpp`).

## Repo map
- `src/game/shared/tf/` — code compiled into both client and server (movement,
  weapons, class data). Most gameplay changes start here.
- `src/game/server/tf/` — server-only game logic.
- `src/game/client/tf/` — client-only code: rendering, prediction, HUD.
- `src/game/client/tf/vgui/` — menus and HUD panels.
- `src/game/client/client_tf.vpc`, `src/game/server/server_tf.vpc` — project
  definitions. Add new .cpp/.h files here, then regenerate projects.
- `src/vpc_scripts/groups.vgc` — which projects each solution includes.
- `src/thirdparty/` — vendored libraries. Never modify.
- `game/mod_tf/` — the mod's game folder (`gameinfo.txt`, overriding content).
  Base TF2 content is loaded through `gameinfo.txt` search paths. To override a
  file, place it at the same relative path under `game/mod_tf/`.

## Build and verify
- `tools/build.ps1` builds the TF client and server DLLs and prints a summary of
  errors. Run it with:
  `powershell -ExecutionPolicy Bypass -File tools/build.ps1`
  - `-Regenerate` reruns VPC first. Required after editing any `.vpc` or `.vgc` file.
  - `-Full` builds `everything.sln` (all tools) instead of `game_tf.sln`.
  - `-Config Debug` for a debug build. Default is Release.
- Full log: `tools/logs/last-build.log`.
- `game_tf.sln` comes from `src/creategameprojects.bat`, which uses VPC's `game`
  group. It skips tools such as `qc_eyes` that need MFC.
- `.sln`, `.vcxproj` and `.vcxproj.filters` files are generated by VPC. Never edit
  them; change the `.vpc` file and regenerate.
- After any C++ change, run the build and fix every error before reporting done.

## Working rules
- Mark every change to Valve's code with an `// OWTF2:` comment explaining it, so
  changes are easy to find when merging upstream SDK updates.
- Prefer new files for new systems (the hero's abilities, HUD panels) over large
  edits inside Valve's files.
- Client and server must agree: anything predicted or networked usually needs
  matching changes in `shared/`, `client/` and `server/`. Check all three.
- Don't commit unless asked.

## The Veteran class
Added as `TF_CLASS_VETERAN` (10) in `ETFClass`, `src/game/shared/tf/tf_shareddefs.h`,
right after Engineer. Civilian (now 11) is still `TF_LAST_NORMAL_CLASS`; Random is now 13.
`docs/veteran-class-changes.txt` lists every file touched and why.
- Class data: `game/mod_tf/scripts/playerclasses/veteran.txt` (plain `.txt`
  is read before the encrypted `.ctx`).
- Class menu: `client/tf/vgui/tf_classmenu.cpp` plus the
  `game/mod_tf/resource/ui/ClassSelection*.res` overrides.
- Name and tips: `game/mod_tf/resource/mod_tf_english.txt` (UTF-16LE, CR line endings).
- Movement and abilities: `shared/tf/tf_gamemovement.cpp`.
- Many per-class tables have a `COMPILE_TIME_ASSERT` on their length. If you add
  another class, the build will point at tables that need a row. Also grep for
  `TF_CLASS_ENGINEER`, `TF_CLASS_COUNT` and `TF_LAST_NORMAL_CLASS`, because some
  tables have no assert.
- Stock TF2 content keyed by class number (tips, menu music) uses the old enum,
  where Random was 12. See `GetStockContentClassIndex` in `tf_classmenu.cpp`.
- Placeholders: Sniper model, stats, sounds and art; no weapons; no item loadout
  (the loadout screen covers only the 9 stock classes).

## Constraints
- **License:** the SDK is non-commercial. The mod and all its content must be free.
- **IP:** recreate gameplay mechanics only. No Blizzard models, sounds, voice
  lines, names, icons or UI art in anything distributed. The hero gets an original
  name and original art.
- **Toolchain:** Windows, Visual Studio 2022 Community with the Desktop
  development with C++ workload, MSVC v143 build tools, a Windows 10/11 SDK, and
  Python 3.13 or later (needed by the project generator). Source SDK Base 2013
  Multiplayer must be installed through Steam.

## References
- Valve Developer Community: https://developer.valvesoftware.com/wiki/Main_Page
- SDK setup: https://developer.valvesoftware.com/wiki/Setting_up_Source_SDK_Base_2013_Multiplayer
- Upstream repo: https://github.com/ValveSoftware/source-sdk-2013
