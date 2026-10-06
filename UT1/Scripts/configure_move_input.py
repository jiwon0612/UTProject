import unreal


MAPPING_CONTEXT_PATH = "/Game/Input/IMC_Default"
MOVE_ACTION_PATH = "/Game/Input/Actions/IA_Move"


def add_mapping(mapping_context, move_action, key_name, modifiers):
    mapping = mapping_context.map_key(move_action, unreal.Key(key_name))
    mapping.modifiers = modifiers


mapping_context = unreal.load_asset(MAPPING_CONTEXT_PATH)
move_action = unreal.load_asset(MOVE_ACTION_PATH)

if not mapping_context or not move_action:
    raise RuntimeError("Could not load IMC_Default or IA_Move.")

mapping_context.modify()
mapping_context.unmap_all_keys_from_action(move_action)

add_mapping(mapping_context, move_action, "D", [])
add_mapping(mapping_context, move_action, "A", [unreal.InputModifierNegate()])
add_mapping(mapping_context, move_action, "W", [unreal.InputModifierSwizzleAxis()])
add_mapping(
    mapping_context,
    move_action,
    "S",
    [unreal.InputModifierSwizzleAxis(), unreal.InputModifierNegate()],
)

if not unreal.EditorAssetLibrary.save_loaded_asset(mapping_context):
    raise RuntimeError("Failed to save IMC_Default.")

unreal.log("Configured IA_Move: W/S = Y axis, A/D = X axis.")
