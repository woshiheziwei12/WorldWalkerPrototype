"""Import the deterministic CC0 moon-night HDRI used by W01.

The source file stays under Saved/ and is not committed. W00 and W01 may reuse
the same verified source cache, but this script always creates independent W01
TextureCube and Material assets so neither world references the other's content.
"""

import hashlib
import os

import unreal


EXPECTED_SHA256 = "05e53b6a04836127bd7a91a69f09e66d0185f24d1fcbb59d8bb5ac5f7be6e683"
W01_DEFAULT_SOURCE = os.path.abspath(
    os.path.join(
        unreal.Paths.project_saved_dir(),
        "W01AssetSources",
        "qwantani_moon_noon_puresky_2k.hdr",
    )
)
VERIFIED_SHARED_SOURCE = os.path.abspath(
    os.path.join(
        unreal.Paths.project_saved_dir(),
        "W00AssetSources",
        "qwantani_moon_noon_puresky_2k.hdr",
    )
)
SOURCE_FILE = os.environ.get(
    "WORLDWALKER_W01_HDRI",
    W01_DEFAULT_SOURCE
    if os.path.isfile(W01_DEFAULT_SOURCE)
    else VERIFIED_SHARED_SOURCE,
)
ASSET_DIRECTORY = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/PolyHaven/Sky"
)
HDRI_ASSET_PATH = f"{ASSET_DIRECTORY}/T_W01_QwantaniMoonNoon"
SKY_MATERIAL_PATH = f"{ASSET_DIRECTORY}/M_W01_QwantaniMoonNoonSky"
NIGHT_GRADED_SKY_MATERIAL_PATH = (
    f"{ASSET_DIRECTORY}/M_W01_QwantaniMoonNoonSkyNight"
)
SKY_DETAIL_MATERIAL_PATH = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/Materials/"
    "M_W01_EmissiveSkyDetail"
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
        raise RuntimeError(f"Unable to load required W01 asset: {path}")
    if expected_type is not None and not isinstance(asset, expected_type):
        raise RuntimeError(
            f"Unexpected asset type for {path}: {asset.get_class().get_name()}"
        )
    return asset


def import_hdri():
    if not os.path.isfile(SOURCE_FILE):
        raise RuntimeError(f"W01 HDRI source is missing: {SOURCE_FILE}")
    actual_hash = file_sha256(SOURCE_FILE)
    if actual_hash.lower() != EXPECTED_SHA256:
        raise RuntimeError(
            "W01 HDRI hash mismatch: "
            f"expected={EXPECTED_SHA256} actual={actual_hash}"
        )

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE_FILE)
    task.set_editor_property("destination_path", ASSET_DIRECTORY)
    task.set_editor_property("destination_name", "T_W01_QwantaniMoonNoon")
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
        # material already rooted by a native LoadObject/ConstructorHelpers
        # reference. The TextureCube keeps its stable object path on reimport,
        # so the existing verified material remains valid and is reused.
        unreal.log(f"Keeping existing W01 sky material: {SKY_MATERIAL_PATH}")
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_W01_QwantaniMoonNoonSky",
        ASSET_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(f"Unable to create W01 sky material: {SKY_MATERIAL_PATH}")

    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", True)
    material.set_editor_property(
        "shading_model",
        unreal.MaterialShadingModel.MSM_UNLIT,
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("is_sky", True)

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
        raise RuntimeError("Unable to create W01 sky material expressions")

    texture_sample.set_editor_property("parameter_name", "NightCubemap")
    texture_sample.set_editor_property("texture", hdri)
    intensity.set_editor_property("parameter_name", "SkyBrightness")
    intensity.set_editor_property("default_value", 0.14)

    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        reflection,
        "",
        texture_sample,
        "",
    ):
        raise RuntimeError("Unable to connect W01 sky reflection vector")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        texture_sample,
        "RGB",
        multiply,
        "A",
    ):
        raise RuntimeError("Unable to connect W01 sky cubemap")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        intensity,
        "",
        multiply,
        "B",
    ):
        raise RuntimeError("Unable to connect W01 sky brightness")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        multiply,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    ):
        raise RuntimeError("Unable to connect W01 sky emissive output")

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(f"W01 sky material compile failed: {compiler_errors}")
    unreal.EditorAssetLibrary.save_loaded_asset(
        material,
        only_if_is_dirty=False,
    )
    return material


