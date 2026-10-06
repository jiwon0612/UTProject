# PYW structure

PYW uses the same feature folders as `CJW` (`Entities/`, `Weapons/`, `Combat/`);
folders CJW has no counterpart for keep their own name:

```text
PYW/
├── AI/        Enemy Behavior Tree controller and custom BT nodes
├── Combat/    Combat helpers shared by enemies and projectiles (EnemyEffects)
├── Editor/    Editor-only helpers (UPYWEditorLibrary) used by the former PYW setup scripts
├── Entities/  Shared enemy character plus melee/ranged/brute/assassin/bomber/guardian/artillery archetypes
└── Weapons/   Enemy projectile
```

`Entities/EnemyCharacter.h` is included by LSW rooms, so its path is kept stable.

`Lvl_EnemyBTTest` uses CJW's `BP_DavGameMods`, so enemies are tested against the
real `BP_Player` combat rules instead of a separate test pawn.

# Enemy structure

`AUT1Entity` is the shared character base used by `AUT1Player` and now
`AEnemyCharacter`. The existing Entity currently supplies no health/combat API.

- `EnemyCharacter`: enemy settings, movement speed by state, attack pattern
  selection, lunge/area/multi-hit resolution, poise/stagger, enrage, health,
  damage and directional death.
- `MeleeEnemyCharacter`: soldier. Strafes during cooldown; slash, double strike,
  heavy smash (super armor, knockback) and a gap-closing lunge slash.
- `RangedEnemyCharacter`: caster. Kites (backs off inside `RetreatDistance`);
  magic bolt, triple burst, spread volley and a close-range repel nova.
- `BruteEnemyCharacter`: 1.3x scale, 320 HP, hard to stagger. Holds ground;
  smash, ground slam (area), mid-range charge, and a leap slam only when enraged
  below 40% HP.
- `AssassinEnemyCharacter`: 0.92x scale, 65 HP, fast reaction and strafe;
  quick stab, three-hit flurry and a lunge stab.
- `BomberEnemyCharacter`: 0.8x scale, 40 HP, sprints in and self-destructs after
  a 1.1s fuse. Any stagger cancels the fuse.
- `GuardianEnemyCharacter`: shield guard, 1.1x scale, 150 HP, turns slowly. While not
  attacking it takes only 20% of damage coming from its front 65 degrees, so the
  player flanks it or punishes its swings; shield bash, double bash, heavy chop and a
  mid-range shield charge.
- `ArtilleryEnemyCharacter`: 0.95x scale, 70 HP, kites at long range (1300). Shells a
  circle marked at the target's feet (lands 1.6s later), a scattered 4-shell barrage,
  and a close-range blast that pushes the target away.
- `EnemyAIController`: owns Blackboard and the runtime Unreal Behavior Tree.
- `EnemyBTNodes`: target acquisition/loss and branch selection. Tasks delegate
  attack rules and speed selection to the character.

The BT is built at runtime; there is no editable BT asset in Content:

```text
Root (FindTarget service)
├── Stagger     [IsStaggered]          wait out the hit reaction
├── Alert       [IsAlerting]           face the newly spotted target for ReactionTime
├── Attack      [CanStartAttack]       weighted pattern, waits for swing/lunge to finish
├── Reposition  [cooldown & in band]   HoldGround / Strafe / Kite until cooldown ends
├── Chase       [HasTarget]
├── Search      [LastKnownLocation]    walk there, look around for SearchDuration
└── Walk / Idle
```

The target is dropped when it leaves `LoseTargetRange` or stays out of sight
for `LoseSightTime`; the last seen location then drives Search. A dead target
(`AEnemyCharacter::IsValidCombatTarget`) is dropped together with its last known
location, so enemies return to patrol instead of attacking or searching a corpse. Taking damage
from a non-enemy pawn sets it as the target immediately. Heavy hits accumulate
poise damage; crossing `PoiseThreshold` interrupts the current attack (unless
the pattern has `bSuperArmor`) and restarts the tree into Stagger.

Attacks are data in `AttackPatterns` (`FEnemyAttackPattern`). Each attack picks
randomly by `Weight` among patterns whose `MinRange`/`MaxRange` (capsule-surface
gap), `PatternCooldown` and `bEnragedOnly` allow it; the previous pattern is
down-weighted. `Cooldown` is rest time after the attack finishes, jittered by
`AttackCooldownVariance`. Patterns can also lunge (`LungeDelay`/`LungeSpeed`/
`LungeLift`, speed is capped so the enemy lands at the target), hit an area
(`AreaRadius`; with `bAreaAtTarget` the circle is fixed at the target's feet when the
attack starts, and `AreaScatter` spreads later hits around it; `LobLaunchTime` throws a
visual-only `LobProjectileClass` shell whose launch velocity is solved so it lands in
the circle exactly at the hit time), hit several times (`HitCount`), fire several projectiles
(`ProjectilesPerHit`/`SpreadAngle`), knock back, or consume the attacker.

