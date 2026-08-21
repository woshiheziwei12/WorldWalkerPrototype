"""Download, verify, and import Kenney Castle Kit modules for W02.

Run this file from Unreal Editor's Python environment after the C++ editor
target has been built. The importer is idempotent: it verifies the official
CC0 archive and every selected source file, then imports with stable names and
replace-existing enabled.

The source cache can be overridden before Unreal starts with
``WORLDWALKER_W02_KENNEY_CASTLE_ROOT``. It defaults to
``%TEMP%/WorldWalker_W02_KenneyCastle``. Downloads and extracted source files
stay in that cache; only selected Unreal assets are saved under W02's private
ThirdParty directory.
"""

from __future__ import annotations

import hashlib
import os
import tempfile
import urllib.request
import zipfile
from pathlib import Path

import unreal


SOURCE_ROOT_ENV = "WORLDWALKER_W02_KENNEY_CASTLE_ROOT"
DEFAULT_SOURCE_ROOT = Path(tempfile.gettempdir()) / "WorldWalker_W02_KenneyCastle"
ARCHIVE_NAME = "kenney_castle-kit.zip"
ARCHIVE_URL = (
    "https://www.kenney.nl/media/pages/assets/castle-kit/"
    "a395102d20-1711543616/kenney_castle-kit.zip"
)
ARCHIVE_SHA256 = (
    "921F3F73927BB23106CAE34BC21D5AB4B033A9FC120475E96F714A406E3169DF"
)
EXTRACTION_DIRECTORY = "package"
FBX_SOURCE_ROOT = "Models/FBX format"
DESTINATION_ROOT = (
    "/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/"
    "Kenney/CastleKit/Environment"
)

LICENSE_SPEC = {
    "source": "License.txt",
    "sha256": (
        "AAC944F18106B3A3E29C6FDEEC02523D4CAB4C735ABC01F5A8FA88A79AE173EF"
    ),
}
COLORMAP_SPEC = {
    "source": f"{FBX_SOURCE_ROOT}/Textures/colormap.png",
    "sha256": (
        "66FD49BE148F32E88F6C8CACE67120250D1943A7856601158AD0FF24651DB0B0"
    ),
}


