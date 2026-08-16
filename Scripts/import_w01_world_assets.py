"""Acquire and import the lightweight CC0 world art selected for W01.

Run from Unreal Editor's Python environment after the C++ editor target has
been built.  The script is intentionally idempotent: downloads are verified,
source files are checked again before import, and every Unreal asset uses a
stable destination name with replace-existing enabled.

Source roots can be overridden before Unreal starts:

``WORLDWALKER_W01_WORLD_ASSET_ROOT``
    Medieval Village, Modular Medieval and Nature source archives/extractions.
    Defaults to ``%TEMP%/WorldWalker_W01_WorldAssets``.

``WORLDWALKER_W01_ASSET_ROOT``
    Existing RPG Character Pack extraction.  Defaults to
    ``%TEMP%/WorldWalker_W01_Assets`` for compatibility with
    ``import_w01_fantasy_assets.py``.

``WORLDWALKER_W01_MATERIAL_REPAIR_ONLY``
    Set to ``1`` to rebuild and bind the ten character materials without
    reimporting any FBX.  The normal full import still performs the same repair
    after importing the environment and NPC assets.

``WORLDWALKER_W01_CHARACTER_ROSTER_ONLY``
    Set to ``1`` to import only the red-hood Rogue player profile and repair
    all character materials.  This avoids reimporting the environment or the
    upstream Wizard FBX during a character-only iteration.

The script may download four small OpenGameArt archives when their extracted
sources are absent.  Downloads go only to the configured source roots, never
to Content/.  SHA-256 is checked before extraction and selected FBX/PNG files
are checked once more immediately before import.  After FBX import it also
builds ten explicit texture-sample materials and binds them to the Rogue,
Warrior, Cleric, Wizard and Ranger meshes; this compensates for the source FBX
files not referencing their separately distributed PNG textures.
"""

from __future__ import annotations

import hashlib
import os
import tempfile
import urllib.request
import zipfile
from pathlib import Path

import unreal


WORLD_SOURCE_ROOT_ENV = "WORLDWALKER_W01_WORLD_ASSET_ROOT"
RPG_SOURCE_ROOT_ENV = "WORLDWALKER_W01_ASSET_ROOT"
MATERIAL_REPAIR_ONLY_ENV = "WORLDWALKER_W01_MATERIAL_REPAIR_ONLY"
CHARACTER_ROSTER_ONLY_ENV = "WORLDWALKER_W01_CHARACTER_ROSTER_ONLY"

DEFAULT_WORLD_SOURCE_ROOT = (
    Path(tempfile.gettempdir()) / "WorldWalker_W01_WorldAssets"
)
DEFAULT_RPG_SOURCE_ROOT = Path(tempfile.gettempdir()) / "WorldWalker_W01_Assets"

DESTINATION_ROOT = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius"
)
ENVIRONMENT_DESTINATION = f"{DESTINATION_ROOT}/Environment"
RPG_DESTINATION = f"{DESTINATION_ROOT}/RPGCharacters"


ARCHIVE_SPECS = (
    {
        "root": "world",
        "archive_name": "medieval_village_pack_-_dec_2020.zip",
        "url": (
            "https://opengameart.org/sites/default/files/"
            "medieval_village_pack_-_dec_2020.zip"
        ),
        "sha256": (
            "C38D8632C3C883043809A5DB9B2F47B7033E298AB4BD073BB5C29AD323DF60FC"
        ),
        "extract_subdirectory": "medieval_village",
        "sentinel": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Buildings/FBX/House_1.fbx"
        ),
    },
    {
        "root": "world",
        "archive_name": "Modular Medieval Pack by @Quaternius_0.zip",
        "url": (
            "https://opengameart.org/sites/default/files/"
            "Modular%20Medieval%20Pack%20by%20%40Quaternius_0.zip"
        ),
        "sha256": (
            "A54AB75EBA5EB7FF4A95736BF6D16016684370EEBD986E3EF64F281D66290891"
        ),
        "extract_subdirectory": "modular_medieval",
        "sentinel": (
            "modular_medieval/Modular Medieval Pack by @Quaternius/"
            "FBX/TallWallBricks.fbx"
        ),
    },
    {
        "root": "world",
        "archive_name": "Nature pack vol.3.zip",
        "url": (
            "https://opengameart.org/sites/default/files/"
            "Nature%20pack%20vol.3.zip"
        ),
        "sha256": (
            "7BF32888A521E93C94D82FC2CEBDA482DE52C014CDE9D406DC62DA1575F9070D"
        ),
        "extract_subdirectory": "nature",
        "sentinel": "nature/Nature pack vol.3/FBX/Tree2.fbx",
    },
    {
        "root": "rpg",
        "archive_name": "rpg_characters.zip",
        "url": (
            "https://opengameart.org/sites/default/files/"
            "rpg_characters_-_nov_2020.zip"
        ),
        "sha256": (
            "5399E0CFAF313FF362455DE4086488D093465434B1B93B0CED649F3773A15FD7"
        ),
        "extract_subdirectory": "rpg_characters",
        "sentinel": (
            "rpg_characters/RPG Characters - Nov 2020/FBX/Cleric.fbx"
        ),
    },
)


