"""적 BP에 Dungeon Pack 몬스터 메시와 리타게팅 애니메이션, 투사체 VFX를 연결하고 테스트 레벨에 배치함.

setup_monster_animations.py를 먼저 실행해야 함. 다시 실행해도 같은 결과가 나옴.
"""
import math
import os
import unreal

ROOT = "/Game/PYW"
BP_ROOT = ROOT + "/BluePrint"
ANIM_ROOT = ROOT + "/Animation"
TEST_LEVEL = ROOT + "/Lvl_EnemyBTTest"
MONSTER_ROOT = "/Game/CJW/Assets/Dungeon_Pack/Assets/Pack_Characters/Characters/Monsters"
VFX_ROOT = ROOT + "/ProjectileVFX/Niagara"
PLAYER_GAME_MODE = "/Game/CJW/Blueprints/GameModes/BP_DavGameMods"

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# 예전 PYW 복사본 이름 -> 원본 Mannequin 애니메이션 이름. 리타게팅 결과는 원본 이름에 접두어만 붙음
LEGACY_SOURCE = {
    "AN_MeleeAttack_01": "MM_Attack_01",
    "AN_MeleeAttack_02": "MM_Attack_02",
    "AN_MeleeAttack_03": "MM_Attack_03",
    "AN_RangedQuickCast": "MM_Attack_02",
    "AN_RangedCast": "MM_ChargedAttack",
    "AN_RangedPowerCast": "MM_Attack_03",
    "AN_EnemyDeath": "MM_Death_Front_01",
}


class Profile:
    def __init__(self, key, prefix):
        self.key = key
        self.prefix = prefix
        self.mesh = require(lib.load_asset("{0}/{1}/Assets/SK_Monster_{1}".format(MONSTER_ROOT, key)), "Missing mesh " + key)
        self.rig = require(lib.load_asset("{}/IK_Monster{}".format(ANIM_ROOT, key)), "Run setup_monster_animations.py first")
        self.locomotion = self.anim("BS_Monster{}_Locomotion".format(key), raw=True)

    def anim(self, name, raw=False):
        path = "{}/{}/{}{}".format(ANIM_ROOT, self.key, "" if raw else self.prefix, name)
        return require(lib.load_asset(path), "Missing " + path)

    def retargeted(self, animation):
        name = animation.get_name()
        for prefix in ("SM_", "LM_"):
            if name.startswith(prefix):
                name = name[len(prefix):]
        return self.anim(LEGACY_SOURCE.get(name, name))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def find_redirectors():
    paths = set()
    for path in lib.list_assets(ROOT, recursive=True):
        if lib.find_asset_data(path).asset_class_path.asset_name == "ObjectRedirector":
            paths.add(path.split(".")[0])
    return sorted(paths)


def delete_unreferenced_redirectors(paths):
    """BP를 BluePrint 폴더로 옮길 때 남은 Redirector를 지움.

    Python에는 Fix Up Redirectors가 노출되지 않아서, 참조하던 레벨 액터를 먼저 실제 경로로
    다시 저장한 뒤(place_test_enemies) 남은 참조가 없을 때만 지움.
    """
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    # 방금 저장한 액터 패키지의 참조 정보가 레지스트리에 반영되도록 다시 스캔함
    registry.scan_paths_synchronous(["/Game/__ExternalActors__/PYW/Lvl_EnemyBTTest"], True)
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    for path in paths:
        referencers = registry.get_referencers(path, options)
        require(not referencers, "Redirector {} is still referenced by {}".format(path, referencers))
        lib.delete_asset(path)
        # delete_asset이 Redirector 파일을 남기는 경우가 있어서 참조가 없음을 확인한 뒤 직접 지움
        filename = unreal.Paths.convert_relative_path_to_full(
            unreal.Paths.project_content_dir() + path[len("/Game/"):] + ".uasset")
        if os.path.exists(filename):
            os.remove(filename)
        unreal.log("PYW_REDIRECTOR_REMOVED " + path)


def mesh_yaw(rig, toe, heel):
    """발끝 방향을 액터 +X에 맞추는 메시 Yaw임. Manny는 +Y를 보고 있어서 -90이 나옴."""
    controller = unreal.IKRigController.get_controller(rig)
    forward = controller.get_ref_pose_transform_of_bone(toe).translation - controller.get_ref_pose_transform_of_bone(heel).translation
    # 발이 살짝 바깥을 향해 있어서 90도 단위로 맞춤
    return round(-math.degrees(math.atan2(forward.y, forward.x)) / 90.0) * 90.0


def ensure_blueprint(name, parent):
    path = BP_ROOT + "/" + name
    if lib.does_asset_exist(path):
        return lib.load_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", require(unreal.load_class(None, parent), "Missing class " + parent))
    return require(tools.create_asset(name, BP_ROOT, unreal.Blueprint, factory), "Failed to create " + name)


