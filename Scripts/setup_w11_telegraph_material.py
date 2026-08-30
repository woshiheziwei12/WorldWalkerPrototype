"""Create the W11 enemy ground-telegraph material.

The material is intentionally code-owned and idempotent: enemy presentation can
depend on one stable asset without requiring a hand-authored editor step.
"""

import unreal


MATERIAL_DIRECTORY = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Materials"
MATERIAL_NAME = "M_W11_EnemyTelegraph"
MATERIAL_PATH = f"{MATERIAL_DIRECTORY}/{MATERIAL_NAME}"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create directory: {path}")


def main():
    ensure_directory(MATERIAL_DIRECTORY)
    # UE 5.8 can assert when rebuilding expressions on a loaded material whose
    # nodes were rooted by the editor domain. Once generated, preserve the
    # stable asset; intentional graph revisions should use a versioned asset.
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        if material is None or not isinstance(material, unreal.Material):
            raise RuntimeError(f"Invalid telegraph material: {MATERIAL_PATH}")
        unreal.log(
            "W11_TELEGRAPH_MATERIAL_READY "
            f"asset={MATERIAL_PATH} existing=1 blend=translucent shading=unlit "
            "parameters=DangerColor,Opacity opacity=0.38"
        )
        return
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        MATERIAL_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(f"Unable to create telegraph material: {MATERIAL_PATH}")

    material.modify()
    material.set_editor_property("two_sided", True)
    material.set_editor_property(
        "shading_model", unreal.MaterialShadingModel.MSM_UNLIT
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    danger_color = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        node_pos_x=-320,
        node_pos_y=-80,
    )
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        node_pos_x=-320,
        node_pos_y=120,
    )
    if danger_color is None or opacity is None:
        raise RuntimeError("Unable to create telegraph material parameters")

    danger_color.set_editor_property("parameter_name", "DangerColor")
    danger_color.set_editor_property(
        "default_value", unreal.LinearColor(1.6, 0.035, 0.01, 1.0)
    )
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.38)

    if not unreal.MaterialEditingLibrary.connect_material_property(
        danger_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError("Unable to connect telegraph emissive color")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        opacity, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Unable to connect telegraph opacity")

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(
            f"Telegraph material compile failed: {compiler_errors}"
        )
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        material, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Unable to save telegraph material: {MATERIAL_PATH}")
    unreal.log(
        "W11_TELEGRAPH_MATERIAL_READY "
        f"asset={MATERIAL_PATH} blend=translucent shading=unlit "
        "parameters=DangerColor,Opacity opacity=0.38"
    )


if __name__ == "__main__":
    main()
