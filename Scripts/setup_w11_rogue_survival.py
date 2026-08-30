"""Create or refresh the data-driven W11 Xianxia roguelite vertical slice.

Run after compiling the editor target. The script is idempotent and only owns
the /Game/WorldWalker/Worlds/W11_RogueSurvival content root.
"""

import unreal


ROOT = "/Game/WorldWalker/Worlds/W11_RogueSurvival"
MAP = f"{ROOT}/Maps/L_W11_RogueSurvival"
TEMPLATE_MAP = "/Engine/Maps/Templates/Template_Default"
DATA = f"{ROOT}/Data"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Unable to create directory: {path}")


def ensure_asset(asset_name, directory, asset_class):
    path = f"{directory}/{asset_name}"
    asset = (
        unreal.EditorAssetLibrary.load_asset(path)
        if unreal.EditorAssetLibrary.does_asset_exist(path)
        else None
    )
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", asset_class)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, directory, asset_class, factory
        )
    if asset is None:
        raise RuntimeError(f"Unable to create data asset: {path}")
    return asset


def set_identity(asset, definition_id, display_name):
    asset.set_editor_property("definition_id", unreal.Name(definition_id))
    asset.set_editor_property("display_name", display_name)
    asset.set_editor_property("content_version", "W11-M6-v2")


def modifier(stat, magnitude, source, operation=None):
    value = unreal.W11StatModifier()
    value.set_editor_property("stat", stat)
    value.set_editor_property(
        "operation", operation or unreal.W11ModifierOperation.ADD
    )
    value.set_editor_property("magnitude", magnitude)
    value.set_editor_property("source_id", unreal.Name(source))
    return value


def ensure_map_and_registration():
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP):
        map_asset = unreal.EditorAssetLibrary.duplicate_asset(TEMPLATE_MAP, MAP)
        if map_asset is None:
            raise RuntimeError(f"Unable to create W11 map: {MAP}")
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            map_asset, only_if_is_dirty=False
        ):
            raise RuntimeError(f"Unable to save W11 map: {MAP}")
    definition = ensure_asset(
        "DA_W11_RogueSurvival", DATA, unreal.WorldDefinition
    )
    map_asset = unreal.EditorAssetLibrary.load_asset(MAP)
    definition.set_editor_property("world_id", unreal.Name("W11_RogueSurvival"))
    definition.set_editor_property("display_name", "太虚问道")
    definition.set_editor_property("entry_map", map_asset)
    definition.set_editor_property("is_main_world", False)
    definition.set_editor_property("expose_in_main_world", True)
    definition.set_editor_property("portal_order", 110)
    definition.set_editor_property(
        "portal_color", unreal.LinearColor(0.15, 0.82, 0.72, 1.0)
    )
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)


def make_stat_choices():
    specs = (
        ("QiBlood", "气血充盈", unreal.W11StatType.MAX_HEALTH, 20.0),
        ("TrueQi", "真元流转", unreal.W11StatType.MAX_MANA, 15.0),
        ("QiRecovery", "吐纳归元", unreal.W11StatType.MANA_REGEN, 1.5),
        ("DaoPower", "道行精进", unreal.W11StatType.POWER, 0.15),
        ("BodyGuard", "护体罡气", unreal.W11StatType.ARMOR, 8.0),
        ("SwiftStep", "御风而行", unreal.W11StatType.MOVE_SPEED, 1.08),
        ("FastCast", "心念如电", unreal.W11StatType.ATTACK_SPEED, 1.10),
        ("Fortune", "气运加身", unreal.W11StatType.LUCK, 10.0),
        ("LongReach", "神识延展", unreal.W11StatType.ATTACK_RANGE, 1.12),
        ("WideArt", "术域扩张", unreal.W11StatType.AREA, 1.18),
        ("Critical", "灵台明悟", unreal.W11StatType.CRIT_CHANCE, 0.04),
        ("Haste", "法诀圆融", unreal.W11StatType.ABILITY_HASTE, 0.08),
    )
    multiply_stats = {
        unreal.W11StatType.MOVE_SPEED,
        unreal.W11StatType.ATTACK_SPEED,
        unreal.W11StatType.ATTACK_RANGE,
        unreal.W11StatType.AREA,
    }
    assets = []
    for key, name, stat, amount in specs:
        asset = ensure_asset(
            f"DA_Choice_{key}", f"{DATA}/StatChoices", unreal.W11StatChoiceDefinition
        )
        set_identity(asset, f"Choice.{key}", name)
        op = (
            unreal.W11ModifierOperation.MULTIPLY
            if stat in multiply_stats
            else unreal.W11ModifierOperation.ADD
        )
        asset.set_editor_property("modifiers", [modifier(stat, amount, key, op)])
        asset.set_editor_property("offer_weight", 1.0)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def make_rank_effect(rank, modifiers, behaviors=None):
    effect = unreal.W11ManualRankEffects()
    effect.set_editor_property("rank", rank)
    effect.set_editor_property("stat_modifiers", modifiers)
    effect.set_editor_property("granted_behaviors", behaviors or [])
    return effect