Enemies never hurt each other: `AEnemyCharacter::TakeDamage` drops damage whose
causer, causer's instigator or instigating controller is another enemy (self-damage
such as the self-destruct and the death test still goes through). Enemies also use
RVO avoidance on their CharacterMovement, so a group chasing one target steers
around each other instead of shoving capsules (bodies in contact dropped from 9.6%
to 1.5% of enemy pairs in the test level).

An attack that has started always plays to the end; only stagger cancels it.
Enemies stand still while attacking, and a lunge stops dead when it lands
(`Landed`) instead of sliding through the rest of the motion.

Damage resolves after each pattern's `ImpactDelay`. Melee re-checks range and
a frontal `StrikeHalfAngle` arc at impact, so backing off or sidestepping during
the wind-up avoids the hit. With `bShowAttackDebug`, area attacks draw their
landing circle during the wind-up and state changes ("!", stagger, enrage,
pattern name) are drawn above the enemy.

# Damage numbers and critical hits

Damage numbers come from CJW: `AUT1Entity::TakeDamage` asks
`UUT1CombatFeedbackSubsystem` to show every damage that actually landed, so enemies
get numbers when the player hits them and the player gets numbers when an enemy hits,
with no enemy-side code. Enemy attacks can also crit: `CriticalChance` (10%) and
`CriticalMultiplier` (1.5x) are rolled per strike, and per projectile for ranged shots
(`AEnemyProjectile::DamageTypeClass`). A crit is sent as CJW's
`UUT1DamageType_Critical`, which is how the number knows to show as critical; the
melee hit flash is also 1.5x larger on a crit.

# Level

`EnemyLevel` (1 = the values written in C++/BP) scales an enemy: each level adds
`HealthPerLevel` (20%) to max health and poise threshold, and `DamagePerLevel` (12%)
to attack damage. The damage multiplier is applied once when a pattern is copied
into `ActivePattern`, so melee, area and projectile hits all follow it.
`SetEnemyLevel` can be called after spawning and keeps the current health ratio.

LSW rooms drive it: `AUT1_RoomManager` sets `AUT1_RoomBase::EnemyLevel` from the
room ID before `SetupRoom` (every `RoomsPerEnemyLevel` rooms = +1, default 3), and
the normal/mid-boss rooms call `SetEnemyLevel` on what they spawn.

# Loot

`AEnemyCharacter` owns CJW's `UUT1LootDropComponent` (`LootDrop`), so every enemy
drops loot through the shared `OnDied` rule without enemy-specific code. The
per-enemy tables are set on each enemy Blueprint's `LootDrop`: scrap/wire/cloth
with chance and count ranges, and the weapon blueprints. Weapon data doubles as
the crafting recipe, so a weapon has to be unlocked by picking up its blueprint
before the workbench can make it. Each of the five `MeleeWeapon/DA_Weapon_*`
blueprints (all but the starting katana) drops at 3%, about 14% per kill in total.
Only one blueprint drops at a time and already unlocked blueprints are skipped.

# Monster visuals

C++ defaults still point at Mannequin animations; the Blueprints override them
with Dungeon Pack monster meshes (`Content/CJW/.../Monsters`, read-only here).

```text
Content/PYW/
├── BluePrint/   BP_MeleeEnemy, BP_RangedEnemy (SK_Monster_Small),
│                BP_LargeEnemy (SK_Monster_Large, parent BruteEnemyCharacter),
│                BP_EnemyProjectile (NS_VFX_Evil trail, NS_VFX_Explosion impact)
├── Animation/   IK_Mannequin, IK_MonsterSmall/Large, RTG_MannyToMonsterSmall/Large
│   ├── Small/   SM_* retargeted clips + BS_MonsterSmall_Locomotion
│   └── Large/   LM_* retargeted clips + BS_MonsterLarge_Locomotion
└── ProjectileVFX/  imported Niagara pack
```

- Small and Large monsters share one Rigify-style hierarchy (`spine` = pelvis,
  `spine_003` = chest), so one chain table drives both IK Rigs. Chain names match
  `IK_Mannequin` so chains map with `AutoMapChainType.EXACT`.
- The monster `Root` bone carries a x100 scale from Blender. The IK Retargeter
  ignores bone scale, so the batch output gets `Root` scale 1 and a cm-unit pelvis
  translation, which shrinks the skinned mesh to 1/100. `restore_root_scale`
  writes the reference scale back and divides the pelvis translation by it.
- Locomotion is a simple 4-direction Walk(300)/Jog(600) BlendSpace on the axes
  `AEnemyCharacter::Tick` feeds (Direction -180..180, Speed 0..600). Setting
  `sample_data` from Python does not rebuild the runtime triangulation, which
  makes the BlendSpace evaluate to the T reference pose; the script calls
  `UPYWEditorLibrary::RebuildBlendSpace` (`UBlendSpace::ResampleData`).
- Attack patterns keep their C++ timing; the setup copies the native C++
  patterns into each Blueprint and maps each source Mannequin clip to the
  retargeted `SM_`/`LM_` clip with the same name, so C++ stays the single
  source of tuning.
