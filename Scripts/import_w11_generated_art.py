"""Import generated W11 prototype art as textures and centered Paper2D sprites."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "W11" / "Generated"
DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Prototype"
TRANSLUCENT_SPRITE_MATERIAL = "/Paper2D/TranslucentUnlitSpriteMaterial"
ASSETS = (
    ("W11_Arena_Background_v1.png", "T_W11_Arena", "S_W11_Arena"),
    (
        "W11_FirstArena_Continuous_v3_8K.png",
        "T_W11_FirstArena_Continuous",
        "S_W11_FirstArena_Continuous",
    ),
    (
        "W11_Arena_JadeTile_v1_source.png",
        "T_W11_Arena_JadeTile",
        "S_W11_Arena_JadeTile",
    ),
    ("W11_FirstArena_NW_v1.png", "T_W11_FirstArena_NW", "S_W11_FirstArena_NW"),
    ("W11_FirstArena_NE_v1.png", "T_W11_FirstArena_NE", "S_W11_FirstArena_NE"),
    ("W11_FirstArena_SW_v1.png", "T_W11_FirstArena_SW", "S_W11_FirstArena_SW"),
    ("W11_FirstArena_SE_v1.png", "T_W11_FirstArena_SE", "S_W11_FirstArena_SE"),
    ("RuntimeReady/W11_Player_v1.png", "T_W11_Player", "S_W11_Player"),
    ("RuntimeReady/W11_MiasmaWisp_v1.png", "T_W11_MiasmaWisp", "S_W11_MiasmaWisp"),
    ("RuntimeReady/W11_StoneFiend_v1.png", "T_W11_StoneFiend", "S_W11_StoneFiend"),
    ("RuntimeReady/W11_SwordWraith_v1.png", "T_W11_SwordWraith", "S_W11_SwordWraith"),
)
ENEMY_SPRITES = (
    (
        "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_MiasmaWisp",
        f"{DESTINATION}/S_W11_MiasmaWisp",
    ),
    (
        "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_StoneFiend",
        f"{DESTINATION}/S_W11_StoneFiend",
    ),
    (
        "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_SwordWraith",
        f"{DESTINATION}/S_W11_SwordWraith",
    ),
)


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create asset directory: {path}")


def import_texture(source_name, texture_name):
    source = SOURCE_ROOT / source_name
    if not source.is_file():
        raise RuntimeError(f"Missing W11 source art: {source}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", texture_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(f"{DESTINATION}/{texture_name}")
    if texture is None:
        raise RuntimeError(f"Texture import failed: {texture_name}")
    texture.set_editor_property("srgb", True)
    try:
        if texture_name in (
            "T_W11_FirstArena_Continuous",
            "T_W11_Arena_JadeTile",
        ):
            texture.set_editor_property(
                "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SHARPEN4
            )
            texture.set_editor_property("filter", unreal.TextureFilter.TF_TRILINEAR)
            texture.set_editor_property(
                "compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT
            )
            texture.set_editor_property("max_texture_size", 8192)
        else:
            texture.set_editor_property(
                "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
            )
            texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
            texture.set_editor_property(
                "compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON
            )
    except Exception as error:
        unreal.log_warning(f"Could not disable mips for {texture_name}: {error}")
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def ensure_sprite(sprite_name, texture):
    path = f"{DESTINATION}/{sprite_name}"
    sprite = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if sprite is None:
        factory = unreal.PaperSpriteFactory()
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            sprite_name, DESTINATION, unreal.PaperSprite, factory
        )
    if sprite is None:
        raise RuntimeError(f"PaperSprite creation failed: {path}")
    # UE 5.8 keeps UPaperSpriteFactory::InitialTexture as a native-only field,
    # so Python creates the sprite first. set_editor_property dispatches the
    # property-change notification that initializes the full source rectangle.
    sprite.set_editor_property("source_texture", texture)
    try:
        sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
        sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
        material = unreal.EditorAssetLibrary.load_asset(TRANSLUCENT_SPRITE_MATERIAL)
        if material is None:
            raise RuntimeError("Paper2D translucent sprite material is unavailable")
        sprite.set_editor_property("default_material", material)
    except Exception as error:
        unreal.log_warning(f"Could not normalize {sprite_name}: {error}")
    unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False)
    return sprite


def bind_enemy_sprites():
    for definition_path, sprite_path in ENEMY_SPRITES:
        definition = unreal.EditorAssetLibrary.load_asset(definition_path)
        sprite = unreal.EditorAssetLibrary.load_asset(sprite_path)
        if definition is None or sprite is None:
            raise RuntimeError(
                f"Unable to bind enemy presentation: {definition_path} -> {sprite_path}"
            )
        definition.set_editor_property("idle_sprite", sprite)
        unreal.EditorAssetLibrary.save_loaded_asset(
            definition, only_if_is_dirty=False
        )


def main():
    ensure_directory(DESTINATION)
    for source_name, texture_name, sprite_name in ASSETS:
        texture = import_texture(source_name, texture_name)
        ensure_sprite(sprite_name, texture)
        unreal.log(f"W11_ART_ASSET_READY {sprite_name}")
    bind_enemy_sprites()
    unreal.log(
        "W11_GENERATED_ART_IMPORT_COMPLETE textures=11 sprites=11 enemies=3 "
        "arena_backdrops=3 active_arena_master=8192 high_detail_tile=1254 "
        "legacy_arena_tiles=4"
    )


if __name__ == "__main__":
    main()