def make_manuals():
    specs = (
        ("AzureBody", "青木锻体诀", unreal.W11ManualCategory.BODY_CULTIVATION,
         unreal.W11StatType.MAX_HEALTH, (12.0, 18.0, 26.0, 40.0), 100),
        ("RiverBreath", "沧浪吐纳法", unreal.W11ManualCategory.BREATHING,
         unreal.W11StatType.MANA_REGEN, (0.8, 1.0, 1.3, 1.8), 100),
        ("CloudSword", "流云剑经", unreal.W11ManualCategory.SWORD_AND_MARTIAL,
         unreal.W11StatType.POWER, (0.08, 0.10, 0.14, 0.20), 120),
        ("ThunderSpell", "九霄引雷诀", unreal.W11ManualCategory.SPELL,
         unreal.W11StatType.CRIT_CHANCE, (0.02, 0.025, 0.035, 0.05), 140),
        ("WindStep", "踏风无痕步", unreal.W11ManualCategory.MOVEMENT,
         unreal.W11StatType.MOVE_SPEED, (1.05, 1.05, 1.06, 1.08), 110),
        ("HeavenEye", "太虚观想篇", unreal.W11ManualCategory.DIVINE_SENSE,
         unreal.W11StatType.ATTACK_RANGE, (1.06, 1.07, 1.08, 1.12), 130),
        ("StarFormation", "小周天星阵", unreal.W11ManualCategory.FORMATION,
         unreal.W11StatType.AREA, (1.08, 1.09, 1.11, 1.15), 140),
        ("FortuneBook", "紫微衍运录", unreal.W11ManualCategory.FORTUNE,
         unreal.W11StatType.LUCK, (6.0, 8.0, 11.0, 15.0), 150),
        ("GoldenLight", "大日金光咒", unreal.W11ManualCategory.DIVINE_POWER,
         unreal.W11StatType.ARMOR, (5.0, 7.0, 10.0, 15.0), 160),
    )
    ranks = (
        unreal.W11ComprehensionRank.INITIATE,
        unreal.W11ComprehensionRank.ADEPT,
        unreal.W11ComprehensionRank.MASTERED,
        unreal.W11ComprehensionRank.PERFECTED,
    )
    multiply_stats = {
        unreal.W11StatType.MOVE_SPEED,
        unreal.W11StatType.ATTACK_RANGE,
        unreal.W11StatType.AREA,
    }
    assets = []
    for key, name, category, stat, amounts, price in specs:
        asset = ensure_asset(
            f"DA_Manual_{key}", f"{DATA}/Manuals", unreal.W11ManualDefinition
        )
        set_identity(asset, f"Manual.{key}", name)
        asset.set_editor_property("category", category)
        asset.set_editor_property("rarity", unreal.W11Rarity.YELLOW)
        asset.set_editor_property("base_price", price)
        op = (
            unreal.W11ModifierOperation.MULTIPLY
            if stat in multiply_stats
            else unreal.W11ModifierOperation.ADD
        )
        effects = []
        for index, (rank, amount) in enumerate(zip(ranks, amounts)):
            behaviors = ["ManualBehavior.ActiveBurn"] if key == "ThunderSpell" and index == 0 else []
            effects.append(make_rank_effect(
                rank, [modifier(stat, amount, f"Manual.{key}", op)], behaviors
            ))
        asset.set_editor_property("rank_effects", effects)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def make_treasures():
    specs = (
        ("JadeGourd", "养元玉葫", unreal.W11TreasureCategory.ARTIFACT,
         [(unreal.W11StatType.MAX_MANA, 30.0)], 180),
        ("MountainBell", "镇岳金钟", unreal.W11TreasureCategory.PROTECTIVE_TREASURE,
         [(unreal.W11StatType.ARMOR, 18.0)], 200),
        ("FortuneCoin", "天机古钱", unreal.W11TreasureCategory.CURIO,
         [(unreal.W11StatType.LUCK, 25.0)], 220),
        ("SpiritCompass", "寻灵罗盘", unreal.W11TreasureCategory.FORMATION_DEVICE,
         [(unreal.W11StatType.PICKUP_RADIUS, 180.0)], 170),
        ("HeartMirror", "照心玄镜", unreal.W11TreasureCategory.CURIO,
         [(unreal.W11StatType.CRIT_MULTIPLIER, 0.35)], 240),
        ("BoundaryFlag", "八门界旗", unreal.W11TreasureCategory.FORMATION_DEVICE,
         [(unreal.W11StatType.AREA, 1.35)], 260),
    )
    assets = []
    for key, name, category, values, price in specs:
        asset = ensure_asset(
            f"DA_Treasure_{key}", f"{DATA}/Treasures", unreal.W11TreasureDefinition
        )
        set_identity(asset, f"Treasure.{key}", name)
        asset.set_editor_property("category", category)
        asset.set_editor_property("rarity", unreal.W11Rarity.MYSTIC)
        asset.set_editor_property("base_price", price)
        mods = []
        for stat, amount in values:
            op = (
                unreal.W11ModifierOperation.MULTIPLY
                if stat == unreal.W11StatType.AREA
                else unreal.W11ModifierOperation.ADD
            )
            mods.append(modifier(stat, amount, f"Treasure.{key}", op))
        asset.set_editor_property("stat_modifiers", mods)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def make_services():
    specs = (
        ("Heal", "灵泉疗体", 60, 0.50, 0.0),
        ("RestoreQi", "聚灵归元", 50, 0.0, 0.65),
    )
    assets = []
    for key, name, price, heal, mana in specs:
        asset = ensure_asset(
            f"DA_Service_{key}", f"{DATA}/Services", unreal.W11ShopServiceDefinition
        )
        set_identity(asset, f"Service.{key}", name)
        asset.set_editor_property("base_price", price)
        asset.set_editor_property("heal_fraction", heal)
        asset.set_editor_property("restore_mana_fraction", mana)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def make_abilities():
    specs = (
        ("FlowingCloudSword", "流云剑气", unreal.W11AbilityArchetype.DIRECTED_SWEEP,
         32.0, 1.10, 14.0, 4.0, 420.0, 75.0, 0.0, 0.0),
        ("FiveThunder", "五雷正法", unreal.W11AbilityArchetype.RADIAL_BURST,
         24.0, 1.00, 24.0, 6.0, 0.0, 230.0, 0.0, 0.0),
        ("DarkWaterWard", "玄水护体", unreal.W11AbilityArchetype.SELF_BARRIER,
         0.0, 1.00, 18.0, 8.0, 0.0, 0.0, 35.0, 0.0),
        ("SpringRenewal", "回春诀", unreal.W11AbilityArchetype.SELF_HEAL,
         0.0, 1.00, 22.0, 10.0, 0.0, 0.0, 0.0, 30.0),
        ("MoonChasingStrike", "逐月瞬斩", unreal.W11AbilityArchetype.PIERCING_LINE,
         40.0, 1.25, 20.0, 5.5, 650.0, 42.0, 0.0, 0.0),
        ("TwoPolesFormation", "两仪剑阵", unreal.W11AbilityArchetype.FORMATION_BURST,
         20.0, 0.85, 28.0, 7.5, 0.0, 320.0, 0.0, 0.0),
    )
    assets = []
    for key, name, archetype, damage, coefficient, mana, cooldown, range_, radius, barrier, heal in specs:
        asset = ensure_asset(
            f"DA_Ability_{key}", f"{DATA}/Abilities", unreal.W11AbilityDefinition
        )
        set_identity(asset, f"Ability.{key}", name)
        asset.set_editor_property("archetype", archetype)
        asset.set_editor_property("logical_slot", unreal.W11AbilitySlot.ACTIVE1)
        asset.set_editor_property(
            "target_mode",
            unreal.W11AbilityTargetMode.SELF if archetype in (
                unreal.W11AbilityArchetype.SELF_BARRIER,
                unreal.W11AbilityArchetype.SELF_HEAL,
            ) else unreal.W11AbilityTargetMode.DIRECTION,
        )
        asset.set_editor_property(
            "hit_shape",
            unreal.W11HitShape.CIRCLE if archetype in (
                unreal.W11AbilityArchetype.RADIAL_BURST,
                unreal.W11AbilityArchetype.FORMATION_BURST,
                unreal.W11AbilityArchetype.SELF_BARRIER,
                unreal.W11AbilityArchetype.SELF_HEAL,
            ) else unreal.W11HitShape.LINE,
        )
        asset.set_editor_property("delivery", unreal.W11HitDelivery.IMMEDIATE)
        asset.set_editor_property("cooldown_scaling", unreal.W11CooldownScaling.ABILITY_HASTE)
        asset.set_editor_property(
            "critical_roll_policy",
            unreal.W11CriticalRollPolicy.NEVER if damage <= 0.0
            else unreal.W11CriticalRollPolicy.PER_EXECUTION,
        )
        asset.set_editor_property("base_damage", damage)
        asset.set_editor_property("power_coefficient", coefficient)
        asset.set_editor_property("mana_cost", mana)
        asset.set_editor_property("cooldown", cooldown)
        asset.set_editor_property("base_range", range_)
        asset.set_editor_property("base_radius", radius)
        asset.set_editor_property("barrier_amount", barrier)
        asset.set_editor_property("heal_amount", heal)
        asset.set_editor_property("mana_restore_amount", 0.0)
        asset.set_editor_property("max_targets", 16)
        asset.set_editor_property(
            "pierces_targets",
            archetype == unreal.W11AbilityArchetype.PIERCING_LINE,
        )
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def make_basic_attack():
    asset = ensure_asset(
        "DA_Ability_BasicAttack", f"{DATA}/Abilities", unreal.W11AbilityDefinition
    )
    set_identity(asset, "Ability.BasicAttack", "基础攻击")
    asset.set_editor_property("logical_slot", unreal.W11AbilitySlot.BASIC_ATTACK)
    asset.set_editor_property("target_mode", unreal.W11AbilityTargetMode.DIRECTION)
    asset.set_editor_property("hit_shape", unreal.W11HitShape.LINE)
    asset.set_editor_property("delivery", unreal.W11HitDelivery.IMMEDIATE)
    asset.set_editor_property("cooldown_scaling", unreal.W11CooldownScaling.ATTACK_SPEED)
    asset.set_editor_property(
        "critical_roll_policy", unreal.W11CriticalRollPolicy.PER_EXECUTION
    )
    asset.set_editor_property("archetype", unreal.W11AbilityArchetype.DIRECTED_SWEEP)
    asset.set_editor_property("base_damage", 20.0)
    asset.set_editor_property("power_coefficient", 1.0)
    asset.set_editor_property("mana_cost", 0.0)
    asset.set_editor_property("cooldown", 0.45)
    asset.set_editor_property("base_range", 280.0)
    asset.set_editor_property("base_radius", 80.0)
    asset.set_editor_property("max_targets", 16)
    impact_visual = unreal.EditorAssetLibrary.load_asset(
        f"{DATA}/Presentation/Skills/DA_SkillVisual_CommonHitSpark"
    )
    if impact_visual:
        asset.set_editor_property("visuals", impact_visual)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


