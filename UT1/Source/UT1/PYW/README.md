# PYW structure

PYW uses the same feature folders as `CJW` (`Entities/`, `Weapons/`, `Combat/`);
folders CJW has no counterpart for keep their own name:

```text
PYW/
├── AI/        Enemy Behavior Tree controller and custom BT nodes
├── Combat/    Combat helpers shared by enemies and projectiles (EnemyEffects)
├── Editor/    Editor-only helpers (UPYWEditorLibrary) for the PYW Python scripts
├── Entities/  Shared enemy character plus melee/ranged/brute/assassin/bomber archetypes
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
(`AreaRadius`), hit several times (`HitCount`), fire several projectiles
(`ProjectilesPerHit`/`SpreadAngle`), knock back, or consume the attacker.

Damage resolves after each pattern's `ImpactDelay`. Melee re-checks range and
a frontal `StrikeHalfAngle` arc at impact, so backing off or sidestepping during
the wind-up avoids the hit. With `bShowAttackDebug`, area attacks draw their
landing circle during the wind-up and state changes ("!", stagger, enrage,
pattern name) are drawn above the enemy.

# Loot

`AEnemyCharacter` owns CJW's `UUT1LootDropComponent` (`LootDrop`), so every enemy
drops loot through the shared `OnDied` rule without enemy-specific code. The
per-enemy tables live in `setup_monster_enemies.py` (`SMALL_LOOT`, `LARGE_LOOT`):
scrap/wire/cloth with chance and count ranges, and a 13% chance per weapon
blueprint. Only one blueprint drops at a time and already unlocked blueprints
(the starting katana) are skipped.

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
  clips, three hackNSlash sword clips from CJW (read-only) are retargeted from
  their own `SKM_Manny_Simple`: `Anim_Combo_2_Br_4` (melee lunge thrust),
  `Anim_Combo_1_Br_3` (ranged sweep for the spread volley) and
  `Anim_Combo_6_Br_2` (brute leap slam). Their pelvis travels far, while the
  C++ lunge already moves the actor, so `IN_PLACE` clips keep the pelvis X/Y at
  the first frame (vertical motion such as the jump is kept).
- The retargeter copies source root motion onto the monster `Root` (Mannequin
  `MM_Attack_01` moves it ~1.4m forward). Enemies move only through character
  movement, so every clip gets its `Root` locked to the reference pose;
  otherwise the mesh snaps back when an attack ends. The setup fails if any
  clip still has root drift.
- There is no unarmed cast clip, so ranged attacks use the pistol/rifle aim
  poses without a weapon (arms pushed forward). These are long loops, so
  `FEnemyAttackPattern::AnimationDuration` plays them only until just after the
  last shot. Repel Nova uses `MM_ChargedAttack` (charge, then release).
- Hit reactions are retargeted rifle HitReact clips, so the arms briefly take a
  rifle-holding shape. Replace them when unarmed hit clips are available.

Run order after building UT1Editor (both scripts are re-runnable):

1. `Content/PYW/setup_monster_animations.py` - IK Rigs, Retargeters, retargeted
   clips and locomotion BlendSpaces.
2. `Content/PYW/setup_monster_enemies.py` - BP meshes/animations, projectile VFX,
   test enemies in `Lvl_EnemyBTTest`, and removal of leftover redirectors.

The ProjectileVFX systems are self-contained projectiles: the head particle has
its own velocity, drag and collision, so attaching them to the actor makes the
effect fly apart from the real projectile. The flight visual is therefore made
of actor components only: a shadowless glowing core (`M_EnemyProjectileCore`) at
the collision size and an additive cone tail (`M_EnemyProjectileTail`,
`TailLength`) pointing back along the path. `NS_VFX_Explosion` is still used for
the impact, which plays in place; `ImpactDisabledEmitters` keeps only its
`Explosion_*` emitters.

Spawning actors from Python crashes under `-nullrhi`; run the second script
with `-RenderOffscreen`.

The legacy `AttackAnimations` array is no longer editable; values saved in older
Blueprints are copied into the leading patterns on load and at BeginPlay.
`BP_OnAttack` remains an extension point for weapon trails, sounds and VFX.

Mesh rotation is Pitch=0, Yaw=-90, Roll=0 for both Manny and the monsters. Unreal
Python Rotator positional arguments do not follow C++ FRotator order; always use
named arguments in scripts. Attack facing changes only actor Yaw, including when
a target is above/below it.