STATIC_MESH_SPECS = (
    {
        "label": "Round tower base",
        "source": f"{FBX_SOURCE_ROOT}/tower-base.fbx",
        "sha256": "4C50E4AF142966D2B81597B6A65AFBC0EF7B02ED05721CE95F7983501DBE7CF6",
        "asset_name": "SM_W02_CastleRoundTowerBase",
    },
    {
        "label": "Round tower top",
        "source": f"{FBX_SOURCE_ROOT}/tower-top.fbx",
        "sha256": "DCAE2473F7ED0741FC27D0BF5F25308AF92BA0269BB6ACCDC5569839C7363C70",
        "asset_name": "SM_W02_CastleRoundTowerTop",
    },
    {
        "label": "Hex tower base",
        "source": f"{FBX_SOURCE_ROOT}/tower-hexagon-base.fbx",
        "sha256": "ECA5CD4895390C68BE6C4BCF428234E760EBA527BAB030F88B442E328B721475",
        "asset_name": "SM_W02_CastleHexTowerBase",
    },
    {
        "label": "Hex tower middle",
        "source": f"{FBX_SOURCE_ROOT}/tower-hexagon-mid.fbx",
        "sha256": "CB76D2720B8E4B49E14EAE0D2E8AB3BB36E60DA053EEF1B6EEAD56A3858A162E",
        "asset_name": "SM_W02_CastleHexTowerMid",
    },
    {
        "label": "Hex tower top",
        "source": f"{FBX_SOURCE_ROOT}/tower-hexagon-top.fbx",
        "sha256": "F68EE7720A7786CA754F622FA452BAAE51CD561ACA6093B59C637090B8C267B5",
        "asset_name": "SM_W02_CastleHexTowerTop",
    },
    {
        "label": "Square tower base",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-base.fbx",
        "sha256": "032EED379D7399D66527A7B00B27DDAC5420FBF9239871B0AE96C064637203AC",
        "asset_name": "SM_W02_CastleSquareTowerBase",
    },
    {
        "label": "Square tower middle",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-mid.fbx",
        "sha256": "2E05E5AC1A041897B1F23E94634E7E66EC98895FA830A8FBC6D7E0ED23349C03",
        "asset_name": "SM_W02_CastleSquareTowerMid",
    },
    {
        "label": "Square tower middle door",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-mid-door.fbx",
        "sha256": "B07E21C6C08A2D2BB0BD82EF354A6092BCA3307353B2AEF83ED82686215850AB",
        "asset_name": "SM_W02_CastleSquareTowerMidDoor",
    },
    {
        "label": "Square tower middle open",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-mid-open.fbx",
        "sha256": "97FCAD51A3E37D35E6F9A6AE06C74EA7D0678DE9EDEEB616EC7386EFCD3D6824",
        "asset_name": "SM_W02_CastleSquareTowerMidOpen",
    },
    {
        "label": "Square tower middle windows",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-mid-windows.fbx",
        "sha256": "EEEA9D3280F4C0B0AEFABD88B1C674AC3F7260C52B4A69EB9267A69545E2FFCA",
        "asset_name": "SM_W02_CastleSquareTowerMidWindows",
    },
    {
        "label": "Square tower battlement top",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-top.fbx",
        "sha256": "1386EC47ADF887D6B258A1EF348920B020484AEA767FF8F84AC434685A330951",
        "asset_name": "SM_W02_CastleSquareTowerTop",
    },
    {
        "label": "Square tower high roof",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-top-roof-high.fbx",
        "sha256": "60FE3458E9606B6ACAF5321A75E35CCC08A10A019203C9D0E3F37A792B57B0A7",
        "asset_name": "SM_W02_CastleSquareTowerHighRoof",
    },
    {
        "label": "Square tower arch",
        "source": f"{FBX_SOURCE_ROOT}/tower-square-arch.fbx",
        "sha256": "EEA3751274647B4E478C61ADB0B68A22B2ABDDEBE6B6A42C4743A4498B2DD286",
        "asset_name": "SM_W02_CastleSquareTowerArch",
    },
    {
        "label": "Stone stairs",
        "source": f"{FBX_SOURCE_ROOT}/stairs-stone.fbx",
        "sha256": "9BF3222FD4AF6D8D157A75978F7DD5360007AF0038AE5DE3DCE1628150463D13",
        "asset_name": "SM_W02_CastleStoneStairs",
    },
    {
        "label": "Square stone stairs",
        "source": f"{FBX_SOURCE_ROOT}/stairs-stone-square.fbx",
        "sha256": "065F0C2995D5165DD6ED554AE97F92C0D86D5E68D0A7192F24B75985216AEA50",
        "asset_name": "SM_W02_CastleSquareStoneStairs",
    },
    {
        "label": "Straight bridge",
        "source": f"{FBX_SOURCE_ROOT}/bridge-straight.fbx",
        "sha256": "76178A96497B0AF963EBE2DF49467CAE10E79658A85886284BDB1B74821D4FE8",
        "asset_name": "SM_W02_CastleStraightBridge",
    },
    {
        "label": "Straight bridge pillar",
        "source": f"{FBX_SOURCE_ROOT}/bridge-straight-pillar.fbx",
        "sha256": "2E229402614EC308BD9D00322B23704935CC4B625F633D294B6FDB32D6D6092A",
        "asset_name": "SM_W02_CastleStraightBridgePillar",
    },
    {
        "label": "Castle wall",
        "source": f"{FBX_SOURCE_ROOT}/wall.fbx",
        "sha256": "69CD3C951EBB02249FA1A4A6FB022BF271DC6B0BE36EAAEF1E90B0A4D35CC0C5",
        "asset_name": "SM_W02_CastleWall",
    },
    {
        "label": "Castle wall corner",
        "source": f"{FBX_SOURCE_ROOT}/wall-corner.fbx",
        "sha256": "AC689BF5538268FBB83D22FE6F7C2DD94831F1B2466A716FCE4BA5F42071B842",
        "asset_name": "SM_W02_CastleWallCorner",
    },
    {
        "label": "Castle wall doorway",
        "source": f"{FBX_SOURCE_ROOT}/wall-doorway.fbx",
        "sha256": "F9AE53F61A992AA8AC7F761FEE85C28FCF78D67D602409DF0331A358DF92A33B",
        "asset_name": "SM_W02_CastleWallDoorway",
    },
    {
        "label": "Narrow castle wall",
        "source": f"{FBX_SOURCE_ROOT}/wall-narrow.fbx",
        "sha256": "1659641BFD2A8E1D054F7552598B8C1B5E35DD9E8A6A5D2E9F3388EFA6F9DCF2",
        "asset_name": "SM_W02_CastleWallNarrow",
    },
    {
        "label": "Narrow wall stairs",
        "source": f"{FBX_SOURCE_ROOT}/wall-narrow-stairs.fbx",
        "sha256": "8B39BAD7DF3746408EC6799D410FBC1DC01DFEE0D94207C8224013957053DE2C",
        "asset_name": "SM_W02_CastleWallStairs",
    },
    {
        "label": "Narrow wall stair rail",
        "source": f"{FBX_SOURCE_ROOT}/wall-narrow-stairs-rail.fbx",
        "sha256": "6049488128B483B0ED14799F10BB8F8F1BB719059582A5C9EACC60C9F7B85F83",
        "asset_name": "SM_W02_CastleWallStairRail",
    },
    {
        "label": "Castle wall pillar",
        "source": f"{FBX_SOURCE_ROOT}/wall-pillar.fbx",
        "sha256": "306FEF3A327296097CA7B84DC4445746F586B87AC186BD1876FF0566F129DC24",
        "asset_name": "SM_W02_CastleWallPillar",
    },
    {
        "label": "Castle gate",
        "source": f"{FBX_SOURCE_ROOT}/gate.fbx",
        "sha256": "2902B66F40775E9AC2FB272D22B418307CEC3ADBF17C41AA49073958E6B217A6",
        "asset_name": "SM_W02_CastleGate",
    },
    {
        "label": "Metal portcullis",
        "source": f"{FBX_SOURCE_ROOT}/metal-gate.fbx",
        "sha256": "3233E7EDB7A1B1E4C10B14048BF28879A8963AABB1935AA9B6B17C04FB9811E9",
        "asset_name": "SM_W02_CastleMetalGate",
    },
)


