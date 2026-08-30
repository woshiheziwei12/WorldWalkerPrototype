"""Import the Unity-baked PlayerGirl FBX as a dedicated invisible pose-source mesh."""

from __future__ import annotations

import os
from pathlib import Path

import unreal


FBX_ROOT = Path(
    os.environ.get(
        "WORLDWALKER_PLAYERGIRL_FBX_ROOT",
        "E:/workspace/gensehnimport/exports/PlayerGirl_FBX_PlayerGirlSwordComboFixed",
    )
)
SOURCE = FBX_ROOT / "PlayerGirl@Ani_Avatar_Girl_Standby_Merged.fbx"
DESTINATION = os.environ.get(
    "WORLDWALKER_PLAYERGIRL_SOURCE_DESTINATION",
    "/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource",
)
STABLE_MESH_NAME = os.environ.get(
    "WORLDWALKER_PLAYERGIRL_SOURCE_MESH_NAME",
    "SK_WW_Lumine_OriginalSource",
)
STABLE_MESH_PATH = f"{DESTINATION}/{STABLE_MESH_NAME}"


def import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)
    mesh_data = options.get_editor_property("skeletal_mesh_import_data")
    if mesh_data is not None:
        mesh_data.set_editor_property("import_morph_targets", False)
        mesh_data.set_editor_property("update_skeleton_reference_pose", False)
        mesh_data.set_editor_property("use_t0_as_ref_pose", False)
        mesh_data.set_editor_property("import_meshes_in_bone_hierarchy", True)
        mesh_data.set_editor_property("preserve_smoothing_groups", True)
    return options


def main() -> None:
    if not SOURCE.is_file() or SOURCE.stat().st_size < 1024:
        raise RuntimeError(f"Missing animated-source FBX: {SOURCE}")
    if unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        if not unreal.EditorAssetLibrary.delete_directory(DESTINATION):
            raise RuntimeError(f"Could not rebuild generated directory: {DESTINATION}")
    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        unreal.EditorAssetLibrary.make_directory(DESTINATION)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(SOURCE))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", STABLE_MESH_NAME)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", import_options())
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = [str(value) for value in task.get_editor_property("imported_object_paths")]
    meshes = []
    for asset_path in imported_paths:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.SkeletalMesh):
            meshes.append((asset_path.split(".", 1)[0], asset))
    if len(meshes) != 1:
        for asset_path in unreal.EditorAssetLibrary.list_assets(
            DESTINATION, recursive=False, include_folder=False
        ):
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if isinstance(asset, unreal.SkeletalMesh):
                meshes.append((asset_path.split(".", 1)[0], asset))
        meshes = list({path: asset for path, asset in meshes}.items())
    if len(meshes) != 1:
        raise RuntimeError(f"Expected one animated-source mesh, found {meshes}")

    current_path, mesh = meshes[0]
    if current_path != STABLE_MESH_PATH:
        if unreal.EditorAssetLibrary.does_asset_exist(STABLE_MESH_PATH):
            unreal.EditorAssetLibrary.delete_asset(STABLE_MESH_PATH)
        if not unreal.EditorAssetLibrary.rename_asset(current_path, STABLE_MESH_PATH):
            raise RuntimeError(f"Could not rename {current_path} -> {STABLE_MESH_PATH}")
        mesh = unreal.EditorAssetLibrary.load_asset(STABLE_MESH_PATH)
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        raise RuntimeError("Animated-source mesh has no skeleton")
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "WW_PLAYERGIRL_ORIGINAL_SOURCE_IMPORTED "
        f"mesh={STABLE_MESH_PATH} skeleton={skeleton.get_path_name()} source={SOURCE}"
    )


main()
