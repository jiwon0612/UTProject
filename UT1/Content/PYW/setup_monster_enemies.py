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

# 드랍표. 재료: (데이터, 확률, 최소, 최대). 설계도: (무기 데이터, 확률)
# 설계도는 위에서부터 굴려 처음 당첨된 1장만 떨어지므로 드문 것부터 둠. 이미 해금한 설계도는 건너뜀
MATERIALS = "/Game/CJW/Blueprints/Crafting/Materials/"
WEAPONS = "/Game/CJW/Blueprints/WeaponData/"
SMALL_LOOT = {
    "materials": [("DA_Mat_Scrap", 0.9, 1, 3), ("DA_Mat_Wire", 0.5, 1, 2), ("DA_Mat_Cloth", 0.35, 1, 2)],
    # 무기 설계도는 종류마다 13%. 카타나(TestWeaponData)는 시작 무기라 이미 해금되어 있으면 건너뜀
    "blueprints": [("TestWeaponData3", 0.13), ("TestWeaponData2", 0.13), ("TestWeaponData", 0.13)],
}
# 대형 적은 잡기 어려운 만큼 재료를 더 많이 떨굼. 설계도 확률은 다른 적과 같음
LARGE_LOOT = {
    "materials": [("DA_Mat_Scrap", 1.0, 3, 5), ("DA_Mat_Wire", 0.8, 2, 3), ("DA_Mat_Cloth", 0.6, 1, 3)],
    "blueprints": SMALL_LOOT["blueprints"],
}

# 패턴 이름 -> (타격 이펙트, 배율, 끌 이미터). C++ 패턴을 복사한 뒤 에셋 의존 데이터만 여기서 붙임
PATTERN_EFFECTS = {
    # 자폭: 폭발 시스템에서 비행용 이미터(Projectile_*)와 발사 고리를 끄고 폭발(Explosion_*)만 씀.
    # 검은 폭발 연기(Explosion_Smoke*)는 화면 전체를 가려서 끄고, 충격 고리와 불꽃만 남김
    "BomberSelfDestruct": (VFX_ROOT + "/NS_VFX_Explosion", 1.3,
                           ["Emitter_LeafRing", "Projectile_Smoke", "Projectile_VFX", "Projectile_Particle001",
                            "Explosion_Smoke", "Explosion_Smoke001"]),
}

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


def configure_loot(cdo, table):
    """AEnemyCharacter가 기본으로 가진 UT1LootDrop 컴포넌트에 적 종류별 드랍표를 채움."""
    materials = []
    for name, chance, low, high in table["materials"]:
        drop = unreal.UT1MaterialDrop()
        drop.set_editor_property("material", require(lib.load_asset(MATERIALS + name), "Missing " + name))
        drop.set_editor_property("chance", chance)
        drop.set_editor_property("min_count", low)
        drop.set_editor_property("max_count", high)
        materials.append(drop)
    blueprints = []
    for name, chance in table["blueprints"]:
        drop = unreal.UT1BlueprintDrop()
        drop.set_editor_property("weapon", require(lib.load_asset(WEAPONS + name), "Missing " + name))
        drop.set_editor_property("chance", chance)
        blueprints.append(drop)
    loot = cdo.get_editor_property("loot_drop")
    loot.set_editor_property("material_drops", materials)
    loot.set_editor_property("blueprint_drops", blueprints)


