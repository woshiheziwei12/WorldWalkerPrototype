"""Import the Traveler sword and bind deterministic, game-texture materials."""

from pathlib import Path

import unreal


SOURCE_ROOT = Path("E:/workspace/content/Models/Weapons/Sword/Traveler")
DESTINATION = "/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/Weapon"
PARTS_DESTINATION = f"{DESTINATION}/Parts"
MESH_NAME = "SM_WW_Lumine_TravelerSword"
TEXTURES = (
    ("Equip_Sword_Traveler_01_Tex_Diffuse.png", "T_WW_TravelerSword_01_D"),
    ("Equip_Sword_Traveler_02_Tex_Diffuse.png", "T_WW_TravelerSword_02_D"),
)


def _ensure_directory() -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
        unreal.EditorAssetLibrary.make_directory(DESTINATION)


def _import(source: Path, destination_name: str, options=None, destination=DESTINATION):
    if not source.is_file():
        raise RuntimeError(f"Missing sword source: {source}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    if options is not None:
        task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(path) for path in task.get_editor_property("imported_object_paths")]


def _import_texture(source_name: str, asset_name: str):
    _import(SOURCE_ROOT / source_name, asset_name)
    texture = unreal.EditorAssetLibrary.load_asset(f"{DESTINATION}/{asset_name}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Sword texture import failed: {asset_name}")
    return texture


def _make_material(name: str, texture):
    path = f"{DESTINATION}/{name}"
    material = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew()
        )
    )
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"Could not create sword material: {path}")
    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("two_sided", False)
    unlit = getattr(unreal.MaterialShadingModel, "MSM_UNLIT", None)
    if unlit is None:
        unlit = getattr(unreal.MaterialShadingModel, "UNLIT")
    material.set_editor_property("shading_model", unlit)

    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, node_pos_x=-360, node_pos_y=-40
    )
    brightness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, node_pos_x=-160, node_pos_y=100
    )
    multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, node_pos_x=40, node_pos_y=-20
    )
    if not all((sample, brightness, multiply)):
        raise RuntimeError(f"Could not build sword material graph: {name}")
    sample.set_editor_property("texture", texture)
    brightness.set_editor_property("r", 0.09)
    unreal.MaterialEditingLibrary.connect_material_expressions(sample, "RGB", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(brightness, "", multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    errors = list(unreal.MaterialEditingLibrary.recompile_material(material))
    if errors:
        raise RuntimeError(f"Sword material compile failed: {name}: {errors}")
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def _make_hidden_effect_material():
    name = "M_WW_TravelerSword_EffectHidden"
    path = f"{DESTINATION}/{name}"
    material = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew()
        )
    )
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"Could not create sword effect material: {path}")
    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, node_pos_x=-160, node_pos_y=0
    )
    if opacity is None:
        raise RuntimeError("Could not build hidden sword effect material")
    opacity.set_editor_property("r", 0.0)
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity, "", unreal.MaterialProperty.MP_OPACITY_MASK
    )
    errors = list(unreal.MaterialEditingLibrary.recompile_material(material))
    if errors:
        raise RuntimeError(f"Sword effect material compile failed: {errors}")
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def _static_options():
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    data = options.get_editor_property("static_mesh_import_data")
    if data is not None:
        data.set_editor_property("combine_meshes", False)
        data.set_editor_property("convert_scene", True)
        data.set_editor_property("convert_scene_unit", True)
        data.set_editor_property("generate_lightmap_u_vs", True)
        data.set_editor_property("auto_generate_collision", False)
    return options


def main() -> None:
    _ensure_directory()
    textures = [_import_texture(*spec) for spec in TEXTURES]
    materials = [
        _make_material("M_WW_TravelerSword_01", textures[0]),
        _make_material("M_WW_TravelerSword_02", textures[1]),
    ]
    hidden_effect = _make_hidden_effect_material()
    if not unreal.EditorAssetLibrary.does_directory_exist(PARTS_DESTINATION):
        unreal.EditorAssetLibrary.make_directory(PARTS_DESTINATION)
    paths = _import(
        SOURCE_ROOT / "Equip_Sword_Traveler.fbx",
        "Equip_Sword_Traveler",
        _static_options(),
        PARTS_DESTINATION,
    )
    candidates = []
    for path in list(paths) + list(
        unreal.EditorAssetLibrary.list_assets(
            PARTS_DESTINATION, recursive=True, include_folder=False
        )
    ):
        candidate = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(candidate, unreal.StaticMesh):
            candidates.append((path.split(".", 1)[0], candidate))
    candidates = list({path: asset for path, asset in candidates}.items())
    model_candidates = [
        (path, asset)
        for path, asset in candidates
        if "model" in asset.get_name().casefold()
        and "effect" not in asset.get_name().casefold()
    ]
    if len(model_candidates) != 1:
        raise RuntimeError(
            "Expected exactly one non-effect Traveler sword model; "
            f"found={[(path, asset.get_name()) for path, asset in candidates]}"
        )
    source_model_path, _ = model_candidates[0]
    stable_path = f"{DESTINATION}/{MESH_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(stable_path):
        unreal.EditorAssetLibrary.delete_asset(stable_path)
    if not unreal.EditorAssetLibrary.duplicate_asset(source_model_path, stable_path):
        raise RuntimeError(f"Could not duplicate sword model: {source_model_path} -> {stable_path}")
    mesh = unreal.EditorAssetLibrary.load_asset(stable_path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"Traveler sword stable mesh is missing: {stable_path}")
    slots = list(mesh.get_editor_property("static_materials"))
    for index in range(max(1, len(slots))):
        slot_name = ""
        if index < len(slots):
            try:
                slot_name = str(slots[index].get_editor_property("imported_material_slot_name"))
            except Exception:
                slot_name = str(slots[index])
        mesh.set_material(
            index,
            hidden_effect if "effect" in slot_name.casefold() else materials[0],
        )
    try:
        mesh.post_edit_change()
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "WW_LUMINE_TRAVELER_SWORD_IMPORTED "
        f"mesh={mesh.get_path_name()} source_part={source_model_path} "
        f"slots={len(slots)} effect_attached=0"
    )


main()
