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
        "portal_order": 0,
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
        "portal_order": 10,
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
W01_CHAPTER_ROOT = f"{W01_ROOT}/Data/Encounters"
W01_BLESSING_ROOT = f"{W01_ROOT}/Data/Blessings"
W01_CARD_ART_ROOT = (
    f"{W01_ROOT}/ThirdParty/Zonked/FantasyActionIcons/Cards"
)
W00_MAP = "/Game/WorldWalker/Worlds/W00_MainWorld/Maps/L_W00_MainWorld_Night"
W00_DEFINITION = "/Game/WorldWalker/Worlds/W00_MainWorld/Data/DA_W00_MainWorld"

# Historical W01 rune-prototype data is intentionally kept here only so older
# asset names remain easy to audit during migration.  It is not generated; the
# classic-mode candidate data below replaces it atomically.
_LEGACY_CARD_SPECS_DO_NOT_GENERATE = (
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
    # Victory-only progression cards.  They reuse the existing CC0 icon set,
    # remain outside the ten-card starter deck, and are discovered through the
    # CardDefinition reward flag rather than a hard-coded runtime list.
    {
        "asset_name": "DA_Card_BlackthornRiposte",
        "card_id": "BlackthornRiposte",
        "display_name": "黑棘还击",
        "description": "架开兵刃后立刻反斩，兼顾钢铁系的攻防节奏。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.ATTACK,
        "school": unreal.CardSchool.STEEL,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 9,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 4,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_LongSwordSlash",
        "copies": 0,
        "reward_eligible": True,
    },
    {
        "asset_name": "DA_Card_FreeCompanyBanner",
        "card_id": "FreeCompanyBanner",
        "display_name": "自由佣兵旗",
        "description": "立下战旗，使本场战斗中的钢铁攻击愈发凌厉。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.OATH,
        "school": unreal.CardSchool.STEEL,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.APPLY_STATUS,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 1,
                "status": unreal.FantasyCombatStatus.STRENGTH,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.GAIN_VALOR,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 1,
            },
        ),
        "retain": False,
        "exhaust": True,
        "artwork_name": "T_Card_LionheartJudgment",
        "copies": 0,
        "reward_eligible": True,
    },
    {
        "asset_name": "DA_Card_DawnAegis",
        "card_id": "DawnAegis",
        "display_name": "黎明圣盾",
        "description": "晨曦驱散破甲诅咒，并赐予厚重守护。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SKILL,
        "school": unreal.CardSchool.FAITH,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 12,
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
        "artwork_name": "T_Card_KiteShieldGuard",
        "copies": 0,
        "reward_eligible": True,
    },
    {
        "asset_name": "DA_Card_PilgrimsSanctuary",
        "card_id": "PilgrimsSanctuary",
        "display_name": "朝圣者圣所",
        "description": "以圣徽构筑短暂圣所，同时疗愈伤势。",
        "energy_cost": 2,
        "valor_cost": 0,
        "card_type": unreal.CardType.SKILL,
        "school": unreal.CardSchool.FAITH,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.HEAL,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 8,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.BLOCK,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 8,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_KnightsPrayer",
        "copies": 0,
        "reward_eligible": True,
    },
    {
        "asset_name": "DA_Card_RuneCascade",
        "card_id": "RuneCascade",
        "display_name": "符文奔流",
        "description": "释放奥术奔流后窥见下一枚符文。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SPELL,
        "school": unreal.CardSchool.ARCANE,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.DAMAGE,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 8,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.DRAW,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 1,
            },
        ),
        "retain": False,
        "exhaust": False,
        "artwork_name": "T_Card_ArcaneSpark",
        "copies": 0,
        "reward_eligible": True,
    },
    {
        "asset_name": "DA_Card_HourglassHex",
        "card_id": "HourglassHex",
        "display_name": "沙漏咒缚",
        "description": "扭曲敌人的行动节奏，并立即补充一张手牌。",
        "energy_cost": 1,
        "valor_cost": 0,
        "card_type": unreal.CardType.SPELL,
        "school": unreal.CardSchool.ARCANE,
        "effects": (
            {
                "effect_type": unreal.FantasyCombatEffectType.APPLY_STATUS,
                "target": unreal.FantasyCombatTarget.OPPONENT,
                "magnitude": 2,
                "status": unreal.FantasyCombatStatus.WEAK,
            },
            {
                "effect_type": unreal.FantasyCombatEffectType.DRAW,
                "target": unreal.FantasyCombatTarget.SELF,
                "magnitude": 1,
            },
        ),
        "retain": False,
        "exhaust": True,
        "artwork_name": "T_Card_ReadOpening",
        "copies": 0,
        "reward_eligible": True,
    },
)

_LEGACY_ENEMY_SPEC_DO_NOT_GENERATE = {
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


# Reference boundary for the data generated below:
# - Public research confirms these classic-mode card names, their broad types,
#   female-knight availability, and each enemy's minimum named-card set.
# - Public sources do NOT expose an auditable current-version starting-deck
#   copy list, enemy copies/levels, or complete numeric rules.
# - Consequently every `copies` value and every value not explicitly cited in
#   the nearby card comment is WorldWalker prototype tuning. Marker logs repeat
#   this fact so generated assets cannot be mistaken for an original snapshot.
CLASSIC_PROTOTYPE_BOUNDARY = (
    "公开资料核验卡名/职业及部分数值；初始牌份数与未能完整复原的规则为项目调参，"
    "不声称精确还原《月圆之夜》当前版本。"
)
ENEMY_PROTOTYPE_BOUNDARY = (
    "公开资料核验该对手的最低牌名集合及少量数值；牌组份数、等级和其余数值为项目调参。"
)


def _effect(
    effect_type,
    magnitude,
    target=unreal.FantasyCombatTarget.OPPONENT,
    status=unreal.FantasyCombatStatus.NONE,
    piercing=False,
    scales_with_strength=False,
    payload_id="",
    limit=0,
    multiplier=1,
):
    return {
        "effect_type": effect_type,
        "target": target,
        "magnitude": magnitude,
        "status": status,
        "piercing": piercing,
        "scales_with_strength": scales_with_strength,
        "payload_id": payload_id,
        "limit": limit,
        "multiplier": multiplier,
    }


def _card(
    asset_name,
    card_id,
    display_name,
    card_type,
    effects,
    artwork_name,
    description,
    *,
    card_set_id="W01_EasternHorror",
    copies=0,
    reward_eligible=False,
    action_cost=0,
    mana_cost=0,
    energy_cost=0,
    valor_cost=0,
    retain=False,
    exhaust=False,
    equipment_attack=0,
    equipment_block=0,
    equipment_draw=0,
    profession=None,
    school=unreal.CardSchool.NONE,
    upgrade_level=0,
    upgrade_card_id=None,
    build_tags=(),
):
    if profession is None:
        profession = (
            unreal.FantasyPlayerProfession.KNIGHT
            if card_set_id == "W01_EasternHorror"
            else unreal.FantasyPlayerProfession.NONE
        )
    return {
        "asset_name": asset_name,
        "card_id": card_id,
        "card_set_id": card_set_id,
        "display_name": display_name,
        "description": description,
        "energy_cost": energy_cost,
        "action_cost": action_cost,
        "mana_cost": mana_cost,
        "use_classic_resources": True,
        "valor_cost": valor_cost,
        "card_type": card_type,
        "school": school,
        "profession": profession,
        "effects": tuple(effects),
        "retain": retain,
        "exhaust": exhaust,
        "artwork_name": artwork_name,
        "copies": copies,
        "reward_eligible": reward_eligible,
        "equipment_attack": equipment_attack,
        "equipment_block": equipment_block,
        "equipment_draw": equipment_draw,
        "upgrade_level": upgrade_level,
        "upgrade_card_id": upgrade_card_id,
        "build_tags": tuple(build_tags),
    }


def _player_description(summary, reward=False, profession="女骑士"):
    source_kind = (
        f"经典{profession}公开牌名"
        if reward
        else f"{profession}初始牌原型候选"
    )
    return f"【{source_kind}】{summary} {CLASSIC_PROTOTYPE_BOUNDARY}"


def _enemy_description(summary):
    return f"【敌方最低确认牌名】{summary} {ENEMY_PROTOTYPE_BOUNDARY}"


# Ten-card female-knight starter *candidate*: all six names are verified as
# classic player cards available to the female knight.  The 5/1/1/1/1/1 copy
# distribution is deliberately project-owned tuning because no public source
# provides a current, auditable exact starting-deck copy list.
PLAYER_CARD_SPECS = (
    _card(
        "DA_Card_LongswordSlash", "Knight_NormalAttack", "普通攻击",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),),
        "T_Card_LongSwordSlash",
        _player_description("朴素的无费用攻击。"),
        copies=5,
    ),
    _card(
        "DA_Card_KiteShieldGuard", "Knight_SwiftAttack", "迅捷攻击",
        unreal.CardType.ATTACK,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 5),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_LongSwordSlash",
        _player_description("造成伤害并补充手牌。"),
        copies=1,
    ),
    _card(
        "DA_Card_ArcaneSpark", "Knight_Focus", "专注",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                2,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_ReadOpening",
        _player_description("消耗行动力整理手牌。"),
        action_cost=1,
        copies=1,
    ),
    _card(
        "DA_Card_ReadTheOpening", "Knight_ShortSword", "短剑",
        unreal.CardType.EQUIPMENT,
        (),
        "T_Card_LongSwordSlash",
        _player_description("装备后强化攻击牌。"),
        equipment_attack=1,
        copies=1,
    ),
    _card(
        "DA_Card_KnightsPrayer", "Knight_RoundShield", "圆盾",
        unreal.CardType.EQUIPMENT,
        (),
        "T_Card_KiteShieldGuard",
        _player_description("装备后每回合提供格挡。"),
        equipment_block=4,
        copies=1,
    ),
    _card(
        "DA_Card_LionheartJudgment", "Knight_StrengthBlessing", "力量祝福",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.APPLY_STATUS,
                3,
                unreal.FantasyCombatTarget.SELF,
                unreal.FantasyCombatStatus.STRENGTH,
            ),
        ),
        "T_Card_LionheartJudgment",
        _player_description("获得持续力量。"),
        action_cost=1,
        copies=1,
    ),
)


# A verified-name female-knight reward pool.  Effects intentionally use the
# project's composable effect language instead of copying original rule prose.
PLAYER_REWARD_CARD_SPECS = (
    _card(
        "DA_Card_BlackthornRiposte", "Knight_Crush", "粉碎",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 15),),
        "T_Card_LongSwordSlash", _player_description("沉重的一击。", True),
        reward_eligible=True,
    ),
    _card(
        "DA_Card_FreeCompanyBanner", "Knight_DesperateDuel", "舍命相搏",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 9, piercing=True),),
        "T_Card_LionheartJudgment",
        _player_description("造成穿刺伤害。", True),
        reward_eligible=True,
    ),
    _card(
        "DA_Card_DawnAegis", "Knight_DoubleStrike", "二次打击",
        unreal.CardType.ATTACK,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 2),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                2,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_LongSwordSlash",
        _player_description("造成 2 点伤害并抽 2 张牌。", True),
        reward_eligible=True,
    ),
    _card(
        "DA_Card_PilgrimsSanctuary", "Knight_ChargedStrike", "蓄力一击",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 12),),
        "T_Card_LongSwordSlash", _player_description("蓄势后挥出重击。", True),
        # The public index confirms damage/removal/action tags but not enough
        # rule syntax to reproduce the card honestly. Keep the researched name
        # as a disabled candidate until filtered draw/conditional scaling lands.
        reward_eligible=False, exhaust=True,
    ),
    _card(
        "DA_Card_RuneCascade", "Knight_SpikedShield", "尖刺盾牌",
        unreal.CardType.ATTACK,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 5),
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                5,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _player_description("同时进攻与防守。", True),
        reward_eligible=False,
    ),
    _card(
        "DA_Card_HourglassHex", "Knight_AbsoluteDefense", "绝对防御",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                6,
                unreal.FantasyCombatTarget.SELF,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _player_description("防守并补充手牌。", True),
        reward_eligible=True, action_cost=1,
    ),
    _card(
        "DA_Card_BattleHorn", "Knight_BattleHorn", "战斗号角",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.APPLY_STATUS,
                1,
                unreal.FantasyCombatTarget.SELF,
                unreal.FantasyCombatStatus.STRENGTH,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_LionheartJudgment", _player_description("整备下一轮攻势。", True),
        reward_eligible=False, action_cost=1, exhaust=True,
    ),
    _card(
        "DA_Card_BraveHeart", "Knight_BraveHeart", "勇敢的心",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.APPLY_STATUS,
                1,
                unreal.FantasyCombatTarget.SELF,
                unreal.FantasyCombatStatus.STRENGTH,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KnightsPrayer", _player_description("鼓起勇气继续行动。", True),
        reward_eligible=False, action_cost=1,
    ),
    _card(
        "DA_Card_HoldFast", "Knight_HoldFast", "苦守",
        unreal.CardType.ACTION,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 1, piercing=True),
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                10,
                unreal.FantasyCombatTarget.SELF,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _player_description("坚守阵线。", True),
        reward_eligible=True, action_cost=1,
    ),
    _card(
        "DA_Card_Rashomon", "Knight_Rashomon", "罗生门",
        unreal.CardType.ACTION,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 4),
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                8,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _player_description("攻防一体。", True),
        reward_eligible=False, action_cost=1,
    ),
    _card(
        "DA_Card_AngelShelter", "Knight_AngelShelter", "天使庇护",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                10,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KnightsPrayer", _player_description("获得大量格挡。", True),
        reward_eligible=True, action_cost=1, retain=True,
    ),
    _card(
        "DA_Card_BerserkerAxe", "Knight_BerserkerAxe", "狂战斧",
        unreal.CardType.EQUIPMENT,
        (),
        "T_Card_LongSwordSlash", _player_description("装备后强化攻击。", True),
        # The indexed rule is attack-card copying, which the current runtime
        # does not implement. Do not offer a knowingly different substitute.
        reward_eligible=False, equipment_attack=2,
    ),
)


