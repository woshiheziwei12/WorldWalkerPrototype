"""Import W11 combat frames, build Paper2D flipbooks and bind presentation assets.

Run after the WorldWalkerW11 editor module compiles and after
process_w11_combat_animation_art.py. The script is idempotent.
"""

from __future__ import annotations

import json
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
MANIFEST_PATH = PROJECT_ROOT / "SourceArt" / "W11" / "Animations" / "W11_AnimationManifest_v1.json"
CONTENT_ROOT = "/Game/WorldWalker/Worlds/W11_RogueSurvival"
ART_ROOT = f"{CONTENT_ROOT}/Art/Animations"
PRESENTATION_ROOT = f"{CONTENT_ROOT}/Data/Presentation"
TRANSLUCENT_SPRITE_MATERIAL = "/Paper2D/TranslucentUnlitSpriteMaterial"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create directory: {path}")


def ensure_data_asset(name, directory, asset_class):
    ensure_directory(directory)
    path = f"{directory}/{name}"
    asset = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", asset_class)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, directory, asset_class, factory
        )
    if asset is None:
        raise RuntimeError(f"Unable to create data asset: {path}")
    return asset


def set_identity(asset, definition_id, display_name):
    asset.set_editor_property("definition_id", unreal.Name(definition_id))
    asset.set_editor_property("display_name", display_name)
    asset.set_editor_property("content_version", "W11-ANIM-M1-v1")


def import_texture(source_path, destination, texture_name):
    ensure_directory(destination)
    asset_path = f"{destination}/{texture_name}"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(PROJECT_ROOT / source_path))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", texture_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        raise RuntimeError(f"Texture import failed: {asset_path}")
    texture.set_editor_property("srgb", True)
    try:
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    except Exception as error:
        unreal.log_warning(f"Texture normalization warning {asset_path}: {error}")
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def ensure_sprite(
    texture,
    directory,
    sprite_name,
    bottom_pivot,
    pixels_per_unreal_unit=1.0,
):
    ensure_directory(directory)
    path = f"{directory}/{sprite_name}"
    sprite = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if sprite is None:
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            sprite_name, directory, unreal.PaperSprite, unreal.PaperSpriteFactory()
        )
    if sprite is None:
        raise RuntimeError(f"PaperSprite creation failed: {path}")
    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("pixels_per_unreal_unit", pixels_per_unreal_unit)
    sprite.set_editor_property(
        "pivot_mode",
        unreal.SpritePivotMode.BOTTOM_CENTER if bottom_pivot else unreal.SpritePivotMode.CENTER_CENTER,
    )
    try:
        material = unreal.EditorAssetLibrary.load_asset(TRANSLUCENT_SPRITE_MATERIAL)
        if material is None:
            raise RuntimeError("Paper2D translucent sprite material is unavailable")
        sprite.set_editor_property("default_material", material)
    except Exception as error:
        unreal.log_warning(f"Sprite material normalization warning {path}: {error}")
    unreal.EditorAssetLibrary.save_loaded_asset(sprite, only_if_is_dirty=False)
    return sprite


def ensure_flipbook(directory, flipbook_name, sprites, fps):
    ensure_directory(directory)
    path = f"{directory}/{flipbook_name}"
    flipbook = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if flipbook is None:
        flipbook = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            flipbook_name, directory, unreal.PaperFlipbook, unreal.PaperFlipbookFactory()
        )
    if flipbook is None:
        raise RuntimeError(f"PaperFlipbook creation failed: {path}")
    key_frames = []
    for sprite in sprites:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.set_editor_property("sprite", sprite)
        key_frame.set_editor_property("frame_run", 1)
        key_frames.append(key_frame)
    flipbook.set_editor_property("frames_per_second", fps)
    flipbook.set_editor_property("key_frames", key_frames)
    unreal.EditorAssetLibrary.save_loaded_asset(flipbook, only_if_is_dirty=False)
    return flipbook


