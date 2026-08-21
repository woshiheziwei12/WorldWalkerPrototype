"""Import Quaternius' CC0 Universal Animation Library 2 into W02.

Run from Unreal Editor 5.8's Python environment.  The script follows the
author's official itch.io zero-price download flow, pins the Standard archive
and FBX hashes, imports the animation-only FBX onto the existing Anime
Characters skeleton, and gives the four W02 gameplay clips stable names.

Set WORLDWALKER_W02_UAL2_ROOT to a directory containing either the pinned ZIP
or an already extracted ``UAL2_Standard.fbx`` to work from an offline cache.
"""

from __future__ import annotations

import hashlib
import html
import http.cookiejar
import json
import os
import re
import tempfile
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path

import unreal


SOURCE_ROOT_ENV = "WORLDWALKER_W02_UAL2_ROOT"
DEFAULT_SOURCE_ROOT = Path(tempfile.gettempdir()) / "WorldWalker_Quaternius_UAL2"
SOURCE_PAGE_URL = "https://quaternius.com/packs/universalanimationlibrary2.html"
ITCH_PROJECT_URL = "https://quaternius.itch.io/universal-animation-library-2"
ITCH_PURCHASE_URL = f"{ITCH_PROJECT_URL}/purchase"
ITCH_DOWNLOAD_URL_ENDPOINT = f"{ITCH_PROJECT_URL}/download_url"
ITCH_STANDARD_UPLOAD_ID = "17958478"
ARCHIVE_FILENAME = "Universal Animation Library 2 Standard.zip"
ARCHIVE_SHA256 = "4008EA208A604773A2B2177D965F0F5D3195498B5BF838C3F5785D68E95F2A68"
FBX_MEMBER = (
    "Universal Animation Library 2[Standard]/Unity/UAL2_Standard.fbx"
)
LICENSE_MEMBER = "Universal Animation Library 2[Standard]/License.txt"
FBX_SHA256 = "D26D0E9F4A202D473194C056045143095A605A53BA1D823EF24055BE4B86851D"

DESTINATION = (
    "/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Quaternius/"
    "UniversalAnimationLibrary2/Animations"
)
TARGET_MESH_PATH = (
    "/Game/AnimeCharacters/Blueprints/Characters/Female_Average/Head/Starter/"
    "skl_AnimeF_Head_1.skl_AnimeF_Head_1"
)
REQUIRED_CLIPS = {
    "ninjajumpstart": "A_W02_Jump_Start",
    "ninjajumpidleloop": "A_W02_Jump_Loop",
    "ninjajumpland": "A_W02_Jump_Land",
    "climbup1m": "A_W02_Climb_1m",
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


def _verify_hash(path: Path, expected: str, label: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"Missing {label}: {path}")
    actual = _sha256(path)
    if actual != expected:
        raise RuntimeError(
            f"SHA-256 mismatch for {label}: expected={expected} "
            f"actual={actual} path={path}"
        )


def _csrf(page: str) -> str:
    match = re.search(r'<meta name="csrf_token" value="([^"]+)"', page)
    if not match:
        raise RuntimeError("itch.io page did not contain a CSRF token")
    return html.unescape(match.group(1))


def _request(opener, url: str, data: dict[str, str] | None = None):
    encoded = urllib.parse.urlencode(data).encode("utf-8") if data else None
    request = urllib.request.Request(
        url,
        data=encoded,
        headers={
            "User-Agent": "WorldWalkerPrototype-W02-UAL2Importer/1.0",
            "Referer": ITCH_PROJECT_URL,
        },
    )
    return opener.open(request, timeout=90)


def _download_archive(root: Path) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    archive = root / ARCHIVE_FILENAME
    if archive.is_file():
        _verify_hash(archive, ARCHIVE_SHA256, "UAL2 Standard archive")
        return archive

    cookie_jar = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(
        urllib.request.HTTPCookieProcessor(cookie_jar)
    )
    with _request(opener, ITCH_PURCHASE_URL) as response:
        purchase_page = response.read().decode("utf-8")
    with _request(
        opener,
        ITCH_DOWNLOAD_URL_ENDPOINT,
        {"csrf_token": _csrf(purchase_page)},
    ) as response:
        generated_page = json.loads(response.read().decode("utf-8"))
    download_page_url = generated_page.get("url")
    if not download_page_url:
        raise RuntimeError("itch.io did not return the UAL2 download page")

    with _request(opener, download_page_url) as response:
        download_page = response.read().decode("utf-8")
    file_endpoint = f"{ITCH_PROJECT_URL}/file/{ITCH_STANDARD_UPLOAD_ID}"
    with _request(
        opener,
        file_endpoint,
        {"csrf_token": _csrf(download_page)},
    ) as response:
        generated_file = json.loads(response.read().decode("utf-8"))
    file_url = generated_file.get("url")
    if not file_url:
        raise RuntimeError("itch.io did not return the UAL2 Standard file URL")

    temporary = archive.with_suffix(".zip.part")
    if temporary.exists():
        temporary.unlink()
    with _request(opener, file_url) as response, temporary.open("wb") as output:
        while True:
            block = response.read(1024 * 1024)
            if not block:
                break
            output.write(block)
    _verify_hash(temporary, ARCHIVE_SHA256, "downloaded UAL2 Standard archive")
    temporary.replace(archive)
    return archive


def _prepare_fbx(root: Path) -> Path:
    direct_fbx = root / "UAL2_Standard.fbx"
    if direct_fbx.is_file():
        _verify_hash(direct_fbx, FBX_SHA256, "cached UAL2 animation FBX")
        return direct_fbx

    archive = _download_archive(root)
    extract_root = root / "Extracted"
    output_fbx = extract_root / "UAL2_Standard.fbx"
    if output_fbx.is_file():
        _verify_hash(output_fbx, FBX_SHA256, "extracted UAL2 animation FBX")
        return output_fbx

    with zipfile.ZipFile(archive, "r") as package:
        members = set(package.namelist())
        required = {FBX_MEMBER, LICENSE_MEMBER}
        missing = sorted(required - members)
        if missing:
            raise RuntimeError(f"UAL2 archive is missing pinned members: {missing}")
        license_text = package.read(LICENSE_MEMBER).decode("utf-8", errors="replace")
        if "CC0 1.0 Universal" not in license_text:
            raise RuntimeError("UAL2 package license is not the expected CC0 1.0 text")
        extract_root.mkdir(parents=True, exist_ok=True)
        output_fbx.write_bytes(package.read(FBX_MEMBER))
    _verify_hash(output_fbx, FBX_SHA256, "extracted UAL2 animation FBX")
    return output_fbx


def _ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def _target_skeleton():
    mesh = unreal.EditorAssetLibrary.load_asset(TARGET_MESH_PATH)
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"Missing Anime Characters target mesh: {TARGET_MESH_PATH}")
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        raise RuntimeError(f"Target mesh has no skeleton: {TARGET_MESH_PATH}")
    return skeleton