# The mage is the classic-mode Little Witch profession. Names and the numeric
# fragments below come from the verified card index. Starter copies and rules
# that the public index does not fully preserve remain explicit project tuning.
MAGE_CARD_SPECS = (
    _card(
        "DA_Card_MageNormalAttack", "Mage_NormalAttack", "普通攻击",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),),
        "T_Card_LongSwordSlash",
        _player_description("基础攻击。", profession="小女巫"),
        copies=3, profession=unreal.FantasyPlayerProfession.MAGE,
    ),
    _card(
        "DA_Card_MageMana", "Mage_Mana", "法力",
        unreal.CardType.MANA,
        (_effect(
            unreal.FantasyCombatEffectType.GAIN_MANA,
            6,
            unreal.FantasyCombatTarget.SELF,
        ),),
        "T_Card_ArcaneSpark",
        _player_description("获得 6 点法力。", profession="小女巫"),
        copies=2, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageWisdom", "Mage_Wisdom", "智慧",
        unreal.CardType.MANA,
        (
            _effect(
                unreal.FantasyCombatEffectType.GAIN_MANA,
                5,
                unreal.FantasyCombatTarget.SELF,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_ReadOpening",
        _player_description("获得 5 点法力并抽 1 张牌。", profession="小女巫"),
        copies=1, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageFireSeed", "Mage_FireSeed", "火苗",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),),
        "T_Card_ArcaneSpark",
        _player_description(
            "公开索引保留 2、2 点火属性片段；当前合并为 4 点伤害。",
            profession="小女巫",
        ),
        copies=1, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageFireBlast", "Mage_FireBlast", "火焰冲击",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 10),),
        "T_Card_ArcaneSpark",
        _player_description("消耗 4 法力造成 10 点伤害。", profession="小女巫"),
        copies=2, mana_cost=4,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageFocus", "Mage_Focus", "专注",
        unreal.CardType.ACTION,
        (_effect(
            unreal.FantasyCombatEffectType.DRAW,
            2,
            unreal.FantasyCombatTarget.SELF,
        ),),
        "T_Card_ReadOpening",
        _player_description("消耗 1 行动力抽 2 张牌。", profession="小女巫"),
        copies=1, action_cost=1,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
)


MAGE_REWARD_CARD_SPECS = (
    _card(
        "DA_Card_MageManaSource", "Mage_ManaSource", "法力源泉",
        unreal.CardType.MANA,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 5, unreal.FantasyCombatTarget.SELF),),
        "T_Card_ArcaneSpark",
        _player_description("获得 5 点法力。", True, "小女巫"),
        reward_eligible=True, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageManaTide", "Mage_ManaTide", "法力之潮",
        unreal.CardType.MANA,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 12, unreal.FantasyCombatTarget.SELF),),
        "T_Card_ArcaneSpark",
        _player_description("获得 12 点法力。", True, "小女巫"),
        reward_eligible=True, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageSpellProphecy", "Mage_SpellProphecy", "法术预言",
        unreal.CardType.MANA,
        (
            _effect(unreal.FantasyCombatEffectType.GAIN_MANA, 5, unreal.FantasyCombatTarget.SELF),
            _effect(unreal.FantasyCombatEffectType.DRAW, 1, unreal.FantasyCombatTarget.SELF),
        ),
        "T_Card_ReadOpening",
        _player_description("获得 5 点法力并抽 1 张牌。", True, "小女巫"),
        reward_eligible=True, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageFireball", "Mage_Fireball", "火球",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 18),),
        "T_Card_ArcaneSpark",
        _player_description("消耗 7 法力造成 18 点伤害。", True, "小女巫"),
        reward_eligible=True, mana_cost=7,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MagePyroblast", "Mage_Pyroblast", "炎爆术",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 30),),
        "T_Card_ArcaneSpark",
        _player_description("消耗 9 法力造成 30 点伤害。", True, "小女巫"),
        reward_eligible=True, mana_cost=9,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageAcidSpray", "Mage_AcidSpray", "酸性喷雾",
        unreal.CardType.SPELL,
        (_effect(
            unreal.FantasyCombatEffectType.APPLY_STATUS,
            4,
            unreal.FantasyCombatTarget.OPPONENT,
            unreal.FantasyCombatStatus.POISON,
        ),),
        "T_Card_ArcaneSpark",
        _player_description("消耗 4 法力施加 4 层中毒。", True, "小女巫"),
        reward_eligible=True, mana_cost=4,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageIceShield", "Mage_IceShield", "冰盾",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.BLOCK, 6, unreal.FantasyCombatTarget.SELF),),
        "T_Card_KiteShieldGuard",
        _player_description("消耗 4 法力获得 6 格挡。", True, "小女巫"),
        reward_eligible=True, mana_cost=4,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
    _card(
        "DA_Card_MageWindStone", "Mage_WindStone", "风之石",
        unreal.CardType.SPELL,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 4),
            _effect(unreal.FantasyCombatEffectType.DRAW, 1, unreal.FantasyCombatTarget.SELF),
        ),
        "T_Card_ReadOpening",
        _player_description("消耗 1 法力造成 4 点伤害并抽 1 张牌。", True, "小女巫"),
        reward_eligible=True, mana_cost=1,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
    ),
)

M2_MAGE_CARD_SPECS = (
    _card(
        "DA_Card_MageEmberSigil", "Mage_EmberSigil", "余烬印记",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.BURNING),),
        "T_Card_ArcaneSpark", "【M2 火焰原型】施加 2 层燃烧。",
        reward_eligible=True, mana_cost=1,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_EmberSigilPlus",
        build_tags=("Archetype.Fire", "Element.Fire"),
    ),
    _card(
        "DA_Card_MageEmberSigilPlus", "Mage_EmberSigilPlus", "余烬印记+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.BURNING),),
        "T_Card_ArcaneSpark", "【M2 火焰升级】施加 3 层燃烧。",
        mana_cost=1, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Fire", "Element.Fire"),
    ),
    _card(
        "DA_Card_MageFlameFan", "Mage_FlameFan", "焰扇",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.BURNING)),
        "T_Card_ArcaneSpark", "【M2 火焰原型】造成 4 伤害并施加 2 层燃烧。",
        reward_eligible=True, mana_cost=2,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_FlameFanPlus",
        build_tags=("Archetype.Fire", "Element.Fire"),
    ),
    _card(
        "DA_Card_MageFlameFanPlus", "Mage_FlameFanPlus", "焰扇+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.BURNING)),
        "T_Card_ArcaneSpark", "【M2 火焰升级】造成 5 伤害并施加 3 层燃烧。",
        mana_cost=2, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Fire", "Element.Fire"),
    ),
    _card(
        "DA_Card_MageConflagrate", "Mage_Conflagrate", "引燃",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_DAMAGE, 0,
                 status=unreal.FantasyCombatStatus.BURNING, multiplier=3),),
        "T_Card_ArcaneSpark", "【M2 火焰原型】消耗全部燃烧，每层造成 3 伤害。",
        reward_eligible=True, mana_cost=2,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_ConflagratePlus",
        build_tags=("Archetype.Fire", "Element.Fire"),
    ),
    _card(
        "DA_Card_MageConflagratePlus", "Mage_ConflagratePlus", "引燃+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_DAMAGE, 0,
                 status=unreal.FantasyCombatStatus.BURNING, multiplier=4),),
        "T_Card_ArcaneSpark", "【M2 火焰升级】消耗全部燃烧，每层造成 4 伤害。",
        mana_cost=2, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Fire", "Element.Fire"),
    ),
    _card(
        "DA_Card_MageFrostBolt", "Mage_FrostBolt", "寒霜箭",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.CHILL)),
        "T_Card_ArcaneSpark", "【M2 冰霜原型】造成 4 伤害并施加 2 层寒冷。",
        reward_eligible=True, mana_cost=1,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_FrostBoltPlus", build_tags=("Archetype.Frost",),
    ),
    _card(
        "DA_Card_MageFrostBoltPlus", "Mage_FrostBoltPlus", "寒霜箭+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.CHILL)),
        "T_Card_ArcaneSpark", "【M2 冰霜升级】造成 5 伤害并施加 3 层寒冷。",
        mana_cost=1, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Frost",),
    ),
    _card(
        "DA_Card_MageColdWard", "Mage_ColdWard", "冷雾护符",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.CHILL),
         _effect(unreal.FantasyCombatEffectType.BLOCK, 5,
                 unreal.FantasyCombatTarget.SELF)),
        "T_Card_KiteShieldGuard", "【M2 冰霜原型】施加 2 层寒冷并获得 5 格挡。",
        reward_eligible=True, mana_cost=2,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_ColdWardPlus", build_tags=("Archetype.Frost",),
    ),
    _card(
        "DA_Card_MageColdWardPlus", "Mage_ColdWardPlus", "冷雾护符+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.CHILL),
         _effect(unreal.FantasyCombatEffectType.BLOCK, 7,
                 unreal.FantasyCombatTarget.SELF)),
        "T_Card_KiteShieldGuard", "【M2 冰霜升级】施加 3 层寒冷并获得 7 格挡。",
        mana_cost=2, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Frost",),
    ),
    _card(
        "DA_Card_MageIceHarvest", "Mage_IceHarvest", "采冰",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_BLOCK, 0,
                 status=unreal.FantasyCombatStatus.CHILL, multiplier=3),),
        "T_Card_KiteShieldGuard", "【M2 冰霜原型】消耗全部寒冷，每层获得 3 格挡。",
        reward_eligible=True, mana_cost=1,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_IceHarvestPlus", build_tags=("Archetype.Frost",),
    ),
    _card(
        "DA_Card_MageIceHarvestPlus", "Mage_IceHarvestPlus", "采冰+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_BLOCK, 0,
                 status=unreal.FantasyCombatStatus.CHILL, multiplier=4),),
        "T_Card_KiteShieldGuard", "【M2 冰霜升级】消耗全部寒冷，每层获得 4 格挡。",
        mana_cost=1, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Frost",),
    ),
    _card(
        "DA_Card_MageManaThread", "Mage_ManaThread", "法力丝线",
        unreal.CardType.MANA,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 4,
                 unreal.FantasyCombatTarget.SELF),
         _effect(unreal.FantasyCombatEffectType.DRAW, 1,
                 unreal.FantasyCombatTarget.SELF)),
        "T_Card_ReadOpening", "【M2 奥术原型】获得 4 法力并抽 1 张牌。",
        reward_eligible=True, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_ManaThreadPlus", build_tags=("Archetype.Arcane",),
    ),
    _card(
        "DA_Card_MageManaThreadPlus", "Mage_ManaThreadPlus", "法力丝线+",
        unreal.CardType.MANA,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 5,
                 unreal.FantasyCombatTarget.SELF),
         _effect(unreal.FantasyCombatEffectType.DRAW, 1,
                 unreal.FantasyCombatTarget.SELF)),
        "T_Card_ReadOpening", "【M2 奥术升级】获得 5 法力并抽 1 张牌。",
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Arcane",),
    ),
    _card(
        "DA_Card_MageArcaneFocus", "Mage_ArcaneFocus", "奥术专注",
        unreal.CardType.ACTION,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 2,
                 unreal.FantasyCombatTarget.SELF),
         _effect(unreal.FantasyCombatEffectType.DRAW, 2,
                 unreal.FantasyCombatTarget.SELF)),
        "T_Card_ReadOpening", "【M2 奥术原型】获得 2 法力并抽 2 张牌。",
        reward_eligible=True, action_cost=1,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_ArcaneFocusPlus", build_tags=("Archetype.Arcane",),
    ),
    _card(
        "DA_Card_MageArcaneFocusPlus", "Mage_ArcaneFocusPlus", "奥术专注+",
        unreal.CardType.ACTION,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 3,
                 unreal.FantasyCombatTarget.SELF),
         _effect(unreal.FantasyCombatEffectType.DRAW, 2,
                 unreal.FantasyCombatTarget.SELF)),
        "T_Card_ReadOpening", "【M2 奥术升级】获得 3 法力并抽 2 张牌。",
        action_cost=1, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Arcane",),
    ),
    _card(
        "DA_Card_MageOverload", "Mage_Overload", "奥术过载",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE_PER_MANA, 4, multiplier=2),),
        "T_Card_ArcaneSpark", "【M2 奥术原型】造成 4 伤害，结算法力每点追加 2 伤害。",
        reward_eligible=True, mana_cost=2,
        profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE,
        upgrade_card_id="Mage_OverloadPlus", build_tags=("Archetype.Arcane",),
    ),
    _card(
        "DA_Card_MageOverloadPlus", "Mage_OverloadPlus", "奥术过载+",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE_PER_MANA, 5, multiplier=3),),
        "T_Card_ArcaneSpark", "【M2 奥术升级】造成 5 伤害，结算法力每点追加 3 伤害。",
        mana_cost=2, profession=unreal.FantasyPlayerProfession.MAGE,
        school=unreal.CardSchool.ARCANE, upgrade_level=1,
        build_tags=("Archetype.Arcane",),
    ),
)


def _mage_upgrade_pair(
    stem,
    display_name,
    archetype,
    card_type,
    base_effects,
    upgrade_effects,
    *,
    mana_cost=0,
    action_cost=0,
    artwork="T_Card_ArcaneSpark",
    extra_tags=(),
):
    card_id = f"Mage_{stem}"
    upgrade_id = f"{card_id}Plus"
    tags = (f"Archetype.{archetype}",) + tuple(extra_tags)
    return (
        _card(
            f"DA_Card_Mage{stem}", card_id, display_name, card_type,
            tuple(base_effects), artwork,
            f"【M3 {archetype} 构筑】十八层旅途核心牌。",
            reward_eligible=True, mana_cost=mana_cost, action_cost=action_cost,
            profession=unreal.FantasyPlayerProfession.MAGE,
            school=unreal.CardSchool.ARCANE,
            upgrade_card_id=upgrade_id, build_tags=tags,
        ),
        _card(
            f"DA_Card_Mage{stem}Plus", upgrade_id, f"{display_name}+", card_type,
            tuple(upgrade_effects), artwork,
            f"【M3 {archetype} 升级】十八层旅途唯一升级。",
            mana_cost=mana_cost, action_cost=action_cost,
            profession=unreal.FantasyPlayerProfession.MAGE,
            school=unreal.CardSchool.ARCANE, upgrade_level=1, build_tags=tags,
        ),
    )


M3_MAGE_CARD_SPECS = sum((
    _mage_upgrade_pair(
        "CinderWard", "余烬护壁", "Fire", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.BURNING),
         _effect(unreal.FantasyCombatEffectType.BLOCK, 6,
                 unreal.FantasyCombatTarget.SELF)),
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.BURNING),
         _effect(unreal.FantasyCombatEffectType.BLOCK, 8,
                 unreal.FantasyCombatTarget.SELF)),
        mana_cost=2, artwork="T_Card_KiteShieldGuard", extra_tags=("Element.Fire",),
    ),
    _mage_upgrade_pair(
        "InfernoPulse", "烈焰脉冲", "Fire", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.BURNING)),
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 8),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 4,
                 status=unreal.FantasyCombatStatus.BURNING)),
        mana_cost=3, extra_tags=("Element.Fire",),
    ),
    _mage_upgrade_pair(
        "PhoenixRite", "凤凰仪式", "Fire", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_DAMAGE, 0,
                 status=unreal.FantasyCombatStatus.BURNING, multiplier=3),
         _effect(unreal.FantasyCombatEffectType.HEAL, 6,
                 unreal.FantasyCombatTarget.SELF)),
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_DAMAGE, 0,
                 status=unreal.FantasyCombatStatus.BURNING, multiplier=4),
         _effect(unreal.FantasyCombatEffectType.HEAL, 8,
                 unreal.FantasyCombatTarget.SELF)),
        mana_cost=4, extra_tags=("Element.Fire",),
    ),
    _mage_upgrade_pair(
        "Snowblind", "雪盲", "Frost", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.CHILL),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                 status=unreal.FantasyCombatStatus.WEAK)),
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 4,
                 status=unreal.FantasyCombatStatus.CHILL),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.WEAK)),
        mana_cost=2,
    ),
    _mage_upgrade_pair(
        "FrozenRampart", "冻结壁垒", "Frost", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.CHILL),
         _effect(unreal.FantasyCombatEffectType.BLOCK, 10,
                 unreal.FantasyCombatTarget.SELF)),
        (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 3,
                 status=unreal.FantasyCombatStatus.CHILL),
         _effect(unreal.FantasyCombatEffectType.BLOCK, 13,
                 unreal.FantasyCombatTarget.SELF)),
        mana_cost=3, artwork="T_Card_KiteShieldGuard",
    ),
    _mage_upgrade_pair(
        "AbsoluteZero", "绝对零度", "Frost", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_DAMAGE, 0,
                 status=unreal.FantasyCombatStatus.CHILL, multiplier=2),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                 status=unreal.FantasyCombatStatus.WEAK)),
        (_effect(unreal.FantasyCombatEffectType.CONSUME_STATUS_FOR_DAMAGE, 0,
                 status=unreal.FantasyCombatStatus.CHILL, multiplier=3),
         _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                 status=unreal.FantasyCombatStatus.WEAK)),
        mana_cost=3,
    ),
    _mage_upgrade_pair(
        "ManaVault", "法力秘库", "Arcane", unreal.CardType.MANA,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 8,
                 unreal.FantasyCombatTarget.SELF),),
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 11,
                 unreal.FantasyCombatTarget.SELF),),
    ),
    _mage_upgrade_pair(
        "AstralDraw", "星界牵引", "Arcane", unreal.CardType.ACTION,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 2,
                 unreal.FantasyCombatTarget.SELF),
         _effect(unreal.FantasyCombatEffectType.DRAW, 3,
                 unreal.FantasyCombatTarget.SELF)),
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 3,
                 unreal.FantasyCombatTarget.SELF),
         _effect(unreal.FantasyCombatEffectType.DRAW, 4,
                 unreal.FantasyCombatTarget.SELF)),
        action_cost=1, artwork="T_Card_ReadOpening",
    ),
    _mage_upgrade_pair(
        "Starfall", "星陨术", "Arcane", unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE_PER_MANA, 8, multiplier=2),),
        (_effect(unreal.FantasyCombatEffectType.DAMAGE_PER_MANA, 10, multiplier=3),),
        mana_cost=4,
    ),
), ())


