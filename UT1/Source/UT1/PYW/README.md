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
`AN_MeleeAttack_01..03`, while ranged attacks use `AN_RangedCast`.
Lethal damage stops the Behavior Tree, movement and collision, plays
`AN_EnemyDeath`, then removes the actor after the configured cleanup delay.
`BP_OnAttack` remains an extension point for weapon trails, sounds and VFX.

Manny mesh rotation is Pitch=0, Yaw=-90, Roll=0. Unreal Python Rotator positional
arguments do not follow C++ FRotator order; always use named arguments in scripts.
Attack facing changes only actor Yaw, including when a target is above/below it.

After building UT1Editor, run Content/PYW/repair_enemy.py using the Unreal Python
commandlet to update the existing BP and loaded test enemies without recreating
the map. The full create_enemy_test_content.py script replaces the test map and
should only be used when intentionally rebuilding it.
