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
)

W01_ROOT = "/Game/WorldWalker/Worlds/W01_EasternHorror"
W01_CARD_ROOT = f"{W01_ROOT}/Data/Cards"
W01_ENEMY_ROOT = f"{W01_ROOT}/Data/Enemies"
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
):
    return {
        "effect_type": effect_type,
        "target": target,
        "magnitude": magnitude,
        "status": status,
        "piercing": piercing,
        "scales_with_strength": scales_with_strength,
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


CARD_SPECS = (
    PLAYER_CARD_SPECS
    + PLAYER_REWARD_CARD_SPECS
    + MAGE_CARD_SPECS
    + MAGE_REWARD_CARD_SPECS
    + ENEMY_CARD_SPECS
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
        owner_tag = (
            "Profession.Mage"
            if spec["profession"] == unreal.FantasyPlayerProfession.MAGE
            else "Profession.Knight"
        )
    else:
        rarity = (
            unreal.FantasyCardRarity.COMMON
            if spec.get("reward_eligible", False)
            else unreal.FantasyCardRarity.UNCOMMON
        )
        owner_tag = (
            "Profession.Mage"
            if spec["profession"] == unreal.FantasyPlayerProfession.MAGE
            else "Profession.Knight"
        )

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
    enemy.set_editor_property("chapter", 1)
    enemy.set_editor_property("danger_rating", danger)
    enemy.set_editor_property("unlock_condition", unreal.Name("W01.Chapter1"))
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
    enemy.set_editor_property("visual_profile", spec["visual_profile"])
    enemy.set_editor_property("boss", spec.get("boss", False))
    enemy.set_editor_property(
        "intent_cycle",
        [make_enemy_intent(intent) for intent in spec["intents"]],
    )
    unreal.EditorAssetLibrary.save_loaded_asset(enemy, only_if_is_dirty=False)
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
    knight_reward_count = sum(
        1 for spec in PLAYER_REWARD_CARD_SPECS
        if spec.get("reward_eligible", False)
    )
    mage_reward_count = sum(
        1 for spec in MAGE_REWARD_CARD_SPECS
        if spec.get("reward_eligible", False)
    )
    unreal.log(
        "W01_CARD_PROGRESSION_SETUP_COMPLETE "
        f"definitions={len(CARD_SPECS)} rewards={reward_count}"
    )
    unreal.log(
        "W01_CLASSIC_PLAYER_CARD_SETUP_COMPLETE "
        f"professions=2 starter_definitions="
        f"{len(PLAYER_CARD_SPECS) + len(MAGE_CARD_SPECS)} "
        f"starter_copies={knight_starter_copy_count + mage_starter_copy_count} "
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
    unreal.log(
        "W01_ENEMY_DECK_SETUP_COMPLETE "
        f"cards={len(ENEMY_CARD_SPECS)} enemies={len(ENEMY_SPECS)} "
        f"deck_copies={enemy_deck_copy_count} "
        "evidence=verified-minimum-names copies=project-tuned"
    )
    unreal.log(
        "W01_CONTENT_CONTRACT_SETUP_COMPLETE "
        f"cards={len(CARD_SPECS)} enemies={len(ENEMY_SPECS)} "
        "normal=8 elite=2 boss=1 upgrade_rule=single-level"
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
