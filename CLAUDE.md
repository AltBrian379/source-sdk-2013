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
- Start the steps once `GetListenServerHost()` isn't null (commands sent earlier are lost). Send
  `hidepanel info; jointeam red`, then `joinclass <class>`, then `hidepanel team; hidepanel class_red` a few
  seconds later, or the MOTD, team or class menu stays over the screenshots.
- Bots that stand still: `SendToConsole( "bot" )` once per bot (`SendToServerConsole` doesn't run it, and `bot`
  ignores its team and class arguments). A second later, set them up from VScript: `ForceChangeTeam`,
  `SetPlayerClass`, `m_Shared.m_iDesiredPlayerClass`, `ForceRespawn`; place them with `SetAbsOrigin`. The
  itemtest room is about 800 units deep from the spawn. Aim with `SnapEyeAngles`. `player_hurt` and
  `player_healed` events (`OnGameEvent_*`, `__CollectGameEventCallbacks`) give the numbers.
- Log results with `printl` and read `game/mod_tf/console.log`. `jpeg` takes
  screenshots into `game/mod_tf/screenshots/`.
- Delete the test script, screenshots and `console.log` afterwards.
- `SendToServerConsole` from `mapspawn.nut` doesn't set some server convars (`tf_weapon_criticals` stayed 1; the
  script convar allowlist doesn't load). Read them back with `Convars.GetInt`; set them at launch (`+tf_weapon_criticals 0`).

The main objective is functionality. Polish comes last. Order:
1. Abilities: sprint, helix rockets, healing field (all done), then the ultimate's effect.
2. Weapons: primary hitscan auto rifle.
3. Details: cooldown HUD, original model, art, sounds and class-menu assets.

Stats stay as they are (Sniper values in `veteran.txt`, except max health, 175, and the rifle's reserve ammo, 90, both set on request).
Don't change them unless asked. Every ported hero's base speed is the Demoman's, 280 (`speed_max`), instead of the
standard 300.

Stat scaling for ported heroes (the user's rule; values don't have to be faithful to any one Overwatch patch):
- Health is a judgment call by role, not a formula: match the TF2 class whose role, mobility and damage fit the
  hero (a high-mobility damage hero shouldn't share a slow one's health, like Scout vs Soldier in TF2, or Tracer
  vs Soldier: 76 in Overwatch). The Veteran (Soldier: 76, 250 HP) got the Demoman's 175. The Veteran is the
  reference for every later port.
- Damage and healing scale by the hero's TF2 health over its Overwatch health. For a 250 HP hero at 175 that's
  x 0.7: Soldier: 76's 20 per bullet -> 14 (x / 175 = 20 / 250).
- The goal behind it is the 1v1 time to kill: a hero should need the same number of hits to kill its own mirror
  in TF2 as in Overwatch (Soldier: 76 vs Soldier: 76: 250 / 20 = 13 bullets; Veteran vs Veteran: 175 / 14 = 13).
  Check new damage numbers against that.
- The scaled number is the base. TF2's own modifiers (close-range bonus and falloff, crits, minicrits, resistances)
  apply on top as they would to any TF2 weapon, so the hero stays consistent with TF2's mechanics, even though it
  moves the time to kill away from 13 off the 512-unit mark. Only remove or change a TF2 modifier for a hero when
  the user asks. Distance follows the source hero: Soldier: 76 has falloff, so the Veteran's rifle keeps TF2's
  distance rules; Ana's rifle has none, so the Biotic Rifle is flat (like TF2's sniper rifles).