STATIC_MESH_SPECS = (
    {
        "label": "House A",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Buildings/FBX/House_1.fbx"
        ),
        "sha256": (
            "437024ABC6B756F859B69844B644C4BDBC3178C759AF900F40D913F34FFCE536"
        ),
        "asset_name": "SM_W01_HouseA",
    },
    {
        "label": "House B",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Buildings/FBX/House_3.fbx"
        ),
        "sha256": (
            "42A853231116C948F42ED0106B6599F9BA212E23BA6BD5A989E1A7161DCEFAA1"
        ),
        "asset_name": "SM_W01_HouseB",
    },
    {
        "label": "Tower",
        "source": (
            "modular_medieval/Modular Medieval Pack by @Quaternius/"
            "FBX/LargeSquareTowerBricks.fbx"
        ),
        "sha256": (
            "43229A1553F5954376AD03297B78A01A5765FBFD1A4935D138D06CD111289E0F"
        ),
        "asset_name": "SM_W01_Tower",
    },
    {
        "label": "Wall",
        "source": (
            "modular_medieval/Modular Medieval Pack by @Quaternius/"
            "FBX/TallWallBricks.fbx"
        ),
        "sha256": (
            "F8D42EDB44BF9E38260F02608F0B3896F93B0F7285051BC72B76B4AD5E341CFA"
        ),
        "asset_name": "SM_W01_Wall",
    },
    {
        "label": "Arch",
        "source": (
            "modular_medieval/Modular Medieval Pack by @Quaternius/"
            "FBX/WallEntranceBricks.fbx"
        ),
        "sha256": (
            "5870A562CF2F988CCBEA1496540C9B4B1E6743D06E21F26B94C03CAC8367FFEB"
        ),
        "asset_name": "SM_W01_Arch",
    },
    {
        "label": "Tree",
        "source": "nature/Nature pack vol.3/FBX/Tree2.fbx",
        "sha256": (
            "5EA88C268C80B9F2AFFFECB84CC08CA836BA5C110A7EF0A217A21598C7739C94"
        ),
        "asset_name": "SM_W01_Tree",
    },
    {
        "label": "Barrel",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Barrel.fbx"
        ),
        "sha256": (
            "4D7E1C91BA831F0E8E47E8805CEF951B1D3620A1D32E80BD8B200098941F77B2"
        ),
        "asset_name": "SM_W01_Barrel",
    },
    {
        "label": "Crate",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Crate.fbx"
        ),
        "sha256": (
            "1AD18751ED1898CB1FE668F5DFAAAFC100FB2E55DD85D14480E213A262B1FEA3"
        ),
        "asset_name": "SM_W01_Crate",
    },
    {
        "label": "Campfire",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Bonfire_Lit.fbx"
        ),
        "sha256": (
            "581AAF1BA3EB7CF647FA9C2745BE990F73FCD87399194AFDD64F15DB64EFCEBE"
        ),
        "asset_name": "SM_W01_Campfire",
    },
    {
        "label": "Cart",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Cart.fbx"
        ),
        "sha256": (
            "4486C2C2EBB6C4A7EFC4566FF3A87CFF2749848C4C7633CB0CFAD1B52B5208D8"
        ),
        "asset_name": "SM_W01_Cart",
    },
    {
        "label": "Fence",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Fence.fbx"
        ),
        "sha256": (
            "693D8E47DF04A81BF3688ADEA9572884AAB01126B7024CDF48251E85C7CB39B3"
        ),
        "asset_name": "SM_W01_Fence",
    },
    {
        "label": "Gazebo",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Gazebo.fbx"
        ),
        "sha256": (
            "A459DFAC73559B26BDE0F4A35ACCB7932ECCF512EA197D17F838EDB7E1FC6111"
        ),
        "asset_name": "SM_W01_Gazebo",
    },
    {
        "label": "Market Stand",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/MarketStand_1.fbx"
        ),
        "sha256": (
            "BB8BF43800EA3F26188DD1B47FD04B2BE3411019D035855D9906E645D4B73B7A"
        ),
        "asset_name": "SM_W01_MarketStand",
    },
    {
        "label": "Path",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Path_Straight.fbx"
        ),
        "sha256": (
            "72378811BC35900C0F5B8ADB108E180B7E8F19257084FF105D31358D67173A72"
        ),
        "asset_name": "SM_W01_Path",
    },
    {
        "label": "Well",
        "source": (
            "medieval_village/Medieval Village Pack - Dec 2020/"
            "Props/FBX/Well.fbx"
        ),
        "sha256": (
            "4A634278D8DB32DC46C180DE54B29132C0D9AC17D6AB12BF55CCF56120966595"
        ),
        "asset_name": "SM_W01_Well",
    },
    {
        "label": "Bush",
        "source": "nature/Nature pack vol.3/FBX/Bush2.fbx",
        "sha256": (
            "072181D5B16468A00827C5F4D9EFB74197E1C58EF822FCC61E9F3EF025F7773D"
        ),
        "asset_name": "SM_W01_Bush",
    },
    {
        "label": "Grass",
        "source": "nature/Nature pack vol.3/FBX/Grass2.fbx",
        "sha256": (
            "E9B0D9D58276F8D0FF64ED54F2370055DFFF699FBA404FE77F75BD0537C672CE"
        ),
        "asset_name": "SM_W01_Grass",
    },
    {
        "label": "Rock",
        "source": "nature/Nature pack vol.3/FBX/Rock2.fbx",
        "sha256": (
            "8A4C889BB1043078B39C93175A20CBE15AC3A4CEFD1FD088FA73204C0533DE8F"
        ),
        "asset_name": "SM_W01_Rock",
    },
)