def make_status(status_id, status_type, duration, magnitude, tick_interval=1.0,
                max_stacks=1, stacking_rule=None, boss_duration_scale=0.5):
    status = unreal.W11StatusEffectSpec()
    status.set_editor_property("status_id", status_id)
    status.set_editor_property("type", status_type)
    status.set_editor_property("duration_seconds", duration)
    status.set_editor_property("magnitude", magnitude)
    status.set_editor_property("tick_interval_seconds", tick_interval)
    status.set_editor_property("max_stacks", max_stacks)
    status.set_editor_property(
        "stacking_rule",
        stacking_rule or unreal.W11StatusStackingRule.REFRESH_DURATION,
    )
    status.set_editor_property("dispellable", True)
    status.set_editor_property("boss_duration_scale", boss_duration_scale)
    return status


def make_enemy_abilities():
    specs = (
        (
            "MiasmaBolt", "瘴气弹", unreal.W11AbilityTargetMode.DIRECTION,
            unreal.W11HitShape.PROJECTILE, unreal.W11HitDelivery.PROJECTILE,
            unreal.W11TelegraphShape.PROJECTILE_PATH,
            15.0, 760.0, 30.0, 360.0, 560.0, 0.70, 0.06, 1.10, 3.0,
        ),
        (
            "SwordWraithDash", "残剑冲斩", unreal.W11AbilityTargetMode.DIRECTION,
            unreal.W11HitShape.LINE, unreal.W11HitDelivery.IMMEDIATE,
            unreal.W11TelegraphShape.LINE,
            18.0, 360.0, 52.0, 0.0, 320.0, 0.55, 0.12, 1.05, 0.1,
        ),
        (
            "StoneFiendSlam", "石甲重砸", unreal.W11AbilityTargetMode.TARGET_POINT,
            unreal.W11HitShape.CIRCLE, unreal.W11HitDelivery.IMMEDIATE,
            unreal.W11TelegraphShape.CIRCLE,
            24.0, 190.0, 170.0, 0.0, 175.0, 0.85, 0.08, 1.45, 0.1,
        ),
    )
    assets = {}
    for (
        key, name, target_mode, hit_shape, delivery, telegraph, damage, range_,
        radius, projectile_speed, preferred_range, windup, active, recovery,
        projectile_lifetime,
    ) in specs:
        asset = ensure_asset(
            f"DA_Ability_{key}", f"{DATA}/Abilities/Enemies", unreal.W11AbilityDefinition
        )
        set_identity(asset, f"EnemyAbility.{key}", name)
        asset.set_editor_property("logical_slot", unreal.W11AbilitySlot.ACTIVE1)
        asset.set_editor_property("target_mode", target_mode)
        asset.set_editor_property("hit_shape", hit_shape)
        asset.set_editor_property("delivery", delivery)
        asset.set_editor_property("cooldown_scaling", unreal.W11CooldownScaling.NONE)
        asset.set_editor_property("critical_roll_policy", unreal.W11CriticalRollPolicy.NEVER)
        asset.set_editor_property("base_damage", damage)
        asset.set_editor_property("power_coefficient", 1.0)
        asset.set_editor_property("mana_cost", 0.0)
        asset.set_editor_property("cooldown", windup + active + recovery)
        asset.set_editor_property("base_range", range_)
        asset.set_editor_property("base_radius", radius)
        asset.set_editor_property("projectile_speed", projectile_speed)
        asset.set_editor_property("preferred_range", preferred_range)
        asset.set_editor_property("windup_seconds", windup)
        asset.set_editor_property("active_seconds", active)
        asset.set_editor_property("recovery_seconds", recovery)
        asset.set_editor_property("projectile_lifetime", projectile_lifetime)
        asset.set_editor_property("telegraph_shape", telegraph)
        asset.set_editor_property("max_targets", 4)
        asset.set_editor_property(
            "status_effects",
            [make_status(
                "Status.Slow.Miasma", unreal.W11StatusType.SLOW,
                2.5, 0.65, boss_duration_scale=0.5,
            )] if key == "MiasmaBolt" else [],
        )
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets[key] = asset
    return assets