def build_flipbook(
    frame_paths,
    directory,
    prefix,
    fps,
    bottom_pivot,
    pixels_per_unreal_unit=1.0,
):
    sprites = []
    for index, source_path in enumerate(frame_paths):
        texture = import_texture(source_path, directory, f"T_{prefix}_{index:02d}")
        sprites.append(
            ensure_sprite(
                texture,
                directory,
                f"S_{prefix}_{index:02d}",
                bottom_pivot,
                pixels_per_unreal_unit,
            )
        )
    return ensure_flipbook(directory, f"FB_{prefix}", sprites, fps)


def make_character_presentations(manifest):
    hero_names = {
        "Wei": "威", "Dong": "栋", "Tian": "天", "Xiang": "翔", "Pu": "普",
        "Ying": "迎", "Li": "丽", "Jia": "佳", "Yu": "玉", "Meng": "梦",
    }
    for entry in manifest["characters"]:
        key = entry["hero_id"].split(".")[-1]
        directional = {}
        for facing in ("Right", "Left"):
            action_set = unreal.W11CharacterActionFlipbooks()
            for action in ("Idle", "Move", "Cast", "Hit"):
                action_data = entry["actions"][action]
                frame_paths = action_data[facing.lower()]
                directory = f"{ART_ROOT}/Characters/{key}/{facing}/{action}"
                flipbook = build_flipbook(
                    frame_paths,
                    directory,
                    f"W11_Hero_{key}_{facing}_{action}",
                    action_data["fps"],
                    True,
                    action_data.get("pixels_per_unreal_unit", 1.0),
                )
                action_set.set_editor_property(action.lower(), flipbook)
            # Dodge has no dedicated source frames in the current delivery.
            # Leave the field null so AW11Character's documented runtime
            # fallback resolves Dodge to the facing's Move flipbook.
            directional[facing] = action_set

        presentation = ensure_data_asset(
            f"DA_CharacterAnimation_{key}",
            f"{PRESENTATION_ROOT}/Characters",
            unreal.W11CharacterAnimationSet,
        )
        set_identity(presentation, f"CharacterAnimation.Hero.{key}", f"{hero_names[key]}·通用战斗动作")
        presentation.set_editor_property("right", directional["Right"])
        presentation.set_editor_property("left", directional["Left"])
        presentation.set_editor_property("keep_last_horizontal_facing_for_vertical_movement", True)
        presentation.set_editor_property("world_scale", 0.72)
        unreal.EditorAssetLibrary.save_loaded_asset(presentation, only_if_is_dirty=False)

        hero = unreal.EditorAssetLibrary.load_asset(f"{CONTENT_ROOT}/Data/Heroes/DA_Hero_{key}")
        if hero is None:
            raise RuntimeError(f"Missing hero definition: {key}")
        hero.set_editor_property("combat_animations", presentation)
        unreal.EditorAssetLibrary.save_loaded_asset(hero, only_if_is_dirty=False)
        unreal.log(f"W11_CHARACTER_PRESENTATION_BOUND hero={key}")


def make_enemy_samples(manifest):
    for entry in manifest.get("enemy_samples", []):
        key = entry["enemy_id"].split(".")[-1]
        move = entry["move"]
        ppu = move.get("pixels_per_unreal_unit", 1.0)
        flipbooks = {}
        for facing in ("Right", "Left"):
            directory = f"{ART_ROOT}/Enemies/{key}/{facing}/Move"
            flipbooks[facing] = build_flipbook(
                move[facing.lower()],
                directory,
                f"W11_Enemy_{key}_{facing}_Move",
                move["fps"],
                True,
                ppu,
            )
        enemy = unreal.EditorAssetLibrary.load_asset(
            f"{CONTENT_ROOT}/Data/Enemies/DA_Enemy_{key}"
        )
        if enemy is None:
            raise RuntimeError(f"Missing enemy definition: {key}")
        enemy.set_editor_property("move_flipbook", flipbooks["Right"])
        enemy.set_editor_property("move_flipbook_left", flipbooks["Left"])
        unreal.EditorAssetLibrary.save_loaded_asset(enemy, only_if_is_dirty=False)
        unreal.log(
            f"W11_ENEMY_MOVE_SAMPLE_BOUND enemy={key} frames={len(move['right'])}"
        )


