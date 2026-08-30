"""Import W11 combat SFX and bind them to shared, character-independent visuals."""

import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceArt", "W11", "CombatAudio")
DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Combat"
PRESENTATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Presentation/Skills"
ABILITY_ROOT = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities"


def set_impact_profile(visual, hit_stop, camera_strength):
    visual.set_editor_property("hit_stop_seconds", hit_stop)
    visual.set_editor_property("critical_hit_stop_seconds", min(0.12, hit_stop + 0.020))
    visual.set_editor_property("defeat_hit_stop_seconds", min(0.12, hit_stop + 0.040))
    visual.set_editor_property("camera_impact_strength", camera_strength)
    visual.set_editor_property("critical_camera_impact_strength", min(8.0, camera_strength + 2.0))
    visual.set_editor_property("defeat_camera_impact_strength", min(8.0, camera_strength + 3.5))
    visual.set_editor_property("camera_impact_duration", 0.12)


def import_sounds():
    names = [
        "BasicAttack", "AbilityCast", "EnemyAttack",
        "EnemyMiasma", "EnemySwordDash", "EnemyStoneSlam",
        "Hit", "Critical", "Defeat",
    ]
    tasks = []
    for name in names:
        source = os.path.join(SOURCE_DIR, f"W11_{name}.wav")
        if not os.path.isfile(source):
            raise RuntimeError(f"Missing generated combat SFX: {source}")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", DESTINATION)
        task.set_editor_property("destination_name", f"S_W11_{name}")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    sounds = {name: unreal.EditorAssetLibrary.load_asset(f"{DESTINATION}/S_W11_{name}") for name in names}
    if not all(sounds.values()):
        raise RuntimeError(f"Combat SFX import incomplete: {sounds}")
    return sounds


def bind_sounds(sounds):
    common = unreal.EditorAssetLibrary.load_asset(f"{PRESENTATION}/DA_SkillVisual_CommonHitSpark")
    if not common:
        raise RuntimeError("Missing shared hit visual")
    common.set_editor_property("cast_sound", sounds["BasicAttack"])
    common.set_editor_property("impact_sound", sounds["Hit"])
    common.set_editor_property("critical_sound", sounds["Critical"])
    common.set_editor_property("defeat_sound", sounds["Defeat"])
    common.set_editor_property("content_version", "W11-M6-v2")
    set_impact_profile(common, 0.025, 1.5)
    unreal.EditorAssetLibrary.save_loaded_asset(common, only_if_is_dirty=False)

    for asset_path in unreal.EditorAssetLibrary.list_assets(PRESENTATION, recursive=False, include_folder=False):
        visual = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(visual, unreal.W11SkillVisualDefinition) or visual == common:
            continue
        definition_id = str(visual.get_editor_property("definition_id"))
        if definition_id.startswith("SkillVisual.Enemy"):
            continue
        visual.set_editor_property("cast_sound", sounds["AbilityCast"])
        visual.set_editor_property("impact_sound", sounds["Hit"])
        visual.set_editor_property("critical_sound", sounds["Critical"])
        visual.set_editor_property("defeat_sound", sounds["Defeat"])
        visual.set_editor_property("content_version", "W11-M6-v2")
        set_impact_profile(visual, 0.030, 2.0)
        unreal.EditorAssetLibrary.save_loaded_asset(visual, only_if_is_dirty=False)

    enemy_specs = {
        "MiasmaBolt": ("EnemyMiasma", "瘴气弹·蚀风声纹"),
        "SwordWraithDash": ("EnemySwordDash", "残剑魂·破空声纹"),
        "StoneFiendSlam": ("EnemyStoneSlam", "石甲妖·沉岩声纹"),
    }
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.W11SkillVisualDefinition)
    for name, (sound_name, display_name) in enemy_specs.items():
        visual_name = f"DA_SkillVisual_Enemy{name}"
        visual_path = f"{PRESENTATION}/{visual_name}"
        enemy_visual = unreal.EditorAssetLibrary.load_asset(visual_path) \
            if unreal.EditorAssetLibrary.does_asset_exist(visual_path) else None
        if not enemy_visual:
            enemy_visual = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                visual_name, PRESENTATION, unreal.W11SkillVisualDefinition, factory
            )
        if not enemy_visual:
            raise RuntimeError(f"Failed to create enemy visual: {visual_name}")
        enemy_visual.set_editor_property("definition_id", f"SkillVisual.Enemy{name}")
        enemy_visual.set_editor_property("display_name", display_name)
        enemy_visual.set_editor_property("content_version", "W11-M6-v2")
        enemy_visual.set_editor_property("cast_sound", sounds[sound_name])
        enemy_visual.set_editor_property("impact_sound", sounds["Hit"])
        enemy_visual.set_editor_property("critical_sound", sounds["Critical"])
        enemy_visual.set_editor_property("defeat_sound", sounds["Defeat"])
        set_impact_profile(enemy_visual, 0.030, 2.5)
        unreal.EditorAssetLibrary.save_loaded_asset(enemy_visual, only_if_is_dirty=False)

        ability = unreal.EditorAssetLibrary.load_asset(f"{ABILITY_ROOT}/Enemies/DA_Ability_{name}")
        if not ability:
            raise RuntimeError(f"Missing enemy ability: {name}")
        ability.set_editor_property("visuals", enemy_visual)
        unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)


if __name__ == "__main__":
    imported = import_sounds()
    bind_sounds(imported)
    unreal.log("W11_COMBAT_SFX_READY count=9 enemy_signatures=3 enemy_abilities=3")
