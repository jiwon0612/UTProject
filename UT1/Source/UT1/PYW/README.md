# PYW structure

PYW follows the project-wide feature grouping used by `CJW` and
`Variant_TwinStick`:

```text
PYW/
├── AI/        Enemy Behavior Tree controller and custom BT nodes
├── Entities/  Shared enemy character plus melee/ranged/brute/assassin/bomber archetypes
├── Gameplay/  Enemy projectile behavior
└── Player/    WASD test GameMode and PlayerController
```

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
for `LoseSightTime`; the last seen location then drives Search. Taking damage
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

Locomotion uses `Content/PYW/BS_EnemyLocomotion`. The first three melee/ranged
patterns use `AN_MeleeAttack_01..03` and `AN_RangedQuickCast`/`Cast`/`PowerCast`;
the other patterns and archetypes use the Mannequin animations set in C++.
Hit reactions use the Mannequin rifle HitReact clips. Death picks front/back/
left/right by damage direction, with `AN_EnemyDeath` as the front clip.
The legacy `AttackAnimations` array is no longer editable; values saved in older
Blueprints are copied into the leading patterns on load and at BeginPlay.
`BP_OnAttack` remains an extension point for weapon trails, sounds and VFX.

Manny mesh rotation is Pitch=0, Yaw=-90, Roll=0. Unreal Python Rotator positional
arguments do not follow C++ FRotator order; always use named arguments in scripts.
Attack facing changes only actor Yaw, including when a target is above/below it.

After building UT1Editor, run Content/PYW/setup_enemy_archetypes.py to create
BP_BruteEnemy/BP_AssassinEnemy/BP_BomberEnemy and place them in the test level.
Content/PYW/repair_enemy.py updates the existing BP and loaded test enemies
without recreating the map. The full create_enemy_test_content.py script
replaces the test map and should only be used when intentionally rebuilding it.
