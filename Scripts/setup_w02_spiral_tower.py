"""Create the self-contained W02 spiral-tower content skeleton.

Run this script from Unreal Editor's Python environment after the C++ editor
target has been built. It is intentionally idempotent: existing authored W02
assets are kept, while missing maps, data assets, directories, and private art
copies are created without downloading any new resources.

The selected Quaternius meshes are duplicated one directory at a time instead
of copying only their primary StaticMesh assets. The FBX importer isolated each
mesh and its generated materials in that private directory, so this preserves
all dependencies and keeps W02 independent from W01 at runtime.
"""

import unreal


TEMPLATE_MAP = "/Engine/Maps/Templates/Template_Default"
W02_ROOT = "/Game/WorldWalker/Worlds/W02_SpiralTower"
W02_MAP = f"{W02_ROOT}/Maps/L_W02_SpiralTower"
W02_DEFINITION = f"{W02_ROOT}/Data/DA_W02_SpiralTower"
W01_ENVIRONMENT_ROOT = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/"
    "Quaternius/Environment"
)
W02_ENVIRONMENT_ROOT = f"{W02_ROOT}/ThirdParty/Quaternius/Environment"
W01_WARRIOR_DIRECTORY = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/"
    "Quaternius/RPGCharacters/Warrior"
)
W02_WARRIOR_DIRECTORY = (
    f"{W02_ROOT}/ThirdParty/Quaternius/RPGCharacters/Warrior"
)
PRIVATE_ENVIRONMENT_ASSETS = (
    "SM_W01_Tower",
    "SM_W01_Wall",
    "SM_W01_Arch",
    "SM_W01_Campfire",
    "SM_W01_Path",
)
PORTAL_COLOR = unreal.LinearColor(1.0, 0.32, 0.04, 1.0)


def ensure_directory(path):
    if unreal.EditorAssetLibrary.does_directory_exist(path):
        return
    if not unreal.EditorAssetLibrary.make_directory(path):
        raise RuntimeError(f"Failed to create content directory: {path}")


def ensure_world_directories():
    directories = (
        W02_ROOT,
        f"{W02_ROOT}/Maps",
        f"{W02_ROOT}/Data",
        f"{W02_ROOT}/Data/Cards",
        f"{W02_ROOT}/Data/Enemies",
        f"{W02_ROOT}/Data/Encounters",
        f"{W02_ROOT}/Data/Events",
        f"{W02_ROOT}/Data/NPCs",
        f"{W02_ROOT}/Art",
        f"{W02_ROOT}/Art/Environment",
        f"{W02_ROOT}/Art/Characters",
        f"{W02_ROOT}/Art/Props",
        f"{W02_ROOT}/Art/Materials",
        f"{W02_ROOT}/Art/VFX",
        f"{W02_ROOT}/UI",
        f"{W02_ROOT}/Audio",
        f"{W02_ROOT}/ThirdParty",
        f"{W02_ROOT}/ThirdParty/Quaternius",
        f"{W02_ROOT}/ThirdParty/Quaternius/RPGCharacters",
        W02_ENVIRONMENT_ROOT,
    )
    for directory in directories:
        ensure_directory(directory)


def ensure_map():
    if unreal.EditorAssetLibrary.does_asset_exist(W02_MAP):
        unreal.log(f"Keeping existing W02 map: {W02_MAP}")
        return W02_MAP

    duplicated = unreal.EditorAssetLibrary.duplicate_asset(TEMPLATE_MAP, W02_MAP)
    if duplicated is None:
        raise RuntimeError(
            f"Failed to duplicate template map {TEMPLATE_MAP} to {W02_MAP}"
        )
    unreal.log(f"Created W02 map from Template_Default: {W02_MAP}")
    return W02_MAP