def ensure_night_graded_sky_material(hdri):
    if unreal.EditorAssetLibrary.does_asset_exist(NIGHT_GRADED_SKY_MATERIAL_PATH):
        material = load_required_asset(
            NIGHT_GRADED_SKY_MATERIAL_PATH,
            unreal.Material,
        )
        unreal.log(
            "Keeping existing W01 night-graded sky material: "
            f"{NIGHT_GRADED_SKY_MATERIAL_PATH}"
        )
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_W01_QwantaniMoonNoonSkyNight",
        ASSET_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(
            "Unable to create W01 night-graded sky material: "
            f"{NIGHT_GRADED_SKY_MATERIAL_PATH}"
        )

    material.modify()
    material.set_editor_property("two_sided", True)
    material.set_editor_property(
        "shading_model",
        unreal.MaterialShadingModel.MSM_UNLIT,
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    reflection = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionReflectionVectorWS,
        node_pos_x=-920,
        node_pos_y=-60,
    )
    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameterCube,
        node_pos_x=-700,
        node_pos_y=-60,
    )
    intensity = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        node_pos_x=-700,
        node_pos_y=180,
    )
    tint = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        node_pos_x=-460,
        node_pos_y=190,
    )
    intensity_multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        node_pos_x=-400,
        node_pos_y=-30,
    )
    tint_multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        node_pos_x=-120,
        node_pos_y=0,
    )
    if not all(
        (
            reflection,
            texture_sample,
            intensity,
            tint,
            intensity_multiply,
            tint_multiply,
        )
    ):
        raise RuntimeError("Unable to create W01 night-graded sky expressions")

    texture_sample.set_editor_property("parameter_name", "NightCubemap")
    texture_sample.set_editor_property("texture", hdri)
    intensity.set_editor_property("parameter_name", "SkyBrightness")
    intensity.set_editor_property("default_value", 0.045)
    tint.set_editor_property("parameter_name", "SkyTint")
    tint.set_editor_property(
        "default_value",
        unreal.LinearColor(0.20, 0.35, 1.0, 1.0),
    )

    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        reflection,
        "",
        texture_sample,
        "",
    ):
        raise RuntimeError("Unable to connect W01 night sky reflection vector")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        texture_sample,
        "RGB",
        intensity_multiply,
        "A",
    ):
        raise RuntimeError("Unable to connect W01 night sky cubemap")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        intensity,
        "",
        intensity_multiply,
        "B",
    ):
        raise RuntimeError("Unable to connect W01 night sky brightness")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        intensity_multiply,
        "",
        tint_multiply,
        "A",
    ):
        raise RuntimeError("Unable to connect W01 night sky intensity output")
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        tint,
        "",
        tint_multiply,
        "B",
    ):
        raise RuntimeError("Unable to connect W01 night sky tint")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        tint_multiply,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    ):
        raise RuntimeError("Unable to connect W01 night sky emissive output")

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(
            f"W01 night-graded sky material compile failed: {compiler_errors}"
        )
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
            # Repair only the usage flag in place. Clearing expressions here is
            # unsafe once native code has rooted the material or its graph.
            unreal.MaterialEditingLibrary.set_base_material_usage(
                material,
                instanced_usage,
                True,
            )
            compiler_errors = list(
                unreal.MaterialEditingLibrary.recompile_material(material)
            )
            if compiler_errors:
                raise RuntimeError(
                    "Existing W01 sky-detail material usage repair failed: "
                    f"{compiler_errors}"
                )
            if not unreal.MaterialEditingLibrary.has_material_usage(
                material,
                instanced_usage,
            ):
                raise RuntimeError(
                    "Existing W01 sky-detail material still lacks "
                    "InstancedStaticMeshes usage"
                )
            unreal.EditorAssetLibrary.save_loaded_asset(
                material,
                only_if_is_dirty=False,
            )
        unreal.log(
            f"Keeping existing W01 sky-detail material: {SKY_DETAIL_MATERIAL_PATH}"
        )
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_W01_EmissiveSkyDetail",
        "/Game/WorldWalker/Worlds/W01_EasternHorror/Materials",
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(
            f"Unable to create W01 sky-detail material: {SKY_DETAIL_MATERIAL_PATH}"
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
        raise RuntimeError("Unable to create W01 sky-detail color parameter")
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
        raise RuntimeError("Unable to connect W01 sky-detail emissive output")
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
            f"W01 sky-detail material compile failed: {compiler_errors}"
        )
    if not unreal.MaterialEditingLibrary.has_material_usage(
        material,
        instanced_usage,
    ):
        raise RuntimeError("W01 sky-detail material usage verification failed")
    unreal.EditorAssetLibrary.save_loaded_asset(
        material,
        only_if_is_dirty=False,
    )
    return material


def main():
    import_hdri_asset = import_hdri()
    ensure_sky_material(import_hdri_asset)
    ensure_night_graded_sky_material(import_hdri_asset)
    sky_detail_material = ensure_sky_detail_material()
    if not isinstance(load_required_asset(HDRI_ASSET_PATH), unreal.TextureCube):
        raise RuntimeError("W01 HDRI verification failed")
    if not isinstance(load_required_asset(SKY_MATERIAL_PATH), unreal.Material):
        raise RuntimeError("W01 sky material verification failed")
    if not isinstance(
        load_required_asset(NIGHT_GRADED_SKY_MATERIAL_PATH),
        unreal.Material,
    ):
        raise RuntimeError("W01 night-graded sky material verification failed")
    if not isinstance(sky_detail_material, unreal.Material):
        raise RuntimeError("W01 sky-detail material verification failed")
    unreal.log(
        "W01_ENVIRONMENT_ASSET_IMPORT_COMPLETE "
        "hdri=1/1 sky_material=2/2 sky_detail_material=1/1 "
        f"sha256={EXPECTED_SHA256}"
    )


main()
