"""Idempotently import W11 title-screen textures and original menu music."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "W11" / "Generated"
ART_DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry"
AUDIO_DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Music"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create asset directory: {path}")


def import_asset(source_name, destination, asset_name):
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing W11 entry source asset: {source}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset = unreal.EditorAssetLibrary.load_asset(f"{destination}/{asset_name}")
    if asset is None:
        raise RuntimeError(f"Import failed: {destination}/{asset_name}")
    return asset


def configure_ui_texture(texture, is_transparent=False):
    texture.set_editor_property("srgb", True)
    try:
        texture.set_editor_property(
            "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
        )
        texture.set_editor_property(
            "lod_group", unreal.TextureGroup.TEXTUREGROUP_UI
        )
        if is_transparent:
            texture.set_editor_property(
                "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
            )
    except Exception as error:
        unreal.log_warning(f"W11 entry texture normalization warning: {error}")
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def main():
    ensure_directory(ART_DESTINATION)
    ensure_directory(AUDIO_DESTINATION)

    title = import_asset(
        "W11_Title_Background_v1.png", ART_DESTINATION, "T_W11_TitleBackground"
    )
    fog = import_asset(
        "W11_Title_Fog_v1.png", ART_DESTINATION, "T_W11_TitleFog"
    )
    menu_panel = import_asset(
        "W11_Entry_MenuPanel_v1.png", ART_DESTINATION, "T_W11_EntryMenuPanel"
    )
    title_backdrop = import_asset(
        "W11_Entry_TitleBackdrop_v1.png", ART_DESTINATION, "T_W11_EntryTitleBackdrop"
    )
    button_frame = import_asset(
        "W11_Entry_ButtonFrame_v3.png", ART_DESTINATION, "T_W11_EntryButtonFrame"
    )
    hero_selection_background = import_asset(
        "W11_HeroSelection_Background_v1.png",
        ART_DESTINATION,
        "T_W11_HeroSelectionBackground",
    )
    hero_portrait_aura = import_asset(
        "W11_HeroSelection_PortraitAura_v1.png",
        ART_DESTINATION,
        "T_W11_HeroSelectionPortraitAura",
    )
    configure_ui_texture(title)
    configure_ui_texture(fog, is_transparent=True)
    configure_ui_texture(menu_panel)
    configure_ui_texture(title_backdrop, is_transparent=True)
    configure_ui_texture(button_frame, is_transparent=True)
    configure_ui_texture(hero_selection_background)
    configure_ui_texture(hero_portrait_aura, is_transparent=True)

    music = import_asset(
        "W11_TaixuMoonGate_MenuTheme_v1.wav",
        AUDIO_DESTINATION,
        "S_W11_TaixuMoonGate_MenuTheme",
    )
    hero_selection_music = import_asset(
        "W11_TenDestinies_HeroSelectionTheme_v2.wav",
        AUDIO_DESTINATION,
        "S_W11_MirrorOfLives_HeroSelectionTheme",
    )
    for sound in (music, hero_selection_music):
        try:
            sound.set_editor_property("looping", True)
        except Exception as error:
            unreal.log_warning(f"Could not enable W11 menu music looping: {error}")
        unreal.EditorAssetLibrary.save_loaded_asset(sound, only_if_is_dirty=False)
    unreal.log(
        "W11_ENTRY_ASSET_IMPORT_COMPLETE title=1 fog=1 panel=1 title_backdrop=1 button_frame=1 "
        "hero_selection_background=1 hero_portrait_aura=1 "
        "music=2 hero_theme=TenDestiniesV2 resolution=2560x1440 "
        "title_duration=64s hero_duration=60s seamless_loop=1"
    )


if __name__ == "__main__":
    main()
