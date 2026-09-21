import unreal


TEST_LEVEL = "/Game/PYW/Lvl_EnemyBTTest"


levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(TEST_LEVEL), "Failed to load the PYW test level"
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
game_mode = unreal.load_class(None, "/Script/UT1.WASDGameMode")
assert world and game_mode, "WASDGameMode is unavailable; build UT1Editor first"
world.get_world_settings().set_editor_property("default_game_mode", game_mode)
assert levels.save_current_level(), "Failed to save the PYW test level"
unreal.log("PYW_WASD_SETUP_SUCCESS")
