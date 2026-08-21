"""Import Poly Haven's CC0 Modular Fort 01 into W02.

Run this file from Unreal Editor 5.8's Python environment.  The importer is
deliberately self-contained and repeatable: it downloads only the pinned 1K
FBX/JPG files, verifies every SHA-256 before import, creates three deterministic
fort PBR materials plus a Wood Planks material, assigns the fort materials to
every imported mesh slot, requests generated simple collision, and saves
everything below W02's private Content directory.

Set WORLDWALKER_W02_POLYHAVEN_FORT_ROOT to use an existing verified download
cache.  Otherwise files are cached in the operating-system temporary folder.
"""

from __future__ import annotations

import hashlib
import os
import re
import tempfile
import urllib.request
from pathlib import Path

import unreal


SOURCE_ROOT_ENV = "WORLDWALKER_W02_POLYHAVEN_FORT_ROOT"
DEFAULT_SOURCE_ROOT = (
    Path(tempfile.gettempdir()) / "WorldWalker_PolyHaven_ModularFort01"
)
ASSET_PAGE_URL = "https://polyhaven.com/a/modular_fort_01"
WOOD_PLANKS_ASSET_PAGE_URL = "https://polyhaven.com/a/wood_planks"
LICENSE_URL = "https://polyhaven.com/license"
DOWNLOAD_ROOT = "https://dl.polyhaven.org/file/ph-assets/Models"
MODULAR_FORT_TEXTURE_DOWNLOAD_ROOT = (
    f"{DOWNLOAD_ROOT}/jpg/1k/modular_fort_01"
)
WOOD_PLANKS_TEXTURE_DOWNLOAD_ROOT = (
    "https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/wood_planks"
)

DESTINATION_ROOT = (
    "/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/"
    "PolyHaven/ModularFort01"
)
MESH_DESTINATION = f"{DESTINATION_ROOT}/Meshes"
TEXTURE_DESTINATION = f"{DESTINATION_ROOT}/Textures"
MATERIAL_DESTINATION = f"{DESTINATION_ROOT}/Materials"

FBX_SPEC = {
    "label": "Modular Fort 01 1K FBX",
    "filename": "modular_fort_01_1k.fbx",
    "url": (
        f"{DOWNLOAD_ROOT}/fbx/1k/modular_fort_01/"
        "modular_fort_01_1k.fbx"
    ),
    "sha256": (
        "267369DC754AFD545A91F302EF1819893BF8A907100010C097B58A2C576EB281"
    ),
}

