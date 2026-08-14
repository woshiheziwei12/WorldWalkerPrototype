"""Import the lightweight CC0 fantasy assets prepared for W01.

Run this file from Unreal Editor's Python environment.  The default source is
``%TEMP%/WorldWalker_W01_Assets``.  Set ``WORLDWALKER_W01_ASSET_ROOT`` before
starting Unreal Editor to point at a persistent copy of the same extracted
directory.

The six card textures are required.  Warrior and Skeleton FBX imports are
best-effort so an FBX importer/version issue cannot prevent the card art from
being prepared.  Every import uses a stable destination name and replaces the
existing asset instead of creating a numbered duplicate.
"""

from __future__ import annotations

import hashlib
import os
import tempfile
from pathlib import Path

import unreal


SOURCE_ROOT_ENV = "WORLDWALKER_W01_ASSET_ROOT"
DEFAULT_SOURCE_ROOT = Path(tempfile.gettempdir()) / "WorldWalker_W01_Assets"

DESTINATION_ROOT = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty"
)
CARD_DESTINATION = f"{DESTINATION_ROOT}/Zonked/FantasyActionIcons/Cards"
WARRIOR_OBJECT_PATH = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/"
    "RPGCharacters/Warrior/SK_W01_Warrior.SK_W01_Warrior"
)
BLACKTHORN_OBJECT_PATH = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/"
    "Monsters/Skeleton/SK_W01_Blackthorn.SK_W01_Blackthorn"
)

CARD_TEXTURES = (
    {
        "source": "actions/slash_a.png",
        "asset_name": "T_Card_LongSwordSlash",
        "sha256": "504A8D1F7F8393BC40044F46C2A587BFB22522FA92D6D41E794ED1205835CF4F",
    },
    {
        "source": "actions/block_a.png",
        "asset_name": "T_Card_KiteShieldGuard",
        "sha256": "3005C97C475DF69AB98E53EC73F49010AB79C5D921CEBD4FFFB500F14DFD26B9",
    },
    {
        "source": "actions/charged_up_a.png",
        "asset_name": "T_Card_ArcaneSpark",
        "sha256": "EA27AB3E479B97E862C889E12C91DFD13001858C946F1734B5004F491FDEEA10",
    },
    {
        "source": "actions/quick_thinking_a.png",
        "asset_name": "T_Card_ReadOpening",
        "sha256": "D36B9D559DDDFD6E572ADB174B3AD93D12AFDF4169823926EB85CB659115158A",
    },
    {
        "source": "actions/meditation.png",
        "asset_name": "T_Card_KnightsPrayer",
        "sha256": "57BBEBB6C3C5D1DDA6CD0B8602FE5B317B27AE8BCE3DCFC56D3687A1F97F2532",
    },
    {
        "source": "actions/heavy_swing.png",
        "asset_name": "T_Card_LionheartJudgment",
        "sha256": "776D1860037789F3C7A7E4E868995FA3622079E1F14A7ED9B9B05D068B8328A0",
    },
)

MODEL_SPECS = (
    {
        "label": "Warrior",
        "source": (
            "rpg_characters/RPG Characters - Nov 2020/FBX/Warrior.fbx"
        ),
        "sha256": "5840E821ABA0F4CFBC7316DB456DD28042D35FCD1173F8809E22D25D040B265F",
        "destination": (
            f"{DESTINATION_ROOT}/Quaternius/RPGCharacters/Warrior"
        ),
        "asset_name": "SK_W01_Warrior",
        "object_path": WARRIOR_OBJECT_PATH,
        "supporting_textures": (
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Warrior_Texture.png"
                ),
                "asset_name": "Warrior_Texture",
                "sha256": "5D016C19DB78E8B6077EEAD3E1086A3BB3453CFAA8ADB5E583B08DC39869FF17",
            },
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Warrior_Sword_Texture.png"
                ),
                "asset_name": "Warrior_Sword_Texture",
                "sha256": "AE902ECABA0EB6D15D47EF428E81D63F9CA2D8A266AAD7DAE393A8363EA9344A",
            },
        ),
    },
    {
        "label": "Skeleton",
        "source": (
            "animated_monsters/Animated Monster Pack by @Quaternius/"
            "FBX/Skeleton.fbx"
        ),
        "sha256": "BE0A5992B677E563C6C9601E76FDCB9D50E91C195AEBB1EB70604FBFE8FB26D5",
        "destination": (
            f"{DESTINATION_ROOT}/Quaternius/Monsters/Skeleton"
        ),
        "asset_name": "SK_W01_Blackthorn",
        "object_path": BLACKTHORN_OBJECT_PATH,
        "supporting_textures": (),
    },
)


def _source_root() -> Path:
    configured = os.environ.get(SOURCE_ROOT_ENV)
    return Path(configured).expanduser() if configured else DEFAULT_SOURCE_ROOT


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def _verified_source(root: Path, spec: dict[str, object]) -> Path:
    path = root / str(spec["source"])
    if not path.is_file():
        raise RuntimeError(f"Missing W01 source file: {path}")

    expected_hash = str(spec["sha256"]).upper()
    actual_hash = _sha256(path)
    if actual_hash != expected_hash:
        raise RuntimeError(
            "W01 source checksum mismatch: "
            f"{path} expected={expected_hash} actual={actual_hash}"
        )
    return path


def _ensure_directory(path: str) -> None:
    if unreal.EditorAssetLibrary.does_directory_exist(path):
        return
    if not unreal.EditorAssetLibrary.make_directory(path):
        raise RuntimeError(f"Failed to create Unreal content directory: {path}")


def _run_import_task(
    source: Path,
    destination: str,
    asset_name: str,
    options: object | None = None,
) -> list[str]:
    _ensure_directory(destination)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    if options is not None:
        task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(path) for path in task.get_editor_property("imported_object_paths")]