def make_heroes():
    specs = (
        ("Wei", "威", "赤心铁骨", "临阵如山，以伤换势的前阵修士。", unreal.W11HeroGender.MALE,
         [(unreal.W11StatType.MAX_HEALTH, 24.0, unreal.W11ModifierOperation.ADD),
          (unreal.W11StatType.ARMOR, 8.0, unreal.W11ModifierOperation.ADD)]),
        ("Dong", "栋", "厚土承天", "沉稳可靠，善于承压并借力复原。", unreal.W11HeroGender.MALE,
         [(unreal.W11StatType.ARMOR, 12.0, unreal.W11ModifierOperation.ADD),
          (unreal.W11StatType.HEALING_RECEIVED, 1.10, unreal.W11ModifierOperation.MULTIPLY)]),
        ("Tian", "天", "灵台澄明", "真元深厚，法诀运转比常人更快。", unreal.W11HeroGender.MALE,
         [(unreal.W11StatType.MAX_MANA, 18.0, unreal.W11ModifierOperation.ADD),
          (unreal.W11StatType.ABILITY_HASTE, 0.10, unreal.W11ModifierOperation.ADD)]),
        ("Xiang", "翔", "御风逐云", "身轻如燕，擅长游走与连续出手。", unreal.W11HeroGender.MALE,
         [(unreal.W11StatType.MOVE_SPEED, 1.10, unreal.W11ModifierOperation.MULTIPLY),
          (unreal.W11StatType.ATTACK_SPEED, 1.06, unreal.W11ModifierOperation.MULTIPLY)]),
        ("Pu", "普", "凡心有运", "出身平凡却福缘暗藏，善于搜罗战场机缘。", unreal.W11HeroGender.MALE,
         [(unreal.W11StatType.LUCK, 12.0, unreal.W11ModifierOperation.ADD),
          (unreal.W11StatType.PICKUP_RADIUS, 60.0, unreal.W11ModifierOperation.ADD)]),
        ("Ying", "迎", "春风化雨", "气息绵长，善纳灵机并承受疗愈。", unreal.W11HeroGender.FEMALE,
         [(unreal.W11StatType.MANA_REGEN, 1.2, unreal.W11ModifierOperation.ADD),
          (unreal.W11StatType.HEALING_RECEIVED, 1.12, unreal.W11ModifierOperation.MULTIPLY)]),
        ("Li", "丽", "绛霞流辉", "法域舒展，真元充沛，擅长覆盖大片战场。", unreal.W11HeroGender.FEMALE,
         [(unreal.W11StatType.AREA, 1.15, unreal.W11ModifierOperation.MULTIPLY),
          (unreal.W11StatType.MAX_MANA, 12.0, unreal.W11ModifierOperation.ADD)]),
        ("Jia", "佳", "一念破妄", "出手精准，能在瞬息间放大会心之威。", unreal.W11HeroGender.FEMALE,
         [(unreal.W11StatType.CRIT_CHANCE, 0.05, unreal.W11ModifierOperation.ADD),
          (unreal.W11StatType.CRIT_MULTIPLIER, 0.15, unreal.W11ModifierOperation.ADD)]),
        ("Yu", "玉", "玉魄观微", "神识敏锐，能从更远处引动更强攻势。", unreal.W11HeroGender.FEMALE,
         [(unreal.W11StatType.ATTACK_RANGE, 1.12, unreal.W11ModifierOperation.MULTIPLY),
          (unreal.W11StatType.POWER, 0.08, unreal.W11ModifierOperation.ADD)]),
        ("Meng", "梦", "镜花入梦", "心游幻境，善令法诀余韵绵长、周转不息。", unreal.W11HeroGender.FEMALE,
         [(unreal.W11StatType.DURATION, 1.18, unreal.W11ModifierOperation.MULTIPLY),
          (unreal.W11StatType.ABILITY_HASTE, 0.06, unreal.W11ModifierOperation.ADD)]),
    )
    assets = []
    for key, name, epithet, biography, gender, values in specs:
        asset = ensure_asset(f"DA_Hero_{key}", f"{DATA}/Heroes", unreal.W11HeroDefinition)
        set_identity(asset, f"Hero.{key}", name)
        asset.set_editor_property("epithet", epithet)
        asset.set_editor_property("biography", biography)
        asset.set_editor_property("gender", gender)
        portrait = unreal.EditorAssetLibrary.load_asset(f"{ROOT}/Art/Heroes/T_W11_Hero_{key}")
        if portrait is None:
            raise RuntimeError(f"Missing imported hero portrait: {key}")
        avatar = unreal.EditorAssetLibrary.load_asset(
            f"{ROOT}/Art/Heroes/Avatars/T_W11_Hero_{key}_Avatar"
        )
        if avatar is None:
            raise RuntimeError(f"Missing imported hero avatar: {key}")
        asset.set_editor_property("portrait", portrait)
        asset.set_editor_property("avatar", avatar)
        asset.set_editor_property("icon", avatar)
        asset.set_editor_property("starting_modifiers", [
            modifier(stat, amount, f"Hero.{key}", operation)
            for stat, amount, operation in values
        ])
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def make_sects(abilities):
    specs = (
        ("AzureCloudSword", "青云剑宗", "云海御剑", "以身御剑，剑气向前横扫。", 0, (0.08, 0.42, 0.42, 1.0)),
        ("ThunderManor", "玄霄雷府", "玄霄驭雷", "引五雷落身，震荡周遭群敌。", 1, (0.30, 0.24, 0.55, 1.0)),
        ("CanglanPalace", "沧澜水宫", "沧海玄护", "玄水绕体，凝成可抵伤害的护障。", 2, (0.06, 0.30, 0.50, 1.0)),
        ("DanxiaValley", "丹霞药谷", "丹霞济世", "调和气血，在危急时自愈伤势。", 3, (0.40, 0.18, 0.16, 1.0)),
        ("ShadowMoonTower", "影月楼", "月影瞬杀", "月影成线，远距贯穿前方妖邪。", 4, (0.27, 0.18, 0.38, 1.0)),
        ("TianjiFormation", "天机阵宗", "天机演阵", "布两仪阵域，对周遭敌人同时施压。", 5, (0.34, 0.28, 0.09, 1.0)),
    )
    assets = []
    for key, name, style_tagline, description, ability_index, color in specs:
        asset = ensure_asset(f"DA_Sect_{key}", f"{DATA}/Sects", unreal.W11SectDefinition)
        set_identity(asset, f"Sect.{key}", name)
        asset.set_editor_property("description", description)
        asset.set_editor_property("style_tagline", style_tagline)
        artwork = unreal.EditorAssetLibrary.load_asset(
            f"{ROOT}/Art/Sects/T_W11_Sect_{key}"
        )
        if artwork is None:
            unreal.log_warning(
                f"Sect selection artwork for {key} is not imported yet; "
                "run import_w11_sect_selection_art.py after this setup pass"
            )
        asset.set_editor_property("selection_artwork", artwork)
        asset.set_editor_property("initial_ability", abilities[ability_index])
        asset.set_editor_property("accent_color", unreal.LinearColor(*color))
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        assets.append(asset)
    return assets