PLAYER_SPECS = (
    {
        "label": "Rogue",
        "source": (
            "rpg_characters/RPG Characters - Nov 2020/FBX/Rogue.fbx"
        ),
        "sha256": (
            "5B9BC7A1EC778D0C83F44609971E6C538480B6EF2A3F9CDDE5853DEE5E24586E"
        ),
        "asset_name": "SK_W01_Rogue",
        "textures": (
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Rogue_Texture.png"
                ),
                "asset_name": "Rogue_Texture",
                "sha256": (
                    "540694206878779AA43F8F17524062E88BEA35D2BB666B3F2081D23079A081D3"
                ),
            },
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Rogue_Dagger_Texture.png"
                ),
                "asset_name": "Rogue_Dagger_Texture",
                "sha256": (
                    "B4C25A611639BB03C7AA454C2C6D053BDC34FCF0A622E88212E6A6F085816EC6"
                ),
            },
        ),
    },
)


NPC_SPECS = (
    {
        "label": "Cleric",
        "source": (
            "rpg_characters/RPG Characters - Nov 2020/FBX/Cleric.fbx"
        ),
        "sha256": (
            "DDBD70823DA811625A512EF4CEACCE2FAA9DE8C71AA2C6FEE7E23E0FB52B6DAA"
        ),
        "asset_name": "SK_W01_Cleric",
        "textures": (
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Cleric_Texture.png"
                ),
                "asset_name": "Cleric_Texture",
                "sha256": (
                    "F17F9208CB86FD98DB429BF8A1A0EDC97C713A8357A44A392E562047BC0FE913"
                ),
            },
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Cleric_Staff_Texture.png"
                ),
                "asset_name": "Cleric_Staff_Texture",
                "sha256": (
                    "37FD6917E617738676D5904D15816C11A3A7E8C0CBEF4B3B8F1B95C968D6D785"
                ),
            },
        ),
    },
    {
        "label": "Wizard",
        "source": (
            "rpg_characters/RPG Characters - Nov 2020/FBX/Wizard.fbx"
        ),
        "sha256": (
            "DDD72512FF771380DF9223CEF02731EBF68E31309BE759BF79D7063936F4636D"
        ),
        "asset_name": "SK_W01_Wizard",
        "textures": (
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Wizard_Texture.png"
                ),
                "asset_name": "Wizard_Texture",
                "sha256": (
                    "1D4B9FCF14BE09CA3CE5983950AC0C9F4C8704E8349C3B8D49F63DD7C5EEA88E"
                ),
            },
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Wizard_Staff_Texture.png"
                ),
                "asset_name": "Wizard_Staff_Texture",
                "sha256": (
                    "D56876367B9921139BF96A337C9D3433056044B42DF55912272CB0B433D1AE28"
                ),
            },
        ),
    },
    {
        "label": "Ranger",
        "source": (
            "rpg_characters/RPG Characters - Nov 2020/FBX/Ranger.fbx"
        ),
        "sha256": (
            "F1547EBF853BD2F7849642D7A5D98B7DFA0619BBA7A2AA37D43DCAEDC49EB6D2"
        ),
        "asset_name": "SK_W01_Ranger",
        "textures": (
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Ranger_Texture.png"
                ),
                "asset_name": "Ranger_Texture",
                "sha256": (
                    "21679F0FAF9C5243A58FDC952A5427E2BBB0EA29A00B117308E6FB5FD6D94588"
                ),
            },
            {
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Ranger_Bow_Texture.png"
                ),
                "asset_name": "Ranger_Bow_Texture",
                "sha256": (
                    "36F9FF48610B97C0F599C133D95DE62C6678B29C4EC1A54E03C1AEE838B9BA3D"
                ),
            },
        ),
    },
)