def _source_root() -> Path:
    configured = os.environ.get(SOURCE_ROOT_ENV)
    return (
        Path(configured).expanduser()
        if configured
        else DEFAULT_SOURCE_ROOT
    )


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


def _download_verified_archive(root: Path) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    archive = root / ARCHIVE_NAME
    if archive.is_file():
        _verify_hash(archive, ARCHIVE_SHA256, "cached Kenney Castle Kit archive")
        return archive

    temporary_archive = archive.with_suffix(archive.suffix + ".part")
    request = urllib.request.Request(
        ARCHIVE_URL,
        headers={
            "User-Agent": "WorldWalkerPrototype-W02-KenneyCastleImporter/1.0"
        },
    )
    unreal.log(f"W02 downloading official CC0 Castle Kit: {ARCHIVE_URL}")
    with urllib.request.urlopen(request, timeout=90) as response:
        with temporary_archive.open("wb") as output:
            while True:
                block = response.read(1024 * 1024)
                if not block:
                    break
                output.write(block)

    _verify_hash(
        temporary_archive,
        ARCHIVE_SHA256,
        "downloaded Kenney Castle Kit archive",
    )
    os.replace(temporary_archive, archive)
    unreal.log(f"W02 verified official Castle Kit archive: {archive}")
    return archive


def _safe_extract_zip(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    resolved_destination = destination.resolve()
    with zipfile.ZipFile(archive, "r") as source_zip:
        for member in source_zip.infolist():
            target = (destination / member.filename).resolve()
            try:
                target.relative_to(resolved_destination)
            except ValueError as exc:
                raise RuntimeError(
                    f"Unsafe path in source archive {archive}: {member.filename}"
                ) from exc
        source_zip.extractall(destination)


def _expected_source_paths(extraction_root: Path) -> list[Path]:
    relative_paths = [
        str(LICENSE_SPEC["source"]),
        str(COLORMAP_SPEC["source"]),
    ]
    relative_paths.extend(str(spec["source"]) for spec in STATIC_MESH_SPECS)
    return [extraction_root / relative_path for relative_path in relative_paths]


def _prepare_sources(root: Path) -> Path:
    extraction_root = root / EXTRACTION_DIRECTORY
    if not all(path.is_file() for path in _expected_source_paths(extraction_root)):
        archive = _download_verified_archive(root)
        _safe_extract_zip(archive, extraction_root)

    cached_archive = root / ARCHIVE_NAME
    if cached_archive.is_file():
        _verify_hash(
            cached_archive,
            ARCHIVE_SHA256,
            "cached Kenney Castle Kit archive",
        )
    _verify_hash(
        extraction_root / str(LICENSE_SPEC["source"]),
        str(LICENSE_SPEC["sha256"]),
        "Kenney Castle Kit License.txt",
    )
    _verify_hash(
        extraction_root / str(COLORMAP_SPEC["source"]),
        str(COLORMAP_SPEC["sha256"]),
        "Kenney Castle Kit colormap",
    )
    return extraction_root


def _verified_source(extraction_root: Path, spec: dict[str, object]) -> Path:
    source = extraction_root / str(spec["source"])
    _verify_hash(source, str(spec["sha256"]), str(spec["label"]))
    return source


def _ensure_directory(path: str) -> None:
    if unreal.EditorAssetLibrary.does_directory_exist(path):
        return
    if not unreal.EditorAssetLibrary.make_directory(path):
        raise RuntimeError(f"Failed to create Unreal content directory: {path}")


def _try_set_editor_property(asset: object, name: str, value: object) -> None:
    try:
        asset.set_editor_property(name, value)
    except Exception as exc:
        unreal.log_warning(
            f"W02 Castle Kit import could not set {name} on {asset}: {exc}"
        )


def _make_static_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property(
        "mesh_type_to_import",
        unreal.FBXImportType.FBXIT_STATIC_MESH,
    )
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)

    static_data = options.get_editor_property("static_mesh_import_data")
    if static_data is not None:
        _try_set_editor_property(static_data, "combine_meshes", True)
        _try_set_editor_property(static_data, "convert_scene", True)
        _try_set_editor_property(static_data, "convert_scene_unit", True)
        _try_set_editor_property(static_data, "generate_lightmap_u_vs", True)
        _try_set_editor_property(static_data, "auto_generate_collision", True)
        _try_set_editor_property(static_data, "remove_degenerates", True)
    return options


