import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_FONT = os.path.join(PROJECT_DIR, "SourceArt", "W11", "Fonts", "MaShanZheng-Regular.ttf")
DESTINATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/UI/Fonts"
FACE_NAME = "FF_W11_MaShanZheng"
FONT_NAME = f"{FACE_NAME}_Font"


def import_font() -> None:
    if not os.path.isfile(SOURCE_FONT):
        raise RuntimeError(f"Missing font source: {SOURCE_FONT}")

    factory = unreal.FontFileImportFactory()
    factory.set_editor_property(
        "batch_create_font_asset",
        unreal.BatchCreateFontAsset.CREATE_IF_NO_FONT_EXISTS,
    )

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE_FONT)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", FACE_NAME)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("factory", factory)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    face_path = f"{DESTINATION}/{FACE_NAME}.{FACE_NAME}"
    font_path = f"{DESTINATION}/{FONT_NAME}.{FONT_NAME}"
    face_asset = unreal.EditorAssetLibrary.load_asset(face_path)
    font_asset = unreal.EditorAssetLibrary.load_asset(font_path)
    if not face_asset or not font_asset:
        raise RuntimeError(
            "W11 calligraphy font import incomplete: "
            f"face={bool(face_asset)} font={bool(font_asset)} imported={task.imported_object_paths}"
        )

    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "W11_CALLIGRAPHY_FONT_READY "
        f"face={face_path} font={font_path} source={SOURCE_FONT}"
    )


if __name__ == "__main__":
    import_font()
