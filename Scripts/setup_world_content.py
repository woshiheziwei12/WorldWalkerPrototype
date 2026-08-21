import unreal


TEMPLATE_MAP = "/Engine/Maps/Templates/Template_Default"

WORLD_SPECS = (
    {
        "world_id": "W00_MainWorld",
        "display_name": "Main World",
        "root": "/Game/WorldWalker/Worlds/W00_MainWorld",
        "map_name": "L_W00_MainWorld_Night",
        "definition_name": "DA_W00_MainWorld",
        "is_main": True,
        "portal_color": unreal.LinearColor(0.05, 0.8, 1.0, 1.0),
    },
    {
        "world_id": "W01_EasternHorror",
        # Keep the stable identifier and package path; only the player-facing
        # name changes as W01 adopts its western-fantasy identity.
        "display_name": "灰烬王国",
        "root": "/Game/WorldWalker/Worlds/W01_EasternHorror",
        "map_name": "L_W01_Entry",
        "definition_name": "DA_W01_EasternHorror",
        "is_main": False,
        "portal_color": unreal.LinearColor(0.65, 0.08, 0.9, 1.0),
    },
    {
        "world_id": "W02_SpiralTower",
        "display_name": "螺旋高塔",
        "root": "/Game/WorldWalker/Worlds/W02_SpiralTower",
        "map_name": "L_W02_SpiralTower",
        "definition_name": "DA_W02_SpiralTower",
        "is_main": False,
        "portal_color": unreal.LinearColor(1.0, 0.32, 0.04, 1.0),
    },
)

W01_ROOT = "/Game/WorldWalker/Worlds/W01_EasternHorror"
W01_CARD_ROOT = f"{W01_ROOT}/Data/Cards"
W01_ENEMY_ROOT = f"{W01_ROOT}/Data/Enemies"
W01_CARD_ART_ROOT = (
    f"{W01_ROOT}/ThirdParty/Zonked/FantasyActionIcons/Cards"
)

CARD_SPECS = (
    {
        "asset_name": "DA_Card_LongswordSlash",
        "card_id": "LongswordSlash",
        "display_name": "长剑斩",
        "description": "以钢铁之印挥出严谨而致命的一击。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.ATTACK,
        "school": unreal.CardSchool.STEEL,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 8,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_LongSwordSlash",
        "copies": 3,
    },
    {
        "asset_name": "DA_Card_KiteShieldGuard",
        "card_id": "KiteShieldGuard",
        "display_name": "鸢盾格挡",
        "description": "唤起圣徽之力，在骑士盾后稳住阵脚。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SKILL,
        "school": unreal.CardSchool.FAITH,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 8,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_KiteShieldGuard",
        "copies": 2,
    },
    {
        "asset_name": "DA_Card_ArcaneSpark",
        "card_id": "ArcaneSpark",
        "display_name": "秘法火花",
        "description": "携带奥术之印的迅捷法术。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SPELL,
        "school": unreal.CardSchool.ARCANE,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 6,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_ArcaneSpark",
        "copies": 2,
    },
    {
        "asset_name": "DA_Card_ReadTheOpening",
        "card_id": "ReadTheOpening",
        "display_name": "洞悉破绽",
        "description": "看穿敌人防守中的一线空隙。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SPELL,
        "school": unreal.CardSchool.ARCANE,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.APPLY_STATUS,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 1,
                "status": unreal.FantasyCombatStatus.EXPOSED,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_ReadOpening",
        "copies": 1,
    },
    {
        "asset_name": "DA_Card_KnightsPrayer",
        "card_id": "KnightsPrayer",
        "display_name": "骑士祷言",
        "description": "可保留在手中的祷言，同时守护并净化自身。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SKILL,
        "school": unreal.CardSchool.FAITH,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 6,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.REMOVE_STATUS,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 1,
                "status": unreal.FantasyCombatStatus.EXPOSED,
            },
        ),
        "retain": True,
        "exhaust": False,
        "artwork_name": "T_Card_KnightsPrayer",
        "copies": 1,
    },
    {
        "asset_name": "DA_Card_LionheartJudgment",
        "card_id": "LionheartJudgment",
        "display_name": "狮心裁决",
        "description": "消耗英勇，以剑与盾兑现最后的誓约。",
        "energy_cost": 0,
        "valor_cost": 2,
        "card_type": unreal.CardType.OATH,
        "school": unreal.CardSchool.NONE,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 18,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 10,
            },
        ),
        "retain": True,
        "exhaust": True,
        "artwork_name": "T_Card_LionheartJudgment",
        "copies": 1,
    },
)

