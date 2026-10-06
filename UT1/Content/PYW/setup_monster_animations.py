"""Mannequin 애니메이션을 Dungeon Pack 소형/대형 몬스터 스켈레톤으로 리타게팅함.

다시 실행해도 같은 결과가 나오도록 IK Rig, Retargeter, 리타게팅 결과, 이동 BlendSpace를
매번 새로 만든다. 소형/대형 몬스터는 본 계층이 같아서 체인 정의 하나를 함께 씀.
"""
import unreal

ROOT = "/Game/PYW/Animation"
MANNY_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
MANNY_RIG = ROOT + "/IK_Mannequin"
MONSTER_ROOT = "/Game/CJW/Assets/Dungeon_Pack/Assets/Pack_Characters/Characters/Monsters"
ANIMS = "/Game/Characters/Mannequins/Anims"
# CJW의 hackNSlash 검술 콤보는 별도 SK_Mannequin 사본을 써서 그 메시를 원본으로 리타게팅함 (읽기만 함)
HNS_ANIMS = "/Game/CJW/Assets/hackNSlash/Animations"
HNS_MESH = "/Game/CJW/Assets/hackNSlash/Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple"
SOURCE_MESHES = ((HNS_ANIMS, HNS_MESH), (ANIMS, MANNY_MESH))

PROFILES = (
    # key, 접두어, 대상 메시
    ("Small", "SM_", MONSTER_ROOT + "/Small/Assets/SK_Monster_Small"),
    ("Large", "LM_", MONSTER_ROOT + "/Large/Assets/SK_Monster_Large"),
)

SOURCES = (
    ANIMS + "/Unarmed/MM_Idle",
    ANIMS + "/Unarmed/Walk/MF_Unarmed_Walk_Fwd",
    ANIMS + "/Unarmed/Walk/MF_Unarmed_Walk_Bwd",
    ANIMS + "/Unarmed/Walk/MF_Unarmed_Walk_Left",
    ANIMS + "/Unarmed/Walk/MF_Unarmed_Walk_Right",
    ANIMS + "/Unarmed/Jog/MF_Unarmed_Jog_Fwd",
    ANIMS + "/Unarmed/Jog/MF_Unarmed_Jog_Bwd",
    ANIMS + "/Unarmed/Jog/MF_Unarmed_Jog_Left",
    ANIMS + "/Unarmed/Jog/MF_Unarmed_Jog_Right",
    ANIMS + "/Unarmed/Attack/MM_Attack_01",
    ANIMS + "/Unarmed/Attack/MM_Attack_02",
    ANIMS + "/Unarmed/Attack/MM_Attack_03",
    ANIMS + "/Unarmed/Attack/MM_ChargedAttack",
    ANIMS + "/Death/MM_Death_Front_01",
    ANIMS + "/Death/MM_Death_Back_01",
    ANIMS + "/Death/MM_Death_Left_01",
    ANIMS + "/Death/MM_Death_Right_01",
    ANIMS + "/Rifle/HitReact/MM_HitReact_Front_Med_01",
    ANIMS + "/Rifle/HitReact/MM_HitReact_Back_Med_01",
    # 원거리 시전 자세로 쓰는 조준 모션 (무기 없이 손을 뻗은 모습)
    ANIMS + "/Pistol/MF_Pistol_Idle_ADS",
    ANIMS + "/Rifle/MF_Rifle_Idle_ADS",
    # 패턴마다 다른 모션을 주기 위한 검술 모션 (무기 없이 쓰면 맨손 찌르기·휘두르기·내려찍기로 보임)
    HNS_ANIMS + "/Combo_2/Anim_Combo_2_Br_4",
    HNS_ANIMS + "/Combo_1/Anim_Combo_1_Br_3",
    HNS_ANIMS + "/Combo_6/Anim_Combo_6_Br_2",
)

