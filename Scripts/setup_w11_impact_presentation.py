import unreal


PRESENTATION = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Presentation/Skills"


def set_profile(visual, hit_stop, camera_strength):
    visual.set_editor_property("hit_stop_seconds", hit_stop)
    visual.set_editor_property("critical_hit_stop_seconds", min(0.12, hit_stop + 0.020))
    visual.set_editor_property("defeat_hit_stop_seconds", min(0.12, hit_stop + 0.040))
    visual.set_editor_property("camera_impact_strength", camera_strength)
    visual.set_editor_property("critical_camera_impact_strength", min(8.0, camera_strength + 2.0))
    visual.set_editor_property("defeat_camera_impact_strength", min(8.0, camera_strength + 3.5))
    visual.set_editor_property("camera_impact_duration", 0.12)
    visual.set_editor_property("content_version", "W11-M6-v2")


def main():
    updated = 0
    for path in unreal.EditorAssetLibrary.list_assets(
        PRESENTATION, recursive=False, include_folder=False
    ):
        visual = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(visual, unreal.W11SkillVisualDefinition):
            continue
        definition_id = str(visual.get_editor_property("definition_id"))
        if definition_id == "SkillVisual.CommonHitSpark":
            set_profile(visual, 0.025, 1.5)
        elif definition_id.startswith("SkillVisual.Enemy"):
            set_profile(visual, 0.030, 2.5)
        else:
            set_profile(visual, 0.030, 2.0)
        if not unreal.EditorAssetLibrary.save_loaded_asset(visual, only_if_is_dirty=False):
            raise RuntimeError(f"failed to save impact profile: {path}")
        updated += 1
    if updated < 4:
        raise RuntimeError(f"expected shared and enemy impact visuals, updated only {updated}")
    unreal.log(
        f"W11_IMPACT_PRESENTATION_READY visuals={updated} "
        "hit_stop_cap=0.120 camera_cap=8.0 global_time_dilation=unchanged"
    )


main()
