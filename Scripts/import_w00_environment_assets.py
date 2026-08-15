"""Import the deterministic CC0 moon-night HDRI used by W00.

The source file is deliberately kept under Saved/ so it is not committed. Run
this commandlet after downloading the exact Poly Haven file documented in the
W00 license record. The script verifies the source hash before creating stable
Unreal assets under the W00-owned Content directory.
"""

import hashlib
import os

import unreal


EXPECTED_SHA256 = "05e53b6a04836127bd7a91a69f09e66d0185f24d1fcbb59d8bb5ac5f7be6e683"
DEFAULT_SOURCE = os.path.abspath(
    os.path.join(
        unreal.Paths.project_saved_dir(),
        "W00AssetSources",
        "qwantani_moon_noon_puresky_2k.hdr",
    )
)
SOURCE_FILE = os.environ.get("WORLDWALKER_W00_HDRI", DEFAULT_SOURCE)
ASSET_DIRECTORY = (
    "/Game/WorldWalker/Worlds/W00_MainWorld/ThirdParty/PolyHaven/Sky"
)
HDRI_ASSET_PATH = f"{ASSET_DIRECTORY}/T_W00_QwantaniMoonNoon"
SKY_MATERIAL_PATH = f"{ASSET_DIRECTORY}/M_W00_QwantaniMoonNoonSky"
SKY_DETAIL_MATERIAL_PATH = (
    "/Game/WorldWalker/Worlds/W00_MainWorld/Materials/"
    "M_W00_EmissiveSkyDetail"
)


def file_sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_required_asset(path, expected_type=None):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Unable to load required W00 asset: {path}")
    if expected_type is not None and not isinstance(asset, expected_type):
        raise RuntimeError(
            f"Unexpected asset type for {path}: {asset.get_class().get_name()}"
        )
    return asset


def import_hdri():
    if not os.path.isfile(SOURCE_FILE):
        raise RuntimeError(f"W00 HDRI source is missing: {SOURCE_FILE}")
    actual_hash = file_sha256(SOURCE_FILE)
    if actual_hash.lower() != EXPECTED_SHA256:
        raise RuntimeError(
            "W00 HDRI hash mismatch: "
            f"expected={EXPECTED_SHA256} actual={actual_hash}"
        )

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE_FILE)
    task.set_editor_property("destination_path", ASSET_DIRECTORY)
    task.set_editor_property("destination_name", "T_W00_QwantaniMoonNoon")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    hdri = load_required_asset(HDRI_ASSET_PATH, unreal.TextureCube)
    hdri.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_HDR,
    )
    hdri.set_editor_property("max_texture_size", 2048)
    hdri.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(hdri, only_if_is_dirty=False)
    return hdri


def ensure_sky_material(hdri):
    if unreal.EditorAssetLibrary.does_asset_exist(SKY_MATERIAL_PATH):
        material = load_required_asset(SKY_MATERIAL_PATH, unreal.Material)
        # UE 5.8 can assert when DeleteAllMaterialExpressions is called on a
        # material already rooted by a native ConstructorHelpers reference.
        # The texture keeps the same stable object path on reimport, so the
        # existing verified material remains valid and should be reused.
        unreal.log(f"Keeping existing W00 sky material: {SKY_MATERIAL_PATH}")
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_W00_QwantaniMoonNoonSky",
        ASSET_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(f"Unable to create W00 sky material: {SKY_MATERIAL_PATH}")

    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", True)
    material.set_editor_property(
        "shading_model",
        unreal.MaterialShadingModel.MSM_UNLIT,
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    reflection = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionReflectionVectorWS,
        node_pos_x=-700,
        node_pos_y=-40,
    )
    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameterCube,
        node_pos_x=-470,
        node_pos_y=-40,
    )
    intensity = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        node_pos_x=-460,
        node_pos_y=180,
    )
    multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        node_pos_x=-180,
        node_pos_y=0,
    )
    if not all((reflection, texture_sample, intensity, multiply)):
        raise RuntimeError("Unable to create W00 sky material expressions")

    texture_sample.set_editor_property("parameter_name", "NightCubemap")
    texture_sample.set_editor_property("texture", hdri)
    intensity.set_editor_property("parameter_name", "SkyBrightness")
    intensity.set_editor_property("default_value", 0.18)

    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        reflection,
        "",
        texture_sample,
        "",
    ):
        raise RuntimeError("Unable to connect W00 sky reflection vector")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        texture_sample,
        "RGB",
        multiply,
        "A",
    ):
        raise RuntimeError("Unable to connect W00 sky cubemap")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        intensity,
        "",
        multiply,
        "B",
    ):
        raise RuntimeError("Unable to connect W00 sky brightness")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        multiply,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    ):
        raise RuntimeError("Unable to connect W00 sky emissive output")

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(f"W00 sky material compile failed: {compiler_errors}")
    unreal.EditorAssetLibrary.save_loaded_asset(
        material,
        only_if_is_dirty=False,
    )
    return material


