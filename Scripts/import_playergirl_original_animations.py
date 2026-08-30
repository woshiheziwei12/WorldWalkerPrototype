"""Import original PlayerGirl clips onto the current-game Lumine skeleton."""

from __future__ import annotations

import os
import re
from pathlib import Path

import unreal


DEFAULT_FBX_ROOT = Path(
    "E:/workspace/gensehnimport/exports/PlayerGirl_FBX_PlayerGirlSwordComboFixed"
)
FBX_ROOT = Path(os.environ.get("WORLDWALKER_PLAYERGIRL_FBX_ROOT", str(DEFAULT_FBX_ROOT)))
MODEL_ROOT = "/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine"
MESH_PATH = os.environ.get(
    "WORLDWALKER_PLAYERGIRL_MESH_PATH",
    f"{MODEL_ROOT}/OriginalSource/SK_WW_Lumine_OriginalSource",
)
DESTINATION = os.environ.get(
    "WORLDWALKER_PLAYERGIRL_ANIMATION_DESTINATION",
    f"{MODEL_ROOT}/OriginalSource/Actions",
)

EXPECTED_CLIPS = (
    "Attack_01",
    "Attack_02",
    "Attack_03",
    "Attack_04",
    "Attack_05",
    "ClimbU",
    "CrouchRoll",
    "Death",
    "ExtraAttack",
    "FallingAttack_Loop",
    "FallingAttack_Strike",
    "FallToGroundH",
    "FallToGroundL",
    "FlyNormal",
    "Hit_H",
    "Hit_L",
    "Jump",
    "RunBS",
    "RunCycle",
    "RunStopL",
    "SprintBS",
    "SprintCycle",
    "SprintStopL",
    "Standby",
    "SwimF",
    "SwimStandby",
    "WalkCycle",
    "WalkStopL",
)


def _source_path(clip_name: str) -> Path:
    merged = FBX_ROOT / (
        "PlayerGirl@Ani_Avatar_Girl_" + clip_name + "_Merged.fbx"
    )
    if merged.is_file():
        return merged
    return FBX_ROOT / (
        "PlayerGirl@Ani_Avatar_Girl_Sword_PlayerGirl_" + clip_name + ".fbx"
    )


def _validate_sources() -> list[tuple[str, Path]]:
    sources: list[tuple[str, Path]] = []
    requested = {
        value.strip()
        for value in os.environ.get("WORLDWALKER_PLAYERGIRL_CLIP_FILTER", "").split(",")
        if value.strip()
    }
    clip_names = (
        [name for name in EXPECTED_CLIPS if name in requested]
        if requested
        else EXPECTED_CLIPS
    )
    for clip_name in clip_names:
        source = _source_path(clip_name)
        if not source.is_file() or source.stat().st_size < 1024:
            raise RuntimeError(f"Missing or invalid PlayerGirl FBX: {source}")
        sources.append((clip_name, source))
    return sources


def _animation_options(skeleton: object) -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)
    data = options.get_editor_property("anim_sequence_import_data")
    if data is not None:
        data.set_editor_property("import_bone_tracks", True)
        data.set_editor_property("remove_redundant_keys", True)
        data.set_editor_property("import_custom_attribute", False)
        data.set_editor_property("use_default_sample_rate", False)
        data.set_editor_property("custom_sample_rate", 60)
        data.set_editor_property("snap_to_closest_frame_boundary", True)
        data.set_editor_property(
            "animation_length",
            unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME,
        )
    return options


def _normalise(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def _run_import(source: Path, destination_name: str, skeleton: object) -> list[str]:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", _animation_options(skeleton))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(path) for path in task.get_editor_property("imported_object_paths")]


def _import_clip(clip_name: str, source: Path, skeleton: object) -> str:
    stable_name = f"A_WW_PlayerGirl_{clip_name}"
    stable_path = f"{DESTINATION}/{stable_name}"
    before = set(unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=False, include_folder=False))
    imported = _run_import(source, stable_name, skeleton)
    after = set(unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=False, include_folder=False))
    candidates = list(dict.fromkeys(imported + sorted(after - before)))
    sequences: list[tuple[str, unreal.AnimSequence]] = []
    for asset_path in candidates:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.AnimSequence):
            sequences.append((asset_path, asset))
    if len(sequences) != 1:
        sequences = []
        for asset_path in unreal.EditorAssetLibrary.list_assets(
            DESTINATION, recursive=False, include_folder=False
        ):
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if isinstance(asset, unreal.AnimSequence) and _normalise(clip_name) in _normalise(str(asset.get_name())):
                sequences.append((asset_path, asset))
    if len(sequences) != 1:
        raise RuntimeError(
            f"Expected one AnimSequence for {clip_name}, found {len(sequences)}: {candidates}"
        )

    source_path, sequence = sequences[0]
    if sequence.get_editor_property("skeleton") != skeleton:
        raise RuntimeError(f"Animation uses the wrong skeleton: {source_path}")
    if sequence.get_play_length() <= 0.0:
        raise RuntimeError(f"Animation has zero duration: {source_path}")
    source_package_path = source_path.split(".", 1)[0]
    if source_package_path != stable_path:
        if unreal.EditorAssetLibrary.does_asset_exist(stable_path):
            unreal.EditorAssetLibrary.delete_asset(stable_path)
        if not unreal.EditorAssetLibrary.rename_asset(source_package_path, stable_path):
            raise RuntimeError(f"Could not rename {source_package_path} -> {stable_path}")
    unreal.log(
        f"WW_PLAYERGIRL_ANIMATION_IMPORTED name={clip_name} "
        f"length={sequence.get_play_length():.3f} asset={stable_path}"
    )
    return stable_path


def main() -> None:
    sources = _validate_sources()
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"Missing current-game Lumine mesh: {MESH_PATH}")
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        raise RuntimeError(f"Current-game Lumine mesh has no skeleton: {MESH_PATH}")
    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        unreal.EditorAssetLibrary.make_directory(DESTINATION)
    imported: list[str] = []
    failures: list[str] = []
    for name, source in sources:
        try:
            imported.append(_import_clip(name, source, skeleton))
        except Exception as error:
            failures.append(name)
            unreal.log_error(f"WW_PLAYERGIRL_ANIMATION_IMPORT_FAILED name={name} error={error}")
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "WW_PLAYERGIRL_ORIGINAL_ANIMATION_IMPORT_COMPLETE "
        f"animations={len(imported)}/{len(sources)} failures={','.join(failures) or 'none'} "
        f"skeleton={skeleton.get_path_name()} "
        f"source={FBX_ROOT}"
    )


main()