def _try_set_editor_property(asset: object, name: str, value: object) -> None:
    try:
        asset.set_editor_property(name, value)
    except Exception as exc:  # Unreal property names can vary by engine release.
        unreal.log_warning(
            f"W01 import: could not set {name} on {asset}: {exc}"
        )


def _import_texture(
    root: Path,
    spec: dict[str, object],
    destination: str,
    configure_for_ui: bool,
) -> str:
    source = _verified_source(root, spec)
    asset_name = str(spec["asset_name"])
    expected_path = f"{destination}/{asset_name}"

    _run_import_task(source, destination, asset_name)
    texture = unreal.EditorAssetLibrary.load_asset(expected_path)
    if texture is None or not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(
            f"Texture import did not create the expected asset: {expected_path}"
        )

    if configure_for_ui:
        _try_set_editor_property(texture, "srgb", True)
        _try_set_editor_property(texture, "never_stream", True)
        editor_icon_compression = getattr(
            unreal.TextureCompressionSettings,
            "TC_EDITOR_ICON",
            None,
        )
        if editor_icon_compression is not None:
            _try_set_editor_property(
                texture,
                "compression_settings",
                editor_icon_compression,
            )
        ui_texture_group = getattr(
            unreal.TextureGroup,
            "TEXTUREGROUP_UI",
            None,
        )
        if ui_texture_group is not None:
            _try_set_editor_property(texture, "lod_group", ui_texture_group)

    unreal.EditorAssetLibrary.save_loaded_asset(
        texture,
        only_if_is_dirty=False,
    )
    unreal.log(f"W01 texture ready: {expected_path}")
    return expected_path


def _make_skeletal_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property(
        "mesh_type_to_import",
        unreal.FBXImportType.FBXIT_SKELETAL_MESH,
    )
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("create_physics_asset", True)

    skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
    if skeletal_data is not None:
        _try_set_editor_property(skeletal_data, "convert_scene", True)
        _try_set_editor_property(skeletal_data, "import_morph_targets", False)
        _try_set_editor_property(skeletal_data, "use_t0_as_ref_pose", False)
    return options


def _find_imported_skeletal_mesh(imported_paths: list[str]):
    for imported_path in imported_paths:
        imported_asset = unreal.EditorAssetLibrary.load_asset(imported_path)
        if imported_asset is not None and isinstance(
            imported_asset,
            unreal.SkeletalMesh,
        ):
            return imported_asset
    return None


def _import_skeletal_mesh(root: Path, spec: dict[str, object]) -> str:
    source = _verified_source(root, spec)
    destination = str(spec["destination"])
    asset_name = str(spec["asset_name"])
    expected_path = f"{destination}/{asset_name}"

    # Keep the original texture basenames available before FBX material import.
    for texture_spec in spec.get("supporting_textures", ()):
        try:
            _import_texture(
                root,
                texture_spec,
                f"{destination}/Textures",
                configure_for_ui=False,
            )
        except Exception as exc:
            unreal.log_warning(
                f"W01 {spec['label']} supporting texture import failed: {exc}"
            )

    imported_paths = _run_import_task(
        source,
        destination,
        asset_name,
        _make_skeletal_import_options(),
    )

    mesh = unreal.EditorAssetLibrary.load_asset(expected_path)
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        mesh = _find_imported_skeletal_mesh(imported_paths)
        if mesh is not None:
            imported_path = unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(
                mesh
            )
            if not unreal.EditorAssetLibrary.does_asset_exist(expected_path):
                if not unreal.EditorAssetLibrary.rename_asset(
                    imported_path,
                    expected_path,
                ):
                    raise RuntimeError(
                        f"Could not rename {imported_path} to {expected_path}"
                    )
                mesh = unreal.EditorAssetLibrary.load_asset(expected_path)

    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(
            "FBX import did not create the required SkeletalMesh at "
            f"{expected_path}. Imported objects: {imported_paths}"
        )

    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    object_path = f"{expected_path}.{asset_name}"
    if object_path != str(spec["object_path"]):
        raise RuntimeError(
            f"Unexpected configured object path for {spec['label']}: "
            f"{object_path} != {spec['object_path']}"
        )
    unreal.log(f"W01 skeletal mesh ready: {object_path}")
    return object_path


def main() -> None:
    source_root = _source_root()
    if not source_root.is_dir():
        raise RuntimeError(
            f"W01 extracted source directory is missing: {source_root}. "
            f"Set {SOURCE_ROOT_ENV} before starting Unreal Editor if the files "
            "were moved out of the system temp directory."
        )

    _ensure_directory(DESTINATION_ROOT)

    imported_cards = []
    for card_spec in CARD_TEXTURES:
        imported_cards.append(
            _import_texture(
                source_root,
                card_spec,
                CARD_DESTINATION,
                configure_for_ui=True,
            )
        )

    imported_models = []
    for model_spec in MODEL_SPECS:
        try:
            imported_models.append(
                _import_skeletal_mesh(source_root, model_spec)
            )
        except Exception as exc:
            # Model import is intentionally non-blocking; the required card art
            # remains usable even if an engine FBX importer changes behavior.
            unreal.log_warning(
                f"W01 optional model import failed for {model_spec['label']}: "
                f"{exc}"
            )

    unreal.EditorAssetLibrary.save_directory(
        DESTINATION_ROOT,
        only_if_is_dirty=False,
        recursive=True,
    )
    unreal.log(
        "W01_FANTASY_ASSET_IMPORT_COMPLETE "
        f"cards={len(imported_cards)}/{len(CARD_TEXTURES)} "
        f"models={len(imported_models)}/{len(MODEL_SPECS)}"
    )


if __name__ == "__main__":
    main()