def ensure_sky_detail_material():
    instanced_usage = getattr(
        unreal.MaterialUsage,
        "MATUSAGE_INSTANCED_STATIC_MESHES",
        None,
    )
    if instanced_usage is None:
        instanced_usage = getattr(
            unreal.MaterialUsage,
            "INSTANCED_STATIC_MESHES",
            None,
        )
    if instanced_usage is None:
        raise RuntimeError("UE does not expose InstancedStaticMeshes material usage")

    if unreal.EditorAssetLibrary.does_asset_exist(SKY_DETAIL_MATERIAL_PATH):
        material = load_required_asset(SKY_DETAIL_MATERIAL_PATH, unreal.Material)
        if not unreal.MaterialEditingLibrary.has_material_usage(
            material,
            instanced_usage,
        ):
            raise RuntimeError(
                "Existing W00 sky-detail material lacks InstancedStaticMeshes usage"
            )
        unreal.log(
            f"Keeping existing W00 sky-detail material: {SKY_DETAIL_MATERIAL_PATH}"
        )
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_W00_EmissiveSkyDetail",
        "/Game/WorldWalker/Worlds/W00_MainWorld/Materials",
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(
            f"Unable to create W00 sky-detail material: {SKY_DETAIL_MATERIAL_PATH}"
        )

    material.modify()
    material.set_editor_property("two_sided", True)
    material.set_editor_property(
        "shading_model",
        unreal.MaterialShadingModel.MSM_UNLIT,
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    color = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        node_pos_x=-260,
        node_pos_y=0,
    )
    if color is None:
        raise RuntimeError("Unable to create W00 sky-detail color parameter")
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property(
        "default_value",
        unreal.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    if not unreal.MaterialEditingLibrary.connect_material_property(
        color,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    ):
        raise RuntimeError("Unable to connect W00 sky-detail emissive output")
    unreal.MaterialEditingLibrary.set_base_material_usage(
        material,
        instanced_usage,
        True,
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(
            f"W00 sky-detail material compile failed: {compiler_errors}"
        )
    if not unreal.MaterialEditingLibrary.has_material_usage(
        material,
        instanced_usage,
    ):
        raise RuntimeError("W00 sky-detail material usage verification failed")
    unreal.EditorAssetLibrary.save_loaded_asset(
        material,
        only_if_is_dirty=False,
    )
    return material


def main():
    hdri = import_hdri()
    material = ensure_sky_material(hdri)
    sky_detail_material = ensure_sky_detail_material()
    if not isinstance(load_required_asset(HDRI_ASSET_PATH), unreal.TextureCube):
        raise RuntimeError("W00 HDRI verification failed")
    if not isinstance(load_required_asset(SKY_MATERIAL_PATH), unreal.Material):
        raise RuntimeError("W00 sky material verification failed")
    if not isinstance(sky_detail_material, unreal.Material):
        raise RuntimeError("W00 sky-detail material verification failed")
    unreal.log(
        "W00_ENVIRONMENT_ASSET_IMPORT_COMPLETE "
        "hdri=1/1 sky_material=1/1 sky_detail_material=1/1 "
        f"sha256={EXPECTED_SHA256}"
    )


main()
