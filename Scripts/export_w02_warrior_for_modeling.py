"""Export the W02-private Warrior skeletal mesh as a Blender rigging template."""

from pathlib import Path

import unreal


SOURCE_ASSET = (
    "/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Quaternius/"
    "RPGCharacters/Warrior/SK_W01_Warrior"
)


def main() -> None:
    source_mesh = unreal.EditorAssetLibrary.load_asset(SOURCE_ASSET)
    if not source_mesh:
        raise RuntimeError(f"Missing W02 Warrior source mesh: {SOURCE_ASSET}")

    output_directory = (
        Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_intermediate_dir()))
        / "W02CharacterModeling"
    )
    output_directory.mkdir(parents=True, exist_ok=True)
    output_fbx = output_directory / "W02_Warrior_RigTemplate.fbx"

    options = unreal.FbxExportOption()
    options.set_editor_property("ascii", False)
    options.set_editor_property("level_of_detail", False)
    options.set_editor_property("export_source_mesh", False)

    task = unreal.AssetExportTask()
    task.set_editor_property("object", source_mesh)
    task.set_editor_property("filename", str(output_fbx))
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("options", options)

    if not unreal.Exporter.run_asset_export_task(task) or not output_fbx.is_file():
        raise RuntimeError(f"Failed to export W02 Warrior rig template: {output_fbx}")

    unreal.log(f"W02_WARRIOR_RIG_EXPORT_COMPLETE file={output_fbx}")

    # This script is also used with a render-offscreen Editor process because
    # UE 5.8's skeletal exporter requires an initialized mesh render object.
    if "-run=pythonscript" not in unreal.SystemLibrary.get_command_line().lower():
        unreal.SystemLibrary.quit_editor()


main()