def ensure_world_definition(map_path):
    definition = (
        unreal.EditorAssetLibrary.load_asset(W02_DEFINITION)
        if unreal.EditorAssetLibrary.does_asset_exist(W02_DEFINITION)
        else None
    )
    if definition is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.WorldDefinition)
        definition = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_W02_SpiralTower",
            f"{W02_ROOT}/Data",
            unreal.WorldDefinition,
            factory,
        )
        if definition is None:
            raise RuntimeError(
                f"Failed to create W02 WorldDefinition: {W02_DEFINITION}"
            )
        unreal.log(f"Created W02 WorldDefinition: {W02_DEFINITION}")

    map_asset = unreal.EditorAssetLibrary.load_asset(map_path)
    if map_asset is None:
        raise RuntimeError(f"Unable to load W02 entry map: {map_path}")

    definition.set_editor_property("world_id", unreal.Name("W02_SpiralTower"))
    definition.set_editor_property("display_name", "螺旋高塔")
    definition.set_editor_property("entry_map", map_asset)
    definition.set_editor_property("is_main_world", False)
    definition.set_editor_property("portal_color", PORTAL_COLOR)
    unreal.EditorAssetLibrary.save_loaded_asset(
        definition,
        only_if_is_dirty=False,
    )
    unreal.log(f"W02 WorldDefinition ready: {W02_DEFINITION}")
    return W02_DEFINITION


def ensure_private_environment_copy(asset_name):
    source_directory = f"{W01_ENVIRONMENT_ROOT}/{asset_name}"
    destination_directory = f"{W02_ENVIRONMENT_ROOT}/{asset_name}"
    destination_asset = f"{destination_directory}/{asset_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(destination_asset):
        unreal.log(f"Keeping existing W02 private mesh directory: {asset_name}")
        return "kept"

    if not unreal.EditorAssetLibrary.does_directory_exist(source_directory):
        unreal.log_warning(
            "W02 source environment directory is missing; runtime geometry "
            f"fallback will remain available: {source_directory}"
        )
        return "missing"

    if not unreal.EditorAssetLibrary.duplicate_directory(
        source_directory,
        destination_directory,
    ):
        unreal.log_warning(
            "W02 could not duplicate the private environment directory; "
            f"runtime geometry fallback will remain available: {source_directory}"
        )
        return "missing"

    if not unreal.EditorAssetLibrary.does_asset_exist(destination_asset):
        unreal.log_warning(
            "W02 directory copy completed without its expected primary mesh; "
            f"runtime geometry fallback will remain available: {destination_asset}"
        )
        return "missing"

    unreal.log(
        "Duplicated W02 private environment directory: "
        f"{source_directory} -> {destination_directory}"
    )
    return "duplicated"


def ensure_private_warrior_copy():
    expected_mesh = f"{W02_WARRIOR_DIRECTORY}/SK_W01_Warrior"
    if unreal.EditorAssetLibrary.does_asset_exist(expected_mesh):
        unreal.log("Keeping existing W02 private medieval climber directory")
        return "kept"
    if not unreal.EditorAssetLibrary.does_directory_exist(W01_WARRIOR_DIRECTORY):
        unreal.log_warning(
            "W02 medieval climber source is missing; prototype body fallback "
            f"will remain available: {W01_WARRIOR_DIRECTORY}"
        )
        return "missing"
    if not unreal.EditorAssetLibrary.duplicate_directory(
        W01_WARRIOR_DIRECTORY,
        W02_WARRIOR_DIRECTORY,
    ):
        unreal.log_warning(
            "W02 could not duplicate its private medieval climber directory: "
            f"{W01_WARRIOR_DIRECTORY}"
        )
        return "missing"
    if not unreal.EditorAssetLibrary.does_asset_exist(expected_mesh):
        unreal.log_warning(
            "W02 medieval climber copy completed without the expected mesh: "
            f"{expected_mesh}"
        )
        return "missing"
    unreal.log(
        "Duplicated W02 private medieval climber directory: "
        f"{W01_WARRIOR_DIRECTORY} -> {W02_WARRIOR_DIRECTORY}"
    )
    return "duplicated"


def main():
    ensure_world_directories()
    map_path = ensure_map()
    definition_path = ensure_world_definition(map_path)

    results = [
        ensure_private_environment_copy(asset_name)
        for asset_name in PRIVATE_ENVIRONMENT_ASSETS
    ]
    warrior_result = ensure_private_warrior_copy()
    unreal.EditorAssetLibrary.save_directory(
        W02_ROOT,
        only_if_is_dirty=False,
        recursive=True,
    )
    unreal.log(
        "W02_SPIRAL_TOWER_SETUP_COMPLETE "
        f"map={map_path} definition={definition_path} "
        f"duplicated={results.count('duplicated')} "
        f"kept={results.count('kept')} missing={results.count('missing')} "
        f"warrior={warrior_result}"
    )


if __name__ == "__main__":
    main()