ENEMY_SPEC = {
    "asset_name": "DA_BlackthornOathKnight",
    "enemy_id": "BlackthornOathKnight",
    "display_name": "黑棘誓约骑士",
    "max_health": 92,
    "intents": (
        {
            "intent_id": "ShieldAdvance",
            "display_name": "盾阵推进",
            "effects": (
                {
                    "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                    "target": unreal.FantasyCombatTarget.SELF,
                    "magnitude": 8,
                },
                {
                    "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                    "target": unreal.FantasyCombatTarget.OPPONENT,
                    "magnitude": 7,
                },
            ),
        },
        {
            "intent_id": "ArmorBreak",
            "display_name": "破甲斩",
            "effects": (
                {
                    "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                    "target": unreal.FantasyCombatTarget.OPPONENT,
                    "magnitude": 12,
                },
                {
                    "effect_type": unreal.FantasyCombatEffectType.APPLY_STATUS,
                    "target": unreal.FantasyCombatTarget.OPPONENT,
                    "magnitude": 1,
                    "status": unreal.FantasyCombatStatus.EXPOSED,
                },
            ),
        },
        {
            "intent_id": "DarkOath",
            "display_name": "黑暗誓言",
            "effects": (
                {
                    "effect_type": unreal.FantasyCombatEffectType.APPLY_STATUS,
                    "target": unreal.FantasyCombatTarget.SELF,
                    "magnitude": 2,
                    "status": unreal.FantasyCombatStatus.STRENGTH,
                },
                {
                    "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                    "target": unreal.FantasyCombatTarget.SELF,
                    "magnitude": 6,
                },
            ),
        },
        {
            "intent_id": "JudgmentCleave",
            "display_name": "审判重斩",
            "effects": (
                {
                    "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                    "target": unreal.FantasyCombatTarget.OPPONENT,
                    "magnitude": 18,
                },
            ),
        },
    ),
}


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Failed to create content directory: {path}")


def ensure_world_directories(root):
    directories = (
        root,
        f"{root}/Maps",
        f"{root}/Data",
        f"{root}/Data/Cards",
        f"{root}/Data/Enemies",
        f"{root}/Data/Encounters",
        f"{root}/Data/Events",
        f"{root}/Data/NPCs",
        f"{root}/Art",
        f"{root}/Art/Environment",
        f"{root}/Art/Characters",
        f"{root}/Art/Props",
        f"{root}/Art/Materials",
        f"{root}/Art/VFX",
        f"{root}/UI",
        f"{root}/Audio",
    )
    for directory in directories:
        ensure_directory(directory)


def ensure_map(spec):
    destination = f"{spec['root']}/Maps/{spec['map_name']}"
    if not unreal.EditorAssetLibrary.does_asset_exist(destination):
        if spec["is_main"]:
            raise RuntimeError(
                "Authored W00 night map is missing; refusing to replace it "
                f"with the default template: {destination}"
            )
        duplicated = unreal.EditorAssetLibrary.duplicate_asset(TEMPLATE_MAP, destination)
        if duplicated is None:
            raise RuntimeError(f"Failed to duplicate template map to: {destination}")
        unreal.log(f"Created world map: {destination}")
    else:
        unreal.log(f"Keeping existing world map: {destination}")
    return destination


def ensure_world_definition(spec, map_path):
    definition_path = f"{spec['root']}/Data/{spec['definition_name']}"
    definition = (
        unreal.EditorAssetLibrary.load_asset(definition_path)
        if unreal.EditorAssetLibrary.does_asset_exist(definition_path)
        else None
    )

    if definition is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.WorldDefinition)
        definition = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            spec["definition_name"],
            f"{spec['root']}/Data",
            unreal.WorldDefinition,
            factory,
        )
        if definition is None:
            raise RuntimeError(f"Failed to create WorldDefinition: {definition_path}")
        unreal.log(f"Created WorldDefinition: {definition_path}")

    map_asset = unreal.EditorAssetLibrary.load_asset(map_path)
    if map_asset is None:
        raise RuntimeError(f"Unable to load entry map: {map_path}")

    definition.set_editor_property("world_id", unreal.Name(spec["world_id"]))
    definition.set_editor_property("display_name", spec["display_name"])
    definition.set_editor_property("entry_map", map_asset)
    definition.set_editor_property("is_main_world", spec["is_main"])
    definition.set_editor_property("portal_color", spec["portal_color"])
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    return definition_path


def make_combat_effect(spec):
    effect = unreal.FantasyCombatEffectSpec()
    effect.set_editor_property("effect_type", spec["effect_type"])
    effect.set_editor_property("target", spec["target"])
    effect.set_editor_property("magnitude", spec["magnitude"])
    effect.set_editor_property(
        "status",
        spec.get("status", unreal.FantasyCombatStatus.NONE),
    )
    return effect