# The Quaternius RPG FBX files use material names such as ``Cleric_Texture``
# but do not embed or reference the separately distributed PNG files.  UE 5.8
# Interchange therefore creates white MaterialInstanceConstant assets whose
# only override is a white DiffuseColor.  These explicit bindings create real
# texture-sample materials after every FBX import and replace the two skeletal
# mesh slots deterministically.  Warrior is repaired here as well because it
# comes from the same pack and has the same source-data issue.
CHARACTER_MATERIAL_SPECS = (
    {
        "label": "Rogue",
        "mesh_asset_name": "SK_W01_Rogue",
        "bindings": (
            {
                "slot_names": ("Rogue_Texture",),
                "slot_fallback_index": 0,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Rogue_Texture.png"
                ),
                "texture_asset_name": "Rogue_Texture",
                "sha256": (
                    "540694206878779AA43F8F17524062E88BEA35D2BB666B3F2081D23079A081D3"
                ),
                "material_asset_name": "M_W01_Rogue_Body",
            },
            {
                "slot_names": ("Rogue_Dagger_Texture",),
                "slot_fallback_index": 1,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Rogue_Dagger_Texture.png"
                ),
                "texture_asset_name": "Rogue_Dagger_Texture",
                "sha256": (
                    "B4C25A611639BB03C7AA454C2C6D053BDC34FCF0A622E88212E6A6F085816EC6"
                ),
                "material_asset_name": "M_W01_Rogue_Weapon",
            },
        ),
    },
    {
        "label": "Warrior",
        "mesh_asset_name": "SK_W01_Warrior",
        "bindings": (
            {
                "slot_names": ("Warrior_Texture",),
                "slot_fallback_index": 0,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Warrior_Texture.png"
                ),
                "texture_asset_name": "Warrior_Texture",
                "sha256": (
                    "5D016C19DB78E8B6077EEAD3E1086A3BB3453CFAA8ADB5E583B08DC39869FF17"
                ),
                "material_asset_name": "M_W01_Warrior_Body",
            },
            {
                "slot_names": ("Warrior_Sword_Texture",),
                "slot_fallback_index": 1,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Warrior_Sword_Texture.png"
                ),
                "texture_asset_name": "Warrior_Sword_Texture",
                "sha256": (
                    "AE902ECABA0EB6D15D47EF428E81D63F9CA2D8A266AAD7DAE393A8363EA9344A"
                ),
                "material_asset_name": "M_W01_Warrior_Weapon",
            },
        ),
    },
    {
        "label": "Cleric",
        "mesh_asset_name": "SK_W01_Cleric",
        "bindings": (
            {
                "slot_names": ("Cleric_Texture",),
                "slot_fallback_index": 0,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Cleric_Texture.png"
                ),
                "texture_asset_name": "Cleric_Texture",
                "sha256": (
                    "F17F9208CB86FD98DB429BF8A1A0EDC97C713A8357A44A392E562047BC0FE913"
                ),
                "material_asset_name": "M_W01_Cleric_Body",
            },
            {
                "slot_names": ("Cleric_Staff_Texture",),
                "slot_fallback_index": 1,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Cleric_Staff_Texture.png"
                ),
                "texture_asset_name": "Cleric_Staff_Texture",
                "sha256": (
                    "37FD6917E617738676D5904D15816C11A3A7E8C0CBEF4B3B8F1B95C968D6D785"
                ),
                "material_asset_name": "M_W01_Cleric_Weapon",
            },
        ),
    },
    {
        "label": "Wizard",
        "mesh_asset_name": "SK_W01_Wizard",
        "bindings": (
            {
                "slot_names": ("Wizard_Texture",),
                "slot_fallback_index": 0,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Wizard_Texture.png"
                ),
                "texture_asset_name": "Wizard_Texture",
                "sha256": (
                    "1D4B9FCF14BE09CA3CE5983950AC0C9F4C8704E8349C3B8D49F63DD7C5EEA88E"
                ),
                "material_asset_name": "M_W01_Wizard_Body",
            },
            {
                "slot_names": ("Wizard_Staff_Texture",),
                "slot_fallback_index": 1,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Wizard_Staff_Texture.png"
                ),
                "texture_asset_name": "Wizard_Staff_Texture",
                "sha256": (
                    "D56876367B9921139BF96A337C9D3433056044B42DF55912272CB0B433D1AE28"
                ),
                "material_asset_name": "M_W01_Wizard_Weapon",
            },
        ),
    },
    {
        "label": "Ranger",
        "mesh_asset_name": "SK_W01_Ranger",
        "bindings": (
            {
                "slot_names": ("Ranger_Texture",),
                "slot_fallback_index": 0,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Ranger_Texture.png"
                ),
                "texture_asset_name": "Ranger_Texture",
                "sha256": (
                    "21679F0FAF9C5243A58FDC952A5427E2BBB0EA29A00B117308E6FB5FD6D94588"
                ),
                "material_asset_name": "M_W01_Ranger_Body",
            },
            {
                # The FBX calls this slot Bow_Texture although the separately
                # distributed PNG is named Ranger_Bow_Texture.png.
                "slot_names": ("Bow_Texture", "Ranger_Bow_Texture"),
                "slot_fallback_index": 1,
                "source": (
                    "rpg_characters/RPG Characters - Nov 2020/Textures/"
                    "Ranger_Bow_Texture.png"
                ),
                "texture_asset_name": "Ranger_Bow_Texture",
                "sha256": (
                    "36F9FF48610B97C0F599C133D95DE62C6678B29C4EC1A54E03C1AEE838B9BA3D"
                ),
                "material_asset_name": "M_W01_Ranger_Weapon",
            },
        ),
    },
)


