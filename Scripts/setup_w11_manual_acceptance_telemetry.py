import unreal


RULES_PATH = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/DA_W11_RunRules"
EXPECTED_SCHEMA = 3


def main():
    rules = unreal.EditorAssetLibrary.load_asset(RULES_PATH)
    if not isinstance(rules, unreal.W11RunRuleSet):
        raise RuntimeError(f"failed to load W11 run rules: {RULES_PATH}")
    rules.set_editor_property("combat_log_schema_version", EXPECTED_SCHEMA)
    if not unreal.EditorAssetLibrary.save_loaded_asset(rules, only_if_is_dirty=False):
        raise RuntimeError(f"failed to save W11 run rules: {RULES_PATH}")
    actual = int(rules.get_editor_property("combat_log_schema_version"))
    if actual != EXPECTED_SCHEMA:
        raise RuntimeError(f"combat log schema is {actual}, expected {EXPECTED_SCHEMA}")
    unreal.log(
        "W11_MANUAL_ACCEPTANCE_TELEMETRY_READY "
        f"schema={actual} recovery_punish_field=RecoveryPunishHits"
    )


main()