def _profession_reward_pair(profession_key, profession_enum, stem, display_name,
                            archetype, index, is_nun=False):
    card_id = f"{profession_key}_{stem}"
    upgrade_id = f"{card_id}Plus"
    tags = (f"Archetype.{profession_key}.{archetype}",)
    if not is_nun and archetype == "Precision":
        card_type = unreal.CardType.ATTACK
        base_effects = (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6 + index),)
        plus_effects = (_effect(unreal.FantasyCombatEffectType.DAMAGE, 9 + index),)
        artwork = "T_Card_LongSwordSlash"
        action_cost = mana_cost = 0
    elif not is_nun and archetype == "Agility":
        card_type = unreal.CardType.ACTION
        base_effects = (_effect(unreal.FantasyCombatEffectType.BLOCK, 4 + index,
                                unreal.FantasyCombatTarget.SELF),
                        _effect(unreal.FantasyCombatEffectType.DRAW, 1,
                                unreal.FantasyCombatTarget.SELF))
        plus_effects = (_effect(unreal.FantasyCombatEffectType.BLOCK, 7 + index,
                                unreal.FantasyCombatTarget.SELF),
                        _effect(unreal.FantasyCombatEffectType.DRAW, 1 + (index % 2),
                                unreal.FantasyCombatTarget.SELF))
        artwork = "T_Card_ReadOpening"
        action_cost, mana_cost = 1, 0
    elif not is_nun:
        card_type = unreal.CardType.ACTION
        base_effects = (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2 + index // 2,
                                status=unreal.FantasyCombatStatus.POISON),)
        plus_effects = (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 4 + index // 2,
                                status=unreal.FantasyCombatStatus.POISON),)
        artwork = "T_Card_ArcaneSpark"
        action_cost, mana_cost = 1, 0
    elif archetype == "Prayer":
        card_type = unreal.CardType.PRAYER
        base_effects = (_effect(unreal.FantasyCombatEffectType.BLOCK, 5 + index,
                                unreal.FantasyCombatTarget.SELF),
                        _effect(unreal.FantasyCombatEffectType.HEAL, 2 + index // 2,
                                unreal.FantasyCombatTarget.SELF))
        plus_effects = (_effect(unreal.FantasyCombatEffectType.BLOCK, 8 + index,
                                unreal.FantasyCombatTarget.SELF),
                        _effect(unreal.FantasyCombatEffectType.HEAL, 4 + index // 2,
                                unreal.FantasyCombatTarget.SELF))
        artwork = "T_Card_KnightsPrayer"
        action_cost = mana_cost = 0
    elif archetype == "Judgment":
        card_type = unreal.CardType.ATTACK
        base_effects = (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5 + index,
                                piercing=index >= 4),)
        plus_effects = (_effect(unreal.FantasyCombatEffectType.DAMAGE, 8 + index,
                                piercing=index >= 3),)
        artwork = "T_Card_LionheartJudgment"
        action_cost = mana_cost = 0
    else:
        card_type = unreal.CardType.SPELL
        base_effects = (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 3 + index // 2,
                                unreal.FantasyCombatTarget.SELF),
                        _effect(unreal.FantasyCombatEffectType.DRAW, 1,
                                unreal.FantasyCombatTarget.SELF))
        plus_effects = (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 5 + index // 2,
                                unreal.FantasyCombatTarget.SELF),
                        _effect(unreal.FantasyCombatEffectType.DRAW, 2,
                                unreal.FantasyCombatTarget.SELF))
        artwork = "T_Card_ArcaneSpark"
        action_cost, mana_cost = 0, 1
    return (
        _card(f"DA_Card_{profession_key}{stem}", card_id, display_name, card_type,
              base_effects, artwork, f"【M5 {archetype} 构筑】{profession_key}职业奖励牌。",
              reward_eligible=True, action_cost=action_cost, mana_cost=mana_cost,
              profession=profession_enum, upgrade_card_id=upgrade_id, build_tags=tags),
        _card(f"DA_Card_{profession_key}{stem}Plus", upgrade_id, f"{display_name}+", card_type,
              plus_effects, artwork, f"【M5 {archetype} 升级】唯一一级升级。",
              action_cost=action_cost, mana_cost=mana_cost, profession=profession_enum,
              upgrade_level=1, build_tags=tags),
    )


RANGER_CARD_SPECS = (
    _card("DA_Card_RangerNormalAttack", "Ranger_NormalAttack", "短弓射击", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 8),), "T_Card_LongSwordSlash",
          "【M5 游侠初始牌】稳定射击。", copies=4, profession=unreal.FantasyPlayerProfession.RANGER),
    _card("DA_Card_RangerQuickShot", "Ranger_QuickShot", "迅捷射击", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),
           _effect(unreal.FantasyCombatEffectType.DRAW, 1, unreal.FantasyCombatTarget.SELF)),
          "T_Card_LongSwordSlash", "【M5 游侠初始牌】攻击并抽牌。", copies=2,
          profession=unreal.FantasyPlayerProfession.RANGER),
    _card("DA_Card_RangerFocus", "Ranger_Focus", "猎人专注", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.DRAW, 2, unreal.FantasyCombatTarget.SELF),),
          "T_Card_ReadOpening", "【M5 游侠初始牌】整理手牌。", copies=1, action_cost=1,
          profession=unreal.FantasyPlayerProfession.RANGER),
    _card("DA_Card_RangerLongbow", "Ranger_Longbow", "长弓", unreal.CardType.EQUIPMENT, (),
          "T_Card_LongSwordSlash", "【M5 游侠初始牌】攻击牌伤害 +2。", copies=1,
          equipment_attack=3, profession=unreal.FantasyPlayerProfession.RANGER),
    _card("DA_Card_RangerDodge", "Ranger_Dodge", "闪避", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.BLOCK, 10, unreal.FantasyCombatTarget.SELF),),
          "T_Card_KiteShieldGuard", "【M5 游侠初始牌】获得格挡。", copies=1, action_cost=1,
          profession=unreal.FantasyPlayerProfession.RANGER),
    _card("DA_Card_RangerVenomTip", "Ranger_VenomTip", "淬毒箭", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 5,
                   status=unreal.FantasyCombatStatus.POISON),),
          "T_Card_ArcaneSpark", "【M5 游侠初始牌】施加中毒。", copies=1, action_cost=1,
          profession=unreal.FantasyPlayerProfession.RANGER),
)

NUN_CARD_SPECS = (
    _card("DA_Card_NunNormalAttack", "Nun_NormalAttack", "圣杖轻击", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),), "T_Card_LongSwordSlash",
          "【M5 修女初始牌】稳定攻击。", copies=3, profession=unreal.FantasyPlayerProfession.NUN),
    _card("DA_Card_NunPrayer", "Nun_Prayer", "晨祷", unreal.CardType.PRAYER,
          (_effect(unreal.FantasyCombatEffectType.BLOCK, 5, unreal.FantasyCombatTarget.SELF),),
          "T_Card_KnightsPrayer", "【M5 修女初始牌】获得格挡。", copies=2,
          profession=unreal.FantasyPlayerProfession.NUN),
    _card("DA_Card_NunFocus", "Nun_Focus", "静思", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.DRAW, 2, unreal.FantasyCombatTarget.SELF),),
          "T_Card_ReadOpening", "【M5 修女初始牌】抽取两张牌。", copies=1, action_cost=1,
          profession=unreal.FantasyPlayerProfession.NUN),
    _card("DA_Card_NunHolyLight", "Nun_HolyLight", "圣光", unreal.CardType.SPELL,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 7),), "T_Card_ArcaneSpark",
          "【M5 修女初始牌】神圣伤害。", copies=1, mana_cost=1,
          profession=unreal.FantasyPlayerProfession.NUN),
    _card("DA_Card_NunRosary", "Nun_Rosary", "念珠", unreal.CardType.MANA,
          (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 5, unreal.FantasyCombatTarget.SELF),),
          "T_Card_ArcaneSpark", "【M5 修女初始牌】获得法力。", copies=1,
          profession=unreal.FantasyPlayerProfession.NUN),
    _card("DA_Card_NunShelter", "Nun_Shelter", "庇护", unreal.CardType.PRAYER,
          (_effect(unreal.FantasyCombatEffectType.BLOCK, 8, unreal.FantasyCombatTarget.SELF),
           _effect(unreal.FantasyCombatEffectType.HEAL, 3, unreal.FantasyCombatTarget.SELF)),
          "T_Card_KnightsPrayer", "【M5 修女初始牌】防护并治疗。", copies=2,
          profession=unreal.FantasyPlayerProfession.NUN),
)

RANGER_REWARD_NAMES = {
    "Precision": ("鹰眼", "穿云箭", "弱点标记", "连珠箭", "狙击", "风切", "猎杀时刻", "终焉箭"),
    "Agility": ("轻足", "翻滚", "乘风", "备用箭袋", "疾跑", "侧身闪", "叶影", "无踪"),
    "Poison": ("毒藤箭", "蛇毒", "孢子囊", "腐蚀箭", "毒雾", "蝎尾", "剧毒爆发", "百毒归一"),
}
NUN_REWARD_NAMES = {
    "Prayer": ("晚祷", "守夜", "圣歌", "恩典", "静默礼拜", "群星祷文", "赦免", "永恒庇护"),
    "Judgment": ("戒律", "惩戒", "圣印", "破邪", "裁决之光", "审判钟", "净罪", "末日审判"),
    "Devotion": ("虔诚", "奉献", "圣泉", "启示", "神恩", "信仰回响", "灵魂共鸣", "神迹"),
}
RANGER_REWARD_CARD_SPECS = sum((
    _profession_reward_pair("Ranger", unreal.FantasyPlayerProfession.RANGER,
                            f"{archetype}{index + 1}", name, archetype, index)
    for archetype, names in RANGER_REWARD_NAMES.items()
    for index, name in enumerate(names)
), ())
NUN_REWARD_CARD_SPECS = sum((
    _profession_reward_pair("Nun", unreal.FantasyPlayerProfession.NUN,
                            f"{archetype}{index + 1}", name, archetype, index, True)
    for archetype, names in NUN_REWARD_NAMES.items()
    for index, name in enumerate(names)
), ())


BLESSING_SPECS = (
    ("IronWill", "钢铁意志", "每场战斗开始时获得 8 格挡。", None,
     unreal.FantasyBlessingTrigger.BATTLE_STARTED,
     (_effect(unreal.FantasyCombatEffectType.BLOCK, 8,
              unreal.FantasyCombatTarget.SELF),), "", 1, 85),
    ("QuickHands", "迅捷之手", "每场战斗开始时额外抽 1 张牌。", None,
     unreal.FantasyBlessingTrigger.BATTLE_STARTED,
     (_effect(unreal.FantasyCombatEffectType.DRAW, 1,
              unreal.FantasyCombatTarget.SELF),), "", 1, 95),
    ("SecondWind", "第二阵风", "每场战斗开始时恢复 6 点生命。", None,
     unreal.FantasyBlessingTrigger.BATTLE_STARTED,
     (_effect(unreal.FantasyCombatEffectType.HEAL, 6,
              unreal.FantasyCombatTarget.SELF),), "", 1, 80),
    ("ThornWard", "荆棘守护", "每场战斗前三次受到生命伤害时反击 2 点。", None,
     unreal.FantasyBlessingTrigger.PLAYER_DAMAGED,
     (_effect(unreal.FantasyCombatEffectType.DAMAGE, 2),), "", 3, 90),
    ("BattleRhythm", "战斗节奏", "每个玩家回合额外获得 1 行动力。", None,
     unreal.FantasyBlessingTrigger.PLAYER_TURN_STARTED,
     (_effect(unreal.FantasyCombatEffectType.GAIN_ACTION, 1,
              unreal.FantasyCombatTarget.SELF),), "", 0, 100),
    ("MercyOfRoad", "旅途仁慈", "跳过战利品时恢复 5 点生命。", None,
     unreal.FantasyBlessingTrigger.REWARD_SKIPPED,
     (_effect(unreal.FantasyCombatEffectType.HEAL, 5,
              unreal.FantasyCombatTarget.SELF),), "", 0, 75),
    ("MageKindling", "余烬火种", "每次打出火焰牌额外施加 1 层燃烧。", "Mage",
     unreal.FantasyBlessingTrigger.PLAYER_CARD_RESOLVED,
     (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
              status=unreal.FantasyCombatStatus.BURNING),), "Element.Fire", 6, 100),
    ("MageWinterVeil", "冬幕", "每次打出冰霜牌获得 3 格挡。", "Mage",
     unreal.FantasyBlessingTrigger.PLAYER_CARD_RESOLVED,
     (_effect(unreal.FantasyCombatEffectType.BLOCK, 3,
              unreal.FantasyCombatTarget.SELF),), "Archetype.Frost", 6, 100),
    ("MageArcaneReserve", "奥术储备", "每场战斗开始时获得 5 法力。", "Mage",
     unreal.FantasyBlessingTrigger.BATTLE_STARTED,
     (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 5,
              unreal.FantasyCombatTarget.SELF),), "", 1, 100),
    ("MageLivingSpellbook", "活体法典", "每个玩家回合额外抽 1 张牌。", "Mage",
     unreal.FantasyBlessingTrigger.PLAYER_TURN_STARTED,
     (_effect(unreal.FantasyCombatEffectType.DRAW, 1,
              unreal.FantasyCombatTarget.SELF),), "", 0, 110),
    ("MageColdSnap", "骤寒", "每场战斗开始时施加 2 层寒冷。", "Mage",
     unreal.FantasyBlessingTrigger.BATTLE_STARTED,
     (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
              status=unreal.FantasyCombatStatus.CHILL),), "", 1, 90),
    ("MageManaWard", "法力护幕", "每个敌方回合开始时获得 4 格挡。", "Mage",
     unreal.FantasyBlessingTrigger.ENEMY_TURN_STARTED,
     (_effect(unreal.FantasyCombatEffectType.BLOCK, 4,
              unreal.FantasyCombatTarget.SELF),), "", 0, 105),
)


