"""Idempotently import W11 sect-selection artwork as UI textures."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "W11" / "Sects"
ART_DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Sects"

SECT_ARTWORK = (
    ("AzureCloudSword", "云海御剑", "W11_Sect_AzureCloudSword_v3_AIUpscale8K.png", "T_W11_Sect_AzureCloudSword"),
    ("ThunderManor", "玄霄驭雷", "W11_Sect_ThunderManor_v3_AIUpscale8K.png", "T_W11_Sect_ThunderManor"),
    ("CanglanPalace", "沧海玄护", "W11_Sect_CanglanPalace_v3_AIUpscale8K.png", "T_W11_Sect_CanglanPalace"),
    ("DanxiaValley", "丹霞济世", "W11_Sect_DanxiaValley_v3_AIUpscale8K.png", "T_W11_Sect_DanxiaValley"),
    ("ShadowMoonTower", "月影瞬杀", "W11_Sect_ShadowMoonTower_v3_AIUpscale8K.png", "T_W11_Sect_ShadowMoonTower"),
    ("TianjiFormation", "天机演阵", "W11_Sect_TianjiFormation_v3_AIUpscale8K.png", "T_W11_Sect_TianjiFormation"),
)


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create asset directory: {path}")


def import_texture(source_name, asset_name):
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing W11 sect artwork: {source}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", ART_DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.EditorAssetLibrary.load_asset(f"{ART_DESTINATION}/{asset_name}")
    if texture is None:
        raise RuntimeError(f"Import failed: {ART_DESTINATION}/{asset_name}")
    texture.set_editor_property("srgb", True)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("max_texture_size", 8192)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)


def main():
    ensure_directory(ART_DESTINATION)
    for sect_key, style_tagline, source_name, asset_name in SECT_ARTWORK:
        import_texture(source_name, asset_name)
        texture = unreal.EditorAssetLibrary.load_asset(
            f"{ART_DESTINATION}/{asset_name}"
        )
        sect = unreal.EditorAssetLibrary.load_asset(
            f"/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Sects/DA_Sect_{sect_key}"
        )
        if sect is None:
            raise RuntimeError(f"Missing W11 sect definition: DA_Sect_{sect_key}")
        sect.set_editor_property("style_tagline", style_tagline)
        sect.set_editor_property("selection_artwork", texture)
        unreal.EditorAssetLibrary.save_loaded_asset(sect, only_if_is_dirty=False)
    unreal.log(
        "W11_SECT_SELECTION_ART_IMPORT_COMPLETE count=6 definitions=6 "
        "resolution=7680x4320 aspect=16:9 max_texture_size=8192 source=v3_AIUpscale8K"
    )


if __name__ == "__main__":
    main()