def configure_enemy(blueprint, profile, capsule=None):
    cdo = unreal.get_default_object(blueprint.generated_class())
    capsule_component = cdo.get_editor_property("capsule_component")
    if capsule:
        capsule_component.set_capsule_size(capsule[0], capsule[1], False)
    mesh = cdo.get_editor_property("mesh")
    mesh.set_skeletal_mesh_asset(profile.mesh)
    # 메시 원점이 발바닥이므로 캡슐 절반 높이만큼 내림 (캡슐 배율은 자식에게 그대로 곱해짐)
    mesh.set_relative_location(unreal.Vector(0.0, 0.0, -capsule_component.get_unscaled_capsule_half_height()), False, False)
    mesh.set_relative_rotation(unreal.Rotator(pitch=0.0, yaw=mesh_yaw(profile.rig, "toe_L", "heel_02_L"), roll=0.0), False, False)

    cdo.set_editor_property("locomotion_animation", profile.locomotion)
    patterns = list(cdo.get_editor_property("attack_patterns"))
    for pattern in patterns:
        pattern.set_editor_property("animation", profile.retargeted(pattern.get_editor_property("animation")))
    cdo.set_editor_property("attack_patterns", patterns)
    cdo.set_editor_property("attack_animation", profile.retargeted(cdo.get_editor_property("attack_animation")))
    cdo.set_editor_property("death_animation", profile.anim("MM_Death_Front_01"))
    cdo.set_editor_property("death_back_animation", profile.anim("MM_Death_Back_01"))
    cdo.set_editor_property("death_left_animation", profile.anim("MM_Death_Left_01"))
    cdo.set_editor_property("death_right_animation", profile.anim("MM_Death_Right_01"))
    cdo.set_editor_property("hit_react_animation", profile.anim("MM_HitReact_Front_Med_01"))
    cdo.set_editor_property("hit_react_back_animation", profile.anim("MM_HitReact_Back_Med_01"))
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    require(lib.save_loaded_asset(blueprint, only_if_is_dirty=False), "Failed to save " + blueprint.get_name())
    unreal.log("PYW_ENEMY_CONFIGURED {} mesh={} yaw={:.1f} patterns={}".format(
        blueprint.get_name(), profile.mesh.get_name(), mesh.get_editor_property("relative_rotation").yaw,
        [(str(p.get_editor_property("name")), p.get_editor_property("animation").get_name()) for p in patterns]))


def configure_projectile(ranged):
    projectile = require(lib.load_asset(BP_ROOT + "/BP_EnemyProjectile"), "Missing BP_EnemyProjectile")
    cdo = unreal.get_default_object(projectile.generated_class())
    cdo.set_editor_property("trail_effect", require(lib.load_asset(VFX_ROOT + "/NS_VFX_Evil"), "Missing NS_VFX_Evil"))
    cdo.set_editor_property("impact_effect", require(lib.load_asset(VFX_ROOT + "/NS_VFX_Explosion"), "Missing NS_VFX_Explosion"))
    unreal.BlueprintEditorLibrary.compile_blueprint(projectile)
    require(lib.save_loaded_asset(projectile, only_if_is_dirty=False), "Failed to save BP_EnemyProjectile")
    # 네이티브 클래스 대신 VFX가 지정된 BP 투사체를 쏘도록 바꿈
    ranged_cdo = unreal.get_default_object(ranged.generated_class())
    ranged_cdo.set_editor_property("projectile_class", projectile.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(ranged)
    require(lib.save_loaded_asset(ranged, only_if_is_dirty=False), "Failed to save " + ranged.get_name())


def place_test_enemies(melee, ranged, large):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    require(levels.load_level(TEST_LEVEL), "Failed to load " + TEST_LEVEL)
    # 실제 전투 규칙으로 시험하도록 CJW 플레이어(BP_Player)를 쓰는 GameMode로 둠
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = require(lib.load_asset(PLAYER_GAME_MODE), "Missing " + PLAYER_GAME_MODE)
    world.get_world_settings().set_editor_property("default_game_mode", game_mode.generated_class())
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    def place(label, blueprint, location, death_delay=0.0):
        actor = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == label), None)
        if actor and actor.get_class() != blueprint.generated_class():
            actors.destroy_actor(actor)
            actor = None
        if not actor:
            actor = require(actors.spawn_actor_from_class(blueprint.generated_class(), location, unreal.Rotator()), "Failed to spawn " + label)
            actor.set_actor_label(label)
        actor.modify()
        actor.set_actor_location(location, False, False)
        actor.set_editor_property("test_death_delay", death_delay)
        unreal.log("PYW_TEST_PLACEMENT {} {} {}".format(label, blueprint.get_name(), location))

    # 근접은 바로 붙고, 원거리는 거리를 벌려 투사체를 쏘고, 대형은 돌진할 만큼 멀리 둠
    place("Enemy_Melee_Test", melee, unreal.Vector(140.0, -80.0, 150.0))
    place("Enemy_Ranged_Test", ranged, unreal.Vector(650.0, 250.0, 150.0))
    place("Enemy_Death_Test", melee, unreal.Vector(400.0, -400.0, 150.0), 1.0)
    place("Enemy_Large_Test", large, unreal.Vector(900.0, -600.0, 200.0))
    require(levels.save_current_level(), "Failed to save " + TEST_LEVEL)
    lib.save_directory("/Game/__ExternalActors__/PYW/Lvl_EnemyBTTest", only_if_is_dirty=True, recursive=True)


redirectors = find_redirectors()
small = Profile("Small", "SM_")
large_profile = Profile("Large", "LM_")
melee = require(lib.load_asset(BP_ROOT + "/BP_MeleeEnemy"), "Missing BP_MeleeEnemy")
ranged = require(lib.load_asset(BP_ROOT + "/BP_RangedEnemy"), "Missing BP_RangedEnemy")
large = ensure_blueprint("BP_LargeEnemy", "/Script/UT1.BruteEnemyCharacter")
configure_enemy(melee, small)
configure_enemy(ranged, small)
# 대형 메시(약 192cm)가 캡슐 안에 들어오도록 키움. Brute의 1.3배 캡슐 배율은 그대로 유지함
configure_enemy(large, large_profile, capsule=(42.0, 96.0))
configure_projectile(ranged)
place_test_enemies(melee, ranged, large)
delete_unreferenced_redirectors(redirectors)
unreal.log("PYW_MONSTER_ENEMIES_SUCCESS")