def _make_import_options(skeleton) -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property(
        "mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION
    )
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)

    animation_data = options.get_editor_property("anim_sequence_import_data")
    if animation_data is not None:
        animation_data.set_editor_property("import_bone_tracks", True)
        animation_data.set_editor_property("remove_redundant_keys", True)
        animation_data.set_editor_property("import_custom_attribute", False)
        animation_data.set_editor_property("use_default_sample_rate", False)
        animation_data.set_editor_property("custom_sample_rate", 30)
        animation_data.set_editor_property(
            "animation_length",
            unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME,
        )
    return options


def _normalise(name: str) -> str:
    return re.sub(r"[^a-z0-9]", "", name.lower())


def _import_animations(fbx: Path) -> None:
    _ensure_directory(DESTINATION)
    skeleton = _target_skeleton()

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(fbx))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", _make_import_options(skeleton))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    sequences = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(
        DESTINATION, recursive=False, include_folder=False
    ):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.AnimSequence):
            sequences.append((asset_path, asset))

    resolved: dict[str, str] = {}
    for source_key, stable_name in REQUIRED_CLIPS.items():
        matches = [
            (path, asset)
            for path, asset in sequences
            if source_key in _normalise(str(asset.get_name()))
        ]
        if len(matches) != 1:
            names = [str(asset.get_name()) for _, asset in sequences]
            raise RuntimeError(
                f"Expected one source clip for {source_key}, found {len(matches)}; "
                f"imported={names}"
            )
        source_path, source_asset = matches[0]
        if source_asset.get_editor_property("skeleton") != skeleton:
            raise RuntimeError(
                f"Imported clip uses the wrong skeleton: {source_path}"
            )
        stable_path = f"{DESTINATION}/{stable_name}"
        if source_path != stable_path:
            if unreal.EditorAssetLibrary.does_asset_exist(stable_path):
                unreal.EditorAssetLibrary.delete_asset(stable_path)
            if not unreal.EditorAssetLibrary.rename_asset(source_path, stable_path):
                raise RuntimeError(
                    f"Could not rename imported clip {source_path} -> {stable_path}"
                )
        resolved[stable_name] = stable_path

    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False)
    unreal.log(
        "W02_UAL2_ANIMATION_IMPORT_COMPLETE "
        f"animations={len(sequences)} required={len(resolved)}/4 "
        f"archive_sha256={ARCHIVE_SHA256} clips={sorted(resolved)}"
    )


def main() -> None:
    root = _source_root()
    fbx = _prepare_fbx(root)
    unreal.log(f"W02 UAL2 source ready: {fbx} sha256={_sha256(fbx)}")
    _import_animations(fbx)


main()
