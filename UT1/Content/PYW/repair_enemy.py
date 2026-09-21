"""Update the existing enemy blueprint and placed enemies without replacing the level."""
import runpy
import unreal

setup = runpy.run_path(unreal.Paths.project_content_dir() + "PYW/create_enemy_test_content.py", run_name="enemy_setup")
setup["create_enemy_blueprint"]()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/Game/PYW/Lvl_EnemyBTTest")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
enemy_class = unreal.load_class(None, "/Script/UT1.EnemyCharacter")
count = 0
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.EnemyCharacter):
        mesh = actor.get_editor_property("mesh")
        mesh.set_relative_rotation(unreal.Rotator(pitch=0.0, yaw=-90.0, roll=0.0), False, False)
        rotation = actor.get_actor_rotation()
        actor.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=rotation.yaw, roll=0.0), False)
        actual = mesh.get_editor_property("relative_rotation")
        assert abs(actual.pitch) < 0.01 and abs(actual.roll) < 0.01
        count += 1
assert count > 0, "No loaded enemy instances found"
assert levels.save_current_level()
unreal.log("ENEMY_REPAIR_SUCCESS count=" + str(count))
