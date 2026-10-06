import unreal


CONTENT_ROOT = "/Game/PYW"
SOURCE_LEVEL = "/Game/TopDown/Lvl_TopDown"
TEST_LEVEL = "/Game/PYW/Lvl_EnemyBTTest"
ENEMY_BLUEPRINT = "/Game/PYW/BP_Enemy"


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def create_enemy_blueprint():
    existing = (
        unreal.EditorAssetLibrary.load_asset(ENEMY_BLUEPRINT)
        if unreal.EditorAssetLibrary.does_asset_exist(ENEMY_BLUEPRINT)
        else None
    )

    parent_class = require(
        unreal.load_class(None, "/Script/UT1.EnemyCharacter"),
        "AEnemyCharacter was not found. Build UT1Editor first.",
    )
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = existing or require(
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "BP_Enemy", CONTENT_ROOT, unreal.Blueprint, factory
        ),
        "Failed to create BP_Enemy.",
    )

    enemy_cdo = unreal.get_default_object(blueprint.generated_class())
    mesh_component = enemy_cdo.get_editor_property("mesh")
    mesh = require(
        unreal.EditorAssetLibrary.load_asset(
            "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
        ),
        "Manny skeletal mesh was not found.",
    )
    mesh_component.set_skeletal_mesh_asset(mesh)
    mesh_component.set_relative_location(
        unreal.Vector(0.0, 0.0, -90.0), False, False
    )
    mesh_component.set_relative_rotation(
        unreal.Rotator(pitch=0.0, yaw=-90.0, roll=0.0), False, False
    )

    anim_class = unreal.load_class(
        None,
        "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C",
    )
    if anim_class:
        mesh_component.set_anim_instance_class(anim_class)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    require(
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False),
        "Failed to save BP_Enemy.",
    )
    return blueprint


def create_test_level(enemy_blueprint):
    if unreal.EditorAssetLibrary.does_asset_exist(TEST_LEVEL):
        require(
            unreal.EditorAssetLibrary.delete_asset(TEST_LEVEL),
            "Failed to replace the previous test level.",
        )

    require(
        unreal.EditorAssetLibrary.duplicate_asset(SOURCE_LEVEL, TEST_LEVEL),
        "Failed to duplicate Lvl_TopDown.",
    )

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    require(level_subsystem.load_level(TEST_LEVEL), "Failed to load the copied test level.")

    wasd_game_mode = require(
        unreal.load_class(None, "/Script/UT1.WASDGameMode"),
        "AWASDGameMode was not found. Build UT1Editor first.",
    )
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", wasd_game_mode)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    starts = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if isinstance(actor, unreal.PlayerStart)
    ]
    if not starts:
        starts.append(
            require(
                actor_subsystem.spawn_actor_from_class(
                    unreal.PlayerStart, unreal.Vector(0.0, 0.0, 100.0), unreal.Rotator()
                ),
                "Failed to place a PlayerStart.",
            )
        )
    start_location = starts[0].get_actor_location()

    directional_light = require(
        actor_subsystem.spawn_actor_from_class(
            unreal.DirectionalLight,
            start_location + unreal.Vector(0.0, 0.0, 600.0),
            unreal.Rotator(),
        ),
        "Failed to create the test Directional Light.",
    )
    directional_light.set_actor_label("Enemy_BT_DirectionalLight")
    directional_light.set_actor_rotation(
        unreal.Rotator(pitch=-45.0, yaw=-35.0, roll=0.0), False
    )
    directional_light.get_editor_property(
        "directional_light_component"
    ).set_editor_property("intensity", 8.0)

    sky_light = require(
        actor_subsystem.spawn_actor_from_class(
            unreal.SkyLight,
            start_location + unreal.Vector(0.0, 0.0, 500.0),
            unreal.Rotator(),
        ),
        "Failed to create the test Sky Light.",
    )
    sky_light.set_actor_label("Enemy_BT_SkyLight")
    sky_light.get_editor_property("light_component").set_editor_property(
        "intensity", 1.0
    )

    floor = require(
        actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor,
            start_location + unreal.Vector(0.0, 0.0, -110.0),
            unreal.Rotator(),
        ),
        "Failed to create the navigation test floor.",
    )
    floor.set_actor_label("Enemy_BT_TestFloor")
    floor.set_actor_scale3d(unreal.Vector(18.0, 18.0, 0.2))
    floor_mesh = require(
        unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube"),
        "Engine cube mesh was not found.",
    )
    floor.get_editor_property("static_mesh_component").set_static_mesh(floor_mesh)

    nav_bounds = require(
        actor_subsystem.spawn_actor_from_class(
            unreal.NavMeshBoundsVolume,
            start_location + unreal.Vector(0.0, 0.0, 200.0),
            unreal.Rotator(),
        ),
        "Failed to create NavMeshBoundsVolume.",
    )
    nav_bounds.set_actor_label("Enemy_BT_NavMeshBounds")
    nav_bounds.set_actor_scale3d(unreal.Vector(18.0, 18.0, 6.0))

    patrol_enemy = require(
        actor_subsystem.spawn_actor_from_class(
            enemy_blueprint.generated_class(),
            start_location + unreal.Vector(700.0, -350.0, 50.0),
            unreal.Rotator(),
        ),
        "Failed to place the patrol enemy in the test level.",
    )
    patrol_enemy.set_actor_label("Enemy_BT_PatrolTest")
    patrol_enemy.set_editor_property("detection_range", 50.0)
    patrol_enemy.set_editor_property("lose_target_range", 100.0)
    patrol_enemy.set_editor_property("patrol_radius", 180.0)
    patrol_enemy.set_editor_property("attack_range", 150.0)

    combat_enemy = require(
        actor_subsystem.spawn_actor_from_class(
            enemy_blueprint.generated_class(),
            start_location + unreal.Vector(700.0, 350.0, 50.0),
            unreal.Rotator(),
        ),
        "Failed to place the combat enemy in the test level.",
    )
    combat_enemy.set_actor_label("Enemy_BT_CombatTest")
    combat_enemy.set_editor_property("detection_range", 1600.0)
    combat_enemy.set_editor_property("lose_target_range", 2200.0)
    combat_enemy.set_editor_property("patrol_radius", 180.0)
    combat_enemy.set_editor_property("attack_range", 150.0)

    attack_enemy = require(
        actor_subsystem.spawn_actor_from_class(
            enemy_blueprint.generated_class(),
            start_location + unreal.Vector(100.0, 0.0, 50.0),
            unreal.Rotator(),
        ),
        "Failed to place the attack enemy in the test level.",
    )
    attack_enemy.set_actor_label("Enemy_BT_AttackTest")
    attack_enemy.set_editor_property("detection_range", 500.0)
    attack_enemy.set_editor_property("lose_target_range", 800.0)
    attack_enemy.set_editor_property("patrol_radius", 100.0)
    attack_enemy.set_editor_property("attack_range", 180.0)

    require(level_subsystem.save_current_level(), "Failed to save Lvl_EnemyBTTest.")


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(CONTENT_ROOT):
        require(unreal.EditorAssetLibrary.make_directory(CONTENT_ROOT), "Failed to create Content/PYW.")
    blueprint = create_enemy_blueprint()
    create_test_level(blueprint)
    unreal.log("PYW_CONTENT_SETUP_SUCCESS: BP_Enemy and Lvl_EnemyBTTest are ready.")


if __name__ == "__main__":
    main()