def ensure_card_definition(spec):
    asset_path = f"{W01_CARD_ROOT}/{spec['asset_name']}"
    card = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )

    if card is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.CardDefinition)
        card = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            spec["asset_name"],
            W01_CARD_ROOT,
            unreal.CardDefinition,
            factory,
        )
        if card is None:
            raise RuntimeError(f"Failed to create CardDefinition: {asset_path}")
        unreal.log(f"Created CardDefinition: {asset_path}")

    card.set_editor_property("card_id", unreal.Name(spec["card_id"]))
    card.set_editor_property("card_set_id", unreal.Name("W01_EasternHorror"))
    card.set_editor_property("display_name", spec["display_name"])
    card.set_editor_property("description", spec["description"])
    card.set_editor_property("energy_cost", spec["energy_cost"])
    card.set_editor_property("valor_cost", spec["valor_cost"])
    card.set_editor_property("card_type", spec["card_type"])
    card.set_editor_property("school", spec["school"])
    card.set_editor_property(
        "effects",
        [make_combat_effect(effect) for effect in spec["effects"]],
    )
    # Unreal's Python reflection removes the native boolean `b` prefix.
    card.set_editor_property("retain", spec["retain"])
    card.set_editor_property("exhaust", spec["exhaust"])

    artwork_name = spec["artwork_name"]
    artwork_path = f"{W01_CARD_ART_ROOT}/{artwork_name}"
    artwork = (
        unreal.EditorAssetLibrary.load_asset(artwork_path)
        if unreal.EditorAssetLibrary.does_asset_exist(artwork_path)
        else None
    )
    card.set_editor_property("artwork", artwork)
    if artwork is None:
        unreal.log_warning(
            f"Card artwork not present yet; keeping Artwork empty: {artwork_path}"
        )

    card.set_editor_property("starting_deck_copies", spec["copies"])
    unreal.EditorAssetLibrary.save_loaded_asset(card, only_if_is_dirty=False)
    return asset_path


def make_enemy_intent(spec):
    intent = unreal.FantasyEnemyIntentStep()
    intent.set_editor_property("intent_id", unreal.Name(spec["intent_id"]))
    intent.set_editor_property("display_name", spec["display_name"])
    intent.set_editor_property(
        "effects",
        [make_combat_effect(effect) for effect in spec["effects"]],
    )
    return intent


def ensure_enemy_definition(spec):
    asset_path = f"{W01_ENEMY_ROOT}/{spec['asset_name']}"
    enemy = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )

    if enemy is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class",
            unreal.FantasyEnemyDefinition,
        )
        enemy = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            spec["asset_name"],
            W01_ENEMY_ROOT,
            unreal.FantasyEnemyDefinition,
            factory,
        )
        if enemy is None:
            raise RuntimeError(
                f"Failed to create FantasyEnemyDefinition: {asset_path}"
            )
        unreal.log(f"Created FantasyEnemyDefinition: {asset_path}")

    enemy.set_editor_property("enemy_id", unreal.Name(spec["enemy_id"]))
    enemy.set_editor_property("display_name", spec["display_name"])
    enemy.set_editor_property("max_health", spec["max_health"])
    enemy.set_editor_property(
        "intent_cycle",
        [make_enemy_intent(intent) for intent in spec["intents"]],
    )
    unreal.EditorAssetLibrary.save_loaded_asset(enemy, only_if_is_dirty=False)
    return asset_path


def main():
    for root in (
        "/Game/WorldWalker",
        "/Game/WorldWalker/Core",
        "/Game/WorldWalker/Shared",
        "/Game/WorldWalker/Shared/Data",
        "/Game/WorldWalker/Shared/Data/Cards",
        "/Game/WorldWalker/Shared/Characters",
        "/Game/WorldWalker/Shared/Cards",
        "/Game/WorldWalker/Shared/UI",
        "/Game/WorldWalker/Shared/VFX",
        "/Game/WorldWalker/Shared/Audio",
        "/Game/ThirdParty",
        "/Game/ThirdParty/Shared",
        "/Game/ThirdParty/W00_MainWorld",
        "/Game/ThirdParty/W01_EasternHorror",
        "/Game/ThirdParty/W02_SpiralTower",
    ):
        ensure_directory(root)

    for spec in WORLD_SPECS:
        ensure_world_directories(spec["root"])
        map_path = ensure_map(spec)
        definition_path = ensure_world_definition(spec, map_path)
        unreal.log(f"World content ready: {spec['world_id']} -> {definition_path}")

    for spec in CARD_SPECS:
        card_path = ensure_card_definition(spec)
        unreal.log(f"Card content ready: {spec['card_id']} -> {card_path}")

    enemy_path = ensure_enemy_definition(ENEMY_SPEC)
    unreal.log(
        f"Enemy content ready: {ENEMY_SPEC['enemy_id']} -> {enemy_path}"
    )

    unreal.EditorAssetLibrary.save_directory("/Game/WorldWalker", only_if_is_dirty=False, recursive=True)
    unreal.log("WORLD_WALKER_SETUP_COMPLETE")


if __name__ == "__main__":
    main()