def configure_enemy(blueprint, profile, native_class, capsule=None, loot=SMALL_LOOT):
    cdo = unreal.get_default_object(blueprint.generated_class())
    configure_loot(cdo, loot)
    # 패턴 수치의 기준은 C++임. BP에 저장된 예전 패턴 대신 C++ 기본값을 복사한 뒤 애니메이션만 몬스터용으로 바꿈
    native = unreal.get_default_object(require(unreal.load_class(None, native_class), "Missing class " + native_class))
    capsule_component = cdo.get_editor_property("capsule_component")
    if capsule:
        capsule_component.set_capsule_size(capsule[0], capsule[1], False)
    mesh = cdo.get_editor_property("mesh")
    mesh.set_skeletal_mesh_asset(profile.mesh)
    # 메시 원점이 발바닥이므로 캡슐 절반 높이만큼 내림 (캡슐 배율은 자식에게 그대로 곱해짐)
    mesh.set_relative_location(unreal.Vector(0.0, 0.0, -capsule_component.get_unscaled_capsule_half_height()), False, False)
    mesh.set_relative_rotation(unreal.Rotator(pitch=0.0, yaw=mesh_yaw(profile.rig, "toe_L", "heel_02_L"), roll=0.0), False, False)

    cdo.set_editor_property("locomotion_animation", profile.locomotion)
    patterns = [pattern.copy() for pattern in native.get_editor_property("attack_patterns")]
    for pattern in patterns:
        pattern.set_editor_property("animation", profile.retargeted(pattern.get_editor_property("animation")))
        effect = PATTERN_EFFECTS.get(str(pattern.get_editor_property("name")))
        if effect:
            pattern.set_editor_property("impact_effect", require(lib.load_asset(effect[0]), "Missing " + effect[0]))
            pattern.set_editor_property("impact_effect_scale", effect[1])
            pattern.set_editor_property("impact_disabled_emitters", effect[2])
    cdo.set_editor_property("attack_patterns", patterns)
    cdo.set_editor_property("attack_animation", profile.retargeted(native.get_editor_property("attack_animation")))
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
        [(str(p.get_editor_property("name")), p.get_editor_property("animation").get_name(),
          round(p.get_editor_property("impact_delay"), 2), round(p.get_editor_property("animation_duration"), 2)) for p in patterns]))


def ensure_core_material():
    """투사체 판정 위치를 보여 주는 단순 발광 머티리얼임. 팩 머티리얼은 Niagara 파티클 색을 입력으로 받아서 정적 메시에 쓰지 않음."""
    folder, name = ROOT + "/Materials", "M_EnemyProjectileCore"
    path = folder + "/" + name
    material = lib.load_asset(path) if lib.does_asset_exist(path) else require(
        tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew()), "Failed to create " + name)
    editing = unreal.MaterialEditingLibrary
    editing.delete_all_material_expressions(material)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
    # 1을 넘는 HDR 값이라 블룸으로 빛나 보임. NS_VFX_Evil의 보라색 계열에 맞춤
    color.set_editor_property("constant", unreal.LinearColor(4.0, 0.5, 6.0, 1.0))
    editing.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.recompile_material(material)
    require(lib.save_loaded_asset(material, only_if_is_dirty=False), "Failed to save " + name)
    return material


def ensure_tail_material():
    """원뿔 꼬리용 가산 머티리얼임. 코어보다 어둡게 해서 판정 위치(코어)가 먼저 눈에 들어오게 함."""
    folder, name = ROOT + "/Materials", "M_EnemyProjectileTail"
    path = folder + "/" + name
    material = lib.load_asset(path) if lib.does_asset_exist(path) else require(
        tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew()), "Failed to create " + name)
    editing = unreal.MaterialEditingLibrary
    editing.delete_all_material_expressions(material)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    color = editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color.set_editor_property("constant", unreal.LinearColor(1.2, 0.15, 1.8, 1.0))
    editing.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.recompile_material(material)
    require(lib.save_loaded_asset(material, only_if_is_dirty=False), "Failed to save " + name)
    return material