ENEMY_CARD_SPECS = (
    _card(
        "DA_EnemyCard_ClawStrike", "Enemy_ClawStrike", "爪击",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),),
        "T_Card_LongSwordSlash", _enemy_description("基础攻击。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_Bash", "Enemy_Bash", "猛击",
        unreal.CardType.ACTION,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 4),
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                4,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _enemy_description("攻击并格挡。"),
        card_set_id="W01_Enemy", action_cost=1,
    ),
    _card(
        "DA_EnemyCard_LifeSteal", "Enemy_LifeSteal", "吸血",
        unreal.CardType.ACTION,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 3),
            _effect(
                unreal.FantasyCombatEffectType.HEAL,
                3,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KnightsPrayer", _enemy_description("伤害并恢复生命。"),
        card_set_id="W01_Enemy", action_cost=1,
    ),
    _card(
        "DA_EnemyCard_Mana", "Enemy_Mana", "法力",
        unreal.CardType.MANA,
        (
            _effect(
                unreal.FantasyCombatEffectType.GAIN_MANA,
                6,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_ArcaneSpark", _enemy_description("积累法力。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_Wisdom", "Enemy_Wisdom", "智慧",
        unreal.CardType.MANA,
        (
            _effect(
                unreal.FantasyCombatEffectType.GAIN_MANA,
                5,
                unreal.FantasyCombatTarget.SELF,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_ReadOpening", _enemy_description("积累法力并抽牌。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_ElementalWave", "Enemy_ElementalWave", "元素波动",
        unreal.CardType.SPELL,
        # BWIKI's preserved numeric fragment gives 2 lightning damage.
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 2),),
        "T_Card_ArcaneSpark", _enemy_description("释放元素伤害。"),
        card_set_id="W01_Enemy", mana_cost=2,
    ),
    _card(
        "DA_EnemyCard_FireBlast", "Enemy_FireBlast", "火焰冲击",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 10),),
        "T_Card_ArcaneSpark", _enemy_description("高法力火焰伤害。"),
        card_set_id="W01_Enemy", mana_cost=4,
    ),
    _card(
        "DA_EnemyCard_Rush", "Enemy_Rush", "急行",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_ReadOpening", _enemy_description("消耗行动力并抽取一张牌。"),
        card_set_id="W01_Enemy", action_cost=1,
    ),
    _card(
        "DA_EnemyCard_NoEntry", "Enemy_NoEntry", "禁止通行！",
        unreal.CardType.COUNTER,
        (
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                7,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _enemy_description("建立防线。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_ShortSword", "Enemy_ShortSword", "短剑",
        unreal.CardType.EQUIPMENT,
        (),
        "T_Card_LongSwordSlash", _enemy_description("装备后强化攻击。"),
        card_set_id="W01_Enemy", equipment_attack=1,
    ),
    _card(
        "DA_EnemyCard_SwiftAttack", "Enemy_SwiftAttack", "迅捷攻击",
        unreal.CardType.ATTACK,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 5),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_LongSwordSlash", _enemy_description("攻击并抽牌。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_Flinch", "Enemy_Flinch", "退缩",
        unreal.CardType.COUNTER,
        (
            _effect(
                unreal.FantasyCombatEffectType.BLOCK,
                5,
                unreal.FantasyCombatTarget.SELF,
            ),
            _effect(
                unreal.FantasyCombatEffectType.DRAW,
                1,
                unreal.FantasyCombatTarget.SELF,
            ),
        ),
        "T_Card_KiteShieldGuard", _enemy_description("防守并补牌。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_Hypnosis", "Enemy_Hypnosis", "催眠",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.DISCARD_RANDOM,
                1,
                unreal.FantasyCombatTarget.OPPONENT,
            ),
            _effect(
                unreal.FantasyCombatEffectType.APPLY_STATUS,
                1,
                unreal.FantasyCombatTarget.OPPONENT,
                unreal.FantasyCombatStatus.WEAK,
            ),
        ),
        "T_Card_ReadOpening", _enemy_description("扰乱手牌并施加虚弱。"),
        card_set_id="W01_Enemy", action_cost=1,
    ),
    _card(
        "DA_EnemyCard_AcidSpray", "Enemy_AcidSpray", "酸性喷雾",
        unreal.CardType.ACTION,
        (
            _effect(
                unreal.FantasyCombatEffectType.APPLY_STATUS,
                4,
                unreal.FantasyCombatTarget.OPPONENT,
                unreal.FantasyCombatStatus.POISON,
            ),
        ),
        "T_Card_ArcaneSpark", _enemy_description("施加中毒。"),
        card_set_id="W01_Enemy", action_cost=1,
    ),
    _card(
        "DA_EnemyCard_Repentance", "Enemy_Repentance", "忏悔",
        unreal.CardType.SPELL,
        (_effect(
            unreal.FantasyCombatEffectType.DAMAGE,
            4,
            piercing=True,
            scales_with_strength=True,
        ),),
        "T_Card_LionheartJudgment", _enemy_description("造成受力量加成的穿刺伤害。"),
        card_set_id="W01_Enemy", mana_cost=2,
    ),
    _card(
        "DA_EnemyCard_FireSeed", "Enemy_FireSeed", "火苗",
        unreal.CardType.ATTACK,
        (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),),
        "T_Card_ArcaneSpark",
        _enemy_description("公开索引保留 2、2 点火属性片段；当前合并为 4 点伤害。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_ManaTotem", "Enemy_ManaTotem", "法力图腾",
        unreal.CardType.MANA,
        (_effect(unreal.FantasyCombatEffectType.GAIN_MANA, 4, unreal.FantasyCombatTarget.SELF),),
        "T_Card_ArcaneSpark",
        _enemy_description("当前作为一次性 4 法力来源；图腾持续规则待图鉴核验。"),
        card_set_id="W01_Enemy",
    ),
    _card(
        "DA_EnemyCard_Heal", "Enemy_Heal", "治愈",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.HEAL, 7, unreal.FantasyCombatTarget.SELF),),
        "T_Card_KnightsPrayer",
        _enemy_description("消耗法力恢复 7 点生命。"),
        card_set_id="W01_Enemy", mana_cost=1,
    ),
    _card(
        "DA_EnemyCard_WindStone", "Enemy_WindStone", "风之石",
        unreal.CardType.SPELL,
        (
            _effect(unreal.FantasyCombatEffectType.DAMAGE, 4),
            _effect(unreal.FantasyCombatEffectType.DRAW, 1, unreal.FantasyCombatTarget.SELF),
        ),
        "T_Card_ReadOpening",
        _enemy_description("消耗法力造成伤害并抽牌。"),
        card_set_id="W01_Enemy", mana_cost=1,
    ),
    _card(
        "DA_EnemyCard_CrystalBall", "Enemy_CrystalBall", "水晶球",
        unreal.CardType.SPELL,
        (_effect(unreal.FantasyCombatEffectType.DRAW, 1, unreal.FantasyCombatTarget.SELF),),
        "T_Card_ReadOpening",
        _enemy_description("原牌含复制语义；当前只保留抽牌，复制机制后续补齐。"),
        card_set_id="W01_Enemy", mana_cost=2,
    ),
)

M2_ENEMY_CARD_SPECS = (
    _card("DA_EnemyCard_WolfBite", "Enemy_WolfBite", "狼咬", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),),
          "T_Card_LongSwordSlash", _enemy_description("稳定撕咬。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_WolfMaul", "Enemy_WolfMaul", "扑杀", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 7),),
          "T_Card_LongSwordSlash", _enemy_description("高伤害扑击。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_SpiderBite", "Enemy_SpiderBite", "毒牙", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 3),
           _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                   status=unreal.FantasyCombatStatus.POISON)),
          "T_Card_ArcaneSpark", _enemy_description("伤害并施加中毒。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_Venom", "Enemy_Venom", "吐毒", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 2,
                   status=unreal.FantasyCombatStatus.POISON),),
          "T_Card_ArcaneSpark", _enemy_description("叠加中毒。"), card_set_id="W01_Enemy", action_cost=1),
    _card("DA_EnemyCard_TreantSlam", "Enemy_TreantSlam", "枝干横扫", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),),
          "T_Card_LongSwordSlash", _enemy_description("以枝干攻击。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_Bark", "Enemy_Bark", "树皮", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.BLOCK, 4,
                   unreal.FantasyCombatTarget.SELF),),
          "T_Card_KiteShieldGuard", _enemy_description("获得格挡。"), card_set_id="W01_Enemy", action_cost=1),
    _card("DA_EnemyCard_DrunkLow", "Enemy_DrunkLow", "踉跄挥击", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 3),),
          "T_Card_LongSwordSlash", _enemy_description("低伤害波动牌。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_DrunkHigh", "Enemy_DrunkHigh", "酒瓶重砸", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 9),),
          "T_Card_LongSwordSlash", _enemy_description("高伤害波动牌。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_DrunkSpill", "Enemy_DrunkSpill", "酒液飞溅", unreal.CardType.ACTION,
          (_effect(unreal.FantasyCombatEffectType.DISCARD_RANDOM, 1,
                   unreal.FantasyCombatTarget.SELF),
           _effect(unreal.FantasyCombatEffectType.DISCARD_RANDOM, 1,
                   unreal.FantasyCombatTarget.OPPONENT)),
          "T_Card_ReadOpening", _enemy_description("双方各随机弃一张牌。"), card_set_id="W01_Enemy", action_cost=1),
    _card("DA_EnemyCard_HunterLongbow", "Enemy_HunterLongbow", "长弓", unreal.CardType.EQUIPMENT,
          (), "T_Card_LongSwordSlash", _enemy_description("装备后攻击 +2。"),
          card_set_id="W01_Enemy", equipment_attack=2),
    _card("DA_EnemyCard_HunterShot", "Enemy_HunterShot", "猎手射击", unreal.CardType.ATTACK,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),),
          "T_Card_LongSwordSlash", _enemy_description("受到长弓加成的攻击。"), card_set_id="W01_Enemy"),
    _card("DA_EnemyCard_WitchHex", "Enemy_WitchHex", "塞入诅咒", unreal.CardType.SPELL,
          (_effect(unreal.FantasyCombatEffectType.ADD_TEMPORARY_CARD, 1,
                   payload_id="Mage_HexCurse", limit=3),),
          "T_Card_ArcaneSpark", _enemy_description("向玩家弃牌堆塞入临时诅咒，每战最多 3 张。"),
          card_set_id="W01_Enemy", mana_cost=1),
    _card("DA_EnemyCard_WitchDrain", "Enemy_WitchDrain", "法力抽离", unreal.CardType.SPELL,
          (_effect(unreal.FantasyCombatEffectType.LOSE_MANA, 2),),
          "T_Card_ArcaneSpark", _enemy_description("使玩家失去 2 法力。"),
          card_set_id="W01_Enemy", mana_cost=1),
    _card("DA_EnemyCard_WitchBolt", "Enemy_WitchBolt", "巫术飞弹", unreal.CardType.SPELL,
          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),),
          "T_Card_ArcaneSpark", _enemy_description("基础巫术伤害。"),
          card_set_id="W01_Enemy", mana_cost=1),
    _card("DA_Card_MageHexCurse", "Mage_HexCurse", "枯萎诅咒", unreal.CardType.SPECIAL,
          (_effect(unreal.FantasyCombatEffectType.LOSE_MANA, 1,
                   unreal.FantasyCombatTarget.SELF),),
          "T_Card_ArcaneSpark", "【战斗临时牌】打出时失去 1 法力，随后消耗。",
          profession=unreal.FantasyPlayerProfession.MAGE, exhaust=True,
          build_tags=("Temporary.Curse",)),
)


CARD_SPECS = (
    PLAYER_CARD_SPECS
    + PLAYER_REWARD_CARD_SPECS
    + MAGE_CARD_SPECS
    + MAGE_REWARD_CARD_SPECS
	+ M2_MAGE_CARD_SPECS
	+ M3_MAGE_CARD_SPECS
	+ RANGER_CARD_SPECS
	+ RANGER_REWARD_CARD_SPECS
	+ NUN_CARD_SPECS
	+ NUN_REWARD_CARD_SPECS
    + ENEMY_CARD_SPECS
    + M2_ENEMY_CARD_SPECS
)


def _card_contract(spec):
    """Derive stable M1 metadata without duplicating it across 52 card specs."""
    is_enemy = spec.get("card_set_id") == "W01_Enemy"
    is_starter = spec.get("copies", 0) > 0
    if is_enemy:
        rarity = unreal.FantasyCardRarity.ENEMY
        owner_tag = "Owner.Enemy"
    elif is_starter:
        rarity = unreal.FantasyCardRarity.STARTER
        owner_tag = f"Profession.{str(spec['profession']).split('.')[-1].title()}"
    else:
        rarity = (
            unreal.FantasyCardRarity.COMMON
            if spec.get("reward_eligible", False)
            else unreal.FantasyCardRarity.UNCOMMON
        )
        owner_tag = f"Profession.{str(spec['profession']).split('.')[-1].title()}"

    card_type_tags = {
        unreal.CardType.ATTACK: "Type.Attack",
        unreal.CardType.SKILL: "Type.Skill",
        unreal.CardType.ACTION: "Type.Action",
        unreal.CardType.SPELL: "Type.Spell",
        unreal.CardType.OATH: "Type.Oath",
        unreal.CardType.MANA: "Type.Mana",
        unreal.CardType.EQUIPMENT: "Type.Equipment",
        unreal.CardType.COUNTER: "Type.Counter",
        unreal.CardType.PRAYER: "Type.Prayer",
        unreal.CardType.SPECIAL: "Type.Special",
    }
    effect_tags = {
        unreal.FantasyCombatEffectType.DAMAGE: "Effect.Damage",
        unreal.FantasyCombatEffectType.BLOCK: "Effect.Block",
        unreal.FantasyCombatEffectType.DRAW: "Effect.Draw",
        unreal.FantasyCombatEffectType.HEAL: "Effect.Heal",
        unreal.FantasyCombatEffectType.GAIN_MANA: "Effect.Mana",
        unreal.FantasyCombatEffectType.APPLY_STATUS: "Effect.Status",
        unreal.FantasyCombatEffectType.DISCARD_RANDOM: "Effect.Discard",
    }
    school_tags = {
        unreal.CardSchool.NONE: "School.Neutral",
        unreal.CardSchool.STEEL: "School.Steel",
        unreal.CardSchool.FAITH: "School.Faith",
        unreal.CardSchool.ARCANE: "School.Arcane",
    }
    tags = [
        owner_tag,
        card_type_tags[spec["card_type"]],
        school_tags[spec["school"]],
    ]
    tags.extend(
        effect_tags[effect["effect_type"]]
        for effect in spec["effects"]
        if effect["effect_type"] in effect_tags
    )
    tags.extend(spec.get("build_tags", ()))
    tags.append(
        "Pool.Starter"
        if is_starter
        else "Pool.Reward" if spec.get("reward_eligible", False) else "Pool.Reserved"
    )
    return {
        "rarity": rarity,
        "build_tags": tuple(dict.fromkeys(tags)),
        "upgrade_level": spec.get("upgrade_level", 0),
        "upgrade_card_id": spec.get("upgrade_card_id"),
    }


def _fallback_intent(intent_id, display_name, effects):
    return {
        "intent_id": intent_id,
        "display_name": display_name,
        "effects": tuple(effects),
    }


def _mechanic(
    mechanic_id,
    trigger,
    effects=(),
    *,
    required_tag="",
    min_actual_damage=0,
    max_target_block=-1,
    limit=unreal.FantasyMechanicLimit.UNLIMITED,
    suppress_target="",
    suppress_turns=0,
):
    return {
        "mechanic_id": mechanic_id,
        "trigger": trigger,
        "effects": tuple(effects),
        "required_tag": required_tag,
        "min_actual_damage": min_actual_damage,
        "max_target_block": max_target_block,
        "limit": limit,
        "suppress_target": suppress_target,
        "suppress_turns": suppress_turns,
    }


def _enemy(
    asset_name,
    enemy_id,
    display_name,
    max_health,
    deck,
    visual_profile,
    passive_id,
    passive_name,
    passive_rule,
    fallback_intents,
    *,
    max_hand=4,
    max_action=2,
    starting_mana=0,
    cards_per_turn=2,
    boss=False,
    mechanics=(),
):
    return {
        "asset_name": asset_name,
        "enemy_id": enemy_id,
        "display_name": display_name,
        "max_health": max_health,
        "deck": tuple(deck),
        "max_hand_size": max_hand,
        "max_action_points": max_action,
        "starting_mana": starting_mana,
        "cards_per_turn": cards_per_turn,
        "passive_id": passive_id,
        "passive_name": passive_name,
        "passive_description": f"{passive_rule} {ENEMY_PROTOTYPE_BOUNDARY}",
        "visual_profile": visual_profile,
        "boss": boss,
        "intents": tuple(fallback_intents),
        "mechanics": tuple(mechanics),
    }