- Every pattern of an enemy uses a different motion. Besides the Mannequin
  clips, hackNSlash sword clips from CJW (read-only) are retargeted from their
  own `SKM_Manny_Simple` (list in `SOURCES`). Their pelvis travels far, while the
  C++ lunge already moves the actor, so `IN_PLACE` clips keep the pelvis X/Y at
  the first frame (vertical motion is kept).
- Motions stay plain on purpose. Sword clips that spin the whole body (measured
  as large accumulated pelvis yaw, e.g. `Anim_Combo_3_Br_1`, `Anim_Combo_7_Br_3`,
  `Anim_Combo_6_Br_2`) read as a jump-spin rather than an attack, so the artillery
  shell/barrage, guardian double bash and brute leap slam use Mannequin clips
  (`MM_ChargedAttack`, rifle aim pose, `MM_Attack_02`) instead.
- The retargeter copies source root motion onto the monster `Root` (Mannequin
  `MM_Attack_01` moves it ~1.4m forward). Enemies move only through character
  movement, so every clip gets its `Root` locked to the reference pose;
  otherwise the mesh snaps back when an attack ends. The setup fails if any
  clip still has root drift.
- There is no unarmed cast clip, and gun aim poses read as shooting, so ranged
  attacks (and the artillery barrage) cut the 0.70-1.40s window out of
  `MM_Pistol_Equip`: the hands come together in front of the chest at 0.95s and
  push forward around 1.10s, which reads as a calm two-hand cast without the
  weapon. `AnimationStartTime` picks the window start, and `AnimationDuration`
  longer than the remaining clip holds the final pose (used while bursts fire).
- Projectiles and lob shells leave from `ProjectileSocket` (`hand_r` on both
  monster meshes) via `GetProjectileOrigin`, so they come out of the aiming hand.
- Hit reactions are retargeted rifle HitReact clips, so the arms briefly take a
  rifle-holding shape. Replace them when unarmed hit clips are available.

These assets were generated once by editor Python setup scripts
(`setup_monster_animations.py`, `setup_monster_enemies.py`), which were removed
after use and can be restored from git history if the pipeline has to be rerun.
The generated assets (IK Rigs, Retargeters, `SM_`/`LM_` clips, BlendSpaces, enemy
Blueprints, materials) are now edited directly in the editor. If C++ attack
patterns change, copy them into the enemy Blueprints by hand (the scripts used to
do this and remap each clip to its retargeted `SM_`/`LM_` version).

The ProjectileVFX systems are self-contained projectiles: the head particle has
its own velocity, drag and collision, so attaching them to the actor makes the
effect fly apart from the real projectile. The flight visual is therefore made
of actor components only, so it can never drift from the hit sphere:

- Core (`M_EnemyProjectileCore`): collision-sized sphere, white-hot center to a
  purple Fresnel rim, pulsing.
- Halo (`M_EnemyProjectileHalo`, `HaloScale`): larger additive sphere that fades
  toward its silhouette, so it reads as soft glow instead of a ball.
- Line, two cones pointing back along the path. Both fade from the core to the tip
  using local position over object bounds, and scroll a noise texture backwards:
  the wide purple outer tail (`M_EnemyProjectileTail`, `TailLength`,
  `TailWidthScale`) and a thin, longer white streak (`M_EnemyProjectileInnerTail`,
  `InnerTailLengthScale`/`InnerTailWidthScale`).
- Glow: shadowless point light that tints the floor under the projectile.

These are PYW's own simple materials (originally generated as node graphs by the
removed setup script), because the pack materials read Niagara particle colour and
render black on static meshes.

Hit feedback is `Combat/EnemyHitFlash` (`AEnemyHitFlash::Spawn`): a soft glowing
sphere, a ring (`SM_Torus`) and a point light that grow and fade over 0.25s,
tinted through `M_EnemyHitFlash`'s `Color` parameter. It fires on projectile
impact (projectile `GlowColor`, larger when a pawn is hit), on a landed melee
strike at the target's body (`HitFlashColor`/`HitFlashRadius`), and where an area
attack goes off (`AreaFlashColor`, size from `AreaRadius`). The ProjectileVFX pack
systems are not used for this: their emitters are chained through events, so a
system with any emitter disabled draws nothing (checked in PIE), and with all
emitters enabled it launches its own flying projectile.

Spawning actors from Python crashes under `-nullrhi`; run the second script
with `-RenderOffscreen`.

The legacy `AttackAnimations` array is no longer editable; values saved in older
Blueprints are copied into the leading patterns on load and at BeginPlay.
`BP_OnAttack` remains an extension point for weapon trails, sounds and VFX.

Mesh rotation is Pitch=0, Yaw=-90, Roll=0 for both Manny and the monsters. Unreal
Python Rotator positional arguments do not follow C++ FRotator order; always use
named arguments in scripts. Attack facing changes only actor Yaw, including when
a target is above/below it.