TEXTURE_SPECS = (
    {
        "surface": "Plaster",
        "channel": "diff",
        "filename": "modular_fort_01_plaster_diff_1k.jpg",
        "sha256": (
            "D453B45CEC06DA3DA9E67B38D016F6358C7C310A99DB9B066E07E3A494731F4D"
        ),
        "asset_name": "T_W02_ModularFort01_Plaster_D",
    },
    {
        "surface": "Plaster",
        "channel": "normal",
        "filename": "modular_fort_01_plaster_nor_dx_1k.jpg",
        "sha256": (
            "7A04D3035CC35244B8A9E2D357C8AA2EC3E82FB45644C6D9AE6B0A70D5D916B3"
        ),
        "asset_name": "T_W02_ModularFort01_Plaster_N",
    },
    {
        "surface": "Plaster",
        "channel": "rough",
        "filename": "modular_fort_01_plaster_rough_1k.jpg",
        "sha256": (
            "815A90BD3084A1360721219A4DD4A838DB3C88388E399BD111F5D269DAD97601"
        ),
        "asset_name": "T_W02_ModularFort01_Plaster_R",
    },
    {
        "surface": "Trim",
        "channel": "diff",
        "filename": "modular_fort_01_trim_diff_1k.jpg",
        "sha256": (
            "FE5811560CF0471321FE6AF08114F279A7F957412B8AA6E2C224112990760556"
        ),
        "asset_name": "T_W02_ModularFort01_Trim_D",
    },
    {
        "surface": "Trim",
        "channel": "normal",
        "filename": "modular_fort_01_trim_nor_dx_1k.jpg",
        "sha256": (
            "831D0FCFAA8B92280A1BC61818EB979FC1EF70A34BC50D7863D038396DF0DBAF"
        ),
        "asset_name": "T_W02_ModularFort01_Trim_N",
    },
    {
        "surface": "Trim",
        "channel": "rough",
        "filename": "modular_fort_01_trim_rough_1k.jpg",
        "sha256": (
            "493BAE77B6125278A9560B9DCB9E0E73C50B77AC8AA37ABFBBF8E399EF59FC97"
        ),
        "asset_name": "T_W02_ModularFort01_Trim_R",
    },
    {
        "surface": "Wall",
        "channel": "diff",
        "filename": "modular_fort_01_wall_diff_1k.jpg",
        "sha256": (
            "8BDEFADCA0688E9B3FD91B4D12F341918B0A3670C25537C18A777FA51BB90BEA"
        ),
        "asset_name": "T_W02_ModularFort01_Wall_D",
    },
    {
        "surface": "Wall",
        "channel": "normal",
        "filename": "modular_fort_01_wall_nor_dx_1k.jpg",
        "sha256": (
            "B282548A4ECD3265BFE770CC7A9209D1D6F02CED55AFD21B0A44F4E0037844FC"
        ),
        "asset_name": "T_W02_ModularFort01_Wall_N",
    },
    {
        "surface": "Wall",
        "channel": "rough",
        "filename": "modular_fort_01_wall_rough_1k.jpg",
        "sha256": (
            "68D3291814689B2AC1D104E4998693FB3B433E04687CB2FE6D31D3F850B97255"
        ),
        "asset_name": "T_W02_ModularFort01_Wall_R",
    },
    {
        "surface": "WoodPlanks",
        "channel": "diff",
        "filename": "wood_planks_diff_1k.jpg",
        "sha256": (
            "3B0669F683E4BF10F5A55A381CFA9669A7B8DFD921901829DAA3B35ACC2BBDEC"
        ),
        "asset_name": "T_W02_WoodPlanks_D",
        "download_root": WOOD_PLANKS_TEXTURE_DOWNLOAD_ROOT,
    },
    {
        "surface": "WoodPlanks",
        "channel": "normal",
        "filename": "wood_planks_nor_dx_1k.jpg",
        "sha256": (
            "D54A8AD99A94B9A7A7185CEBCE2411EB74AE9031D34C5D208DD53DC42CD53917"
        ),
        "asset_name": "T_W02_WoodPlanks_N",
        "download_root": WOOD_PLANKS_TEXTURE_DOWNLOAD_ROOT,
    },
    {
        "surface": "WoodPlanks",
        "channel": "rough",
        "filename": "wood_planks_rough_1k.jpg",
        "sha256": (
            "1B7F115BFA25619B0A2DB554EB1AB88A6FC5EF0B74BA4890611EAE04D00B9829"
        ),
        "asset_name": "T_W02_WoodPlanks_R",
        "download_root": WOOD_PLANKS_TEXTURE_DOWNLOAD_ROOT,
    },
)

MATERIAL_ASSET_NAMES = {
    "plaster": "M_W02_ModularFort01_Plaster",
    "trim": "M_W02_ModularFort01_Trim",
    "wall": "M_W02_ModularFort01_Wall",
    "woodplanks": "M_W02_WoodPlanks",
}


def _source_root() -> Path:
    configured = os.environ.get(SOURCE_ROOT_ENV)
    return Path(configured).expanduser() if configured else DEFAULT_SOURCE_ROOT


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def _verify_hash(path: Path, expected_hash: str, label: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"Missing {label}: {path}")
    actual_hash = _sha256(path)
    if actual_hash != expected_hash:
        raise RuntimeError(
            f"SHA-256 mismatch for {label}: expected={expected_hash} "
            f"actual={actual_hash} path={path}"
        )


def _download_verified(root: Path, spec: dict[str, str]) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    destination = root / spec["filename"]
    if destination.is_file():
        _verify_hash(destination, spec["sha256"], spec["label"])
        return destination

    temporary = destination.with_suffix(destination.suffix + ".part")
    if temporary.exists():
        temporary.unlink()
    request = urllib.request.Request(
        spec["url"],
        headers={
            "User-Agent": "WorldWalkerPrototype-W02-PolyHavenImporter/1.0"
        },
    )
    unreal.log(f"W02 downloading official Poly Haven file: {spec['url']}")
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            with temporary.open("wb") as output:
                while True:
                    block = response.read(1024 * 1024)
                    if not block:
                        break
                    output.write(block)
        _verify_hash(temporary, spec["sha256"], spec["label"])
        os.replace(temporary, destination)
    except Exception:
        if temporary.exists():
            temporary.unlink()
        raise

    unreal.log(f"W02 verified Poly Haven source: {destination}")
    return destination


