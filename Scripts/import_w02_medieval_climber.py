"""Import the generated W02 climber as rigid meshes driven by Warrior bones."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "SourceAssets" / "W02_MedievalClimber"
PALETTE_FBX = SOURCE_ROOT / "SM_W02_Climber_MaterialPalette.fbx"
RIGID_SOURCE = SOURCE_ROOT / "RigidParts"
DESTINATION = "/Game/WorldWalker/Worlds/W02_SpiralTower/Characters/MedievalClimber"
RIGID_DESTINATION = f"{DESTINATION}/RigidParts"
FAILED_SKELETAL_ASSET = f"{DESTINATION}/SK_W02_MedievalClimber_LowPoly"

MATERIAL_NAMES = (
    "M_W02_Climber_Skin",
    "M_W02_Climber_Hair",
    "M_W02_Climber_Linen",
    "M_W02_Climber_DustyRose",
    "M_W02_Climber_Leather",
    "M_W02_Climber_DarkCloth",
    "M_W02_Climber_Metal",
    "M_W02_Climber_Eyes",
)


def try_set(target, property_name: str, value) -> None:
    try:
        target.set_editor_property(property_name, value)
    except Exception as exc:
        unreal.log_warning(
            f"W02 climber import option unavailable: {property_name} ({exc})"
        )


def make_static_options(import_materials: bool) -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property(
        "mesh_type_to_import",
        unreal.FBXImportType.FBXIT_STATIC_MESH,
    )
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", import_materials)
    options.set_editor_property("import_textures", False)
    static_data = options.get_editor_property("static_mesh_import_data")
    if static_data is not None:
        try_set(static_data, "combine_meshes", True)
        try_set(static_data, "convert_scene", True)
        try_set(static_data, "convert_scene_unit", True)
        try_set(static_data, "generate_lightmap_u_vs", True)
        try_set(static_data, "auto_generate_collision", False)
        try_set(static_data, "remove_degenerates", True)
    return options


def import_static(source: Path, destination: str, asset_name: str, import_materials: bool):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", make_static_options(import_materials))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh_path = f"{destination}/{asset_name}.{asset_name}"
    mesh = unreal.load_object(None, mesh_path)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"W02 rigid climber part import failed: {source} -> {mesh_path}")
    return mesh


def slot_name(slot) -> str:
    values: list[str] = []
    for property_name in ("material_slot_name", "imported_material_slot_name"):
        try:
            values.append(str(slot.get_editor_property(property_name)))
        except Exception:
            pass
    return " ".join(values)


def main() -> None:
    if not PALETTE_FBX.is_file():
        raise RuntimeError(f"W02 climber material palette FBX is missing: {PALETTE_FBX}")
    rigid_files = sorted(RIGID_SOURCE.glob("SM_W02_Climber_*.fbx"))
    if len(rigid_files) != 19:
        raise RuntimeError(f"Expected 19 W02 rigid bone parts, found {len(rigid_files)}")

    unreal.EditorAssetLibrary.make_directory(DESTINATION)
    unreal.EditorAssetLibrary.make_directory(RIGID_DESTINATION)
    if unreal.EditorAssetLibrary.does_asset_exist(FAILED_SKELETAL_ASSET):
        unreal.EditorAssetLibrary.delete_asset(FAILED_SKELETAL_ASSET)

    palette = import_static(
        PALETTE_FBX,
        DESTINATION,
        "SM_W02_Climber_MaterialPalette",
        True,
    )
    materials = {}
    for name in MATERIAL_NAMES:
        asset = unreal.load_object(None, f"{DESTINATION}/{name}.{name}")
        if not asset or not isinstance(asset, unreal.MaterialInterface):
            raise RuntimeError(f"W02 climber material was not generated: {name}")
        materials[name] = asset

    meshes = []
    assignments = 0
    for source in rigid_files:
        asset_name = source.stem
        mesh = import_static(source, RIGID_DESTINATION, asset_name, False)
        slots = list(mesh.get_editor_property("static_materials"))
        for index, slot in enumerate(slots):
            normalized = slot_name(slot).lower()
            match = next(
                (material for name, material in materials.items() if name.lower() in normalized),
                None,
            )
            if match is None:
                raise RuntimeError(
                    f"Unrecognized W02 rigid part material slot: mesh={asset_name} slot={normalized}"
                )
            mesh.set_material(index, match)
            assignments += 1
        try:
            mesh.post_edit_change()
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
        meshes.append(mesh)

    unreal.EditorAssetLibrary.save_loaded_asset(palette, only_if_is_dirty=False)
    unreal.log(
        "W02_MEDIEVAL_CLIMBER_IMPORT_COMPLETE "
        f"mode=RigidBoneParts meshes={len(meshes)}/19 materials={len(materials)}/8 "
        f"assignments={assignments} failed_skeletal_removed=1"
    )


main()
