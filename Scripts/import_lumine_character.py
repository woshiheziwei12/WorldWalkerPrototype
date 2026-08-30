"""Import the Lumine model extracted from the locally installed game.

The default source is the dependency-resolved AnimeStudio export under
``E:/workspace/gensehnimport``. Set ``WORLDWALKER_LUMINE_SOURCE`` only when that
processed export is moved. Exact hashes prevent a silent import of a different
game revision or the former GI-Assets model.
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path

import unreal


DEFAULT_SOURCE_DIRECTORY = Path(
    "E:/workspace/gensehnimport/exports/PlayerGirl_Model_CurrentGame/"
    "HeroEntity/Avatar_Girl_Sword_PlayerGirl_HeroEntity"
)
SOURCE_DIRECTORY = Path(
    os.environ.get("WORLDWALKER_LUMINE_SOURCE", str(DEFAULT_SOURCE_DIRECTORY))
)
DESTINATION = "/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine"
MESH_ASSET_NAME = "SK_WW_Lumine_CurrentGame"
MESH_ASSET_PATH = f"{DESTINATION}/{MESH_ASSET_NAME}"

SOURCE_HASHES = {
    "Avatar_Girl_Sword_PlayerGirl_Tex_Body_Diffuse.png":
        "83ec9ce8ccfcad05415c21a5ebaca9a80b6f716dd088c5cadb4ece4f33457557",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Body_Lightmap.png":
        "a809d2e7c2727eb36945dd2f7d38bbcc9d426ebda4f24e10b5fe85dcb1e0a057",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Body_Shadow_Ramp.png":
        "2c823f4a3e09b3cf636f252c6cfc9f89535ffdd3d5e8f507d4f41f4456d65731",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Face_Diffuse.png":
        "0a972a60bf1c2df0ad29456d2e64a68951b6607b75a3369ae7b2916e80d61c0b",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Diffuse.png":
        "1015206f3885cbf70f78bf9447b78f55574766825ee86ebd1cb19b888161b9e2",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Lightmap.png":
        "2b0d1df200a86452135e1e6f878314ebb0d174637525ae8976c7cba87cbe52fe",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Shadow_Ramp.png":
        "f5251ab4a1f5a4187fbd2c3c1a8f48ce56d50cc521bb255236b85d4dca64912c",
    "Avatar_Girl_Tex_FaceLightmap.png":
        "20fd7b7f2eee388205dd9ba23cca4c0262e45affb88172af777c577e5662166c",
    "Avatar_Tex_Face_Shadow.png":
        "2684d7802c1b80470cdf60babfa7f3066c658106d47763e575b6caf0eaf0bd3d",
    "Avatar_Tex_MetalMap.png":
        "3ab44ffc39639e217a9918db4844d97275b76f35eb5bfbf9a19f9eae175f038e",
    "Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx":
        "a6c4c57210256a6ed17085d0c33fbfd6e8a5cc99d06232e2f83dd101fde5d2cb",
}

TEXTURE_IMPORTS = {
    "Avatar_Girl_Sword_PlayerGirl_Tex_Body_Diffuse.png": "T_Lumine_Body_D",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Body_Lightmap.png": "T_Lumine_Body_Lightmap",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Body_Shadow_Ramp.png": "T_Lumine_Body_ShadowRamp",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Face_Diffuse.png": "T_Lumine_Face_D",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Diffuse.png": "T_Lumine_Hair_D",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Lightmap.png": "T_Lumine_Hair_Lightmap",
    "Avatar_Girl_Sword_PlayerGirl_Tex_Hair_Shadow_Ramp.png": "T_Lumine_Hair_ShadowRamp",
    "Avatar_Girl_Tex_FaceLightmap.png": "T_Lumine_FaceLightmap",
    "Avatar_Tex_Face_Shadow.png": "T_Lumine_FaceShadow",
    "Avatar_Tex_MetalMap.png": "T_Lumine_MetalMap",
}


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _validate_sources() -> None:
    if not SOURCE_DIRECTORY.is_dir():
        raise RuntimeError(f"Lumine source directory is missing: {SOURCE_DIRECTORY}")
    for relative_path, expected_hash in SOURCE_HASHES.items():
        source = SOURCE_DIRECTORY / relative_path
        if not source.is_file():
            raise RuntimeError(f"Required Lumine source is missing: {source}")
        actual_hash = _sha256(source)
        if actual_hash.lower() != expected_hash:
            raise RuntimeError(
                "Lumine source hash mismatch: "
                f"file={relative_path} expected={expected_hash} actual={actual_hash}"
            )


def _ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def _run_import(
    source: Path,
    destination_name: str,
    options: object | None = None,
) -> list[str]:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    if options is not None:
        task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(path) for path in task.get_editor_property("imported_object_paths")]


def _import_textures() -> dict[str, unreal.Texture2D]:
    imported: dict[str, unreal.Texture2D] = {}
    linear_textures = {
        "T_Lumine_Body_Lightmap",
        "T_Lumine_Hair_Lightmap",
        "T_Lumine_FaceLightmap",
        "T_Lumine_FaceShadow",
        "T_Lumine_MetalMap",
    }
    for source_name, asset_name in TEXTURE_IMPORTS.items():
        _run_import(SOURCE_DIRECTORY / source_name, asset_name)
        asset_path = f"{DESTINATION}/{asset_name}"
        texture = unreal.EditorAssetLibrary.load_asset(asset_path)
        if texture is None or not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Texture import failed: {asset_path}")
        if asset_name in linear_textures:
            texture.set_editor_property("srgb", False)
            texture.set_editor_property(
                "compression_settings",
                unreal.TextureCompressionSettings.TC_MASKS,
            )
        texture.set_editor_property("never_stream", False)
        unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
        imported[asset_name] = texture
    return imported


def _make_skeletal_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property(
        "mesh_type_to_import",
        unreal.FBXImportType.FBXIT_SKELETAL_MESH,
    )
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("create_physics_asset", False)

    skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
    if skeletal_data is not None:
        skeletal_data.set_editor_property("convert_scene", True)
        skeletal_data.set_editor_property("convert_scene_unit", True)
        skeletal_data.set_editor_property("import_morph_targets", True)
        skeletal_data.set_editor_property("use_t0_as_ref_pose", False)
        skeletal_data.set_editor_property("preserve_smoothing_groups", True)
    return options


def _import_mesh() -> unreal.SkeletalMesh:
    imported_paths = _run_import(
        SOURCE_DIRECTORY / "Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx",
        MESH_ASSET_NAME,
        _make_skeletal_options(),
    )
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_ASSET_PATH)
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        for imported_path in imported_paths:
            candidate = unreal.EditorAssetLibrary.load_asset(imported_path)
            if candidate is not None and isinstance(candidate, unreal.SkeletalMesh):
                mesh = candidate
                break
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(
            f"Lumine FBX did not create a SkeletalMesh: imported={imported_paths}"
        )
    return mesh


def _load_or_create_material(name: str) -> unreal.Material:
    asset_path = f"{DESTINATION}/{name}"
    material = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            DESTINATION,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(f"Unable to create Lumine material: {asset_path}")
    return material


def _enable_skeletal_mesh_usage(material: unreal.Material) -> None:
    usage = getattr(unreal.MaterialUsage, "MATUSAGE_SKELETAL_MESH", None)
    if usage is None:
        usage = getattr(unreal.MaterialUsage, "SKELETAL_MESH", None)
    if usage is None:
        raise RuntimeError("UE does not expose the SkeletalMesh material usage")
    unreal.MaterialEditingLibrary.set_base_material_usage(material, usage, True)


def _configure_hidden_material(name: str) -> unreal.Material:
    """Discard helper/effect geometry that has no standalone runtime shader."""
    material = _load_or_create_material(name)
    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", False)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)

    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        node_pos_x=-180,
        node_pos_y=0,
    )
    if opacity is None:
        raise RuntimeError(f"Unable to build Lumine hidden material graph: {name}")
    opacity.set_editor_property("r", 0.0)
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity,
        "",
        unreal.MaterialProperty.MP_OPACITY_MASK,
    )

    _enable_skeletal_mesh_usage(material)
    compiler_errors = list(unreal.MaterialEditingLibrary.recompile_material(material))
    if compiler_errors:
        raise RuntimeError(f"Lumine hidden material compile failed: {name}: {compiler_errors}")
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def _configure_material(
    name: str,
    diffuse: unreal.Texture2D,
    *,
    emissive_strength: float,
    masked: bool = False,
    two_sided: bool = False,
) -> unreal.Material:
    material = _load_or_create_material(name)
    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", two_sided)
    unlit_shading = getattr(unreal.MaterialShadingModel, "MSM_UNLIT", None)
    if unlit_shading is None:
        unlit_shading = getattr(unreal.MaterialShadingModel, "UNLIT", None)
    if unlit_shading is None:
        raise RuntimeError("UE does not expose the Unlit material shading model")
    material.set_editor_property("shading_model", unlit_shading)
    material.set_editor_property(
        "blend_mode",
        unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_OPAQUE,
    )

    diffuse_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        node_pos_x=-420,
        node_pos_y=-80,
    )
    color_floor = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        node_pos_x=-180,
        node_pos_y=-70,
    )
    emissive = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        node_pos_x=40,
        node_pos_y=-70,
    )
    if not all((diffuse_sample, color_floor, emissive)):
        raise RuntimeError(f"Unable to build Lumine material graph: {name}")
    diffuse_sample.set_editor_property("parameter_name", "BaseColorTexture")
    diffuse_sample.set_editor_property("texture", diffuse)
    color_floor.set_editor_property("parameter_name", "ToonBrightness")
    color_floor.set_editor_property("default_value", emissive_strength)
    unreal.MaterialEditingLibrary.connect_material_expressions(
        diffuse_sample,
        "RGB",
        emissive,
        "A",
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        color_floor,
        "",
        emissive,
        "B",
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )
    if masked:
        unreal.MaterialEditingLibrary.connect_material_property(
            diffuse_sample,
            "A",
            unreal.MaterialProperty.MP_OPACITY_MASK,
        )

    _enable_skeletal_mesh_usage(material)
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(unreal.MaterialEditingLibrary.recompile_material(material))
    if compiler_errors:
        raise RuntimeError(f"Lumine material compile failed: {name}: {compiler_errors}")
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def _create_materials(textures: dict[str, unreal.Texture2D]) -> dict[str, unreal.Material]:
    return {
        "body": _configure_material(
            "M_WW_Lumine_Body",
            textures["T_Lumine_Body_D"],
            emissive_strength=0.08,
        ),
        "dress": _configure_material(
            "M_WW_Lumine_Dress",
            textures["T_Lumine_Body_D"],
            emissive_strength=0.09,
            two_sided=True,
        ),
        "face": _configure_material(
            "M_WW_Lumine_Face",
            textures["T_Lumine_Face_D"],
            emissive_strength=0.09,
            two_sided=True,
        ),
        "hair": _configure_material(
            "M_WW_Lumine_Hair",
            textures["T_Lumine_Hair_D"],
            emissive_strength=0.10,
            two_sided=True,
        ),
        "hidden": _configure_hidden_material("M_WW_Lumine_Hidden"),
    }


def _assign_materials(
    mesh: unreal.SkeletalMesh,
    materials: dict[str, unreal.Material],
) -> list[str]:
    slots = list(mesh.get_editor_property("materials"))
    assigned_names: list[str] = []
    fallback_order = ["body", "dress", "face", "hair"]
    for index, slot in enumerate(slots):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        normalized = slot_name.lower()
        key = "hidden" if "default" in normalized else next(
            (candidate for candidate in fallback_order if candidate in normalized),
            None,
        )
        if key is None:
            key = fallback_order[min(index, len(fallback_order) - 1)]
        slot.set_editor_property("material_interface", materials[key])
        slots[index] = slot
        assigned_names.append(f"{index}:{slot_name}->{key}")
    mesh.set_editor_property("materials", slots)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    return assigned_names


def main() -> None:
    _validate_sources()
    _ensure_directory(DESTINATION)
    textures = _import_textures()
    mesh = _import_mesh()
    materials = _create_materials(textures)
    for visible_key in ("body", "dress", "face", "hair"):
        visible_material = materials[visible_key]
        if visible_material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_OPAQUE:
            raise RuntimeError(f"Lumine visible material must remain opaque: {visible_key}")
    if not materials["dress"].get_editor_property("two_sided"):
        raise RuntimeError("Lumine dress must remain two-sided")
    assignments = _assign_materials(mesh, materials)
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "WW_LUMINE_IMPORT_COMPLETE "
        f"mesh=1 textures={len(textures)}/10 materials={len(materials)}/5 "
        "visible=opaque dress=two-sided "
        f"slots={';'.join(assignments)} source={SOURCE_DIRECTORY}"
    )


main()