def _prepare_sources(root: Path) -> tuple[Path, dict[tuple[str, str], Path]]:
    fbx_source = _download_verified(root, FBX_SPEC)
    texture_sources: dict[tuple[str, str], Path] = {}
    for texture_spec in TEXTURE_SPECS:
        surface = str(texture_spec["surface"])
        source_label = (
            "Wood Planks"
            if surface == "WoodPlanks"
            else f"Modular Fort 01 {surface}"
        )
        download_spec = {
            "label": f"{source_label} {texture_spec['channel']} 1K JPG",
            "filename": str(texture_spec["filename"]),
            "url": (
                f"{texture_spec.get('download_root', MODULAR_FORT_TEXTURE_DOWNLOAD_ROOT)}/"
                f"{texture_spec['filename']}"
            ),
            "sha256": str(texture_spec["sha256"]),
        }
        texture_sources[
            (
                str(texture_spec["surface"]).casefold(),
                str(texture_spec["channel"]),
            )
        ] = _download_verified(root, download_spec)
    return fbx_source, texture_sources


def _ensure_directory(path: str) -> None:
    if unreal.EditorAssetLibrary.does_directory_exist(path):
        return
    if not unreal.EditorAssetLibrary.make_directory(path):
        raise RuntimeError(f"Failed to create Unreal content directory: {path}")


def _try_set_editor_property(asset: object, name: str, value: object) -> bool:
    try:
        asset.set_editor_property(name, value)
        return True
    except Exception as exc:
        unreal.log_warning(
            f"W02 Poly Haven import could not set {name} on {asset}: {exc}"
        )
        return False


def _run_import_task(
    source: Path,
    destination: str,
    asset_name: str = "",
    options: object | None = None,
) -> list[str]:
    _ensure_directory(destination)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    if asset_name:
        task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    if options is not None:
        task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [
        str(path)
        for path in task.get_editor_property("imported_object_paths")
    ]


def _resolve_expected_asset(
    imported_paths: list[str],
    destination: str,
    asset_name: str,
    asset_class: type,
):
    expected_package_path = f"{destination}/{asset_name}"
    asset = unreal.EditorAssetLibrary.load_asset(expected_package_path)
    if asset is not None and isinstance(asset, asset_class):
        return asset

    for imported_path in imported_paths:
        candidate = unreal.EditorAssetLibrary.load_asset(imported_path)
        if candidate is not None and isinstance(candidate, asset_class):
            imported_object_path = (
                unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(
                    candidate
                )
            )
            imported_package_path = imported_object_path.split(".", 1)[0]
            if imported_package_path != expected_package_path:
                if unreal.EditorAssetLibrary.does_asset_exist(
                    expected_package_path
                ):
                    raise RuntimeError(
                        f"Unexpected asset occupies {expected_package_path}"
                    )
                if not unreal.EditorAssetLibrary.rename_asset(
                    imported_package_path,
                    expected_package_path,
                ):
                    raise RuntimeError(
                        f"Could not rename {imported_package_path} to "
                        f"{expected_package_path}"
                    )
                candidate = unreal.EditorAssetLibrary.load_asset(
                    expected_package_path
                )
            return candidate

    raise RuntimeError(
        f"Import did not create {asset_class.__name__} at "
        f"{expected_package_path}. Imported objects: {imported_paths}"
    )


def _texture_compression(channel: str):
    enum_type = unreal.TextureCompressionSettings
    if channel == "normal":
        return getattr(enum_type, "TC_NORMALMAP")
    if channel == "rough":
        return getattr(enum_type, "TC_MASKS")
    return getattr(enum_type, "TC_DEFAULT")


def _import_texture(
    source: Path,
    spec: dict[str, object],
):
    asset_name = str(spec["asset_name"])
    imported_paths = _run_import_task(
        source,
        TEXTURE_DESTINATION,
        asset_name,
    )
    texture = _resolve_expected_asset(
        imported_paths,
        TEXTURE_DESTINATION,
        asset_name,
        unreal.Texture2D,
    )
    channel = str(spec["channel"])
    texture.modify()
    _try_set_editor_property(texture, "srgb", channel == "diff")
    _try_set_editor_property(
        texture,
        "compression_settings",
        _texture_compression(channel),
    )
    if channel == "normal":
        # Poly Haven's nor_dx files already use DirectX's green-channel
        # convention expected by Unreal, so no channel inversion is needed.
        _try_set_editor_property(texture, "flip_green_channel", False)
    try:
        texture.post_edit_change()
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_loaded_asset(
        texture,
        only_if_is_dirty=False,
    )
    return texture


def _sampler_type(channel: str):
    enum_type = unreal.MaterialSamplerType
    candidates = {
        "diff": ("SAMPLERTYPE_COLOR", "COLOR"),
        "normal": ("SAMPLERTYPE_NORMAL", "NORMAL"),
        "rough": ("SAMPLERTYPE_MASKS", "MASKS"),
    }[channel]
    for candidate in candidates:
        value = getattr(enum_type, candidate, None)
        if value is not None:
            return value
    return None