# 골반이 크게 앞으로 나가는 모션임. 적의 이동은 C++ Lunge가 맡으므로 수평 이동을 지워 제자리 모션으로 씀
IN_PLACE = {"Anim_Combo_2_Br_4", "Anim_Combo_1_Br_3", "Anim_Combo_6_Br_2"}

# Mannequin IK_Mannequin의 체인 이름과 똑같이 지어서 EXACT 자동 매핑이 되게 함.
# 몬스터는 Rigify 계열이라 spine이 골반, spine_003이 가슴, spine_004/006이 목/머리임.
MONSTER_CHAINS = [
    ("Spine", "spine_001", "spine_003"),
    ("Neck", "spine_004", "spine_004"),
    ("Head", "spine_006", "spine_006"),
]
for side in ("Left", "Right"):
    s = side[0]
    MONSTER_CHAINS += [
        (side + "Clavicle", "shoulder_" + s, "shoulder_" + s),
        (side + "Arm", "upper_arm_" + s, "hand_" + s),
        (side + "Leg", "thigh_" + s, "toe_" + s),
        (side + "Thumb", "thumb_01_" + s, "thumb_03_" + s),
        (side + "Index", "f_index_01_" + s, "f_index_03_" + s),
        (side + "Middle", "f_middle_01_" + s, "f_middle_03_" + s),
        (side + "Ring", "f_ring_01_" + s, "f_ring_03_" + s),
        (side + "Pinky", "f_pinky_01_" + s, "f_pinky_03_" + s),
    ]

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def delete_if_exists(path):
    if lib.does_asset_exist(path):
        require(lib.delete_asset(path), "Failed to delete " + path)


def load_or_create(name, folder, asset_class, factory):
    # 지우고 같은 이름으로 다시 만들면 메모리에 남은 이전 객체와 충돌해서, 있으면 재사용하고 내용을 초기화함
    path = folder + "/" + name
    if lib.does_asset_exist(path):
        return require(lib.load_asset(path), "Failed to load " + path)
    return require(tools.create_asset(name, folder, asset_class, factory), "Failed to create " + name)


def build_monster_rig(key, mesh):
    rig = load_or_create("IK_Monster" + key, ROOT, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(mesh)
    for chain in list(controller.get_retarget_chains()):
        controller.remove_retarget_chain(chain.get_editor_property("chain_name"))
    require(controller.set_retarget_root("spine"), "Failed to set retarget root")
    for chain, start, end in MONSTER_CHAINS:
        controller.add_retarget_chain(chain, start, end, "")
    require(lib.save_loaded_asset(rig, only_if_is_dirty=False), "Failed to save rig")
    return rig


def build_retargeter(key, source_rig, target_rig):
    retargeter = load_or_create("RTG_MannyToMonster" + key, ROOT, unreal.IKRetargeter, unreal.IKRetargetFactory())
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    controller.remove_all_ops()
    controller.add_default_ops()
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    # Manny는 A포즈, 몬스터는 T포즈라서 대상 리타겟 포즈를 원본 체인 방향에 맞춰 정렬함
    controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    require(lib.save_loaded_asset(retargeter, only_if_is_dirty=False), "Failed to save retargeter")
    return retargeter


def retarget(prefix, folder, target_mesh, retargeter):
    """배치 리타게팅은 /Game 루트에 결과를 만들기 때문에, 만든 뒤 바로 PYW 폴더로 옮김.

    원본 애니메이션의 스켈레톤 에셋마다 원본 메시가 달라서 묶음별로 실행함.
    """
    for path in SOURCES:
        name = prefix + path.rsplit("/", 1)[1]
        delete_if_exists("/Game/" + name)
        delete_if_exists(folder + "/" + name)
    unreal.SystemLibrary.collect_garbage()
    for source_root, source_mesh_path in SOURCE_MESHES:
        group = [path for path in SOURCES if path.startswith(source_root + "/")]
        assets = [require(lib.find_asset_data(path).is_valid() and lib.find_asset_data(path), "Missing " + path) for path in group]
        source_mesh = require(lib.load_asset(source_mesh_path), "Missing " + source_mesh_path)
        unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            assets, source_mesh, target_mesh, retargeter, "", "", prefix, "", False, True)
    result = {}
    for path in SOURCES:
        source_name = path.rsplit("/", 1)[1]
        name = prefix + source_name
        require(lib.rename_asset("/Game/" + name, folder + "/" + name), "Failed to move " + name)
        animation = require(lib.load_asset(folder + "/" + name), "Missing " + name)
        restore_root_scale(animation, target_mesh, source_name in IN_PLACE)
        result[source_name] = animation
    return result


