# PYW structure

PYW follows the project-wide feature grouping used by `CJW` and
`Variant_TwinStick`:

```text
PYW/
├── AI/        Enemy Behavior Tree controller and custom BT nodes
├── Entities/  Shared enemy character plus melee/ranged archetypes
├── Gameplay/  Enemy projectile behavior
└── Player/    WASD test GameMode and PlayerController
```

# Enemy structure

`AUT1Entity` is the shared character base used by `AUT1Player` and now
`AEnemyCharacter`. The existing Entity currently supplies no health/combat API.

- `EnemyCharacter`: enemy settings, movement speed by state, shared 3D attack
  range check, cooldown enforcement, health, damage and death handling.
- `MeleeEnemyCharacter`: medieval close-range archetype with direct damage and
  a looping three-animation attack combo.
- `RangedEnemyCharacter`: ranged archetype that stops at range and launches
  `EnemyProjectile`; its attack animation is a charged casting motion.
- `EnemyAIController`: owns Blackboard and the runtime Unreal Behavior Tree.
- `EnemyBTNodes`: target acquisition and Attack > Chase > Walk/Idle selection.
  Tasks delegate attack rules and speed selection to the character.

The BT is built at runtime; there is no editable BT asset in Content.
Locomotion uses `Content/PYW/BS_EnemyLocomotion`. Melee attacks cycle through
`AN_MeleeAttack_01..03`, while ranged attacks use `AN_RangedQuickCast`,
`AN_RangedCast` and `AN_RangedPowerCast`.
Lethal damage stops the Behavior Tree, movement and collision, plays
`AN_EnemyDeath`, then removes the actor after the configured cleanup delay.
`BP_OnAttack` remains an extension point for weapon trails, sounds and VFX.

Attacks are data in `AttackPatterns` (`FEnemyAttackPattern`) and cycle in array
order without changing the shared Behavior Tree. Each pattern holds its
animation, damage, cooldown, `ImpactDelay`, `HitCount`/`HitInterval`, ranged
`ProjectilesPerHit`/`SpreadAngle` and melee knockback. Constructor defaults:

- Melee: slash, double strike (`HitCount` 2), heavy smash with knockback.
- Ranged: magic bolt, triple burst (`HitCount` 3), spread volley (5 projectiles, 24°).

Tune them per Blueprint in the `AttackPatterns` array. The legacy
`AttackAnimations` array is no longer editable; values saved in older Blueprints
are copied into the matching patterns on load and at BeginPlay.

Damage resolves after each pattern's `ImpactDelay` instead of at attack
start. Melee re-checks range and a frontal `StrikeHalfAngle` arc at impact, so
backing off or sidestepping during the wind-up avoids the hit. Ranged enemies
re-aim at impact. While cooling down, the attack task turns toward the target
and falls back to Chase once the target leaves range. Taking damage from a
non-enemy pawn sets it as the target, and death clears pending attack timers.

Manny mesh rotation is Pitch=0, Yaw=-90, Roll=0. Unreal Python Rotator positional
arguments do not follow C++ FRotator order; always use named arguments in scripts.
Attack facing changes only actor Yaw, including when a target is above/below it.

After building UT1Editor, run Content/PYW/repair_enemy.py using the Unreal Python
commandlet to update the existing BP and loaded test enemies without recreating
the map. The full create_enemy_test_content.py script replaces the test map and
should only be used when intentionally rebuilding it.
