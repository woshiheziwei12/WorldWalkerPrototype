import unreal


DATA = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data"


def load_asset(path, expected_type):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"missing required asset: {path}")
    if not isinstance(asset, expected_type):
        raise RuntimeError(f"unexpected asset type at {path}: {type(asset)}")
    return asset


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"failed to save asset: {asset.get_path_name()}")


def main():
    rules = load_asset(f"{DATA}/DA_W11_RunRules", unreal.W11RunRuleSet)
    stage_one = load_asset(
        f"{DATA}/Encounters/DA_Encounter_01", unreal.W11EncounterDefinition
    )

    # W11's first combat trial owns this multiplier. Regular enemy definitions stay
    # reusable, so later stages can be tuned independently instead of inheriting a
    # global health inflation.
    stage_one.set_editor_property("duration_seconds", 105.0)
    stage_one.set_editor_property("max_concurrent_enemies", 2)
    stage_one.set_editor_property("enemy_base_health_scale", 9.5)
    stage_one.set_editor_property("enemy_base_power_scale", 0.45)
    stage_one.set_editor_property("content_version", "W11-M6-v2")

    rules.set_editor_property("rule_version", "W11-Combat-v4")
    rules.set_editor_property("content_version", "W11-M6-v2")

    save(stage_one)
    save(rules)
    unreal.log(
        "W11_COMBAT_PACING_READY: "
        "Stage=1 DurationTarget=105.0 MaxConcurrent=2 "
        "EnemyHealthScale=9.5 EnemyPowerScale=0.45 "
        "RuleVersion=W11-Combat-v4 ContentVersion=W11-M6-v2"
    )


main()