def restore_root_scale(animation, mesh, in_place=False):
    """리타게터가 버린 루트 본 배율을 되돌림.

    Dungeon Pack 스켈레톤은 Blender에서 넘어와 Root 본에 x100 배율이 있고, 그 아래 본은 미터 단위임.
    리타게터는 본 배율을 지원하지 않아서 결과 Root 배율이 1이 되고, 골반(Root의 직계 자식) 위치는
    cm 값으로 기록됨. 그대로 두면 스키닝에서 메시가 1/100로 줄어들기 때문에, Root 배율을 원래 값으로
    돌리고 직계 자식의 위치를 같은 비율로 나눔. 나머지 본은 원래 로컬 값이 유지되어 손대지 않음.

    in_place이면 골반의 수평(X/Y) 위치를 첫 프레임 값으로 고정해 제자리 모션으로 만듦. 점프 같은
    수직(Z) 움직임은 남김.
    """
    component = unreal.new_object(unreal.SkeletalMeshComponent, name="RootScaleProbe")
    component.set_skeletal_mesh_asset(mesh)
    root = component.get_bone_name(0)
    scale = component.get_ref_pose_transform(0).scale3d
    if abs(scale.x - 1.0) < 1e-3:
        return
    children = [component.get_bone_name(i) for i in range(component.get_num_bones())
                if component.get_parent_bone(component.get_bone_name(i)) == root]
    controller = animation.get_editor_property("controller")
    key_count = animation.get_editor_property("data_model_interface").get_number_of_keys()
    controller.open_bracket("Restore root scale", False)
    for bone in [root] + children:
        poses = [unreal.AnimationLibrary.get_bone_pose_for_frame(animation, bone, frame, False) for frame in range(key_count)]
        positions = [pose.translation for pose in poses]
        rotations = [pose.rotation for pose in poses]
        if bone == root:
            scales = [unreal.Vector(scale.x, scale.y, scale.z) for _ in positions]
        else:
            scales = [unreal.Vector(1.0, 1.0, 1.0) for _ in positions]
            positions = [unreal.Vector(p.x / scale.x, p.y / scale.y, p.z / scale.z) for p in positions]
            if in_place and positions:
                start = positions[0]
                positions = [unreal.Vector(start.x, start.y, p.z) for p in positions]
        controller.set_bone_track_keys(bone, positions, rotations, scales, False)
    controller.close_bracket(False)


def make_blend_parameter(name, minimum, maximum, grid):
    parameter = unreal.BlendParameter()
    parameter.set_editor_property("display_name", name)
    parameter.set_editor_property("min", minimum)
    parameter.set_editor_property("max", maximum)
    parameter.set_editor_property("grid_num", grid)
    return parameter


def make_sample(animation, direction, speed):
    sample = unreal.BlendSample()
    sample.set_editor_property("animation", animation)
    sample.set_editor_property("sample_value", unreal.Vector(direction, speed, 0.0))
    sample.set_editor_property("rate_scale", 1.0)
    return sample