ENEMY_SPECS = (
    _enemy(
        "DA_Enemy_DrowsyBat", "DrowsyBat", "贪睡蝙蝠", 32,
        (("Enemy_ClawStrike", 3), ("Enemy_Bash", 2), ("Enemy_LifeSteal", 2)),
        unreal.FantasyEnemyVisualProfile.BAT,
        "Drowsy", "贪睡", "首次受到生命伤害后苏醒并获得 1 力量。",
        (
            _fallback_intent(
                "FallbackClaw", "爪击（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),),
            ),
        ),
        max_hand=3, max_action=2, starting_mana=0, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_MagicApprentice", "MagicApprentice", "魔法学徒", 38,
        (
            ("Enemy_Mana", 2), ("Enemy_Wisdom", 2),
            ("Enemy_ElementalWave", 2), ("Enemy_FireBlast", 2),
        ),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "ApprenticeWisdom", "智慧", "通过【法力】与【智慧】积累法力，再释放高费用法术。",
        (
            _fallback_intent(
                "FallbackFire", "火焰冲击（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 10),),
            ),
        ),
        max_hand=4, max_action=1, starting_mana=0, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_VillageGuard", "VillageGuard", "村庄守卫", 46,
        (
            ("Enemy_Rush", 2), ("Enemy_NoEntry", 2),
            ("Enemy_ShortSword", 1), ("Enemy_SwiftAttack", 3),
        ),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "GuardPost", "守关", "战斗开始时获得 6 格挡；装备与防线会强化后续攻击。",
        (
            _fallback_intent(
                "FallbackGuard", "守关斩（兼容意图）",
                (
                    _effect(unreal.FantasyCombatEffectType.DAMAGE, 6),
                    _effect(
                        unreal.FantasyCombatEffectType.BLOCK,
                        4,
                        unreal.FantasyCombatTarget.SELF,
                    ),
                ),
            ),
        ),
        max_hand=4, max_action=2, starting_mana=0, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_Hypnotist", "Hypnotist", "催眠师", 44,
        (("Enemy_Flinch", 2), ("Enemy_Hypnosis", 2), ("Enemy_AcidSpray", 2)),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "Mesmerize", "催眠", "以弃牌、虚弱和中毒干扰玩家回合。",
        (
            _fallback_intent(
                "FallbackHypnosis", "催眠（兼容意图）",
                (
                    _effect(
                        unreal.FantasyCombatEffectType.APPLY_STATUS,
                        1,
                        unreal.FantasyCombatTarget.OPPONENT,
                        unreal.FantasyCombatStatus.WEAK,
                    ),
                ),
            ),
        ),
        max_hand=3, max_action=1, starting_mana=0, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_DragonWhelp", "DragonWhelp", "飞龙幼崽", 66,
        (("Enemy_Mana", 2), ("Enemy_FireBlast", 3), ("Enemy_ElementalWave", 2)),
        unreal.FantasyEnemyVisualProfile.DRAGON,
        "DragonScale", "龙鳞", "战斗开始时获得 8 格挡。",
        (
            _fallback_intent(
                "FallbackBreath", "火焰吐息（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 10),),
            ),
        ),
        max_hand=4, max_action=1, starting_mana=2, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_HeadlessKnight", "HeadlessKnight", "无头骑士", 72,
        (("Enemy_Mana", 2), ("Enemy_FireBlast", 3), ("Enemy_Repentance", 3)),
        unreal.FantasyEnemyVisualProfile.SKELETON,
        "RevivalOath", "复生执念", "首次死亡时以半数生命复生，并获得 10 格挡与 2 力量。",
        (
            _fallback_intent(
                "FallbackRepentance", "忏悔斩（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4, piercing=True),),
            ),
        ),
        max_hand=4, max_action=1, starting_mana=2, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_Scarecrow", "Scarecrow", "稻草人", 55,
        (
            ("Enemy_Mana", 2), ("Enemy_FireBlast", 2),
            ("Enemy_ElementalWave", 2), ("Enemy_FireSeed", 2),
            ("Enemy_ManaTotem", 1),
        ),
        unreal.FantasyEnemyVisualProfile.SLIME,
        "", "", "以法力和火焰牌推进战斗。",
        (
            _fallback_intent(
                "FallbackFireSeed", "火苗（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),),
            ),
        ),
        max_hand=4, max_action=1, starting_mana=0, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_FortuneTeller", "FortuneTeller", "女占卜师", 58,
        (
            ("Enemy_Wisdom", 2), ("Enemy_ElementalWave", 2),
            ("Enemy_Heal", 2), ("Enemy_WindStone", 2),
            ("Enemy_CrystalBall", 1),
        ),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "FortuneWisdom", "智慧", "积累法力、抽牌并以治愈维持战线。",
        (
            _fallback_intent(
                "FallbackWindStone", "风之石（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4),),
            ),
        ),
        max_hand=4, max_action=1, starting_mana=0, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_ScarecrowElite", "ScarecrowElite", "稻草人 · 精英", 86,
        (
            ("Enemy_Mana", 3), ("Enemy_FireBlast", 3),
            ("Enemy_ElementalWave", 2), ("Enemy_FireSeed", 3),
            ("Enemy_ManaTotem", 1),
        ),
        unreal.FantasyEnemyVisualProfile.SLIME,
        "AshenKindling", "余烬", "章节后段版本；牌名集合不变，生命和份数为项目调参。",
        (
            _fallback_intent(
                "FallbackEliteFire", "余烬火苗（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),),
            ),
        ),
        max_hand=5, max_action=1, starting_mana=2, cards_per_turn=3,
    ),
    _enemy(
        "DA_Enemy_FortuneTellerElite", "FortuneTellerElite", "女占卜师 · 精英", 90,
        (
            ("Enemy_Wisdom", 3), ("Enemy_ElementalWave", 2),
            ("Enemy_Heal", 2), ("Enemy_WindStone", 3),
            ("Enemy_CrystalBall", 2),
        ),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "DeepFortune", "深层预言", "章节后段版本；牌名集合不变，生命和份数为项目调参。",
        (
            _fallback_intent(
                "FallbackEliteWind", "预言风石（兼容意图）",
                (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),),
            ),
        ),
        max_hand=5, max_action=1, starting_mana=3, cards_per_turn=3,
    ),
    _enemy(
        "DA_Enemy_HeadlessKnightBoss", "HeadlessKnightBoss", "无头骑士 · 守关者", 108,
        (("Enemy_Mana", 3), ("Enemy_FireBlast", 4), ("Enemy_Repentance", 4)),
        unreal.FantasyEnemyVisualProfile.SKELETON,
        "AshenRevival", "灰烬复生", "首次死亡时以半数生命复生，并获得 10 格挡与 2 力量。",
        (
            _fallback_intent(
                "FallbackBossCleave", "灰烬重斩（兼容意图）",
                (
                    _effect(unreal.FantasyCombatEffectType.DAMAGE, 4, piercing=True),
                    _effect(
                        unreal.FantasyCombatEffectType.BLOCK,
                        6,
                        unreal.FantasyCombatTarget.SELF,
                    ),
                ),
            ),
        ),
        max_hand=5, max_action=2, starting_mana=4, cards_per_turn=3, boss=True,
    ),
)

