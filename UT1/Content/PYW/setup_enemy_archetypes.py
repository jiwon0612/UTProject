import unreal


CONTENT_ROOT = "/Game/PYW"
TEST_LEVEL = "/Game/PYW/Lvl_EnemyBTTest"
MANNY_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def ensure_blueprint(asset_name, parent_path):
    asset_path = CONTENT_ROOT + "/" + asset_name
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None
    if not blueprint:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", require(unreal.load_class(None, parent_path), "Missing class " + parent_path))
        blueprint = require(
            unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, CONTENT_ROOT, unreal.Blueprint, factory),
            "Failed to create " + asset_name,
        )
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint


def ensure_animation_copy(asset_name, source_path):
    destination = CONTENT_ROOT + "/" + asset_name
    asset = unreal.EditorAssetLibrary.load_asset(destination) if unreal.EditorAssetLibrary.does_asset_exist(destination) else None
    if not asset:
        asset = require(unreal.EditorAssetLibrary.duplicate_asset(source_path, destination), "Failed to duplicate " + asset_name)
    require(unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), "Failed to save " + asset_name)
    return asset


def configure_enemy(blueprint, locomotion, attacks, death):
    cdo = unreal.get_default_object(blueprint.generated_class())
    mesh = cdo.get_editor_property("mesh")
    mesh.set_skeletal_mesh_asset(require(unreal.load_asset(MANNY_MESH), "Missing Manny mesh"))
    mesh.set_relative_location(unreal.Vector(0.0, 0.0, -90.0), False, False)
    mesh.set_relative_rotation(unreal.Rotator(pitch=0.0, yaw=-90.0, roll=0.0), False, False)
    cdo.set_editor_property("locomotion_animation", locomotion)
    cdo.set_editor_property("attack_animation", attacks[0])
    cdo.set_editor_property("attack_animations", attacks)
    cdo.set_editor_property("death_animation", death)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    require(unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False), "Failed to save " + blueprint.get_name())


locomotion = ensure_animation_copy("BS_EnemyLocomotion", "/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run")
melee_attacks = [
    ensure_animation_copy("AN_MeleeAttack_01", "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01"),
    ensure_animation_copy("AN_MeleeAttack_02", "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02"),
    ensure_animation_copy("AN_MeleeAttack_03", "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03"),
]
ranged_attacks = [
    ensure_animation_copy("AN_RangedCast", "/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack")
]
death_animation = ensure_animation_copy("AN_EnemyDeath", "/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01")

melee = ensure_blueprint("BP_MeleeEnemy", "/Script/UT1.MeleeEnemyCharacter")
ranged = ensure_blueprint("BP_RangedEnemy", "/Script/UT1.RangedEnemyCharacter")
projectile = ensure_blueprint("BP_EnemyProjectile", "/Script/UT1.EnemyProjectile")
configure_enemy(melee, locomotion, melee_attacks, death_animation)
configure_enemy(ranged, locomotion, ranged_attacks, death_animation)
ranged_cdo = unreal.get_default_object(ranged.generated_class())
ranged_cdo.set_editor_property("projectile_class", require(unreal.load_class(None, "/Script/UT1.EnemyProjectile"), "Missing native projectile class"))
unreal.BlueprintEditorLibrary.compile_blueprint(ranged)
require(unreal.EditorAssetLibrary.save_loaded_asset(ranged, only_if_is_dirty=False), "Failed to save BP_RangedEnemy")
require(unreal.EditorAssetLibrary.save_loaded_asset(projectile, only_if_is_dirty=False), "Failed to save BP_EnemyProjectile")

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
require(levels.load_level(TEST_LEVEL), "Failed to load PYW test level")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()
labels = {actor.get_actor_label() for actor in all_actors}
starts = [actor for actor in all_actors if isinstance(actor, unreal.PlayerStart)]
origin = starts[0].get_actor_location() if starts else unreal.Vector(0.0, 0.0, 100.0)

# Replace the old generic test population with the explicit combat archetypes.
generic_enemy = unreal.EditorAssetLibrary.load_asset(CONTENT_ROOT + "/BP_Enemy")
if generic_enemy:
    generic_class = generic_enemy.generated_class()
    for actor in list(actors.get_all_level_actors()):
        if actor.get_class() == generic_class:
            actors.destroy_actor(actor)

def place_test_enemy(label, enemy_class, offset):
    actor = next((item for item in actors.get_all_level_actors() if item.get_actor_label() == label), None)
    if not actor:
        actor = require(actors.spawn_actor_from_class(enemy_class, origin + offset, unreal.Rotator()), "Failed to spawn " + label)
        actor.set_actor_label(label)
    actor.modify()
    actor.set_actor_location(origin + offset, False, False)
    return actor


# Keep the deterministic validation actors inside their own combat ranges. The
# normal BT still decides whether to chase or attack while playing the level.
melee_actor = place_test_enemy("Enemy_Melee_Test", melee.generated_class(), unreal.Vector(140.0, -80.0, 50.0))
ranged_actor = place_test_enemy("Enemy_Ranged_Test", ranged.generated_class(), unreal.Vector(650.0, 250.0, 50.0))
death_actor = place_test_enemy("Enemy_Death_Test", melee.generated_class(), unreal.Vector(400.0, -400.0, 50.0))
melee_actor.set_editor_property("test_death_delay", 0.0)
ranged_actor.set_editor_property("test_death_delay", 0.0)
death_actor.set_editor_property("test_death_delay", 1.0)

for actor in actors.get_all_level_actors():
    if actor.get_actor_label() in ("Enemy_Melee_Test", "Enemy_Ranged_Test", "Enemy_Death_Test") or isinstance(actor, unreal.PlayerStart):
        unreal.log("PYW_TEST_PLACEMENT Label={} Location={}".format(actor.get_actor_label(), actor.get_actor_location()))

require(levels.save_current_level(), "Failed to save PYW test level")
require(unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True), "Failed to save World Partition actor packages")
unreal.log("PYW_ENEMY_ARCHETYPES_SUCCESS")