def build_locomotion(key, prefix, folder, mesh, anims):
    """EnemyCharacter::Tick이 넣는 (Direction -180..180, Speed 0..600) 축에 맞춘 단순 4방향 이동임."""
    factory = unreal.BlendSpaceFactoryNew()
    factory.set_editor_property("target_skeleton", mesh.skeleton)
    factory.set_editor_property("preview_skeletal_mesh", mesh)
    space = load_or_create("BS_Monster" + key + "_Locomotion", folder, unreal.BlendSpace, factory)
    space.set_editor_property("blend_parameters", [
        make_blend_parameter("Direction", -180.0, 180.0, 4),
        make_blend_parameter("Speed", 0.0, 600.0, 2),
        make_blend_parameter("None", 0.0, 100.0, 4),
    ])
    samples = []
    for speed, gait in ((300.0, "Walk"), (600.0, "Jog")):
        get = lambda direction: anims["MF_Unarmed_{}_{}".format(gait, direction)]
        samples += [
            make_sample(get("Bwd"), -180.0, speed),
            make_sample(get("Left"), -90.0, speed),
            make_sample(get("Fwd"), 0.0, speed),
            make_sample(get("Right"), 90.0, speed),
            make_sample(get("Bwd"), 180.0, speed),
        ]
    for direction in (-180.0, -90.0, 0.0, 90.0, 180.0):
        samples.append(make_sample(anims["MM_Idle"], direction, 0.0))
    space.set_editor_property("sample_data", samples)
    # sample_data만 바꾸면 런타임 삼각분할이 비어 T포즈가 나와서 C++ 보조 함수로 다시 계산함
    valid = unreal.PYWEditorLibrary.rebuild_blend_space(space)
    require(valid == len(samples), "BlendSpace {} has only {}/{} valid samples".format(space.get_name(), valid, len(samples)))
    require(lib.save_loaded_asset(space, only_if_is_dirty=False), "Failed to save blend space")
    return space


def bone_motion(animation, bone):
    """리타게팅이 실제로 본을 움직였는지 확인하는 간단한 지표(구간 내 최대 회전 차이, 도)."""
    length = animation.get_play_length()
    poses = [unreal.AnimationLibrary.get_bone_pose_for_time(animation, bone, length * t / 8.0, False) for t in range(9)]
    first = poses[0].rotation
    return max(abs(first.angular_distance(p.rotation)) for p in poses) * 57.2958


def remove_legacy_assets():
    # 빈 IK_Monster로 만들어져 본 매핑이 없던 이전 결과물(대각선 이동, 깨진 SM_BS_Idle_Walk_Run 등)을 정리함
    for key, prefix, _ in PROFILES:
        expected = {prefix + path.rsplit("/", 1)[1] for path in SOURCES} | {"BS_Monster{}_Locomotion".format(key)}
        for path in set(lib.list_assets(ROOT + "/" + key, recursive=False)):
            if path.split(".")[-1] not in expected:
                delete_if_exists(path.split(".")[0])
    delete_if_exists(ROOT + "/RTG_MannyToMonster")
    delete_if_exists(ROOT + "/IK_Monster")


remove_legacy_assets()
manny_rig = require(lib.load_asset(MANNY_RIG), "Missing IK_Mannequin")
for key, prefix, mesh_path in PROFILES:
    folder = ROOT + "/" + key
    lib.make_directory(folder)
    mesh = require(lib.load_asset(mesh_path), "Missing " + mesh_path)
    rig = build_monster_rig(key, mesh)
    retargeter = build_retargeter(key, manny_rig, rig)
    anims = retarget(prefix, folder, mesh, retargeter)
    build_locomotion(key, prefix, folder, mesh, anims)
    for name in ("MM_Attack_01", "MF_Unarmed_Jog_Fwd", "MM_Death_Front_01") + tuple(sorted(IN_PLACE)):
        unreal.log("PYW_MONSTER_ANIM_CHECK {} {} upper_arm_R={:.1f} thigh_L={:.1f}".format(
            key, name, bone_motion(anims[name], "upper_arm_R"), bone_motion(anims[name], "thigh_L")))
    # 전체 dirty 패키지 저장은 다른 팀원 에셋까지 건드릴 수 있어서 PYW 폴더만 저장함
    lib.save_directory(folder, only_if_is_dirty=True, recursive=True)
unreal.log("PYW_MONSTER_ANIMATIONS_SUCCESS")