def configure_projectile(ranged):
    projectile = require(lib.load_asset(BP_ROOT + "/BP_EnemyProjectile"), "Missing BP_EnemyProjectile")
    cdo = unreal.get_default_object(projectile.generated_class())
    # ProjectileVFX 팩 시스템은 머리 파티클이 스스로 속도·감속·충돌을 가져서 액터에 붙여도 따로 날아감.
    # 그래서 비행 비주얼은 액터 컴포넌트인 발광 코어와 원뿔 꼬리로 만들고, 팩 이펙트는 제자리에서 터지는 폭발에만 씀
    cdo.set_editor_property("trail_effect", None)
    cdo.set_editor_property("trail_disabled_emitters", [])
    cdo.set_editor_property("tail_material", ensure_tail_material())
    cdo.set_editor_property("tail_length", 120.0)
    cdo.set_editor_property("impact_effect", require(lib.load_asset(VFX_ROOT + "/NS_VFX_Explosion"), "Missing NS_VFX_Explosion"))
    # 폭발 시스템에도 비행용 이미터(Projectile_*)와 발사 고리가 들어 있어서 폭발(Explosion_*)만 남김
    cdo.set_editor_property("impact_disabled_emitters", [
        "Emitter_LeafRing", "Projectile_Smoke", "Projectile_VFX", "Projectile_Particle001"])
    cdo.set_editor_property("impact_effect_scale", unreal.Vector(0.7, 0.7, 0.7))
    cdo.set_editor_property("core_material", ensure_core_material())
    unreal.BlueprintEditorLibrary.compile_blueprint(projectile)
    require(lib.save_loaded_asset(projectile, only_if_is_dirty=False), "Failed to save BP_EnemyProjectile")
    # 네이티브 클래스 대신 VFX가 지정된 BP 투사체를 쏘도록 바꿈
    ranged_cdo = unreal.get_default_object(ranged.generated_class())
    ranged_cdo.set_editor_property("projectile_class", projectile.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(ranged)
    require(lib.save_loaded_asset(ranged, only_if_is_dirty=False), "Failed to save " + ranged.get_name())


def place_test_enemies(melee, ranged, large, assassin, bomber):
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
    # 암살자는 옆에서 선회하며 파고들고, 자폭병은 멀리서 달려와 터지는 모습이 보이게 둠
    place("Enemy_Assassin_Test", assassin, unreal.Vector(-700.0, 500.0, 150.0))
    place("Enemy_Bomber_Test", bomber, unreal.Vector(1200.0, 300.0, 150.0))
    require(levels.save_current_level(), "Failed to save " + TEST_LEVEL)
    lib.save_directory("/Game/__ExternalActors__/PYW/Lvl_EnemyBTTest", only_if_is_dirty=True, recursive=True)


redirectors = find_redirectors()
small = Profile("Small", "SM_")
large_profile = Profile("Large", "LM_")
melee = require(lib.load_asset(BP_ROOT + "/BP_MeleeEnemy"), "Missing BP_MeleeEnemy")
ranged = require(lib.load_asset(BP_ROOT + "/BP_RangedEnemy"), "Missing BP_RangedEnemy")
large = ensure_blueprint("BP_LargeEnemy", "/Script/UT1.BruteEnemyCharacter")
assassin = ensure_blueprint("BP_AssassinEnemy", "/Script/UT1.AssassinEnemyCharacter")
bomber = ensure_blueprint("BP_BomberEnemy", "/Script/UT1.BomberEnemyCharacter")
configure_enemy(melee, small, "/Script/UT1.MeleeEnemyCharacter")
configure_enemy(ranged, small, "/Script/UT1.RangedEnemyCharacter")
# 대형 메시(약 192cm)가 캡슐 안에 들어오도록 키움. Brute의 1.3배 캡슐 배율은 그대로 유지함
configure_enemy(large, large_profile, "/Script/UT1.BruteEnemyCharacter", capsule=(42.0, 96.0), loot=LARGE_LOOT)
# 암살자(0.92배)와 자폭병(0.8배)은 C++ 캡슐 배율로 소형 몬스터를 줄여 씀
configure_enemy(assassin, small, "/Script/UT1.AssassinEnemyCharacter")
configure_enemy(bomber, small, "/Script/UT1.BomberEnemyCharacter")
configure_projectile(ranged)
place_test_enemies(melee, ranged, large, assassin, bomber)
delete_unreferenced_redirectors(redirectors)
unreal.log("PYW_MONSTER_ENEMIES_SUCCESS")
