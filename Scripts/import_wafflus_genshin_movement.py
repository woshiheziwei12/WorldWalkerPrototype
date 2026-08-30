"""Import Wafflus' 13 Genshin-like movement clips into Unreal Engine.

The Unity project is expected at ``E:/workspace/unity-genshin-impact-movement-system``.
Run ``Scripts/Unity/GenshinAnimationFbxExporter.cs`` through Unity first so its
``ExportedFBX`` folder contains a bind-pose FBX and one FBX per source clip.
"""

from __future__ import annotations

import os
import re
from pathlib import Path

import unreal


DEFAULT_SOURCE_ROOT = Path("E:/workspace/unity-genshin-impact-movement-system")
SOURCE_ROOT = Path(
    os.environ.get("WORLDWALKER_WAFFLUS_ROOT", str(DEFAULT_SOURCE_ROOT))
)
FBX_ROOT = SOURCE_ROOT / "ExportedFBX"
BIND_POSE_FBX = FBX_ROOT / "WWMixamo_Skeleton.fbx"
DESTINATION = "/Game/WorldWalker/Shared/ThirdParty/Wafflus/GenshinMovement"
MESH_NAME = "SK_WW_GenshinMotionSource"
MESH_PATH = f"{DESTINATION}/{MESH_NAME}"

REQUIRED_CLIPS = (
    "Idle",
    "Walk",
    "Run",
    "Sprint",
    "Dash",
    "Jump",
    "Fall",
    "LightLanding",
    "HardLanding",
    "Roll",
    "LightStop",
    "MediumStop",
    "HardStop",
)


def _ensure_sources() -> list[tuple[str, Path]]:
    if not BIND_POSE_FBX.is_file():
        raise RuntimeError(f"Missing exported bind-pose FBX: {BIND_POSE_FBX}")
    resolved: list[tuple[str, Path]] = []
    for clip_name in REQUIRED_CLIPS:
        path = FBX_ROOT / f"WWMixamo@{clip_name}.fbx"
        if not path.is_file() or path.stat().st_size < 1024:
            raise RuntimeError(f"Missing or invalid exported animation FBX: {path}")
        resolved.append((clip_name, path))
    return resolved


def _ensure_destination() -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        unreal.EditorAssetLibrary.make_directory(DESTINATION)


def _run_import(
    source: Path,
    destination_name: str,
    options: unreal.FbxImportUI,
) -> list[str]:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(path) for path in task.get_editor_property("imported_object_paths")]


def _mesh_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property(
        "mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH
    )
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)
    data = options.get_editor_property("skeletal_mesh_import_data")
    if data is not None:
        data.set_editor_property("convert_scene", True)
        data.set_editor_property("convert_scene_unit", True)
        data.set_editor_property("import_morph_targets", False)
        data.set_editor_property("preserve_smoothing_groups", True)
    return options


def _import_source_mesh() -> tuple[unreal.SkeletalMesh, object]:
    imported = _run_import(BIND_POSE_FBX, MESH_NAME, _mesh_options())
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        candidates = [unreal.EditorAssetLibrary.load_asset(path) for path in imported]
        mesh = next(
            (candidate for candidate in candidates if isinstance(candidate, unreal.SkeletalMesh)),
            None,
        )
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"Wafflus bind-pose import did not create a skeletal mesh: {imported}")
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        raise RuntimeError(f"Imported Wafflus mesh has no skeleton: {MESH_PATH}")
    return mesh, skeleton


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
        data.set_editor_property("custom_sample_rate", 30)
        data.set_editor_property(
            "animation_length",
            unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME,
        )
    return options


def _normalise(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.lower())


def _import_clip(name: str, source: Path, skeleton: object) -> str:
    stable_name = f"A_WW_Genshin_{name}"
    before = set(
        unreal.EditorAssetLibrary.list_assets(
            DESTINATION, recursive=False, include_folder=False
        )
    )
    imported = _run_import(source, stable_name, _animation_options(skeleton))
    after = set(
        unreal.EditorAssetLibrary.list_assets(
            DESTINATION, recursive=False, include_folder=False
        )
    )
    candidates = list(dict.fromkeys(imported + sorted(after - before)))
    sequences: list[tuple[str, unreal.AnimSequence]] = []
    for asset_path in candidates:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.AnimSequence):
            sequences.append((asset_path, asset))
    if len(sequences) != 1:
        matching: list[tuple[str, unreal.AnimSequence]] = []
        for asset_path in unreal.EditorAssetLibrary.list_assets(
            DESTINATION, recursive=False, include_folder=False
        ):
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if (
                isinstance(asset, unreal.AnimSequence)
                and _normalise(name) in _normalise(str(asset.get_name()))
            ):
                matching.append((asset_path, asset))
        sequences = matching
    if len(sequences) != 1:
        raise RuntimeError(
            f"Expected one AnimSequence for {name}, found {len(sequences)}: {candidates}"
        )

    source_path, sequence = sequences[0]
    if sequence.get_editor_property("skeleton") != skeleton:
        raise RuntimeError(f"Imported clip uses the wrong skeleton: {source_path}")
    if sequence.get_play_length() <= 0.0:
        raise RuntimeError(f"Imported clip has zero duration: {source_path}")
    stable_path = f"{DESTINATION}/{stable_name}"
    # Interchange returns object paths (``/Game/Foo/Bar.Bar``), while
    # EditorAssetLibrary rename/delete APIs operate on package paths
    # (``/Game/Foo/Bar``).  Treat both spellings as the same asset.
    source_package_path = source_path.split(".", 1)[0]
    if source_package_path != stable_path:
        if unreal.EditorAssetLibrary.does_asset_exist(stable_path):
            unreal.EditorAssetLibrary.delete_asset(stable_path)
        if not unreal.EditorAssetLibrary.rename_asset(source_package_path, stable_path):
            raise RuntimeError(
                f"Could not rename {source_package_path} -> {stable_path}"
            )
    return stable_path


def main() -> None:
    sources = _ensure_sources()
    _ensure_destination()
    mesh, skeleton = _import_source_mesh()
    imported = [_import_clip(name, source, skeleton) for name, source in sources]
    unreal.EditorAssetLibrary.save_directory(
        DESTINATION, only_if_is_dirty=False, recursive=True
    )
    unreal.log(
        "WW_WAFFLUS_GENSHIN_MOVEMENT_IMPORT_COMPLETE "
        f"mesh={mesh.get_path_name()} animations={len(imported)}/13 "
        f"source={FBX_ROOT}"
    )


main()