- Combat Medic: Ana also has 250 HP, so 175 HP and the same x 0.7 on her damage and healing.
- Use current Overwatch values as the starting point; check them before each phase.
- Distances and speeds: 1 m = 39.37 HU (TF2's world scale, 1 HU = 1 inch). Round converted values to a
  nearby multiple of 8 when it's close (126 -> 128, 2045 -> 2048); they're starting points for balancing.
- Comment every initial default with where it came from (the Overwatch value and the conversion).
- Reserve ammo is 3x the clip (Overwatch has no reserve): Veteran 30 + 90, Combat Medic 15 + 45.

Next hero: an Ana-like support, **Combat Medic** (`TF_CLASS_COMBATMED`). The plan, with every design decision,
is `docs/combatmed-plan.txt`. Follow it. Phases 0 (class skeleton), 1 (shared systems), 2 (Biotic Rifle),
3 (Biotic Grenade), 4 (Sleep Dart) and 5 (Nano Boost) are done. Phase 6 (details) remains.

Third hero: a Reinhardt-like tank, **Crusader** (`TF_CLASS_CRUSADER`). Draft plan: `docs/crusader-plan.txt`.
Phase 0 (class skeleton) is done: Demoman placeholders, 385 HP (the user's value). Phase 2 (hammer) mostly done.
Ahead of phase 2: the hammer `tf_weapon_crusader_hammer` (`CTFCrusaderHammer`, item 60005, primary, key 1, a melee
weapon; 70 damage; 1.05 s swing: 0.4 s cast, 0.21 s of rays, recovery; PLACEHOLDER Eyelander model and sounds) with
alternating swings, a sweep of rays, and
TF2's damage-based knockback pointed sideways along the swing (`CTFPlayer::AdjustDamageDirection`). Animation rules
(readability first): plan's "Animations" section; names: `docs/demoman-animations.txt`.

Done: Nano Boost, the ultimate (`ultimate_cost` 2300, instant). Q needs a living teammate under the crosshair, else
the one closest to it within `tf_combatmed_nano_aim_cone` (20 degrees, PLACEHOLDER), in `tf_combatmed_nano_range`
(1576) and line of sight (`CombatMed_FindNanoTarget`); with none, nothing happens and the charge is kept. The target is
healed `tf_combatmed_nano_heal` (175; its own healing earns no charge), then for `tf_combatmed_nano_duration` (8 s) gets
`TF_COND_ENERGY_BUFF` (Crit-a-Cola's minicrits and glow) and `TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK` (the Battalion's
ring; resistance `tf_combatmed_nano_resistance` 0.35 via `SetDefenseBuffResistance`). `m_flNanoBoostEndTime`
(networked, server-written) lets the predicted speed skip the Heavy's energy-buff speed bonus (the user's choice).
No self-target. Healing credit helper for the mod's heals: `HealModifiers_CreditHealing`.
A Heavy who eats the Buffalo Steak (melee lock, `TF_COND_CANNOT_SWITCH_FROM_MELEE`) during Nano Boost does get the
Steak's speed; the lock refreshes speed when it starts or ends. On use: a PLACEHOLDER voice line,
`vo/medic_battlecry01.mp3` (`COMBATMED_ULTIMATE_VOICE_LINE`), heard like the Veteran's
(`tf_combatmed_ultimate_voice_radius` 2048, `tf_combatmed_ultimate_voice_pitch` 100); when it ends (0.53 s) the
target says the Battle Cry voice command (`VoiceCommand( target, 2, 1 )`, voice menu C then 2).

Phase 6 (details) so far:
- No-heal: the spawn-room "no entry" sign over your health cross (`m_pNoHealImage`, `tf_hud_playerstatus.cpp`;
  material `materials/vgui/owtf2/no_heal.vmt`, the `overlays/no_entry` texture without its distance/team fading).
- Biotic Rifle: Crusader's Crossbow model and animations, Sydney Sleeper fire sound. Sleep Dart: Hitman's Heatmaker
  fire sound. Biotic Grenade: third-person throw `ACT_MP_THROW` (the throwables' and spells' overhand throw, as a
  `PLAYERANIMEVENT_CUSTOM_GESTURE` like the spellbook's).
- Biotic Grenade burst healing is never boosted (not even by an earlier grenade's boost: it's a status effect, not a
  stack); a newer grenade's boost or no-heal replaces the old one. Enemy buildings in the burst take its damage only.
- Healing over time carries the fraction left after the heal modifiers to the next hit (boosted 52 -> 78).
- Nano Boost target outline: while the local Combat Medic's ultimate is ready, the teammate Q would pick
  (`CombatMed_FindNanoTarget`, now shared) gets a yellow outline (`CCombatMedNanoTargetOutline`, a client
  `CGlowObject`, drawn when visible).
- Damage over time (`DamageOverTime_Apply`) takes any damageable entity: the Biotic Rifle's hits on buildings are
  damage over time too (the grenade's burst on buildings stays instant).

Done: Sleep Dart, the melee slot `tf_weapon_combatmed_sleepdart` (`CTFCombatMedSleepDart`, a `CTFCastAbilityWeapon`,
`TF_WEAPON_COMBATMED_SLEEPDART`), item 60004. SHIFT (`+attack4`, `CombatMed_AbilityPostFrame` switches to it) or key 3.
Instant switch, cast `tf_combatmed_sleepdart_cast_time` 0.24 s, then the dart, then a recovery on the dart
(`tf_combatmed_sleepdart_recovery_time` 0.75 s; `GetRecoveryTime`, new in the cast-weapon base) before the rifle;
cooldown `tf_combatmed_sleepdart_cooldown` 14 s (SLEEP meter, `HudItemEffectMeter_CombatMedSleep.res`, left of
GRENADE). It doesn't interrupt the grenade's cast (the grenade can't be holstered mid-cast; SHIFT does nothing).
Projectile `tf_projectile_sleep_dart` (`server/tf`, the crossbow bolt: syringe model at 3x, `healshot_trail`): 2360
HU/s, straight, range 7872, own swept trace; passes through the shooter and every teammate. An enemy hit takes 3, then
`CombatMed_PutToSleep`: TF2's full stun (`TF_STUN_MOVEMENT | TF_STUN_CONTROLS`, stars and stun animation as
PLACEHOLDER) and `m_flSleepEndTime` (`CTFPlayerShared`, server only); `Ultimate_InterruptCast` on the target.
ÜberCharge blocks the damage and the sleep. Any damage that lands wakes the sleeper (`CombatMed_OnDamageTaken`, from
`CTFPlayer::OnTakeDamage_Alive` after health is taken); healing doesn't. The sleep ends at
`tf_combatmed_sleep_duration` (5 s) exactly: `CombatMed_SleepThink` (every player, every tick) ends it, because TF2
lengthens control stuns for its get-up animation by an amount that doesn't simply add up.

Done: Biotic Grenade, the secondary `tf_weapon_combatmed_grenade` (`CTFCombatMedGrenade`, a `CTFCastAbilityWeapon`,
`TF_WEAPON_COMBATMED_GRENADE`), item 60003, key 2. Instant switch, Medic's throw animation (`ACT_THROWABLE_VM_FIRE`,
in Medic's arms though no stock item uses it), thrown after `tf_combatmed_grenade_cast_time` (0.25 s, PLACEHOLDER),
cooldown `tf_combatmed_grenade_cooldown` 14 s (GRENADE meter, TF2's default item effect meter spot), ready again on spawn
(`CombatMed_ResetAbilities`). Projectile `tf_projectile_biotic_grenade` (`server/tf/tf_projectile_biotic_grenade.cpp/.h`,
server only, a plain model on clients): speed 1184, TF2 gravity, moved by its own swept trace; bursts on the world,
any player but the thrower, or a building. Burst (`tf_combatmed_grenade_radius` 160, line of sight, no falloff):
teammates and the thrower healed 52 at once (never boosted), then heal boost x1.5 for 3 s;
enemies 52 damage (`DMG_GENERIC`, no knockback), then no-heal for 3 s; ÜberCharged enemies are skipped; enemy buildings
the damage only.
PLACEHOLDER: Jarate in hand, Grenade Launcher grenade and trail, Mad Milk splash, Jarate break sound.
Tints (`CProxyUrineLevel`, the `YellowLevel` material proxy, `c_tf_player.cpp`): about 20%, purple under no-heal
(1.20 0.84 1.28), green under heal boost (0.88 1.20 0.88); Jarate's yellow wins over both.
Class-wide hooks: `tf_combatmed_abilities.cpp/.h` (like the Veteran's).

Done: Biotic Rifle, `tf_weapon_combatmed_rifle` (`CTFCombatMedRifle`, `TF_WEAPON_COMBATMED_RIFLE`), item 60002, a
`CTFWeaponBaseGun` (not the sniper rifle, whose charge, unscope-after-shot and headshots would all need undoing).
PLACEHOLDER: the Crusader's Crossbow's model and animations, the Sydney Sleeper's fire sound. Script `tf_weapon_combatmed_rifle.txt`: clip 15,
`TimeFireDelay` 0.8, `TimeReload` 1.5 (fixed, like the Veteran's `DefaultReload`); reserve 45 in `combatmed.txt`.
- Unscoped: `tf_projectile_biotic_round` (`tf_projectile_biotic_round.cpp/.h`), `tf_combatmed_rifle_projectile_speed`
  4920. No physics: each tick it sweeps its movement with `CTraceFilterSkipFullHealthTeammates`. Clients draw a
  syringe temp entity with the Syringe Gun's trail (effect `ClientProjectile_BioticRound`), like TF2's syringes.
- Scoped (hold MOUSE2): hitscan from the eye, lag compensated, same filter, `bullet_tracer01` tracer. FOV
  `tf_combatmed_rifle_zoom_fov` (20, TF2's sniper zoom); max speed x `tf_combatmed_rifle_scoped_speed_mult` (0.35).
  The scope overlay is TF2's (`TF_COND_ZOOMED`); its charge meter and charge proxy now skip non-sniper weapons.
  Reloading or switching away unscopes; MOUSE2 must be released and pressed again to scope. A new MOUSE2 press
  interrupts a reload (`UpdateScope`, also run from `ItemBusyFrame`). An empty clip reloads even with MOUSE2 held
  (TF2 only auto-reloads with no fire button down), except after scoping interrupted the reload: then it reloads
  once unscoped (`m_bScopeStoppedReload`).
- Hits (`CTFCombatMedRifle::ApplyBioticHit`, server): enemy player `tf_combatmed_rifle_damage` (52) over
  `tf_combatmed_rifle_effect_time` (0.59 s) as damage over time; hurt teammate `tf_combatmed_rifle_heal` (52) as
  healing over time (no overheal); enemy buildings the same damage over time. Full-health teammates and the shooter are
  passed through. Flat: `DMG_BULLET` without `DMG_USEDISTANCEMOD`; no random crits; no headshots.
- `CTFCombatMedRifle::FireProjectile` replaces the base gun's, so it repeats its per-shot bookkeeping
  (`RemoveProjectileAmmo`, `DoFireEffects`, `UpdatePunchAngles`); without it shots used no ammo.

Shared hero systems (any hero can use them):
- Cast ability weapons: `CTFCastAbilityWeapon` (`tf_weapon_cast_ability.cpp/.h`). Switch to it -> cast -> effect ->
  cooldown -> back to the primary. Subclasses give `GetCastTime`, `GetCooldown`, `OnCastFinished`, and optionally
  `CanUseNow`, `IsInstantSwitch`, `GetCastActivity`, `OnCastStarted`. The Field Rations use it.
- Heal modifiers: no-heal and heal boost windows (`tf_heal_modifiers.cpp/.h`, `HealModifiers_Apply*`). Applied in
  `CTFPlayer::TakeHealth`, the healer-list tick in `CTFPlayerShared` (this frame's healing only) and class regen;
  healing credit is the healing actually done. Networked end times (for tints); cleared on spawn.
  Test: `tf_heal_modifier_test noheal <s> | boost <s> <scale> | clear` (cheat).
- Damage over time: `DamageOverTime_Apply` (`server/tf/tf_damage_over_time.cpp/.h`), hits every
  `tf_damage_over_time_interval` (0.1 s), normal damage path, no knockback. Test: `tf_damage_over_time_test`.
  `HealOverTime_Apply` (same file) heals the same way through `TakeHealth` (heal modifiers, no overheal), credits the
  healer (`Event_PlayerHealedOther`: scores, ultimate charge) and fires `player_healed`.
- Defense buff resistance per application: `CTFPlayerShared::SetDefenseBuffResistance` (server; < 0 = TF2's 35% / 50%
  vs sentries), used by `ApplyOnDamageModifyRules`, reset when a defense buff is removed. Test: `tf_defensebuff_test`.
- Teammate-hitting shots: `CTraceFilterSkipFullHealthTeammates` / `OWTF2_IsHurtTeammate` (`tf_teammate_trace.h`).
  TF2 bullets already stop at teammates; the filter lets full-health ones through. Weapons return true from
  `CTFWeaponBase::HitsTeammates()` to have teammates lag-compensated (not observable with standing bots).
- Alternating melee swings: `CTFAlternatingSwingMelee` (`tf_weapon_alternating_melee.cpp/.h`). Left first; the next
  swing goes right only if it starts on the tick the last one allows (held or buffered press), else left again. Per
  side: viewmodel sequence by name, third-person gesture per stance (`GetSwingAnims`). Used by the Crusader's hammer.
  Optional sweep hit (`GetSweepParams`): N rays across an arc from the camera, one at a time through the swing, in the
  swing's direction; the space between rays is filled; each enemy player or building hit once per swing; server side,
  lag compensated, through `OnSwingHit` (made protected in `tf_weaponbase_melee.h`). Debug: `tf_alternating_swing_debug 1`.
- `Ultimate_InterruptCast` ends an ultimate that's still casting; the charge is lost. Test: `tf_ultimate_interrupt_test`.

Ability conventions:
- Logic goes in `src/game/shared/tf/tf_veteran_abilities.cpp/.h` (shared and predicted).
  State goes in `CTFPlayerShared` as networked, predicted vars (`DT_TFPlayerSharedLocal`
  plus a `DEFINE_PRED_FIELD`). Only predicted state gets a `DEFINE_PRED_FIELD`: a var only the
  server writes (like the ultimate's charge, active flag and cast end time) must not have one. Otherwise each
  change is a prediction error that forces a full re-prediction, which shows as movement jitter.
  Check with `cl_showerror 2`.
- Every tunable number is a replicated convar named `tf_veteran_*`
  (`FCVAR_REPLICATED | FCVAR_NOTIFY`), so it can be tuned without a rebuild.
- Input: `+attack4` (`IN_ATTACK4`, bit 26) is the class ability key, bound to SHIFT
  by default. Add another `+attackN` only when a new key is needed.
- HUD must match stock TF2 elements: cooldown meters look like the Mad Milk and
  Jarate meters. The ultimate bar mirrors the Ubercharge meter, including its
  effects when fully charged.

Done: Sprint (toggle on `+attack4`, `tf_veteran_sprint_speed_mult` = 1.5). It also ends when forward movement
stops: no forward input (anywhere), or on the ground with forward speed under `tf_veteran_sprint_blocked_speed`
(walking head-on into a wall). Sliding along a wall and being in the air keep it on. `Veteran_SprintPostMove`,
called from `CTFGameMovement::PlayerMove`. Firing the rifle and using any ability end it too: new abilities
must call `Veteran_StopSprint` when used.
Done: primary weapon `tf_weapon_veteran_rifle` (`CTFVeteranRifle`, `TF_WEAPON_VETERAN_RIFLE`), item 60000.
It subclasses the SMG (model, sounds and animations are still the SMG's). Base damage 14. Stats in
`tf_weapon_veteran_rifle.txt`: spread 0, clip 30, `TimeFireDelay` 1/9 s, `TimeReload` 1.5; reserve 90 is `tf_ammo_primary`
in `veteran.txt`. Code:
- Exact 9 shots/s: `PrimaryAttack` schedules the next shot from the previous one's due time while the trigger is
  held (TF2 schedules from the current tick, which rounds 1/9 s up to 8 ticks, 8.3 shots/s).
- Reload: `DefaultReload` uses the script's `TimeReload` instead of the animation length (`SetReloadTimer`, the same
  entry point reload-speed attributes use), and stretches the animation to fit. It keeps `m_flReloadPriorNextFire`
  from the base reload: firing with rounds in the clip cancels a reload (`CTFWeaponBase::ItemBusyFrame`) once that
  time passes, and a second `SetReloadTimer` call would overwrite it with the reload's end.
- Empty rifle: `HasAnyAmmo` is always true (helix needs no ammo), so with no clip or reserve it isn't auto-switched
  away from (that would cast the Field Rations), can still be drawn, and the Field Rations can switch back to it.
- Falloff: `CTFVeteranRifle::GetDistanceModCenter`, called for the rifle's bullets from the distance modifier in
  `ApplyOnDamageModifyRules`: TF2's close-range bonus up to `tf_veteran_rifle_falloff_start` (512, full damage there),
  down to TF2's minimum (-50%) at `tf_veteran_rifle_falloff_end` (640). Measured with the old base 8: 11 at 256, 8 at 512, 5 at 576, 4 from 640 (base 14 scales the same way).
- Recoil (client only, the view angles are the client's): `m_iBurstShots` counts shots while the trigger is held.
  From shot `tf_veteran_rifle_recoil_start_shot` (6) on, each shot climbs the view `tf_veteran_rifle_recoil_climb`
  (0.392 degrees); a full clip climbs 9.8. Applied in `C_TFPlayer::CreateMove` (`VeteranRifleRecoil_CreateMove`).
  Pulling down pays the recoil off. After `tf_veteran_rifle_recoil_recovery_delay` (0.15 s) without a shot (keep it above the 1/9 s between shots), what's
  left recovers at `tf_veteran_rifle_recoil_recovery_rate` (15 degrees/s), paused while there's vertical input;
  horizontal input doesn't pause it. Testing: VScript can't move the mouse; keyboard look (`+lookdown`) only works
  with `cl_mouselook 0`, which can only be set at launch (`+cl_mouselook 0`) and is saved to config.cfg.
Done: ultimate system (reusable by any class), `src/game/shared/tf/tf_ultimate.cpp/.h`.
- Per class: `"ultimate_cost"` (0 = no ultimate), `"ultimate_duration"` (seconds to drain; 0 = instant) and
  `"ultimate_cast_time"` (seconds from the key to the effect; 0 = immediate) in `scripts/playerclasses/<class>.txt`.
  Veteran: 2310, 6 and 1.2. During the cast the ultimate is active (`Ultimate_IsCasting`) but doesn't drain; after it,
  `Ultimate_IsEffectActive`. Cast end time: `m_flUltimateCastEndTime`, networked.
- Charge: passive `tf_ultimate_charge_per_second` (5) once the round is running and setup is over;
  damage to enemy players, not buildings (`tf_ultimate_charge_per_damage`; overheal portion
  x `tf_ultimate_overheal_damage_scale` 0.5); healing (`tf_ultimate_charge_per_heal`, through
  `CTFGameStats::Event_PlayerHealedOther`; health packs credit their dropper through `Event_PlayerHealedOtherAssist`,
  which gives no charge, and a self pickup credits nobody). Capped at the cost. Nothing builds while active, from
  self or team damage, or from damage by an ultimate (active, or an entity passed to `Ultimate_MarkEntity`).
- Kept through death; reset on round start (`teamplay_round_start` listener) and class change.
  Dying ends an active ultimate.
- Use: `+attack5` (`IN_ATTACK5`, bit 27), default key Q (replaces TF2's default Q = lastinv).
  Activation is decided by the server. Per-hero hooks, dispatched by class in `tf_ultimate.cpp`: can use
  (`Veteran_CanUseUltimate`), cast start (`Veteran_OnUltimateStart`), cast end (`Veteran_OnUltimateCastEnd`), end.
- HUD: `CHudUltimateMeter` (`client/tf/tf_hud_ultimate.cpp`, `resource/ui/HudUltimate.res`), shown for any
  class with an ultimate. It sits in the Medic meter's spot (bottom right, r138/r69). While it shows, the ammo counter
  (`CTFHudWeaponAmmo`, `tf_hud_ammostatus.cpp`) moves up to sit on top of it, and the item effect (cooldown) meters
  line up in a row between them, right-aligned with the ultimate meter, the first declared meter on the right
  (`CItemEffectMeterManager::PositionAboveUltimateMeter`, `tf_hud_itemeffectmeter.cpp`). It pulses like
  Ubercharge when full and while active.
- Self-healing: healing yourself with an ability (the healing field, the Biotic Grenade) is fine and earns ultimate
  charge. The "self-healing" concern was Overwatch's out-of-combat regeneration, which isn't in TF2 or planned.
- Voice lines: `Ultimate_PlayVoiceLine` (heard within a radius, through walls, no falloff, at a pitch); used by the
  Veteran and the Combat Medic.
- Testing: `tf_ultimate_charge_set <points>` (cheat).
Done: helix rockets, the rifle's alt-fire (`+attack2`, MOUSE2). `CTFVeteranRifle::SecondaryAttack` fires
`tf_projectile_helix_rocket` (`tf_projectile_helix_rocket.cpp/.h`, a copy of the sentry rocket: model,
zero-size hull, radius 146; drawn at half size, `tf_veteran_helix_scale`; speed `tf_veteran_helix_speed` 1968, Overwatch's
50 m/s). Its trail is `helix_rockettrail`, a half-size copy of TF2's `rockettrail` (see Particles below). Launch
sound at 75% volume (`tf_veteran_helix_sound_volume`). Base damage 85 (`tf_veteran_helix_damage`; the sentry rocket's is 100). It's aimed like the Rocket Launcher
(`GetProjectileFireSetup`, same 23.5/12/-3 offset). The cooldown `tf_veteran_helix_cooldown` (6 s) is shown by
the item effect meter (the Jarate's meter, label "HELIX", recharge beep). Damage is `tf_veteran_helix_damage`, blast type
(`DMG_BLAST | DMG_USEDISTANCEMOD`: `CTFProjectile_HelixRocket::GetDamageType` swaps the bullet flag it would get from the rifle's weapon ID).
Helix cast: after a launch the rifle can't fire or reload for `tf_veteran_helix_cast_time` (0.5 s; `m_flHelixCastEndTime`,
networked and predicted; `ItemPostFrame` returns early and `PrimaryAttack` is guarded). Helix interrupts a reload: while
reloading the weapon only gets busy frames, so `CTFVeteranRifle::ItemBusyFrame` drops the reload and launches. Starting
the ultimate cancels the cast (`CancelHelixCast`, from `Veteran_OnUltimateStart`).
Done: rifle headshots minicrit (bullets only, Veteran rifle only). The head hit is registered like the Ambassador's,
from the bullet's hitbox (`LastHitGroup`, set by `CTFPlayer::TraceAttack`), not with `DMG_USE_HITLOCATIONS` (that path
gives full crits). The rule is one `// OWTF2:` case in the minicrit chain of `CTFGameRules::ApplyOnDamageModifyRules`
(`tf_gamerules.cpp`). It checks the inflictor is the player, because helix rockets also report the rifle as their weapon.
Done: the Veteran's ultimate.
- Q starts a 1.2 s cast. During it the rifle can't fire, use helix or reload (`ItemPostFrame` / `ItemBusyFrame`
  return early; a reload in progress is dropped), the rifle can't be put away (`CanHolster`), the Field Rations
  can't be selected, and Sprint can't be toggled (and turns off). It can't be used mid-Field-Rations cast.
- Cast start: a PLACEHOLDER voice line, `vo/sniper_battlecry05.mp3` (`VETERAN_ULTIMATE_VOICE_LINE`), played to every
  player within `tf_veteran_ultimate_voice_radius` (2048) of the Veteran, through walls, with no distance falloff
  (`SNDLVL_NONE`, like the announcer), at pitch `tf_veteran_ultimate_voice_pitch` (85%, which also slows it).
- Cast end: the clip refills for free (the reserve isn't used). Then, for the 6 s drain: reload
  `tf_veteran_ultimate_reload_time` (0.5 s), and the rifle's bullets aim themselves (`CTFVeteranRifle::FireBullet`,
  `FindAutoAimTarget`) at the nearest living enemy player within `tf_veteran_ultimate_aim_range` (2048), inside
  `tf_veteran_ultimate_aim_cone` (45 degrees full angle, 22.5 each side of the crosshair), in line of sight. Never
  buildings; skips the spies a sentry skips (mostly invisible, disguised as our team or as a dispenser); line of sight
  with the sentry's mask. Aims at the body, else the head, else the legs. The target is picked with lag compensation,
  then the aimed angles go to `FX_FireBullets`. Helix rockets aren't aimed; recoil still climbs. The rifle's bullets
  have no falloff (the close-range bonus stays): an `// OWTF2:` check after `GetDistanceModCenter` in
  `ApplyOnDamageModifyRules` keeps the center at 0.5 or above. Tested: 14 at 700 units (7 without the ultimate), 19 at 256.
- The Wrangler only points a sentry at the crosshair; target selection comes from the sentry
  (`CObjectSentrygun::ValidTargetPlayer`, nearest target).
Done: healing field, the secondary weapon `tf_weapon_veteran_field_rations` (`CTFVeteranFieldRations`,
`TF_WEAPON_VETERAN_FIELD_RATIONS`), item 60001, Jarate model placeholder. Switching to it (key 2) casts it
(`tf_veteran_field_cast_time` 0.6 s; the player can't switch away mid-cast). Then it places a `tf_veteran_healing_field`
(`tf_veteran_healing_field.cpp/.h`), starts the cooldown (`tf_veteran_field_cooldown` 15 s, the FIELD meter,
`HudItemEffectMeter_VeteranField.res`, left of HELIX) and switches back to the rifle. It can't be selected on cooldown.
The field has no collision and uses the small health pack model. It's placed `tf_veteran_field_offset` (16) units ahead
of the feet, traced down to the ground. Placeholders copy the dispenser: its area (a box of +-`tf_veteran_field_radius`
128, twice the dispenser's, around a center `tf_veteran_field_center_height` 32 above the ground, touching the player's bounding box),
line of sight, team and disguise rules, and the level 2 heal rate (`tf_veteran_field_heal_rate` 15 HP/s, no overheal).
It lasts `tf_veteran_field_duration` (5 s). Effects: Quick-Fix ÜberCharge ring `medic_megaheal_red/blue` on the floor,
dispenser beams `dispenser_heal_red/blue` to each healed player (networked as a player bitmask).

## Known issues to revisit (numbers and interactions; functionality comes first)
Still to decide (TBA):
- Helix damage follows TF2's normal distance modifier from the shooter (+-50% around 512 units:
  about 128 point-blank, 43 far away with base 85; a direct hit at 512 measured 85). Sentry rockets measure from the Engineer, and the Rocket
  Launcher gets half the close-range bonus. Decide the rule, or turn it off (no DMG_USEDISTANCEMOD).
- Nano Boost: no feedback when Q finds no teammate.
- Effects skipped for now (noted by the user): a sleep effect instead of TF2's stun stars, a Nano Boost effect on
  the target, the Biotic Grenade's own splash and trail.
- Placeholders: helix sound (Building_Sentrygun.FireRocket) and third-person attack gesture; the healing field's
  ring (the Quick-Fix ring's size is fixed by the particle, so it doesn't exactly match the area), no heal or cast
  sound, the Jarate's draw animation; every Combat Medic model, sound and icon (original art comes later).
Decided (keep as is):
- Helix rockets damage and push the Veteran like Soldier rockets (a point-blank test hit took 125 to 118).
- Healing field: it heals the Veteran too, and that self-healing builds ultimate charge (1 point per HP healed,
  teammates and self alike). Self-healing through abilities is fine and counts.
- Rifle damage-over-time hits (about 6 per shot) give hit sounds and damage numbers like the Pyro's flamethrower:
  merged or one by one by the player's settings (`hud_combattext_batching`, `tf_dingalingaling_repeat_delay`).
Testing note: `hud_fastswitch 0` (bucket mode) needs a click after pressing 2, like any stock weapon.

## Particles
- Mod particle files go in `game/mod_tf/particles/` and must be listed in `game/mod_tf/particles/particles_manifest.txt`.
  That file is a copy of TF2's manifest (from `tf2_misc_dir.vpk`) plus the mod's entries, because it replaces TF2's.
  Refresh it when TF2 adds particle files.
- Source 2013 can't scale a particle effect at runtime. To make a resized copy: extract the .pcf with TF2's
  `bin/x64/vpk.exe`, convert it to text with `bin/x64/dmxconvert.exe -oe keyvalues2`, copy the systems under new
  names and new element ids, change the values, and convert back with `-oe binary`.
- `helix_rockettrail.pcf`: `helix_rockettrail`, `_burst` and `_fire`, TF2's rocket trail with every size value halved
  (base radius, random radius, spawn distances, offsets, drift and spread speeds; the radius-scale multipliers are
  unchanged).

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
  `item_slot` match. The Veteran has a primary and a secondary (no melee), so its loadout page
  shows those two slots (`g_LoadoutPanelPositioning_Veteran`, `class_loadout_panel.cpp`).

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

## The mod's classes
`TF_CLASS_VETERAN` (10), `TF_CLASS_COMBATMED` (11) and `TF_CLASS_CRUSADER` (12) in `ETFClass`,
`src/game/shared/tf/tf_shareddefs.h`, right after Engineer. Civilian (now 13) is still `TF_LAST_NORMAL_CLASS`;
Random is now 15. `docs/veteran-class-changes.txt` lists every file touched and why (section 7 for the Combat
Medic, 15 for the Crusader).
- Combat Medic: `combatmed.txt` (Medic model and sounds, 175 HP, speed 280), menu slot 11, Medic art
  placeholders (`GetPlaceholderContentClass` in `tf_classmenu.cpp`). No weapons or ultimate yet.
- Crusader: `crusader.txt` (Demoman model and sounds, 385 HP, speed 280), menu slot 12, Demoman art placeholders.
  Only the hammer so far, no abilities or ultimate. Its chalkboard tips are `ClassTips_crusader_*`: stock Random's are `ClassTips_12_*`,
  so mod classes numbered 12 and up are keyed by raw name (`GetClassTipsKey`, `tf_classmenu.cpp`).
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
- Veteran placeholders: Sniper model, sounds and art.

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
