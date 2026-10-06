import unreal

root = "/Game/Characters/Mannequins/Anims/Unarmed/"
movement = unreal.load_asset(root + "BS_Idle_Walk_Run")
attack = unreal.load_asset(root + "Attack/MM_Attack_01")
mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
assert movement and attack and mesh
for asset in (movement, attack, mesh):
    unreal.log_warning("ENEMY_ANIM_ASSET " + asset.get_name() + " skeleton=" + str(asset.get_editor_property("skeleton")))
for index, axis in enumerate(movement.get_editor_property("blend_parameters")):
    unreal.log_warning("ENEMY_ANIM_AXIS " + str(index) + " " + axis.get_editor_property("display_name") + " " + str(axis.get_editor_property("min")) + " " + str(axis.get_editor_property("max")))
for sample in movement.get_editor_property("sample_data"):
    unreal.log_warning("ENEMY_ANIM_SAMPLE " + str(sample.get_editor_property("sample_value")) + " " + str(sample.get_editor_property("animation")))
unreal.log_warning("ENEMY_ANIM_LENGTH " + str(attack.get_editor_property("sequence_length")))