M2_ENEMY_SPECS = (
    _enemy(
        "DA_Enemy_ForestWolf", "ForestWolf", "森林狼", 36,
        (("Enemy_WolfBite", 3), ("Enemy_WolfMaul", 2)),
        unreal.FantasyEnemyVisualProfile.BAT,
        "ForestWolf.Pursuit", "追击", "攻击实际造成生命伤害且攻击前玩家格挡不高于 2 时，每回合首次追击 3 点。",
        (_fallback_intent("FallbackWolf", "狼咬（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),)),),
        max_hand=3, max_action=1, cards_per_turn=1,
        mechanics=(_mechanic(
            "ForestWolf.Pursuit", unreal.FantasyMechanicTrigger.ENEMY_ATTACK_RESOLVED,
            (_effect(unreal.FantasyCombatEffectType.DAMAGE, 3),),
            min_actual_damage=1, max_target_block=2,
            limit=unreal.FantasyMechanicLimit.ONCE_PER_TURN,
        ),),
    ),
    _enemy(
        "DA_Enemy_PoisonSpider", "PoisonSpider", "毒蜘蛛", 34,
        (("Enemy_SpiderBite", 3), ("Enemy_Venom", 2)),
        unreal.FantasyEnemyVisualProfile.SLIME,
        "PoisonSpider.Venom", "毒腺", "通过卡牌效果持续施加中毒。",
        (_fallback_intent("FallbackVenom", "毒牙（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                                   status=unreal.FantasyCombatStatus.POISON),)),),
        max_hand=3, max_action=1, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_Treant", "Treant", "树精", 54,
        (("Enemy_TreantSlam", 3), ("Enemy_Bark", 2)),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "Treant.Growth", "生长", "敌方回合开始获得 5 格挡；玩家火焰牌会压制下一次生长。",
        (_fallback_intent("FallbackSlam", "横扫（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),)),),
        max_hand=3, max_action=1, cards_per_turn=1,
        mechanics=(
            _mechanic("Treant.Growth", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
                      (_effect(unreal.FantasyCombatEffectType.BLOCK, 5,
                               unreal.FantasyCombatTarget.SELF),)),
            _mechanic("Treant.FireSuppression", unreal.FantasyMechanicTrigger.DAMAGE_RESOLVED,
                      required_tag="Element.Fire", suppress_target="Treant.Growth", suppress_turns=1),
        ),
    ),
    _enemy(
        "DA_Enemy_TavernDrunk", "TavernDrunk", "酒馆醉汉", 42,
        (("Enemy_DrunkLow", 3), ("Enemy_DrunkHigh", 2), ("Enemy_DrunkSpill", 2)),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "TavernDrunk.Swing", "醉步", "牌组同时包含高低伤害，并可能让双方随机弃牌。",
        (_fallback_intent("FallbackBottle", "酒瓶挥击（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),)),),
        max_hand=4, max_action=1, starting_mana=3, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_RangerHunter", "RangerHunter", "游侠猎手", 46,
        (("Enemy_HunterLongbow", 1), ("Enemy_HunterShot", 5)),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "RangerHunter.Longbow", "长弓", "长弓进入装备区后，后续攻击获得 2 点加成。",
        (_fallback_intent("FallbackShot", "射击（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5),)),),
        max_hand=4, max_action=1, cards_per_turn=2,
    ),
    _enemy(
        "DA_Enemy_WitchAcolyte", "WitchAcolyte", "女巫学徒", 44,
        (("Enemy_Mana", 2), ("Enemy_WitchHex", 2),
         ("Enemy_WitchDrain", 2), ("Enemy_WitchBolt", 2)),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "WitchAcolyte.Hex", "诅咒", "塞入每战有上限的临时诅咒，并直接削减玩家法力。",
        (_fallback_intent("FallbackHex", "法力抽离（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.LOSE_MANA, 2),)),),
        max_hand=4, max_action=1, starting_mana=1, cards_per_turn=2,
    ),
)

M3_BOSS_SPECS = (
    _enemy(
        "DA_Enemy_WolfKing", "WolfKing", "狼王", 112,
        (("Enemy_WolfBite", 3), ("Enemy_WolfMaul", 3), ("Enemy_Bark", 2)),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "WolfKing.Hunt", "群猎", "每个敌方回合开始时获得 1 力量，形成可预期的软狂暴。",
        (_fallback_intent("FallbackRoyalMaul", "王者扑杀（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 9),)),),
        max_hand=5, max_action=1, cards_per_turn=2, boss=True,
        mechanics=(_mechanic(
            "WolfKing.Hunt", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                     unreal.FantasyCombatTarget.SELF,
                     unreal.FantasyCombatStatus.STRENGTH),),
        ),),
    ),
    _enemy(
        "DA_Enemy_BlackForestWitch", "BlackForestWitch", "黑森林女巫", 124,
        (("Enemy_Mana", 3), ("Enemy_WitchHex", 3),
         ("Enemy_WitchDrain", 2), ("Enemy_FireBlast", 3)),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "BlackForestWitch.Coven", "巫契", "每个敌方回合额外向玩家弃牌堆加入一张有上限的临时诅咒。",
        (_fallback_intent("FallbackCoven", "巫契飞弹（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 8),)),),
        max_hand=5, max_action=1, starting_mana=3, cards_per_turn=3, boss=True,
        mechanics=(_mechanic(
            "BlackForestWitch.Coven", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.ADD_TEMPORARY_CARD, 1,
                     payload_id="Mage_HexCurse", limit=4),),
        ),),
    ),
    _enemy(
        "DA_Enemy_MagicMirrorGuardian", "MagicMirrorGuardian", "魔镜守护者", 138,
        (("Enemy_Wisdom", 3), ("Enemy_CrystalBall", 3),
         ("Enemy_ElementalWave", 3), ("Enemy_FireBlast", 3)),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "MagicMirror.Reflection", "镜面蓄能", "每个敌方回合开始时获得 5 格挡；玩家需规划爆发窗口。",
        (_fallback_intent("FallbackReflection", "镜光（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 10),)),),
        max_hand=5, max_action=1, starting_mana=4, cards_per_turn=3, boss=True,
        mechanics=(_mechanic(
            "MagicMirror.Reflection", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.BLOCK, 5,
                     unreal.FantasyCombatTarget.SELF),),
        ),),
    ),
)

M4_ENEMY_SPECS = (
    _enemy(
        "DA_Enemy_ChurchPenitent", "ChurchPenitent", "教会忏悔者", 56,
        (("Enemy_Repentance", 3), ("Enemy_Hypnosis", 2), ("Enemy_Mana", 2)),
        unreal.FantasyEnemyVisualProfile.SKELETON,
        "ChurchPenitent.Penance", "苦修", "玩家每回合打出第二张牌时受到 1 点穿刺伤害。",
        (_fallback_intent("FallbackPenance", "忏悔（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 4, piercing=True),)),),
        max_hand=4, max_action=1, starting_mana=2, cards_per_turn=2,
        mechanics=(_mechanic(
            "ChurchPenitent.Penance", unreal.FantasyMechanicTrigger.PLAYER_CARD_RESOLVED,
            (_effect(unreal.FantasyCombatEffectType.DAMAGE, 1, piercing=True),),
            limit=unreal.FantasyMechanicLimit.ONCE_PER_TURN),),
    ),
    _enemy(
        "DA_Enemy_WanderingGhost", "WanderingGhost", "游荡幽灵", 52,
        (("Enemy_LifeSteal", 3), ("Enemy_Flinch", 2), ("Enemy_ElementalWave", 2)),
        unreal.FantasyEnemyVisualProfile.SKELETON,
        "WanderingGhost.Ethereal", "灵体", "战斗开始获得首牌免疫；玩家第一张牌的敌方目标效果会失效。",
        (_fallback_intent("FallbackHaunt", "幽触（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 6),)),),
        max_hand=4, max_action=1, starting_mana=2, cards_per_turn=2,
        mechanics=(_mechanic(
            "WanderingGhost.Ethereal", unreal.FantasyMechanicTrigger.BATTLE_STARTED,
            (_effect(unreal.FantasyCombatEffectType.GRANT_FIRST_CARD_IMMUNITY, 1,
                     unreal.FantasyCombatTarget.SELF),),
            limit=unreal.FantasyMechanicLimit.ONCE_PER_BATTLE),),
    ),
    _enemy(
        "DA_Enemy_Gargoyle", "Gargoyle", "石像鬼", 68,
        (("Enemy_Bark", 2), ("Enemy_Bash", 3), ("Enemy_WolfMaul", 2)),
        unreal.FantasyEnemyVisualProfile.SLIME,
        "Gargoyle.Awakening", "苏醒", "以 16 点石肤格挡开战，随后每回合获得 1 力量。",
        (_fallback_intent("FallbackStoneClaw", "石爪（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 7),)),),
        max_hand=4, max_action=1, cards_per_turn=2,
        mechanics=(
            _mechanic("Gargoyle.StoneSleep", unreal.FantasyMechanicTrigger.BATTLE_STARTED,
                      (_effect(unreal.FantasyCombatEffectType.BLOCK, 16,
                               unreal.FantasyCombatTarget.SELF),),
                      limit=unreal.FantasyMechanicLimit.ONCE_PER_BATTLE),
            _mechanic("Gargoyle.Awakening", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
                      (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                               unreal.FantasyCombatTarget.SELF,
                               unreal.FantasyCombatStatus.STRENGTH),)),
        ),
    ),
    _enemy(
        "DA_Enemy_MagicMirror", "MagicMirror", "魔法镜像", 64,
        (("Enemy_CrystalBall", 2), ("Enemy_Wisdom", 2),
         ("Enemy_ElementalWave", 2), ("Enemy_FireBlast", 2)),
        unreal.FantasyEnemyVisualProfile.WIZARD,
        "MagicMirror.Echo", "回响", "玩家每回合首次出牌后，复制其中安全的伤害、防御或状态效果。",
        (_fallback_intent("FallbackEcho", "镜光回响（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 7),)),),
        max_hand=4, max_action=1, starting_mana=3, cards_per_turn=2,
        mechanics=(_mechanic(
            "MagicMirror.Echo", unreal.FantasyMechanicTrigger.PLAYER_CARD_RESOLVED,
            (_effect(unreal.FantasyCombatEffectType.COPY_SOURCE_CARD, 1,
                     unreal.FantasyCombatTarget.SELF),),
            limit=unreal.FantasyMechanicLimit.ONCE_PER_TURN),),
    ),
    _enemy(
        "DA_Enemy_AlchemicalConstruct", "AlchemicalConstruct", "炼金造物", 66,
        (("Enemy_AcidSpray", 2), ("Enemy_Heal", 2),
         ("Enemy_DrunkSpill", 2), ("Enemy_Bash", 2)),
        unreal.FantasyEnemyVisualProfile.SLIME,
        "AlchemicalConstruct.Reaction", "炼金循环", "敌方回合开始时恢复 2 生命并向玩家施加 1 中毒。",
        (_fallback_intent("FallbackAcid", "酸液（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                                   status=unreal.FantasyCombatStatus.POISON),)),),
        max_hand=4, max_action=1, starting_mana=3, cards_per_turn=2,
        mechanics=(_mechanic(
            "AlchemicalConstruct.Reaction", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.HEAL, 2,
                     unreal.FantasyCombatTarget.SELF),
             _effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                     unreal.FantasyCombatTarget.OPPONENT,
                     unreal.FantasyCombatStatus.POISON)),),),
    ),
    _enemy(
        "DA_Enemy_FallenCleric", "FallenCleric", "堕落圣职者", 70,
        (("Enemy_Mana", 2), ("Enemy_Heal", 3),
         ("Enemy_Repentance", 3), ("Enemy_WitchDrain", 2)),
        unreal.FantasyEnemyVisualProfile.SKELETON,
        "FallenCleric.ProfaneGrace", "亵渎恩典", "敌方回合开始恢复 4 生命并造成 2 点穿刺伤害。",
        (_fallback_intent("FallbackGrace", "亵渎祷言（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 5, piercing=True),)),),
        max_hand=5, max_action=1, starting_mana=3, cards_per_turn=2,
        mechanics=(_mechanic(
            "FallenCleric.ProfaneGrace", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.HEAL, 4,
                     unreal.FantasyCombatTarget.SELF),
             _effect(unreal.FantasyCombatEffectType.DAMAGE, 2, piercing=True)),),),
    ),
    _enemy(
        "DA_Enemy_GiantSpiderMatriarch", "GiantSpiderMatriarch", "巨型蜘蛛母体", 96,
        (("Enemy_SpiderBite", 4), ("Enemy_Venom", 3), ("Enemy_WitchHex", 2)),
        unreal.FantasyEnemyVisualProfile.SLIME,
        "GiantSpiderMatriarch.Brood", "毒巢", "每个敌方回合额外向玩家施加 1 中毒。",
        (_fallback_intent("FallbackBrood", "母体毒牙（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 8),)),),
        max_hand=5, max_action=2, starting_mana=3, cards_per_turn=3,
        mechanics=(_mechanic(
            "GiantSpiderMatriarch.Brood", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                     unreal.FantasyCombatTarget.OPPONENT,
                     unreal.FantasyCombatStatus.POISON),),),),
    ),
    _enemy(
        "DA_Enemy_BlackForestHunter", "BlackForestHunter", "黑森林猎杀者", 100,
        (("Enemy_HunterLongbow", 1), ("Enemy_HunterShot", 5), ("Enemy_Hypnosis", 2)),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "BlackForestHunter.Mark", "猎杀标记", "战斗开始令玩家获得 1 破绽；每回合继续磨砺力量。",
        (_fallback_intent("FallbackMarkedShot", "标记射击（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 9),)),),
        max_hand=5, max_action=2, cards_per_turn=3,
        mechanics=(
            _mechanic("BlackForestHunter.Mark", unreal.FantasyMechanicTrigger.BATTLE_STARTED,
                      (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                               unreal.FantasyCombatTarget.OPPONENT,
                               unreal.FantasyCombatStatus.EXPOSED),),
                      limit=unreal.FantasyMechanicLimit.ONCE_PER_BATTLE),
            _mechanic("BlackForestHunter.Aim", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
                      (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                               unreal.FantasyCombatTarget.SELF,
                               unreal.FantasyCombatStatus.STRENGTH),)),
        ),
    ),
    _enemy(
        "DA_Enemy_BlackthornCrossbowman", "BlackthornCrossbowman", "黑棘弩手", 104,
        (("Enemy_HunterLongbow", 1), ("Enemy_HunterShot", 4),
         ("Enemy_WolfMaul", 2), ("Enemy_NoEntry", 2)),
        unreal.FantasyEnemyVisualProfile.WARRIOR,
        "BlackthornCrossbowman.Charge", "公开蓄力", "每回合开始获得 1 力量；被动摘要公开其持续蓄力压力。",
        (_fallback_intent("FallbackBolt", "蓄力弩矢（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 10),)),),
        max_hand=5, max_action=2, cards_per_turn=3,
        mechanics=(_mechanic(
            "BlackthornCrossbowman.Charge", unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED,
            (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                     unreal.FantasyCombatTarget.SELF,
                     unreal.FantasyCombatStatus.STRENGTH),),),),
    ),
    _enemy(
        "DA_Enemy_GraveyardGuard", "GraveyardGuard", "墓园守卫", 108,
        (("Enemy_ShortSword", 1), ("Enemy_NoEntry", 3),
         ("Enemy_Flinch", 2), ("Enemy_Repentance", 3)),
        unreal.FantasyEnemyVisualProfile.SKELETON,
        "GraveyardGuard.Counterwall", "反制壁垒", "战斗开始获得 14 格挡，牌组同时包含装备、防御与弃牌。",
        (_fallback_intent("FallbackGuardCleave", "墓园斩（兼容意图）",
                          (_effect(unreal.FantasyCombatEffectType.DAMAGE, 9),)),),
        max_hand=5, max_action=2, starting_mana=2, cards_per_turn=3,
        mechanics=(_mechanic(
            "GraveyardGuard.Counterwall", unreal.FantasyMechanicTrigger.BATTLE_STARTED,
            (_effect(unreal.FantasyCombatEffectType.BLOCK, 14,
                     unreal.FantasyCombatTarget.SELF),),
            limit=unreal.FantasyMechanicLimit.ONCE_PER_BATTLE),),
    ),
)

M5_ENEMY_NAMES = (
    ("MoonHare", "月影兔", unreal.FantasyEncounterTier.NORMAL, "MoonBeast", 2),
    ("MossTroll", "苔藓巨魔", unreal.FantasyEncounterTier.NORMAL, "Troll", 2),
    ("PlagueRat", "疫病鼠", unreal.FantasyEncounterTier.NORMAL, "Vermin", 2),
    ("BriarDryad", "荆棘树灵", unreal.FantasyEncounterTier.NORMAL, "ForestSpirit", 2),
    ("HighwayBandit", "拦路盗匪", unreal.FantasyEncounterTier.NORMAL, "Bandit", 2),
    ("BellKeeper", "丧钟守人", unreal.FantasyEncounterTier.NORMAL, "Church", 2),
    ("GraveRobber", "掘墓人", unreal.FantasyEncounterTier.NORMAL, "Graveyard", 2),
    ("CandleNun", "烛火修女", unreal.FantasyEncounterTier.NORMAL, "Church", 3),
    ("ClockworkHound", "发条猎犬", unreal.FantasyEncounterTier.NORMAL, "Clockwork", 3),
    ("LivingArmor", "活化铠甲", unreal.FantasyEncounterTier.NORMAL, "Armor", 3),
    ("CursedPortrait", "诅咒肖像", unreal.FantasyEncounterTier.NORMAL, "Portrait", 3),
    ("BloodAlchemist", "血炼金师", unreal.FantasyEncounterTier.NORMAL, "Alchemy", 3),
    ("BoneScribe", "白骨书记", unreal.FantasyEncounterTier.NORMAL, "Undead", 3),
    ("CastleJester", "古堡弄臣", unreal.FantasyEncounterTier.NORMAL, "Court", 3),
    ("MoonlitWerewolf", "月夜狼人", unreal.FantasyEncounterTier.ELITE, "MoonBeast", 2),
    ("PlagueDoctor", "瘟疫医师", unreal.FantasyEncounterTier.ELITE, "Plague", 2),
    ("IronInquisitor", "钢铁审判官", unreal.FantasyEncounterTier.ELITE, "Inquisition", 3),
    ("MirrorDuelist", "镜中决斗者", unreal.FantasyEncounterTier.ELITE, "Mirror", 3),
    ("AncientTreantSovereign", "远古树王", unreal.FantasyEncounterTier.BOSS, "ForestSpirit", 2),
    ("BlackthornRegent", "黑棘摄政王", unreal.FantasyEncounterTier.BOSS, "Blackthorn", 3),
)


def _build_m5_enemies():
    results = []
    for index, (enemy_id, display_name, tier, family, chapter) in enumerate(M5_ENEMY_NAMES):
        is_elite = tier == unreal.FantasyEncounterTier.ELITE
        is_boss = tier == unreal.FantasyEncounterTier.BOSS
        health = (148 + index * 3) if is_boss else (102 + index * 2) if is_elite else (58 + index * 2)
        deck = (("Enemy_WolfBite", 3), ("Enemy_Bark", 2), ("Enemy_DrunkSpill", 2))
        if index % 3 == 1:
            deck = (("Enemy_HunterShot", 4), ("Enemy_NoEntry", 2), ("Enemy_HunterLongbow", 1))
        elif index % 3 == 2:
            deck = (("Enemy_Mana", 2), ("Enemy_WitchBolt", 3), ("Enemy_Heal", 2))
        mechanic_id = f"{enemy_id}.Core"
        if index % 4 == 0:
            effects = (_effect(unreal.FantasyCombatEffectType.BLOCK, 4 + (2 if is_elite or is_boss else 0),
                               unreal.FantasyCombatTarget.SELF),)
            trigger = unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED
            limit = unreal.FantasyMechanicLimit.UNLIMITED
            rule_text = "每回合建立可预期的防线。"
        elif index % 4 == 1:
            effects = (_effect(unreal.FantasyCombatEffectType.DAMAGE, 1 + (1 if is_elite or is_boss else 0),
                               piercing=True),)
            trigger = unreal.FantasyMechanicTrigger.PLAYER_CARD_RESOLVED
            limit = unreal.FantasyMechanicLimit.ONCE_PER_TURN
            rule_text = "每回合首次响应玩家出牌并造成穿刺压力。"
        elif index % 4 == 2:
            effects = (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1,
                               unreal.FantasyCombatTarget.SELF,
                               unreal.FantasyCombatStatus.STRENGTH),)
            trigger = unreal.FantasyMechanicTrigger.BATTLE_STARTED
            limit = unreal.FantasyMechanicLimit.ONCE_PER_BATTLE
            rule_text = "开战时获得力量，要求玩家调整伤害竞速。"
        else:
            effects = (_effect(unreal.FantasyCombatEffectType.APPLY_STATUS, 1 + (1 if is_boss else 0),
                               unreal.FantasyCombatTarget.OPPONENT,
                               unreal.FantasyCombatStatus.POISON),)
            trigger = unreal.FantasyMechanicTrigger.ENEMY_TURN_STARTED
            limit = unreal.FantasyMechanicLimit.UNLIMITED
            rule_text = "回合开始施加持续状态压力。"
        mechanics = [_mechanic(mechanic_id, trigger, effects, limit=limit)]
        if enemy_id == "MirrorDuelist":
            mechanics = [_mechanic(
                mechanic_id, unreal.FantasyMechanicTrigger.PLAYER_CARD_RESOLVED,
                (_effect(unreal.FantasyCombatEffectType.COPY_SOURCE_CARD, 1,
                         unreal.FantasyCombatTarget.SELF),),
                limit=unreal.FantasyMechanicLimit.ONCE_PER_TURN)]
            rule_text = "每回合复制玩家第一张牌中安全的伤害、防御或状态效果。"
        if is_boss:
            mechanics.append(_mechanic(
                f"{enemy_id}.PhasePressure", unreal.FantasyMechanicTrigger.DAMAGE_RESOLVED,
                (_effect(unreal.FantasyCombatEffectType.BLOCK, 3,
                         unreal.FantasyCombatTarget.SELF),),
                min_actual_damage=8, limit=unreal.FantasyMechanicLimit.ONCE_PER_TURN))
            rule_text += "受到单次 8 点以上生命伤害后每回合首次获得 3 格挡，形成阶段压力。"
        results.append(_enemy(
            f"DA_Enemy_{enemy_id}", enemy_id, display_name, health, deck,
            unreal.FantasyEnemyVisualProfile.WIZARD if index % 3 == 2
            else unreal.FantasyEnemyVisualProfile.SKELETON if index % 3 == 1
            else unreal.FantasyEnemyVisualProfile.WARRIOR,
            mechanic_id, "核心机制", rule_text,
            (_fallback_intent(f"Fallback{enemy_id}", "应急攻击（兼容意图）",
                              (_effect(unreal.FantasyCombatEffectType.DAMAGE,
                                       10 if is_boss else 8 if is_elite else 6),)),),
            max_hand=5 if is_elite or is_boss else 4,
            max_action=2 if is_elite or is_boss else 1,
            starting_mana=3 if index % 3 == 2 else 0,
            cards_per_turn=3 if is_elite or is_boss else 2,
            boss=is_boss, mechanics=tuple(mechanics)))
    return tuple(results)


M5_ENEMY_SPECS = _build_m5_enemies()
ENEMY_SPECS = ENEMY_SPECS + M2_ENEMY_SPECS + M3_BOSS_SPECS + M4_ENEMY_SPECS + M5_ENEMY_SPECS


ENEMY_CONTENT_CONTRACTS = {
    "DrowsyBat": (unreal.FantasyEncounterTier.NORMAL, "Beast", 1, 0, 0, 1.0),
    "MagicApprentice": (unreal.FantasyEncounterTier.NORMAL, "Mage", 1, 0, 0, 1.0),
    "VillageGuard": (unreal.FantasyEncounterTier.NORMAL, "Guard", 2, 1, 1, 1.0),
    "Hypnotist": (unreal.FantasyEncounterTier.NORMAL, "Occultist", 2, 1, 1, 1.0),
    "Scarecrow": (unreal.FantasyEncounterTier.NORMAL, "Construct", 3, 2, 2, 1.0),
    "FortuneTeller": (unreal.FantasyEncounterTier.NORMAL, "Seer", 3, 2, 2, 1.0),
    "DragonWhelp": (unreal.FantasyEncounterTier.NORMAL, "Dragon", 4, 3, 3, 1.0),
    "HeadlessKnight": (unreal.FantasyEncounterTier.NORMAL, "Undead", 4, 3, 3, 1.0),
    "ScarecrowElite": (unreal.FantasyEncounterTier.ELITE, "Construct", 6, 4, 4, 1.35),
    "FortuneTellerElite": (unreal.FantasyEncounterTier.ELITE, "Seer", 6, 4, 4, 1.35),
    "HeadlessKnightBoss": (unreal.FantasyEncounterTier.BOSS, "Undead", 8, 5, 5, 2.0),
    "ForestWolf": (unreal.FantasyEncounterTier.NORMAL, "ForestBeast", 1, 0, 3, 1.0),
    "PoisonSpider": (unreal.FantasyEncounterTier.NORMAL, "Vermin", 1, 0, 3, 1.0),
    "Treant": (unreal.FantasyEncounterTier.NORMAL, "ForestSpirit", 2, 0, 3, 1.0),
    "TavernDrunk": (unreal.FantasyEncounterTier.NORMAL, "VillageHuman", 2, 0, 3, 1.0),
    "RangerHunter": (unreal.FantasyEncounterTier.NORMAL, "Ranger", 3, 0, 3, 1.0),
    "WitchAcolyte": (unreal.FantasyEncounterTier.NORMAL, "BlackForest", 3, 0, 3, 1.0),
    "WolfKing": (unreal.FantasyEncounterTier.BOSS, "ForestBeast", 9, 5, 5, 2.0),
    "BlackForestWitch": (unreal.FantasyEncounterTier.BOSS, "BlackForest", 9, 5, 5, 2.0),
    "MagicMirrorGuardian": (unreal.FantasyEncounterTier.BOSS, "Construct", 10, 5, 5, 2.0),
    "ChurchPenitent": (unreal.FantasyEncounterTier.NORMAL, "Church", 5, 0, 3, 1.0),
    "WanderingGhost": (unreal.FantasyEncounterTier.NORMAL, "UndeadSpirit", 5, 0, 3, 1.0),
    "Gargoyle": (unreal.FantasyEncounterTier.NORMAL, "StoneConstruct", 6, 1, 3, 1.0),
    "MagicMirror": (unreal.FantasyEncounterTier.NORMAL, "Mirror", 7, 1, 3, 1.0),
    "AlchemicalConstruct": (unreal.FantasyEncounterTier.NORMAL, "Alchemy", 7, 1, 3, 1.0),
    "FallenCleric": (unreal.FantasyEncounterTier.NORMAL, "FallenChurch", 8, 2, 3, 1.0),
    "GiantSpiderMatriarch": (unreal.FantasyEncounterTier.ELITE, "Vermin", 8, 3, 4, 1.35),
    "BlackForestHunter": (unreal.FantasyEncounterTier.ELITE, "BlackForest", 8, 3, 4, 1.35),
    "BlackthornCrossbowman": (unreal.FantasyEncounterTier.ELITE, "Blackthorn", 9, 3, 4, 1.35),
    "GraveyardGuard": (unreal.FantasyEncounterTier.ELITE, "Graveyard", 9, 3, 4, 1.35),
}

for enemy_id, _display_name, tier, family, chapter in M5_ENEMY_NAMES:
    min_depth = 5 if tier == unreal.FantasyEncounterTier.BOSS else 3 if tier == unreal.FantasyEncounterTier.ELITE else 0
    max_depth = 5 if tier == unreal.FantasyEncounterTier.BOSS else 4 if tier == unreal.FantasyEncounterTier.ELITE else 3
    ENEMY_CONTENT_CONTRACTS[enemy_id] = (
        tier, family, 10 if tier == unreal.FantasyEncounterTier.BOSS else 8 if tier == unreal.FantasyEncounterTier.ELITE else 6,
        min_depth, max_depth, 2.0 if tier == unreal.FantasyEncounterTier.BOSS else 1.35 if tier == unreal.FantasyEncounterTier.ELITE else 1.0)

ENEMY_CHAPTERS = {
    "WolfKing": 2,
    "BlackForestWitch": 2,
    "MagicMirrorGuardian": 3,
    "ChurchPenitent": 2,
    "WanderingGhost": 2,
    "Gargoyle": 2,
    "MagicMirror": 3,
    "AlchemicalConstruct": 3,
    "FallenCleric": 3,
    "GiantSpiderMatriarch": 2,
    "BlackForestHunter": 2,
    "BlackthornCrossbowman": 3,
    "GraveyardGuard": 3,
}
ENEMY_CHAPTERS.update({enemy_id: chapter for enemy_id, _name, _tier, _family, chapter in M5_ENEMY_NAMES})


CHAPTER_DEPTH_SPECS = (
    {
        "depth": 0,
        "tier": unreal.FantasyEncounterTier.NORMAL,
        "choice_count": 3,
        "combat_choice_count": 3,
        "non_combat": (),
    },
    {
        "depth": 1,
        "tier": unreal.FantasyEncounterTier.NORMAL,
        "choice_count": 3,
        "combat_choice_count": 0,
        "non_combat": (
            ("D1_MoonlitWell", "月下古井", "恢复、牺牲换牌或洗掉一张基础攻击。",
             unreal.FantasyRouteNodeType.EVENT, "MoonlitWell", 1.0),
            ("D1_AshenSmith", "灰烬铁匠", "负伤换取一张职业牌，或移除一张普通攻击。",
             unreal.FantasyRouteNodeType.EVENT, "AshenSmith", 1.0),
            ("D1_ExileCamp", "流亡者营火", "恢复生命、升级卡牌或获取下一战格挡。",
             unreal.FantasyRouteNodeType.REST, "ExileCamp", 1.0),
        ),
    },
    {
        "depth": 2,
        "tier": unreal.FantasyEncounterTier.NORMAL,
        "choice_count": 3,
        "combat_choice_count": 3,
        "non_combat": (),
    },
    {
        "depth": 3,
        "tier": unreal.FantasyEncounterTier.NORMAL,
        "choice_count": 3,
        "combat_choice_count": 2,
        "non_combat": (
            ("D3_MoonlitWell", "月下古井", "恢复、牺牲换取职业牌或移除一张基础攻击。",
             unreal.FantasyRouteNodeType.EVENT, "MoonlitWell", 1.0),
        ),
    },
    {
        "depth": 4,
        "tier": unreal.FantasyEncounterTier.NORMAL,
        "choice_count": 3,
        "combat_choice_count": 0,
        "non_combat": (
            ("D4_MoonlitWell", "月下古井", "守关战前用生命交换牌组调整。",
             unreal.FantasyRouteNodeType.EVENT, "MoonlitWell", 1.0),
            ("D4_AshenSmith", "灰烬铁匠", "守关战前获取职业牌、删牌或格挡。",
             unreal.FantasyRouteNodeType.EVENT, "AshenSmith", 1.0),
            ("D4_ExileCamp", "流亡者营火", "守关战前最后一次休整。",
             unreal.FantasyRouteNodeType.REST, "ExileCamp", 1.0),
        ),
    },
    {
        "depth": 5,
        "tier": unreal.FantasyEncounterTier.BOSS,
        "choice_count": 1,
        "combat_choice_count": 1,
        "non_combat": (),
    },
)


def chapter_depth_specs(chapter_number):
    if chapter_number == 1:
        return CHAPTER_DEPTH_SPECS
    prefix = f"C{chapter_number}"
    area = "黑森林" if chapter_number == 2 else "诅咒古堡"
    return (
        {"depth": 0, "tier": unreal.FantasyEncounterTier.NORMAL,
         "choice_count": 3, "combat_choice_count": 3, "non_combat": ()},
        {"depth": 1, "tier": unreal.FantasyEncounterTier.NORMAL,
         "choice_count": 3, "combat_choice_count": 0, "non_combat": (
             (f"{prefix}D1_Shop", "夜路商队", "购买职业牌、删牌、恢复或祝福，也可直接离开。",
              unreal.FantasyRouteNodeType.SHOP, "WanderingMerchant", 1.0),
             (f"{prefix}D1_Chest", "封印宝箱", "从金币、祝福和卡牌中选择一项。",
              unreal.FantasyRouteNodeType.TREASURE, "AncientChest", 1.0),
             (f"{prefix}D1_Rest", f"{area}营火", "恢复、升级或准备下一场战斗。",
              unreal.FantasyRouteNodeType.REST, "ExileCamp", 1.0),
         )},
        {"depth": 2, "tier": unreal.FantasyEncounterTier.NORMAL,
         "choice_count": 3, "combat_choice_count": 3, "non_combat": ()},
        {"depth": 3, "tier": unreal.FantasyEncounterTier.ELITE,
         "choice_count": 3, "combat_choice_count": 2, "non_combat": (
             (f"{prefix}D3_Chest", "遗失宝箱", "打开一枚封印锁扣。",
              unreal.FantasyRouteNodeType.TREASURE, "AncientChest", 1.0),
         )},
        {"depth": 4, "tier": unreal.FantasyEncounterTier.NORMAL,
         "choice_count": 3, "combat_choice_count": 0, "non_combat": (
             (f"{prefix}D4_Shop", "守关前商队", "最后一次购买、删牌、恢复或祝福。",
              unreal.FantasyRouteNodeType.SHOP, "WanderingMerchant", 1.0),
             (f"{prefix}D4_Chest", "守关宝箱", "在守关战前取得一项资源。",
              unreal.FantasyRouteNodeType.TREASURE, "AncientChest", 1.0),
             (f"{prefix}D4_Rest", "守关营火", "守关战前恢复、升级或拿取旧盾。",
              unreal.FantasyRouteNodeType.REST, "ExileCamp", 1.0),
         )},
        {"depth": 5, "tier": unreal.FantasyEncounterTier.BOSS,
         "choice_count": 1, "combat_choice_count": 1, "non_combat": ()},
    )


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
        f"{root}/Data/Blessings",
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
    definition.set_editor_property("expose_in_main_world", not spec["is_main"])
    definition.set_editor_property("portal_order", spec["portal_order"])
    definition.set_editor_property("portal_color", spec["portal_color"])
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    return definition_path


def validate_main_world_dependencies():
    """Keep this W01 generator read-only with respect to the parallel W00 world."""
    missing = [
        path
        for path in (W00_MAP, W00_DEFINITION)
        if not unreal.EditorAssetLibrary.does_asset_exist(path)
    ]
    if missing:
        raise RuntimeError(
            "W00 authored content must be prepared by setup_w00_main_world.py "
            "before W01 setup; missing: " + ", ".join(missing)
        )
    unreal.log("W00 dependencies validated read-only; W01 setup will not save W00 assets.")


def make_combat_effect(spec):
    effect = unreal.FantasyCombatEffectSpec()
    effect.set_editor_property("effect_type", spec["effect_type"])
    effect.set_editor_property("target", spec["target"])
    effect.set_editor_property("magnitude", spec["magnitude"])
    effect.set_editor_property(
        "status",
        spec.get("status", unreal.FantasyCombatStatus.NONE),
    )
    effect.set_editor_property("piercing", spec.get("piercing", False))
    effect.set_editor_property(
        "scales_with_strength",
        spec.get("scales_with_strength", False),
    )
    effect.set_editor_property("payload_id", unreal.Name(spec.get("payload_id", "")))
    effect.set_editor_property("limit", spec.get("limit", 0))
    effect.set_editor_property("multiplier", spec.get("multiplier", 1))
    return effect


def ensure_blessing_definition(spec):
    blessing_id, display_name, description, profession, trigger, effects, required_tag, limit, price = spec
    asset_name = f"DA_Blessing_{blessing_id}"
    asset_path = f"{W01_BLESSING_ROOT}/{asset_name}"
    blessing = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )
    if blessing is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.FantasyBlessingDefinition)
        blessing = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, W01_BLESSING_ROOT, unreal.FantasyBlessingDefinition, factory
        )
        if blessing is None:
            raise RuntimeError(f"Failed to create FantasyBlessingDefinition: {asset_path}")
    profession_value = unreal.FantasyPlayerProfession.NONE
    if profession == "Mage":
        profession_value = unreal.FantasyPlayerProfession.MAGE
    blessing.set_editor_property("blessing_id", unreal.Name(blessing_id))
    blessing.set_editor_property("display_name", display_name)
    blessing.set_editor_property("description", description)
    blessing.set_editor_property("profession", profession_value)
    blessing.set_editor_property("trigger", trigger)
    blessing.set_editor_property("required_card_tag", unreal.Name(required_tag))
    blessing.set_editor_property("max_triggers_per_battle", limit)
    blessing.set_editor_property("shop_price", price)
    blessing.set_editor_property(
        "effects", [make_combat_effect(effect) for effect in effects]
    )
    unreal.EditorAssetLibrary.save_loaded_asset(blessing, only_if_is_dirty=False)
    return asset_path


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
    card.set_editor_property(
        "card_set_id",
        unreal.Name(spec.get("card_set_id", "W01_EasternHorror")),
    )
    card.set_editor_property("display_name", spec["display_name"])
    card.set_editor_property("description", spec["description"])
    card.set_editor_property("energy_cost", spec.get("energy_cost", 0))
    card.set_editor_property("action_cost", spec.get("action_cost", 0))
    card.set_editor_property("mana_cost", spec.get("mana_cost", 0))
    card.set_editor_property(
        "use_classic_resources",
        spec.get("use_classic_resources", False),
    )
    card.set_editor_property("valor_cost", spec.get("valor_cost", 0))
    card.set_editor_property("card_type", spec["card_type"])
    card.set_editor_property("school", spec["school"])
    card.set_editor_property("profession", spec["profession"])
    contract = _card_contract(spec)
    card.set_editor_property("rarity", contract["rarity"])
    card.set_editor_property(
        "build_tags",
        [unreal.Name(tag) for tag in contract["build_tags"]],
    )
    card.set_editor_property("upgrade_level", contract["upgrade_level"])
    # Clear stale links before the second pass resolves CardId references.
    card.set_editor_property("upgrade_card", None)
    card.set_editor_property(
        "effects",
        [make_combat_effect(effect) for effect in spec["effects"]],
    )
    # Unreal's Python reflection removes the native boolean `b` prefix.
    card.set_editor_property("retain", spec["retain"])
    card.set_editor_property("exhaust", spec["exhaust"])
    card.set_editor_property(
        "reward_eligible",
        spec.get("reward_eligible", False),
    )
    card.set_editor_property(
        "equipment_attack_bonus",
        spec.get("equipment_attack", 0),
    )
    card.set_editor_property(
        "equipment_turn_start_block",
        spec.get("equipment_block", 0),
    )
    card.set_editor_property(
        "equipment_turn_start_draw",
        spec.get("equipment_draw", 0),
    )

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


def link_card_upgrades(card_assets_by_id):
    for spec in CARD_SPECS:
        upgrade_card_id = _card_contract(spec)["upgrade_card_id"]
        if not upgrade_card_id:
            continue
        card_path = card_assets_by_id[spec["card_id"]]
        upgrade_path = card_assets_by_id.get(upgrade_card_id)
        if not upgrade_path:
            raise RuntimeError(
                f"Card {spec['card_id']} references missing upgrade {upgrade_card_id}"
            )
        card = unreal.EditorAssetLibrary.load_asset(card_path)
        upgrade_card = unreal.EditorAssetLibrary.load_asset(upgrade_path)
        card.set_editor_property("upgrade_card", upgrade_card)
        unreal.EditorAssetLibrary.save_loaded_asset(card, only_if_is_dirty=False)


def make_enemy_intent(spec):
    intent = unreal.FantasyEnemyIntentStep()
    intent.set_editor_property("intent_id", unreal.Name(spec["intent_id"]))
    intent.set_editor_property("display_name", spec["display_name"])
    intent.set_editor_property(
        "effects",
        [make_combat_effect(effect) for effect in spec["effects"]],
    )
    return intent


def make_enemy_deck_entry(card_id, copies, card_assets_by_id):
    card_path = card_assets_by_id.get(card_id)
    card = unreal.EditorAssetLibrary.load_asset(card_path) if card_path else None
    if card is None:
        raise RuntimeError(
            f"Enemy deck references unavailable CardDefinition: {card_id}"
        )

    entry = unreal.FantasyEnemyDeckEntry()
    entry.set_editor_property("card", card)
    entry.set_editor_property("copies", copies)
    return entry


def make_enemy_mechanic(spec):
    rule = unreal.FantasyCombatMechanicRule()
    rule.set_editor_property("mechanic_id", unreal.Name(spec["mechanic_id"]))
    rule.set_editor_property("trigger", spec["trigger"])
    rule.set_editor_property("required_source_card_tag", unreal.Name(spec["required_tag"]))
    rule.set_editor_property("min_actual_damage", spec["min_actual_damage"])
    rule.set_editor_property("max_target_block", spec["max_target_block"])
    rule.set_editor_property("limit", spec["limit"])
    rule.set_editor_property("suppress_target_mechanic_id", unreal.Name(spec["suppress_target"]))
    rule.set_editor_property("suppress_turns", spec["suppress_turns"])
    rule.set_editor_property(
        "effects", [make_combat_effect(effect) for effect in spec["effects"]]
    )
    return rule


def ensure_enemy_definition(spec, card_assets_by_id):
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
    contract = ENEMY_CONTENT_CONTRACTS.get(spec["enemy_id"])
    if contract is None:
        raise RuntimeError(
            f"Enemy is missing its M1 content contract: {spec['enemy_id']}"
        )
    tier, family, danger, min_depth, max_depth, reward_weight = contract
    enemy.set_editor_property("encounter_tier", tier)
    enemy.set_editor_property("family", unreal.Name(family))
    enemy_chapter = ENEMY_CHAPTERS.get(spec["enemy_id"], 1)
    enemy.set_editor_property("chapter", enemy_chapter)
    enemy.set_editor_property("danger_rating", danger)
    enemy.set_editor_property(
        "unlock_condition", unreal.Name(f"W01.Chapter{enemy_chapter}")
    )
    enemy.set_editor_property("min_depth", min_depth)
    enemy.set_editor_property("max_depth", max_depth)
    enemy.set_editor_property("reward_weight", reward_weight)
    enemy.set_editor_property("max_health", spec["max_health"])
    enemy.set_editor_property(
        "deck",
        [
            make_enemy_deck_entry(card_id, copies, card_assets_by_id)
            for card_id, copies in spec["deck"]
        ],
    )
    enemy.set_editor_property("max_hand_size", spec["max_hand_size"])
    enemy.set_editor_property(
        "max_action_points",
        spec["max_action_points"],
    )
    enemy.set_editor_property("starting_mana", spec["starting_mana"])
    enemy.set_editor_property("cards_per_turn", spec["cards_per_turn"])
    enemy.set_editor_property("passive_id", unreal.Name(spec["passive_id"]))
    enemy.set_editor_property("passive_name", spec["passive_name"])
    enemy.set_editor_property(
        "passive_description",
        spec["passive_description"],
    )
    enemy.set_editor_property(
        "mechanics",
        [make_enemy_mechanic(mechanic) for mechanic in spec.get("mechanics", ())],
    )
    enemy.set_editor_property("visual_profile", spec["visual_profile"])
    enemy.set_editor_property("boss", spec.get("boss", False))
    enemy.set_editor_property(
        "intent_cycle",
        [make_enemy_intent(intent) for intent in spec["intents"]],
    )
    unreal.EditorAssetLibrary.save_loaded_asset(enemy, only_if_is_dirty=False)
    return asset_path


def make_non_combat_route_definition(spec):
    node_id, display_name, description, node_type, payload_id, weight = spec
    candidate = unreal.FantasyNonCombatRouteDefinition()
    candidate.set_editor_property("node_id", unreal.Name(node_id))
    candidate.set_editor_property("display_name", display_name)
    candidate.set_editor_property("description", description)
    candidate.set_editor_property("node_type", node_type)
    candidate.set_editor_property("payload_id", unreal.Name(payload_id))
    candidate.set_editor_property("weight", weight)
    return candidate


def make_route_depth_definition(spec):
    depth = unreal.FantasyRouteDepthDefinition()
    depth.set_editor_property("depth", spec["depth"])
    depth.set_editor_property("choice_count", spec.get("choice_count", 3))
    depth.set_editor_property(
        "combat_choice_count",
        spec.get("combat_choice_count", 2),
    )
    depth.set_editor_property("encounter_tier", spec["tier"])
    depth.set_editor_property("require_distinct_enemy_families", True)
    depth.set_editor_property(
        "non_combat_candidates",
        [make_non_combat_route_definition(item) for item in spec["non_combat"]],
    )
    return depth


def ensure_chapter_definitions():
    chapter_names = {
        1: "DA_Chapter_W01_AshenKingdom",
        2: "DA_Chapter_W01_BlackForest",
        3: "DA_Chapter_W01_CursedCastle",
    }
    paths = []
    for chapter_number, asset_name in chapter_names.items():
        asset_path = f"{W01_CHAPTER_ROOT}/{asset_name}"
        chapter = (
            unreal.EditorAssetLibrary.load_asset(asset_path)
            if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
            else None
        )
        if chapter is None:
            factory = unreal.DataAssetFactory()
            factory.set_editor_property(
                "data_asset_class", unreal.FantasyChapterDefinition
            )
            chapter = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                asset_name, W01_CHAPTER_ROOT,
                unreal.FantasyChapterDefinition, factory,
            )
            if chapter is None:
                raise RuntimeError(f"Failed to create FantasyChapterDefinition: {asset_path}")

        enemy_assets = []
        for spec in ENEMY_SPECS:
            enemy_id = spec["enemy_id"]
            tier = ENEMY_CONTENT_CONTRACTS[enemy_id][0]
            introduced = ENEMY_CHAPTERS.get(enemy_id, 1)
            if introduced > chapter_number:
                continue
            if tier == unreal.FantasyEncounterTier.BOSS and introduced != chapter_number:
                continue
            enemy_path = f"{W01_ENEMY_ROOT}/{spec['asset_name']}"
            enemy = unreal.EditorAssetLibrary.load_asset(enemy_path)
            if enemy is None:
                raise RuntimeError(f"Chapter encounter pool references missing enemy: {enemy_path}")
            enemy_assets.append(enemy)

        chapter.set_editor_property(
            "chapter_id", unreal.Name(f"W01.AshenKingdom.Chapter{chapter_number}")
        )
        chapter.set_editor_property("chapter_number", chapter_number)
        chapter.set_editor_property("total_depths", 6)
        chapter.set_editor_property(
            "unlock_condition", unreal.Name(f"W01.Chapter{chapter_number}")
        )
        chapter.set_editor_property("encounter_pool", enemy_assets)
        chapter.set_editor_property(
            "depth_definitions",
            [make_route_depth_definition(spec)
             for spec in chapter_depth_specs(chapter_number)],
        )
        unreal.EditorAssetLibrary.save_loaded_asset(chapter, only_if_is_dirty=False)
        paths.append(asset_path)
    return paths


def ensure_campaign_definition(chapter_paths):
    asset_name = "DA_Campaign_W01_AshenKingdom"
    asset_path = f"{W01_CHAPTER_ROOT}/{asset_name}"
    campaign = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )
    if campaign is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class", unreal.FantasyCampaignDefinition
        )
        campaign = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, W01_CHAPTER_ROOT, unreal.FantasyCampaignDefinition, factory
        )
        if campaign is None:
            raise RuntimeError(f"Failed to create FantasyCampaignDefinition: {asset_path}")

    slots = []
    for number in range(1, 4):
        chapter = unreal.EditorAssetLibrary.load_asset(chapter_paths[number - 1])
        if chapter is None:
            raise RuntimeError(f"Campaign chapter failed to load: {chapter_paths[number - 1]}")
        slot = unreal.FantasyCampaignChapterSlot()
        slot.set_editor_property("chapter_id", unreal.Name(f"W01.AshenKingdom.Chapter{number}"))
        slot.set_editor_property("chapter_number", number)
        slot.set_editor_property("open", True)
        slot.set_editor_property("definition", chapter)
        slots.append(slot)
    campaign.set_editor_property("campaign_id", unreal.Name("W01.AshenKingdom"))
    campaign.set_editor_property("planned_chapter_count", 3)
    campaign.set_editor_property("open_chapter_count", 3)
    campaign.set_editor_property("chapters", slots)
    unreal.EditorAssetLibrary.save_loaded_asset(campaign, only_if_is_dirty=False)
    return asset_path


def main():
    validate_main_world_dependencies()
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
        "/Game/ThirdParty/W01_EasternHorror",
        "/Game/ThirdParty/W02_SpiralTower",
    ):
        ensure_directory(root)

    w01_spec = next(spec for spec in WORLD_SPECS if not spec["is_main"])
    ensure_world_directories(w01_spec["root"])
    map_path = ensure_map(w01_spec)
    definition_path = ensure_world_definition(w01_spec, map_path)
    unreal.log(f"World content ready: {w01_spec['world_id']} -> {definition_path}")

    card_assets_by_id = {}
    for spec in CARD_SPECS:
        card_path = ensure_card_definition(spec)
        card_assets_by_id[spec["card_id"]] = card_path
        unreal.log(f"Card content ready: {spec['card_id']} -> {card_path}")
    link_card_upgrades(card_assets_by_id)

    reward_count = sum(
        1 for spec in CARD_SPECS if spec.get("reward_eligible", False)
    )
    knight_starter_copy_count = sum(spec["copies"] for spec in PLAYER_CARD_SPECS)
    mage_starter_copy_count = sum(spec["copies"] for spec in MAGE_CARD_SPECS)
    ranger_starter_copy_count = sum(spec["copies"] for spec in RANGER_CARD_SPECS)
    nun_starter_copy_count = sum(spec["copies"] for spec in NUN_CARD_SPECS)
    knight_reward_count = sum(
        1 for spec in PLAYER_REWARD_CARD_SPECS
        if spec.get("reward_eligible", False)
    )
    reward_counts_by_profession = {
        profession: sum(1 for spec in CARD_SPECS
                        if spec.get("reward_eligible", False)
                        and spec.get("profession") == profession)
        for profession in (unreal.FantasyPlayerProfession.KNIGHT,
                           unreal.FantasyPlayerProfession.MAGE,
                           unreal.FantasyPlayerProfession.RANGER,
                           unreal.FantasyPlayerProfession.NUN)
    }
    mage_reward_count = reward_counts_by_profession[unreal.FantasyPlayerProfession.MAGE]
    unreal.log(
        "W01_CARD_PROGRESSION_SETUP_COMPLETE "
        f"definitions={len(CARD_SPECS)} rewards={reward_count}"
    )
    unreal.log(
        "W01_CLASSIC_PLAYER_CARD_SETUP_COMPLETE "
        f"professions=4 starter_definitions="
        f"{len(PLAYER_CARD_SPECS) + len(MAGE_CARD_SPECS) + len(RANGER_CARD_SPECS) + len(NUN_CARD_SPECS)} "
        f"starter_copies={knight_starter_copy_count + mage_starter_copy_count + ranger_starter_copy_count + nun_starter_copy_count} "
        f"rewards={reward_count} "
        "evidence=verified-names copies=project-tuned"
    )
    unreal.log(
        "W01_MAGE_CARD_SETUP_COMPLETE "
        f"starter_definitions={len(MAGE_CARD_SPECS)} "
        f"starter_copies={mage_starter_copy_count} rewards={mage_reward_count} "
        "profession=LittleWitch evidence=verified-names copies=project-tuned"
    )
    unreal.log(
        "W01_KNIGHT_CARD_SETUP_COMPLETE "
        f"starter_definitions={len(PLAYER_CARD_SPECS)} "
        f"starter_copies={knight_starter_copy_count} rewards={knight_reward_count}"
    )
    unreal.log(
        "W01_RANGER_CARD_SETUP_COMPLETE "
        f"starter_definitions={len(RANGER_CARD_SPECS)} starter_copies={ranger_starter_copy_count} "
        f"rewards={reward_counts_by_profession[unreal.FantasyPlayerProfession.RANGER]} archetypes=3"
    )
    unreal.log(
        "W01_NUN_CARD_SETUP_COMPLETE "
        f"starter_definitions={len(NUN_CARD_SPECS)} starter_copies={nun_starter_copy_count} "
        f"rewards={reward_counts_by_profession[unreal.FantasyPlayerProfession.NUN]} archetypes=3"
    )

    for blessing_spec in BLESSING_SPECS:
        ensure_blessing_definition(blessing_spec)
    unreal.log(
        "W01_BLESSING_SETUP_COMPLETE "
        f"definitions={len(BLESSING_SPECS)} shared=6 mage=6 data_driven=1"
    )

    enemy_deck_copy_count = sum(
        copies
        for enemy_spec in ENEMY_SPECS
        for _, copies in enemy_spec["deck"]
    )
    for enemy_spec in ENEMY_SPECS:
        enemy_path = ensure_enemy_definition(enemy_spec, card_assets_by_id)
        unreal.log(
            f"Enemy content ready: {enemy_spec['enemy_id']} -> {enemy_path}"
        )
    chapter_paths = ensure_chapter_definitions()
    campaign_path = ensure_campaign_definition(chapter_paths)
    unreal.log(
        "W01_CHAPTER_DEFINITION_SETUP_COMPLETE "
        f"assets={len(chapter_paths)} depths={len(CHAPTER_DEPTH_SPECS) * 3} "
        f"encounters={len(ENEMY_SPECS)} generator=weighted-constrained"
    )
    unreal.log(
        "W01_CAMPAIGN_DEFINITION_SETUP_COMPLETE "
        f"asset={campaign_path} planned_chapters=3 open_chapters=3"
    )
    unreal.log(
        "W01_ENEMY_DECK_SETUP_COMPLETE "
        f"cards={sum(1 for spec in CARD_SPECS if spec.get('card_set_id') == 'W01_Enemy')} "
        f"enemies={len(ENEMY_SPECS)} "
        f"deck_copies={enemy_deck_copy_count} "
        "evidence=verified-minimum-names copies=project-tuned"
    )
    unreal.log(
        "W01_CONTENT_CONTRACT_SETUP_COMPLETE "
        f"cards={len(CARD_SPECS)} enemies={len(ENEMY_SPECS)} "
        "normal=34 elite=10 boss=6 upgrade_rule=single-level"
    )

    # Definitions above are saved at their point of mutation. Avoid recursively
    # resaving maps, materials, and third-party art that this generator did not edit.
    unreal.EditorAssetLibrary.save_directory(
        f"{W01_ROOT}/Data",
        only_if_is_dirty=True,
        recursive=True,
    )
    unreal.log("WORLD_WALKER_SETUP_COMPLETE")


if __name__ == "__main__":
    main()