def make_skill_presentations(manifest):
    display_names = {
        "FlowingCloudSword": "流云剑气",
        "FiveThunder": "五雷正法",
        "DarkWaterWard": "玄水护体",
        "SpringRenewal": "回春诀",
        "MoonChasingStrike": "逐月瞬斩",
        "TwoPolesFormation": "两仪剑阵",
        "CommonHitSpark": "通用命中火花",
    }
    scales = {
        "FlowingCloudSword": 1.20,
        "FiveThunder": 1.45,
        "DarkWaterWard": 1.05,
        "SpringRenewal": 1.05,
        "MoonChasingStrike": 1.50,
        "TwoPolesFormation": 1.55,
        "CommonHitSpark": 0.55,
    }
    visual_assets = {}
    for entry in manifest["skills"]:
        key = entry["visual_id"].split(".")[-1]
        directory = f"{ART_ROOT}/Skills/{key}"
        flipbook = build_flipbook(
            entry["frames"], directory, f"W11_Skill_{key}", entry["fps"], False
        )
        visual = ensure_data_asset(
            f"DA_SkillVisual_{key}",
            f"{PRESENTATION_ROOT}/Skills",
            unreal.W11SkillVisualDefinition,
        )
        set_identity(visual, f"SkillVisual.{key}", display_names[key])
        visual.set_editor_property("effect_flipbook", flipbook)
        visual.set_editor_property("directional", entry["directional"])
        visual.set_editor_property("authored_direction", unreal.W11FacingDirection.RIGHT)
        visual.set_editor_property("attach_to_caster", True)
        visual.set_editor_property("local_offset", unreal.Vector2D(145.0, 0.0) if entry["directional"] else unreal.Vector2D(0.0, 0.0))
        visual.set_editor_property("world_scale", scales[key])
        visual.set_editor_property("translucent_sort_priority", 40)
        unreal.EditorAssetLibrary.save_loaded_asset(visual, only_if_is_dirty=False)
        visual_assets[key] = visual

    common_impact = unreal.EditorAssetLibrary.load_asset(
        f"{ART_ROOT}/Skills/CommonHitSpark/FB_W11_Skill_CommonHitSpark"
    )
    for key, visual in visual_assets.items():
        if key != "CommonHitSpark":
            visual.set_editor_property("impact_flipbook", common_impact)
            unreal.EditorAssetLibrary.save_loaded_asset(visual, only_if_is_dirty=False)
            ability = unreal.EditorAssetLibrary.load_asset(
                f"{CONTENT_ROOT}/Data/Abilities/DA_Ability_{key}"
            )
            if ability is None:
                raise RuntimeError(f"Missing ability definition: {key}")
            ability.set_editor_property("visuals", visual)
            unreal.EditorAssetLibrary.save_loaded_asset(ability, only_if_is_dirty=False)
            unreal.log(f"W11_SKILL_PRESENTATION_BOUND ability={key}")


def main():
    if not MANIFEST_PATH.is_file():
        raise RuntimeError(f"Missing processed animation manifest: {MANIFEST_PATH}")
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    make_character_presentations(manifest)
    make_enemy_samples(manifest)
    make_skill_presentations(manifest)
    unreal.log(
        "W11_COMBAT_ANIMATION_IMPORT_COMPLETE "
        f"characters={len(manifest['characters'])} "
        f"enemy_samples={len(manifest.get('enemy_samples', []))} "
        f"skills={len(manifest['skills'])}"
    )


if __name__ == "__main__":
    main()