def _create_pbr_material(surface: str, textures: dict[str, object]):
    surface_key = surface.casefold()
    asset_name = MATERIAL_ASSET_NAMES[surface_key]
    package_path = f"{MATERIAL_DESTINATION}/{asset_name}"
    material = (
        unreal.EditorAssetLibrary.load_asset(package_path)
        if unreal.EditorAssetLibrary.does_asset_exist(package_path)
        else None
    )
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            MATERIAL_DESTINATION,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(f"Could not create material: {package_path}")

    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    _try_set_editor_property(material, "blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    _try_set_editor_property(material, "two_sided", False)

    node_specs = (
        (
            "diff",
            unreal.MaterialProperty.MP_BASE_COLOR,
            "RGB",
            -400,
            -160,
        ),
        (
            "normal",
            unreal.MaterialProperty.MP_NORMAL,
            "RGB",
            -400,
            80,
        ),
        (
            "rough",
            unreal.MaterialProperty.MP_ROUGHNESS,
            "R",
            -400,
            320,
        ),
    )
    nodes: dict[str, object] = {}
    for channel, material_property, output_name, node_x, node_y in node_specs:
        node = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureSample,
            node_pos_x=node_x,
            node_pos_y=node_y,
        )
        if node is None:
            raise RuntimeError(
                f"Could not create {channel} texture sample in {package_path}"
            )
        node.set_editor_property("texture", textures[channel])
        sampler_type = _sampler_type(channel)
        if sampler_type is not None:
            _try_set_editor_property(node, "sampler_type", sampler_type)
        if not unreal.MaterialEditingLibrary.connect_material_property(
            node,
            output_name,
            material_property,
        ):
            raise RuntimeError(
                f"Could not connect {channel} in {package_path}"
            )
        nodes[channel] = node

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(
            f"Material compile failed for {package_path}: {compiler_errors}"
        )
    for channel, material_property, _output, _x, _y in node_specs:
        input_node = (
            unreal.MaterialEditingLibrary.get_material_property_input_node(
                material,
                material_property,
            )
        )
        if input_node != nodes[channel]:
            raise RuntimeError(
                f"Material input verification failed for {package_path}: "
                f"{channel}"
            )

    unreal.EditorAssetLibrary.save_loaded_asset(
        material,
        only_if_is_dirty=False,
    )
    unreal.log(f"W02 Poly Haven material ready: {package_path}.{asset_name}")
    return material


def _make_static_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property(
        "mesh_type_to_import",
        unreal.FBXImportType.FBXIT_STATIC_MESH,
    )
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)

    static_data = options.get_editor_property("static_mesh_import_data")
    if static_data is not None:
        # Keeping the source modules separate is essential: W02 can select
        # individual straight walls, corners, gates, stairs and walkways.
        _try_set_editor_property(static_data, "combine_meshes", False)
        _try_set_editor_property(static_data, "convert_scene", True)
        _try_set_editor_property(static_data, "convert_scene_unit", True)
        _try_set_editor_property(static_data, "generate_lightmap_u_vs", True)
        _try_set_editor_property(static_data, "auto_generate_collision", True)
        _try_set_editor_property(static_data, "remove_degenerates", True)
    return options


def _import_static_meshes(source: Path) -> list[object]:
    imported_paths = _run_import_task(
        source,
        MESH_DESTINATION,
        options=_make_static_import_options(),
    )
    candidate_paths = set(imported_paths)
    candidate_paths.update(
        str(path)
        for path in unreal.EditorAssetLibrary.list_assets(
            MESH_DESTINATION,
            recursive=True,
            include_folder=False,
        )
    )
    meshes = []
    for candidate_path in sorted(candidate_paths):
        asset = unreal.EditorAssetLibrary.load_asset(candidate_path)
        if asset is not None and isinstance(asset, unreal.StaticMesh):
            meshes.append(asset)
    if not meshes:
        raise RuntimeError(
            "Modular Fort 01 FBX import created no StaticMesh assets. "
            f"Imported objects: {imported_paths}"
        )
    return meshes


def _slot_search_text(slot: object) -> str:
    values: list[str] = []
    for property_name in (
        "material_slot_name",
        "imported_material_slot_name",
    ):
        try:
            values.append(str(slot.get_editor_property(property_name)))
        except Exception:
            pass
    try:
        material_interface = slot.get_editor_property("material_interface")
        if material_interface is not None:
            values.append(str(material_interface.get_name()))
    except Exception:
        pass
    return " ".join(values).casefold()


