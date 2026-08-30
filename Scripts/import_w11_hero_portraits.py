"""Idempotently import the ten W11 portraits and character-selection avatars."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "W11" / "Heroes" / "Generated"
AVATAR_SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "W11" / "Heroes" / "Avatars"
DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Heroes"
AVATAR_DESTINATION = f"{DESTINATION}/Avatars"
HERO_KEYS = ("Wei", "Dong", "Tian", "Xiang", "Pu", "Ying", "Li", "Jia", "Yu", "Meng")


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create asset directory: {path}")


def import_ui_texture(source, destination, asset_name, key):
    if not source.is_file():
        raise RuntimeError(f"Missing W11 hero source texture: {source}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(f"{destination}/{asset_name}")
    if texture is None:
        raise RuntimeError(f"Import failed: {destination}/{asset_name}")
    texture.set_editor_property("srgb", True)
    try:
        texture.set_editor_property(
            "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
        )
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
        )
    except Exception as error:
        unreal.log_warning(f"W11 hero texture normalization warning {key}: {error}")
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def import_portrait(key):
    import_ui_texture(
        SOURCE_ROOT / f"W11_Hero_{key}_v1.png",
        DESTINATION,
        f"T_W11_Hero_{key}",
        key,
    )


def import_avatar(key):
    import_ui_texture(
        AVATAR_SOURCE_ROOT / f"W11_Hero_{key}_Avatar_v1.png",
        AVATAR_DESTINATION,
        f"T_W11_Hero_{key}_Avatar",
        key,
    )


def main():
    ensure_directory(DESTINATION)
    ensure_directory(AVATAR_DESTINATION)
    for key in HERO_KEYS:
        import_portrait(key)
        import_avatar(key)
    unreal.log("W11_HERO_PORTRAIT_IMPORT_COMPLETE portraits=10 avatars=10 alpha=1")


if __name__ == "__main__":
    main()
