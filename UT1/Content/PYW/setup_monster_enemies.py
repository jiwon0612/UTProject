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

# 패턴 이름 -> (타격 Niagara, 배율, 끌 이미터). C++ 패턴을 복사한 뒤 에셋 의존 데이터만 여기서 붙임.
# 지금은 비어 있음: 팩 시스템은 이미터 일부만 켜면 그려지지 않아서, 범위 공격의 폭발은 C++ AEnemyHitFlash가 맡음
PATTERN_EFFECTS = {}

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


NOISE_ROOT = ROOT + "/ProjectileVFX/Textures/"
# NS_VFX_Evil의 보라색 계열임. 1을 넘는 HDR 값이라 블룸으로 빛나 보임
PURPLE = unreal.LinearColor(1.0, 0.12, 1.7, 1.0)
HOT = unreal.LinearColor(1.0, 0.55, 1.2, 1.0)


class MaterialGraph:
    """머티리얼 노드를 코드로 만드는 작은 도우미임. 팩 머티리얼은 Niagara 파티클 색을 입력으로 받아서
    정적 메시에 그대로 쓰면 검게 나오므로, 투사체 메시용 머티리얼은 여기서 직접 만듦."""

    editing = unreal.MaterialEditingLibrary

    def __init__(self, name, blend=unreal.BlendMode.BLEND_OPAQUE, two_sided=False):
        folder = ROOT + "/Materials"
        path = folder + "/" + name
        self.name = name
        self.material = lib.load_asset(path) if lib.does_asset_exist(path) else require(
            tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew()), "Failed to create " + name)
        self.editing.delete_all_material_expressions(self.material)
        self.material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        self.material.set_editor_property("blend_mode", blend)
        self.material.set_editor_property("two_sided", two_sided)
        self.row = 0

    def node(self, cls, **props):
        expression = self.editing.create_material_expression(self.material, getattr(unreal, cls), -600, self.row)
        self.row += 110
        for key, value in props.items():
            expression.set_editor_property(key, value)
        return expression

    def link(self, source, target, target_input="", source_output=""):
        require(self.editing.connect_material_expressions(source, source_output, target, target_input),
                "{}: {}.{} -> {}.{}".format(self.name, source.get_name(), source_output, target.get_name(), target_input))
        return target

    def binary(self, cls, a, b):
        node = self.node(cls)
        self.link(a, node, "A")
        self.link(b, node, "B")
        return node

    def mul(self, *inputs):
        result = inputs[0]
        for other in inputs[1:]:
            result = self.binary("MaterialExpressionMultiply", result, other)
        return result

    def add(self, a, b):
        return self.binary("MaterialExpressionAdd", a, b)

    def scalar(self, value):
        return self.node("MaterialExpressionConstant", r=value)

    def color(self, color, intensity):
        return self.node("MaterialExpressionConstant3Vector", constant=unreal.LinearColor(
            color.r * intensity, color.g * intensity, color.b * intensity, 1.0))

    def unary(self, cls, source, **props):
        return self.link(source, self.node(cls, **props))

    def power(self, base, exponent):
        return self.unary("MaterialExpressionPower", base, const_exponent=exponent)

    def fresnel(self, exponent):
        return self.node("MaterialExpressionFresnel", exponent=exponent, base_reflect_fraction=0.0)

    def pulse(self, period, amount):
        """1 ± amount 사이를 period초 주기로 오가는 값임. 코어가 살아 있는 듯 깜빡이게 함."""
        wave = self.unary("MaterialExpressionSine", self.node("MaterialExpressionTime"), period=period)
        return self.add(self.mul(wave, self.scalar(amount)), self.scalar(1.0))

    def scrolling_noise(self, texture_name, tiling, speed):
        """원뿔 UV를 따라 흐르는 노이즈임. 단색 꼬리가 줄무늬처럼 흘러가 보여 속도감이 생김."""
        texture = require(lib.load_asset(NOISE_ROOT + texture_name), "Missing " + texture_name)
        coords = self.node("MaterialExpressionTextureCoordinate", u_tiling=tiling[0], v_tiling=tiling[1])
        panner = self.link(coords, self.node("MaterialExpressionPanner", speed_x=speed[0], speed_y=speed[1]), "Coordinate")
        compression = texture.get_editor_property("compression_settings")
        srgb = texture.get_editor_property("srgb")
        if compression == unreal.TextureCompressionSettings.TC_GRAYSCALE:
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_GRAYSCALE if srgb else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
        elif compression == unreal.TextureCompressionSettings.TC_MASKS:
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
        else:
            sampler = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if srgb else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
        sample = self.link(panner, self.node("MaterialExpressionTextureSample", texture=texture, sampler_type=sampler), "UVs")
        return self.mask(sample, r=True)

    def mask(self, source, source_output="", r=False, g=False, b=False):
        node = self.node("MaterialExpressionComponentMask", r=r, g=g, b=b, a=False)
        return self.link(source, node, "", source_output)

    def cone_fade(self):
        """원뿔 밑면(코어 쪽) 1 -> 꼭짓점(꼬리 끝) 0인 값임. 엔진 원뿔은 +Z가 꼭짓점이라 로컬 Z를 높이로 나눔."""
        height = self.mask(self.node("MaterialExpressionLocalPosition"), b=True)
        bounds = self.node("MaterialExpressionObjectLocalBounds")
        bottom = self.mask(bounds, "Min", b=True)
        full = self.binary("MaterialExpressionSubtract", self.mask(bounds, "Max", b=True), bottom)
        along = self.binary("MaterialExpressionDivide", self.binary("MaterialExpressionSubtract", height, bottom), full)
        return self.unary("MaterialExpressionSaturate", self.unary("MaterialExpressionOneMinus", along))

    def finish(self, emissive):
        self.editing.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        self.editing.recompile_material(self.material)
        require(lib.save_loaded_asset(self.material, only_if_is_dirty=False), "Failed to save " + self.name)
        unreal.log("PYW_MATERIAL_BUILT {} nodes={}".format(self.name, self.row // 110))
        return self.material


def ensure_core_material():
    """판정 위치를 보여 주는 코어임. 가운데는 하얗게 달아오르고 가장자리는 보라색이며, 빠르게 맥동함."""
    graph = MaterialGraph("M_EnemyProjectileCore")
    rim = graph.node("MaterialExpressionLinearInterpolate")
    graph.link(graph.color(HOT, 10.0), rim, "A")
    graph.link(graph.color(PURPLE, 6.0), rim, "B")
    graph.link(graph.fresnel(2.0), rim, "Alpha")
    return graph.finish(graph.mul(rim, graph.pulse(0.18, 0.25)))


def ensure_halo_material():
    """코어를 감싸는 부드러운 빛무리임. 정면(프레넬 0)이 가장 밝고 테두리로 갈수록 사라져 구체 윤곽이 보이지 않음."""
    graph = MaterialGraph("M_EnemyProjectileHalo", unreal.BlendMode.BLEND_ADDITIVE)
    soft = graph.power(graph.unary("MaterialExpressionOneMinus", graph.fresnel(1.0)), 3.0)
    return graph.finish(graph.mul(graph.color(PURPLE, 2.2), soft, graph.pulse(0.18, 0.2)))


def ensure_tail_material():
    """넓은 바깥 꼬리(라인)임. 코어 쪽이 밝고 끝으로 갈수록 사라지며, 노이즈가 뒤로 흘러 불꽃처럼 일렁임."""
    graph = MaterialGraph("M_EnemyProjectileTail", unreal.BlendMode.BLEND_ADDITIVE, two_sided=True)
    fade = graph.power(graph.cone_fade(), 1.4)
    flicker = graph.add(graph.mul(graph.scrolling_noise("T_VFX_Noise_3", (1.0, 1.5), (0.0, -2.5)), graph.scalar(1.4)),
                        graph.scalar(0.3))
    return graph.finish(graph.mul(graph.color(PURPLE, 3.0), fade, flicker))


def ensure_inner_tail_material():
    """바깥 꼬리 속을 지나는 가늘고 하얀 선임. 더 빨리 흘러서 진행 방향이 또렷하게 읽힘."""
    graph = MaterialGraph("M_EnemyProjectileInnerTail", unreal.BlendMode.BLEND_ADDITIVE, two_sided=True)
    fade = graph.power(graph.cone_fade(), 2.2)
    streak = graph.add(graph.mul(graph.scrolling_noise("T_SmearNoise005", (1.0, 1.0), (0.0, -4.0)), graph.scalar(1.2)),
                       graph.scalar(0.5))
    return graph.finish(graph.mul(graph.color(HOT, 4.0), fade, streak))


def ensure_hit_flash_material():
    """공격이 맞은 자리의 섬광(AEnemyHitFlash)용 가산 머티리얼임. 색·밝기는 Color 파라미터로 런타임에 바꾸고,
    가운데가 밝고 테두리로 사라져 빛이 번지는 모양이 됨."""
    graph = MaterialGraph("M_EnemyHitFlash", unreal.BlendMode.BLEND_ADDITIVE)
    color = graph.node("MaterialExpressionVectorParameter", parameter_name="Color",
                       default_value=unreal.LinearColor(1.0, 0.5, 0.2, 1.0))
    soft = graph.power(graph.unary("MaterialExpressionOneMinus", graph.fresnel(1.0)), 2.0)
    return graph.finish(graph.mul(color, soft))


def configure_projectile(ranged):
    projectile = require(lib.load_asset(BP_ROOT + "/BP_EnemyProjectile"), "Missing BP_EnemyProjectile")
    cdo = unreal.get_default_object(projectile.generated_class())
    # ProjectileVFX 팩 시스템은 머리 파티클이 스스로 속도·감속·충돌을 가져서 액터에 붙여도 따로 날아감.
    # 그래서 비행 비주얼은 액터 컴포넌트인 발광 코어와 원뿔 꼬리로 만들고, 팩 이펙트는 제자리에서 터지는 폭발에만 씀
    cdo.set_editor_property("trail_effect", None)
    cdo.set_editor_property("trail_disabled_emitters", [])
    # 라인은 두 겹임: 넓고 은은한 바깥 꼬리 + 가늘고 긴 하얀 선. 둘 다 노이즈가 뒤로 흘러 속도감을 줌
    cdo.set_editor_property("tail_material", ensure_tail_material())
    cdo.set_editor_property("tail_length", 140.0)
    cdo.set_editor_property("tail_width_scale", 2.0)
    cdo.set_editor_property("inner_tail_material", ensure_inner_tail_material())
    cdo.set_editor_property("inner_tail_length_scale", 1.8)
    cdo.set_editor_property("inner_tail_width_scale", 0.4)
    cdo.set_editor_property("core_material", ensure_core_material())
    cdo.set_editor_property("halo_material", ensure_halo_material())
    cdo.set_editor_property("halo_scale", 2.6)
    cdo.set_editor_property("glow_intensity", 40.0)
    # 팩 시스템은 이미터끼리 이벤트로 엮여 있어서 일부만 켜면 아무것도 그려지지 않음(PIE에서 확인).
    # 그래서 발사·명중 Niagara는 쓰지 않고, 명중은 C++ AEnemyHitFlash(M_EnemyHitFlash) 섬광이 맡음
    ensure_hit_flash_material()
    cdo.set_editor_property("launch_effect", None)
    cdo.set_editor_property("launch_disabled_emitters", [])
    cdo.set_editor_property("impact_effect", None)
    cdo.set_editor_property("impact_disabled_emitters", [])
    cdo.set_editor_property("impact_flash_radius", 40.0)
    unreal.BlueprintEditorLibrary.compile_blueprint(projectile)
    require(lib.save_loaded_asset(projectile, only_if_is_dirty=False), "Failed to save BP_EnemyProjectile")
    # 네이티브 클래스 대신 VFX가 지정된 BP 투사체를 쏘도록 바꿈
    ranged_cdo = unreal.get_default_object(ranged.generated_class())
    ranged_cdo.set_editor_property("projectile_class", projectile.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(ranged)
    require(lib.save_loaded_asset(ranged, only_if_is_dirty=False), "Failed to save " + ranged.get_name())


def place_test_enemies(melee, ranged, large, assassin, bomber, guardian, artillery):
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
    # 바닥이 ±900이라 그 안에 둠. 밖에 두면 시작하자마자 떨어짐
    place("Enemy_Bomber_Test", bomber, unreal.Vector(800.0, 350.0, 150.0))
    place("Enemy_Guardian_Test", guardian, unreal.Vector(-450.0, -350.0, 160.0))
    # 포격병은 멀리서 쏘므로 반대편 끝에 둠
    place("Enemy_Artillery_Test", artillery, unreal.Vector(-750.0, 650.0, 150.0))
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
guardian = ensure_blueprint("BP_GuardianEnemy", "/Script/UT1.GuardianEnemyCharacter")
artillery = ensure_blueprint("BP_ArtilleryEnemy", "/Script/UT1.ArtilleryEnemyCharacter")
configure_enemy(melee, small, "/Script/UT1.MeleeEnemyCharacter")
configure_enemy(ranged, small, "/Script/UT1.RangedEnemyCharacter")
# 대형 메시(약 192cm)가 캡슐 안에 들어오도록 키움. Brute의 1.3배 캡슐 배율은 그대로 유지함
configure_enemy(large, large_profile, "/Script/UT1.BruteEnemyCharacter", capsule=(42.0, 96.0), loot=LARGE_LOOT)
# 암살자(0.92배)와 자폭병(0.8배)은 C++ 캡슐 배율로 소형 몬스터를 줄여 씀
configure_enemy(assassin, small, "/Script/UT1.AssassinEnemyCharacter")
configure_enemy(bomber, small, "/Script/UT1.BomberEnemyCharacter")
# 방패병(1.1배)과 포격병(0.95배)도 소형 몬스터를 캡슐 배율로 키우거나 줄여 씀
configure_enemy(guardian, small, "/Script/UT1.GuardianEnemyCharacter")
configure_enemy(artillery, small, "/Script/UT1.ArtilleryEnemyCharacter")
configure_projectile(ranged)
# 포격병은 같은 투사체 BP를 포물선 포탄(보여 주기 전용)으로 던짐
artillery_cdo = unreal.get_default_object(artillery.generated_class())
artillery_cdo.set_editor_property("lob_projectile_class", lib.load_asset(BP_ROOT + "/BP_EnemyProjectile").generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(artillery)
require(lib.save_loaded_asset(artillery, only_if_is_dirty=False), "Failed to save " + artillery.get_name())
place_test_enemies(melee, ranged, large, assassin, bomber, guardian, artillery)
delete_unreferenced_redirectors(redirectors)
unreal.log("PYW_MONSTER_ENEMIES_SUCCESS")