def _surface_for_slot(slot: object) -> str | None:
    search_text = _slot_search_text(slot)
    for surface in ("plaster", "trim", "wall"):
        if re.search(rf"(^|[^a-z]){surface}([^a-z]|$)", search_text):
            return surface
    return None


def _bind_materials_to_meshes(
    meshes: list[object],
    materials: dict[str, object],
) -> tuple[dict[str, int], int]:
    recognized_counts = {surface: 0 for surface in materials}
    fallback_count = 0
    for mesh in meshes:
        mesh.modify()
        slots = list(mesh.get_editor_property("static_materials"))
        if not slots:
            mesh.set_material(0, materials["wall"])
            fallback_count += 1
        else:
            for slot_index, slot in enumerate(slots):
                surface = _surface_for_slot(slot)
                if surface is None:
                    surface = "wall"
                    fallback_count += 1
                    unreal.log_warning(
                        f"W02 Poly Haven mesh {mesh.get_name()} slot "
                        f"{slot_index} has no recognized source material "
                        "name; using Wall"
                    )
                else:
                    recognized_counts[surface] += 1
                mesh.set_material(slot_index, materials[surface])
        try:
            mesh.post_edit_change()
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_loaded_asset(
            mesh,
            only_if_is_dirty=False,
        )

    missing_surfaces = [
        surface
        for surface, count in recognized_counts.items()
        if count == 0
    ]
    if missing_surfaces:
        raise RuntimeError(
            "Modular Fort FBX material-slot verification failed; source "
            f"did not expose these expected slots: {missing_surfaces}"
        )
    return recognized_counts, fallback_count


def _mesh_bounds_size_text(mesh: object) -> str:
    try:
        bounds = mesh.get_bounds()
        try:
            extent = bounds.get_editor_property("box_extent")
        except Exception:
            extent = bounds.box_extent
        size_x = 2.0 * float(extent.x)
        size_y = 2.0 * float(extent.y)
        size_z = 2.0 * float(extent.z)
        return f"SizeUU=({size_x:.2f},{size_y:.2f},{size_z:.2f})"
    except Exception as exc:
        unreal.log_warning(
            f"W02 Poly Haven could not read bounds for {mesh.get_name()}: "
            f"{exc}"
        )
        return "SizeUU=unavailable"


def main() -> None:
    _ensure_directory(DESTINATION_ROOT)
    _ensure_directory(MESH_DESTINATION)
    _ensure_directory(TEXTURE_DESTINATION)
    _ensure_directory(MATERIAL_DESTINATION)

    fbx_source, texture_sources = _prepare_sources(_source_root())
    imported_textures: dict[tuple[str, str], object] = {}
    for texture_spec in TEXTURE_SPECS:
        key = (
            str(texture_spec["surface"]).casefold(),
            str(texture_spec["channel"]),
        )
        imported_textures[key] = _import_texture(
            texture_sources[key],
            texture_spec,
        )

    materials: dict[str, object] = {}
    for surface in ("Plaster", "Trim", "Wall", "WoodPlanks"):
        surface_key = surface.casefold()
        materials[surface_key] = _create_pbr_material(
            surface,
            {
                channel: imported_textures[(surface_key, channel)]
                for channel in ("diff", "normal", "rough")
            },
        )

    meshes = _import_static_meshes(fbx_source)
    recognized_counts, fallback_count = _bind_materials_to_meshes(
        meshes,
        {
            surface: materials[surface]
            for surface in ("plaster", "trim", "wall")
        },
    )
    unreal.EditorAssetLibrary.save_directory(
        DESTINATION_ROOT,
        only_if_is_dirty=False,
        recursive=True,
    )

    mesh_records = sorted(
        (
            unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(mesh),
            mesh,
        )
        for mesh in meshes
    )
    for mesh_path, mesh in mesh_records:
        unreal.log(
            f"W02 Poly Haven mesh ready: {mesh_path} "
            f"{_mesh_bounds_size_text(mesh)}"
        )
    unreal.log(
        "W02_POLYHAVEN_MODULAR_FORT_IMPORT_COMPLETE "
        f"meshes={len(mesh_records)} textures={len(imported_textures)} "
        f"materials={len(materials)} slots={recognized_counts} "
        f"fallback_slots={fallback_count} fbx_sha256={FBX_SPEC['sha256']} "
        f"fort_source={ASSET_PAGE_URL} "
        f"wood_source={WOOD_PLANKS_ASSET_PAGE_URL} license={LICENSE_URL}"
    )


if __name__ == "__main__":
    main()