def _world_source_root() -> Path:
    configured = os.environ.get(WORLD_SOURCE_ROOT_ENV)
    return (
        Path(configured).expanduser()
        if configured
        else DEFAULT_WORLD_SOURCE_ROOT
    )


def _rpg_source_root() -> Path:
    configured = os.environ.get(RPG_SOURCE_ROOT_ENV)
    return (
        Path(configured).expanduser()
        if configured
        else DEFAULT_RPG_SOURCE_ROOT
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
    if actual_hash != expected_hash.upper():
        raise RuntimeError(
            f"SHA-256 mismatch for {label}: {path} "
            f"expected={expected_hash.upper()} actual={actual_hash}"
        )


def _download_verified_archive(spec: dict[str, object], root: Path) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    archive = root / str(spec["archive_name"])
    expected_hash = str(spec["sha256"])

    if archive.is_file():
        _verify_hash(archive, expected_hash, "cached source archive")
        return archive

    temporary_archive = archive.with_suffix(archive.suffix + ".part")
    request = urllib.request.Request(
        str(spec["url"]),
        headers={"User-Agent": "WorldWalkerPrototype-W01-AssetImporter/1.0"},
    )
    unreal.log(f"W01 downloading CC0 source: {spec['url']}")
    with urllib.request.urlopen(request, timeout=90) as response:
        with temporary_archive.open("wb") as output:
            while True:
                block = response.read(1024 * 1024)
                if not block:
                    break
                output.write(block)

    _verify_hash(temporary_archive, expected_hash, "downloaded source archive")
    os.replace(temporary_archive, archive)
    unreal.log(f"W01 verified archive: {archive}")
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


def _prepare_sources(
    world_root: Path,
    rpg_root: Path,
    *,
    include_world: bool,
) -> None:
    roots = {"world": world_root, "rpg": rpg_root}
    for spec in ARCHIVE_SPECS:
        if spec["root"] == "world" and not include_world:
            continue
        root = roots[str(spec["root"])]
        sentinel = root / str(spec["sentinel"])
        if sentinel.is_file():
            # If a cached archive is present, still verify its provenance.
            cached_archive = root / str(spec["archive_name"])
            if cached_archive.is_file():
                _verify_hash(
                    cached_archive,
                    str(spec["sha256"]),
                    "cached source archive",
                )
            continue

        archive = _download_verified_archive(spec, root)
        extraction_root = root / str(spec["extract_subdirectory"])
        _safe_extract_zip(archive, extraction_root)
        if not sentinel.is_file():
            raise RuntimeError(
                f"Archive extraction did not create expected source: {sentinel}"
            )
        unreal.log(f"W01 extracted verified source: {sentinel}")


def _verified_source(
    root: Path,
    spec: dict[str, object],
    label: str,
) -> Path:
    path = root / str(spec["source"])
    _verify_hash(path, str(spec["sha256"]), label)
    return path


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
            f"W01 world import: could not set {name} on {asset}: {exc}"
        )


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
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    if options is not None:
        task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [str(path) for path in task.get_editor_property("imported_object_paths")]


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
        _try_set_editor_property(skeletal_data, "convert_scene_unit", True)
        _try_set_editor_property(skeletal_data, "import_morph_targets", False)
        _try_set_editor_property(skeletal_data, "use_t0_as_ref_pose", False)
    return options