def stat_block(health, power, armor, speed=1.0):
    stats = unreal.W11StatBlock()
    stats.set_editor_property("max_health", health)
    stats.set_editor_property("max_mana", 0.0)
    stats.set_editor_property("mana_regen_per_second", 0.0)
    stats.set_editor_property("power", power)
    stats.set_editor_property("armor", armor)
    stats.set_editor_property("move_speed_scale", speed)
    return stats


def make_enemies_and_encounters(enemy_abilities):
    enemy_specs = (
        ("MiasmaWisp", "瘴灵", 46.0, 0.8, 0.0, 1, "MiasmaBolt"),
        ("StoneFiend", "石甲妖", 105.0, 1.1, 16.0, 3, "StoneFiendSlam"),
        ("SwordWraith", "残剑魂", 72.0, 1.35, 4.0, 2, "SwordWraithDash"),
    )
    enemies = []
    costs = []
    for key, name, health, power, armor, cost, ability_key in enemy_specs:
        asset = ensure_asset(
            f"DA_Enemy_{key}", f"{DATA}/Enemies", unreal.W11EnemyDefinition
        )
        set_identity(asset, f"Enemy.{key}", name)
        asset.set_editor_property("base_stats", stat_block(health, power, armor))
        asset.set_editor_property("spawn_cost", cost)
        asset.set_editor_property("abilities", [enemy_abilities[ability_key]])
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        enemies.append(asset)
        costs.append(cost)

    elite = ensure_asset(
        "DA_Enemy_MiasmaSwordElite", f"{DATA}/Enemies", unreal.W11EnemyDefinition
    )
    set_identity(elite, "Enemy.MiasmaSwordElite", "瘴剑使")
    elite.set_editor_property("base_stats", stat_block(420.0, 1.25, 18.0, 1.05))
    elite.set_editor_property("spawn_cost", 7)
    elite.set_editor_property("elite", True)
    elite.set_editor_property("boss", False)
    elite.set_editor_property("abilities", [
        enemy_abilities["MiasmaBolt"],
        enemy_abilities["SwordWraithDash"],
        enemy_abilities["StoneFiendSlam"],
    ])
    unreal.EditorAssetLibrary.save_loaded_asset(elite, only_if_is_dirty=False)

    boss = ensure_asset(
        "DA_Enemy_VoidWardenBoss", f"{DATA}/Enemies", unreal.W11EnemyDefinition
    )
    set_identity(boss, "Enemy.VoidWardenBoss", "太虚镇关使")
    boss.set_editor_property("base_stats", stat_block(900.0, 1.45, 28.0, 1.0))
    boss.set_editor_property("spawn_cost", 10)
    boss.set_editor_property("elite", False)
    boss.set_editor_property("boss", True)
    boss.set_editor_property("abilities", [
        enemy_abilities["StoneFiendSlam"],
        enemy_abilities["MiasmaBolt"],
        enemy_abilities["SwordWraithDash"],
    ])
    unreal.EditorAssetLibrary.save_loaded_asset(boss, only_if_is_dirty=False)

    encounters = []
    encounter_specs = (
        (1, unreal.W11EncounterType.CLEAR,
         unreal.W11EncounterCompletionRule.ELIMINATE_ALL, 22, 1, 75.0, enemies, costs),
        (2, unreal.W11EncounterType.WAVES,
         unreal.W11EncounterCompletionRule.ELIMINATE_ALL_WAVES, 15, 3, 75.0, enemies, costs),
        (3, unreal.W11EncounterType.SURVIVAL,
         unreal.W11EncounterCompletionRule.SURVIVE_DURATION, 18, 3, 30.0, enemies, costs),
        (4, unreal.W11EncounterType.ELITE,
         unreal.W11EncounterCompletionRule.ELIMINATE_ALL, 7, 1, 90.0, [elite], [7]),
        (5, unreal.W11EncounterType.BOSS,
         unreal.W11EncounterCompletionRule.ELIMINATE_ALL, 10, 1, 120.0, [boss], [10]),
    )
    for stage, encounter_type, completion_rule, budget, wave_count, duration, stage_enemies, stage_costs in encounter_specs:
        asset = ensure_asset(
            f"DA_Encounter_{stage:02d}", f"{DATA}/Encounters", unreal.W11EncounterDefinition
        )
        set_identity(asset, f"Encounter.Stage{stage}", f"第{stage}重试炼")
        asset.set_editor_property("stage_index", stage)
        asset.set_editor_property("encounter_type", encounter_type)
        asset.set_editor_property("completion_rule", completion_rule)
        asset.set_editor_property("spawn_budget", budget)
        asset.set_editor_property("duration_seconds", duration)
        asset.set_editor_property("wave_count", wave_count)
        asset.set_editor_property("inter_wave_delay_seconds", 1.25)
        asset.set_editor_property("max_concurrent_enemies", 5 if stage == 1 else 12)
        asset.set_editor_property("spawn_safe_radius", 650.0 if stage == 1 else 500.0)
        asset.set_editor_property("spawn_outer_radius", 1250.0 if stage == 1 else 900.0)
        asset.set_editor_property("budget_scale_per_additional_player", 0.35)
        asset.set_editor_property("enemy_base_health_scale", 4.5 if stage == 1 else 1.0)
        # Five simultaneous enemies should increase spatial pressure without
        # multiplying the old two-enemy damage wall (2 * 0.45 ~= 5 * 0.18).
        asset.set_editor_property("enemy_base_power_scale", 0.18 if stage == 1 else 1.0)
        asset.set_editor_property("health_scale_per_additional_player", 0.45)
        pool = []
        for enemy, cost in zip(stage_enemies, stage_costs):
            entry = unreal.W11WeightedEnemyEntry()
            entry.set_editor_property("enemy", enemy)
            entry.set_editor_property("weight", 1.0)
            entry.set_editor_property("spawn_cost", cost)
            pool.append(entry)
        reward = unreal.W11BattleReward()
        reward.set_editor_property("spirit_stones", 120 + stage * 35)
        reward.set_editor_property("cultivation_experience", 100 + stage * 25)
        reward.set_editor_property("elite", encounter_type == unreal.W11EncounterType.ELITE)
        reward.set_editor_property("boss", encounter_type == unreal.W11EncounterType.BOSS)
        asset.set_editor_property("enemy_pool", pool)
        asset.set_editor_property("reward", reward)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        encounters.append(asset)
    return encounters