def _run_import_task(
    source: Path,
    destination: str,
    asset_name: str,
) -> list[str]:
    _ensure_directory(destination)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", _make_static_import_options())
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [
        str(path)
        for path in task.get_editor_property("imported_object_paths")
    ]


def _find_imported_static_mesh(imported_paths: list[str]):
    for imported_path in imported_paths:
        asset = unreal.EditorAssetLibrary.load_asset(imported_path)
        if asset is not None and isinstance(asset, unreal.StaticMesh):
            return asset
    return None


def _resolve_expected_static_mesh(
    imported_paths: list[str],
    destination: str,
    asset_name: str,
):
    expected_package_path = f"{destination}/{asset_name}"
    mesh = unreal.EditorAssetLibrary.load_asset(expected_package_path)
    if mesh is not None and isinstance(mesh, unreal.StaticMesh):
        return mesh

    mesh = _find_imported_static_mesh(imported_paths)
    if mesh is not None:
        imported_object_path = (
            unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(mesh)
        )
        imported_package_path = imported_object_path.split(".", 1)[0]
        if imported_package_path != expected_package_path:
            if unreal.EditorAssetLibrary.does_asset_exist(expected_package_path):
                raise RuntimeError(
                    f"Unexpected asset already occupies {expected_package_path}"
                )
            if not unreal.EditorAssetLibrary.rename_asset(
                imported_package_path,
                expected_package_path,
            ):
                raise RuntimeError(
                    f"Could not rename {imported_package_path} to "
                    f"{expected_package_path}"
                )
            mesh = unreal.EditorAssetLibrary.load_asset(expected_package_path)

    if mesh is None or not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(
            f"Castle Kit import did not create StaticMesh at "
            f"{expected_package_path}. Imported objects: {imported_paths}"
        )
    return mesh


def _import_static_mesh(
    extraction_root: Path,
    spec: dict[str, object],
) -> str:
    source = _verified_source(extraction_root, spec)
    asset_name = str(spec["asset_name"])
    destination = f"{DESTINATION_ROOT}/{asset_name}"
    imported_paths = _run_import_task(source, destination, asset_name)
    mesh = _resolve_expected_static_mesh(
        imported_paths,
        destination,
        asset_name,
    )
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    object_path = f"{destination}/{asset_name}.{asset_name}"
    unreal.log(f"W02 Castle Kit mesh ready: {object_path}")
    return object_path


def main() -> None:
    source_root = _source_root()
    extraction_root = _prepare_sources(source_root)
    _ensure_directory(DESTINATION_ROOT)

    imported_paths = [
        _import_static_mesh(extraction_root, spec)
        for spec in STATIC_MESH_SPECS
    ]
    unreal.EditorAssetLibrary.save_directory(
        DESTINATION_ROOT,
        only_if_is_dirty=False,
        recursive=True,
    )
    unreal.log(
        "W02_KENNEY_CASTLE_IMPORT_COMPLETE "
        f"meshes={len(imported_paths)}/{len(STATIC_MESH_SPECS)} "
        f"archive_sha256={ARCHIVE_SHA256}"
    )


if __name__ == "__main__":
    main()