def _find_imported_asset(imported_paths: list[str], asset_class: type):
    for imported_path in imported_paths:
        imported_asset = unreal.EditorAssetLibrary.load_asset(imported_path)
        if imported_asset is not None and isinstance(imported_asset, asset_class):
            return imported_asset
    return None


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

    asset = _find_imported_asset(imported_paths, asset_class)
    if asset is not None:
        imported_object_path = (
            unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(asset)
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
            asset = unreal.EditorAssetLibrary.load_asset(expected_package_path)

    if asset is None or not isinstance(asset, asset_class):
        raise RuntimeError(
            f"Import did not create {asset_class.__name__} at "
            f"{expected_package_path}. Imported objects: {imported_paths}"
        )
    return asset


def _import_static_mesh(
    world_root: Path,
    spec: dict[str, object],
) -> str:
    source = _verified_source(world_root, spec, str(spec["label"]))
    asset_name = str(spec["asset_name"])
    # Every selected FBX gets a private destination folder. Quaternius packs
    # commonly reuse generic material names; isolating imports prevents one
    # model from overwriting another model's generated materials or textures.
    mesh_destination = f"{ENVIRONMENT_DESTINATION}/{asset_name}"
    imported_paths = _run_import_task(
        source,
        mesh_destination,
        asset_name,
        _make_static_import_options(),
    )
    mesh = _resolve_expected_asset(
        imported_paths,
        mesh_destination,
        asset_name,
        unreal.StaticMesh,
    )
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    object_path = f"{mesh_destination}/{asset_name}.{asset_name}"
    unreal.log(f"W01 environment mesh ready: {object_path}")
    return object_path


def _import_texture(
    rpg_root: Path,
    spec: dict[str, object],
    destination: str,
) -> str:
    source = _verified_source(
        rpg_root,
        spec,
        f"{spec['asset_name']} source texture",
    )
    asset_name = str(spec["asset_name"])
    imported_paths = _run_import_task(source, destination, asset_name)
    texture = _resolve_expected_asset(
        imported_paths,
        destination,
        asset_name,
        unreal.Texture2D,
    )
    _try_set_editor_property(texture, "srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return f"{destination}/{asset_name}.{asset_name}"


def _create_textured_character_material(
    destination: str,
    material_asset_name: str,
    texture: object,
):
    """Create/reset an opaque material whose Base Color is the supplied PNG."""

    _ensure_directory(destination)
    material_package_path = f"{destination}/{material_asset_name}"
    material = (
        unreal.EditorAssetLibrary.load_asset(material_package_path)
        if unreal.EditorAssetLibrary.does_asset_exist(material_package_path)
        else None
    )
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            material_asset_name,
            destination,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if material is None or not isinstance(material, unreal.Material):
        raise RuntimeError(
            "Could not create the deterministic W01 character material: "
            f"{material_package_path}"
        )

    material.modify()
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSample,
        node_pos_x=-320,
        node_pos_y=0,
    )
    if texture_sample is None:
        raise RuntimeError(
            f"Could not create texture sample in {material_package_path}"
        )
    texture_sample.set_editor_property("texture", texture)

    connected = unreal.MaterialEditingLibrary.connect_material_property(
        texture_sample,
        "RGB",
        unreal.MaterialProperty.MP_BASE_COLOR,
    )
    if not connected:
        raise RuntimeError(
            f"Could not connect Base Color in {material_package_path}"
        )

    _try_set_editor_property(material, "blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    _try_set_editor_property(material, "two_sided", False)
    skeletal_usage = getattr(
        unreal.MaterialUsage,
        "MATUSAGE_SKELETAL_MESH",
        None,
    )
    if skeletal_usage is None:
        skeletal_usage = getattr(
            unreal.MaterialUsage,
            "SKELETAL_MESH",
            None,
        )
    if skeletal_usage is None:
        raise RuntimeError("UE does not expose the SkeletalMesh material usage")
    unreal.MaterialEditingLibrary.set_base_material_usage(
        material,
        skeletal_usage,
        True,
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    compiler_errors = list(
        unreal.MaterialEditingLibrary.recompile_material(material)
    )
    if compiler_errors:
        raise RuntimeError(
            f"Material compile failed for {material_package_path}: "
            f"{compiler_errors}"
        )

    base_color_node = (
        unreal.MaterialEditingLibrary.get_material_property_input_node(
            material,
            unreal.MaterialProperty.MP_BASE_COLOR,
        )
    )
    if base_color_node != texture_sample:
        raise RuntimeError(
            f"Base Color verification failed for {material_package_path}"
        )
    if not unreal.MaterialEditingLibrary.has_material_usage(
        material,
        skeletal_usage,
    ):
        raise RuntimeError(
            f"SkeletalMesh usage verification failed for {material_package_path}"
        )

    unreal.EditorAssetLibrary.save_loaded_asset(
        material,
        only_if_is_dirty=False,
    )
    return material


def _skeletal_slot_names(slot: object) -> set[str]:
    names = set()
    for property_name in (
        "material_slot_name",
        "imported_material_slot_name",
    ):
        try:
            value = slot.get_editor_property(property_name)
        except Exception:
            continue
        text = str(value).strip()
        if text and text.lower() != "none":
            names.add(text.casefold())
    return names


def _find_skeletal_slot_index(
    materials: list[object],
    binding: dict[str, object],
    character_label: str,
) -> int:
    expected_names = {
        str(name).casefold() for name in binding.get("slot_names", ())
    }
    for index, slot in enumerate(materials):
        if expected_names.intersection(_skeletal_slot_names(slot)):
            return index

    fallback_index = int(binding["slot_fallback_index"])
    if fallback_index < 0 or fallback_index >= len(materials):
        raise RuntimeError(
            f"{character_label} has {len(materials)} skeletal material slots; "
            f"cannot use required fallback slot {fallback_index}"
        )
    unreal.log_warning(
        f"W01 {character_label}: expected material slot names "
        f"{sorted(expected_names)} were not found; using verified FBX slot "
        f"index {fallback_index}"
    )
    return fallback_index


def _replacement_skeletal_material(
    old_slot: object,
    material: object,
):
    # Preserve the fields used by UE's own Experimental Toolsets helper.  The
    # optional imported slot name is copied when exposed by this engine build.
    replacement = unreal.SkeletalMaterial(
        material_interface=material,
        material_slot_name=old_slot.get_editor_property("material_slot_name"),
        uv_channel_data=old_slot.get_editor_property("uv_channel_data"),
    )
    try:
        imported_name = old_slot.get_editor_property(
            "imported_material_slot_name"
        )
        replacement.set_editor_property(
            "imported_material_slot_name",
            imported_name,
        )
    except Exception:
        # UE versions that do not expose this editor-only field still retain
        # the runtime slot name and UV channel data above.
        pass
    return replacement


def _repair_character_materials(rpg_root: Path) -> list[str]:
    repaired_material_paths = []

    for character_spec in CHARACTER_MATERIAL_SPECS:
        label = str(character_spec["label"])
        mesh_asset_name = str(character_spec["mesh_asset_name"])
        character_destination = f"{RPG_DESTINATION}/{label}"
        mesh_package_path = f"{character_destination}/{mesh_asset_name}"
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_package_path)
        if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
            raise RuntimeError(
                f"W01 character material repair requires {mesh_package_path}. "
                "Run import_w01_fantasy_assets.py first so Warrior exists."
            )

        materials = list(mesh.get_editor_property("materials"))
        if len(materials) < len(character_spec["bindings"]):
            raise RuntimeError(
                f"{mesh_package_path} has only {len(materials)} material slots"
            )

        mesh.modify()
        for binding in character_spec["bindings"]:
            texture_spec = {
                "source": binding["source"],
                "asset_name": binding["texture_asset_name"],
                "sha256": binding["sha256"],
            }
            texture_object_path = _import_texture(
                rpg_root,
                texture_spec,
                f"{character_destination}/Textures",
            )
            texture_package_path = texture_object_path.split(".", 1)[0]
            texture = unreal.EditorAssetLibrary.load_asset(texture_package_path)
            if texture is None or not isinstance(texture, unreal.Texture2D):
                raise RuntimeError(
                    f"W01 material repair could not load {texture_object_path}"
                )

            material_asset_name = str(binding["material_asset_name"])
            material_destination = f"{character_destination}/Materials"
            material = _create_textured_character_material(
                material_destination,
                material_asset_name,
                texture,
            )
            slot_index = _find_skeletal_slot_index(
                materials,
                binding,
                label,
            )
            materials[slot_index] = _replacement_skeletal_material(
                materials[slot_index],
                material,
            )

            material_object_path = (
                f"{material_destination}/{material_asset_name}."
                f"{material_asset_name}"
            )
            repaired_material_paths.append(material_object_path)
            unreal.log(
                f"W01 character material ready: {label} slot={slot_index} "
                f"material={material_object_path} "
                f"texture={texture_object_path}"
            )

        mesh.set_editor_property("materials", materials)
        assigned_materials = list(mesh.get_editor_property("materials"))
        for binding in character_spec["bindings"]:
            slot_index = _find_skeletal_slot_index(
                assigned_materials,
                binding,
                label,
            )
            assigned = assigned_materials[slot_index].get_editor_property(
                "material_interface"
            )
            expected_name = str(binding["material_asset_name"])
            if assigned is None or str(assigned.get_name()) != expected_name:
                raise RuntimeError(
                    f"W01 material assignment verification failed for "
                    f"{label} slot={slot_index}: expected={expected_name} "
                    f"actual={assigned}"
                )

        unreal.EditorAssetLibrary.save_loaded_asset(
            mesh,
            only_if_is_dirty=False,
        )

    return repaired_material_paths


def _animation_paths(destination: str, mesh_asset_name: str) -> list[str]:
    prefix = f"{mesh_asset_name}CharacterArmature_"
    results = []
    for package_path in unreal.EditorAssetLibrary.list_assets(
        destination,
        recursive=False,
        include_folder=False,
    ):
        asset = unreal.EditorAssetLibrary.load_asset(package_path)
        if asset is not None and isinstance(asset, unreal.AnimSequence):
            asset_name = str(asset.get_name())
            if asset_name.startswith(prefix):
                results.append(
                    unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(asset)
                )
    return sorted(results)


def _import_character(
    rpg_root: Path,
    spec: dict[str, object],
) -> tuple[str, list[str]]:
    source = _verified_source(rpg_root, spec, str(spec["label"]))
    asset_name = str(spec["asset_name"])
    destination = f"{RPG_DESTINATION}/{spec['label']}"

    for texture_spec in spec.get("textures", ()):
        _import_texture(
            rpg_root,
            texture_spec,
            f"{destination}/Textures",
        )

    imported_paths = _run_import_task(
        source,
        destination,
        asset_name,
        _make_skeletal_import_options(),
    )
    mesh = _resolve_expected_asset(
        imported_paths,
        destination,
        asset_name,
        unreal.SkeletalMesh,
    )
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)

    object_path = f"{destination}/{asset_name}.{asset_name}"
    animations = _animation_paths(destination, asset_name)
    required_suffixes = ("_Idle", "_Walk", "_Run")
    missing_suffixes = [
        suffix
        for suffix in required_suffixes
        if not any(path.split(".", 1)[0].endswith(suffix) for path in animations)
    ]
    if missing_suffixes:
        raise RuntimeError(
            f"Character {spec['label']} is missing required animations "
            f"{missing_suffixes}; imported={animations}"
        )

    unreal.log(f"W01 character mesh ready: {object_path}")
    for animation in animations:
        unreal.log(f"W01 character animation ready: {animation}")
    return object_path, animations


def main() -> None:
    world_root = _world_source_root()
    rpg_root = _rpg_source_root()
    material_repair_only = os.environ.get(
        MATERIAL_REPAIR_ONLY_ENV,
        "",
    ).strip().casefold() in {"1", "true", "yes"}
    character_roster_only = os.environ.get(
        CHARACTER_ROSTER_ONLY_ENV,
        "",
    ).strip().casefold() in {"1", "true", "yes"}
    if material_repair_only and character_roster_only:
        raise RuntimeError(
            f"{MATERIAL_REPAIR_ONLY_ENV} and {CHARACTER_ROSTER_ONLY_ENV} "
            "cannot both be enabled"
        )

    _prepare_sources(
        world_root,
        rpg_root,
        include_world=not (material_repair_only or character_roster_only),
    )
    _ensure_directory(DESTINATION_ROOT)

    # A visual-QA repair does not need to reimport the FBX files.  In
    # particular, the upstream Wizard FBX contains one empty child mesh that
    # Interchange reports as an error even though the usable skeletal mesh and
    # animations were already saved.  This mode only regenerates and binds the
    # deterministic texture materials to the existing five character meshes.
    if material_repair_only:
        repaired_material_paths = _repair_character_materials(rpg_root)
        expected_material_count = sum(
            len(spec["bindings"]) for spec in CHARACTER_MATERIAL_SPECS
        )
        unreal.EditorAssetLibrary.save_directory(
            RPG_DESTINATION,
            only_if_is_dirty=False,
            recursive=True,
        )
        unreal.log(
            "W01_CHARACTER_MATERIAL_REPAIR_COMPLETE "
            f"materials={len(repaired_material_paths)}/"
            f"{expected_material_count}"
        )
        return

    if character_roster_only:
        player_results = [
            _import_character(rpg_root, spec) for spec in PLAYER_SPECS
        ]
        repaired_material_paths = _repair_character_materials(rpg_root)
        expected_material_count = sum(
            len(spec["bindings"]) for spec in CHARACTER_MATERIAL_SPECS
        )
        unreal.EditorAssetLibrary.save_directory(
            RPG_DESTINATION,
            only_if_is_dirty=False,
            recursive=True,
        )
        unreal.log(
            "W01_RPG_CHARACTER_IMPORT_COMPLETE "
            f"player={len(player_results)}/{len(PLAYER_SPECS)} "
            f"animations={sum(len(result[1]) for result in player_results)} "
            f"materials={len(repaired_material_paths)}/"
            f"{expected_material_count}"
        )
        return

    environment_paths = [
        _import_static_mesh(world_root, spec)
        for spec in STATIC_MESH_SPECS
    ]
    player_results = [
        _import_character(rpg_root, spec) for spec in PLAYER_SPECS
    ]
    npc_results = [
        _import_character(rpg_root, spec) for spec in NPC_SPECS
    ]
    animation_count = sum(
        len(result[1]) for result in player_results + npc_results
    )
    repaired_material_paths = _repair_character_materials(rpg_root)
    expected_material_count = sum(
        len(spec["bindings"]) for spec in CHARACTER_MATERIAL_SPECS
    )

    unreal.EditorAssetLibrary.save_directory(
        DESTINATION_ROOT,
        only_if_is_dirty=False,
        recursive=True,
    )
    unreal.log(
        "W01_WORLD_ASSET_IMPORT_COMPLETE "
        f"environment={len(environment_paths)}/{len(STATIC_MESH_SPECS)} "
        f"player={len(player_results)}/{len(PLAYER_SPECS)} "
        f"npcs={len(npc_results)}/{len(NPC_SPECS)} "
        f"animations={animation_count} "
        f"materials={len(repaired_material_paths)}/{expected_material_count} "
        "lantern=geometry_fallback"
    )


if __name__ == "__main__":
    main()