def make_rules(stat_choices, manuals, treasures, services, encounters, heroes, sects,
               basic_attack, abilities):
    rules = ensure_asset("DA_W11_RunRules", DATA, unreal.W11RunRuleSet)
    set_identity(rules, "RunRules.Standard", "太虚问道·标准试炼")
    rules.set_editor_property("rule_version", "W11-Combat-v4")
    rules.set_editor_property("combat_log_schema_version", 3)
    rules.set_editor_property("max_players", 4)
    rules.set_editor_property("manual_slots", 6)
    rules.set_editor_property("treasure_slots", 4)
    rules.set_editor_property("cultivation_offer_count", 3)
    rules.set_editor_property("manual_shop_slots", 3)
    rules.set_editor_property("treasure_shop_slots", 2)
    rules.set_editor_property("service_shop_slots", 1)
    rules.set_editor_property("ability_shop_slots", 1)
    rules.set_editor_property("dodge_distance", 260.0)
    rules.set_editor_property("dodge_duration", 0.18)
    rules.set_editor_property("dodge_cooldown", 0.75)
    rules.set_editor_property("dodge_invulnerability_duration", 0.20)
    player = unreal.W11StatBlock()
    player.set_editor_property("max_health", 120.0)
    player.set_editor_property("max_mana", 80.0)
    player.set_editor_property("mana_regen_per_second", 4.0)
    player.set_editor_property("power", 1.0)
    player.set_editor_property("move_speed_scale", 1.0)
    rules.set_editor_property("default_player_stats", player)
    rules.set_editor_property("basic_attack_ability", basic_attack)
    rules.set_editor_property("active_abilities", abilities)
    rules.set_editor_property("stat_choices", stat_choices)
    rules.set_editor_property("manuals", manuals)
    rules.set_editor_property("treasures", treasures)
    rules.set_editor_property("services", services)
    rules.set_editor_property("encounters", encounters)
    rules.set_editor_property("heroes", heroes)
    rules.set_editor_property("sects", sects)
    unreal.EditorAssetLibrary.save_loaded_asset(rules, only_if_is_dirty=False)


def main():
    for path in (
        ROOT, f"{ROOT}/Maps", DATA, f"{DATA}/StatChoices", f"{DATA}/Manuals",
        f"{DATA}/Treasures", f"{DATA}/Services", f"{DATA}/Enemies",
        f"{DATA}/Encounters", f"{DATA}/Abilities", f"{DATA}/Abilities/Enemies",
        f"{DATA}/Heroes", f"{DATA}/Sects",
        f"{ROOT}/Art", f"{ROOT}/Art/Heroes", f"{ROOT}/UI", f"{ROOT}/Audio",
    ):
        ensure_directory(path)
    ensure_map_and_registration()
    choices = make_stat_choices()
    manuals = make_manuals()
    treasures = make_treasures()
    services = make_services()
    basic_attack = make_basic_attack()
    abilities = make_abilities()
    enemy_abilities = make_enemy_abilities()
    heroes = make_heroes()
    sects = make_sects(abilities)
    encounters = make_enemies_and_encounters(enemy_abilities)
    make_rules(
        choices, manuals, treasures, services, encounters, heroes, sects,
        basic_attack, abilities,
    )
    unreal.log("W11_ARCHITECTURE_READY: content skeleton refreshed")


if __name__ == "__main__":
    main()
