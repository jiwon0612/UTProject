# Enemy structure

`AUT1Entity` is the shared character base used by `AUT1Player` and now
`AEnemyCharacter`. The existing Entity currently supplies no health/combat API.

- `EnemyCharacter`: enemy settings, movement speed by state, shared 3D attack
  range check, cooldown enforcement and damage/animation event dispatch.
- `EnemyAIController`: owns Blackboard and the runtime Unreal Behavior Tree.
- `EnemyBTNodes`: target acquisition and Attack > Chase > Walk/Idle selection.
  Tasks delegate attack rules and speed selection to the character.

The BT is built at runtime; there is no editable BT asset in Content.
`BP_OnAttack` is an animation extension point, not an implemented attack montage.

Manny mesh rotation is Pitch=0, Yaw=-90, Roll=0. Unreal Python Rotator positional
arguments do not follow C++ FRotator order; always use named arguments in scripts.
Attack facing changes only actor Yaw, including when a target is above/below it.

After building UT1Editor, run Content/PYW/repair_enemy.py using the Unreal Python
commandlet to update the existing BP and loaded test enemies without recreating
the map. The full create_enemy_test_content.py script replaces the test map and
should only be used when intentionally rebuilding it.
