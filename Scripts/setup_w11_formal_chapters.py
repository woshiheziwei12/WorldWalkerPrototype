"""Build the playable five-chapter W11 route and its six authored boss gates.

The node counts are a data candidate. This script deliberately does not create
human freeze/acceptance evidence; the freeze table still reserves that decision.
"""

import unreal


ROOT = "/Game/WorldWalker/Worlds/W11_RogueSurvival"
DATA = f"{ROOT}/Data"
MAPS = f"{ROOT}/Maps"
SOURCE_MAP = f"{MAPS}/L_W11_RogueSurvival"
CONTENT_VERSION = "W11-Chapters-v1-candidate"
RULE_VERSION = "W11-Combat-v5"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"unable to create directory: {path}")


def load(path, expected=None):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"missing asset: {path}")
    if expected and not isinstance(asset, expected):
        raise RuntimeError(f"wrong asset type at {path}: {type(asset)}")
    return asset


def ensure_data_asset(name, directory, cls):
    path = f"{directory}/{name}"
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset:
        return asset
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, directory, cls, factory
    )
    if not asset:
        raise RuntimeError(f"unable to create asset: {path}")
    return asset


def identity(asset, definition_id, display_name):
    asset.set_editor_property("definition_id", unreal.Name(definition_id))
    asset.set_editor_property("display_name", display_name)
    asset.set_editor_property("content_version", CONTENT_VERSION)


def save(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f"unable to save: {asset.get_path_name()}")


def stat_block(health, power, armor, speed=1.0):
    stats = unreal.W11StatBlock()
    stats.set_editor_property("max_health", health)
    stats.set_editor_property("max_mana", 0.0)
    stats.set_editor_property("mana_regen_per_second", 0.0)
    stats.set_editor_property("power", power)
    stats.set_editor_property("armor", armor)
    stats.set_editor_property("move_speed_scale", speed)
    return stats


def reward(stones, cultivation, boss=False):
    value = unreal.W11BattleReward()
    value.set_editor_property("spirit_stones", stones)
    value.set_editor_property("cultivation_experience", cultivation)
    value.set_editor_property("boss", boss)
    return value


def make_boss_ability(key, display_name, shape, damage, windup, active, recovery,
                      range_, radius, projectile_speed=0.0):
    directory = f"{DATA}/Abilities/Enemies/Bosses"
    asset = ensure_data_asset(
        f"DA_Ability_{key}", directory, unreal.W11AbilityDefinition
    )
    identity(asset, f"EnemyAbility.{key}", display_name)
    asset.set_editor_property("logical_slot", unreal.W11AbilitySlot.ACTIVE1)
    asset.set_editor_property("cooldown_scaling", unreal.W11CooldownScaling.NONE)
    asset.set_editor_property("critical_roll_policy", unreal.W11CriticalRollPolicy.NEVER)
    asset.set_editor_property("base_damage", damage)
    asset.set_editor_property("power_coefficient", 1.0)
    asset.set_editor_property("mana_cost", 0.0)
    asset.set_editor_property("cooldown", windup + active + recovery)
    asset.set_editor_property("base_range", range_)
    asset.set_editor_property("base_radius", radius)
    asset.set_editor_property("preferred_range", max(120.0, range_ * 0.72))
    asset.set_editor_property("windup_seconds", windup)
    asset.set_editor_property("active_seconds", active)
    asset.set_editor_property("recovery_seconds", recovery)
    asset.set_editor_property("max_targets", 4)
    if shape == "line":
        asset.set_editor_property("target_mode", unreal.W11AbilityTargetMode.DIRECTION)
        asset.set_editor_property("hit_shape", unreal.W11HitShape.LINE)
        asset.set_editor_property("delivery", unreal.W11HitDelivery.IMMEDIATE)
        asset.set_editor_property("telegraph_shape", unreal.W11TelegraphShape.LINE)
        visual = load(f"{DATA}/Presentation/Skills/DA_SkillVisual_EnemySwordWraithDash")
    elif shape == "circle":
        asset.set_editor_property("target_mode", unreal.W11AbilityTargetMode.TARGET_POINT)
        asset.set_editor_property("hit_shape", unreal.W11HitShape.CIRCLE)
        asset.set_editor_property("delivery", unreal.W11HitDelivery.IMMEDIATE)
        asset.set_editor_property("telegraph_shape", unreal.W11TelegraphShape.CIRCLE)
        visual = load(f"{DATA}/Presentation/Skills/DA_SkillVisual_EnemyStoneFiendSlam")
    else:
        asset.set_editor_property("target_mode", unreal.W11AbilityTargetMode.DIRECTION)
        asset.set_editor_property("hit_shape", unreal.W11HitShape.PROJECTILE)
        asset.set_editor_property("delivery", unreal.W11HitDelivery.PROJECTILE)
        asset.set_editor_property("telegraph_shape", unreal.W11TelegraphShape.PROJECTILE_PATH)
        asset.set_editor_property("projectile_speed", projectile_speed or 520.0)
        asset.set_editor_property("projectile_lifetime", 4.0)
        visual = load(f"{DATA}/Presentation/Skills/DA_SkillVisual_EnemyMiasmaBolt")
    asset.set_editor_property("visuals", visual)
    save(asset)
    return asset


def boss_phase(phase_id, threshold, mechanic_id, abilities):
    phase = unreal.W11BossPhaseDefinition()
    phase.set_editor_property("phase_id", unreal.Name(phase_id))
    phase.set_editor_property("enter_at_health_fraction", threshold)
    phase.set_editor_property("mechanic_id", unreal.Name(mechanic_id))
    phase.set_editor_property("abilities", abilities)
    return phase


def enemy_pool_entry(enemy, cost=10):
    entry = unreal.W11WeightedEnemyEntry()
    entry.set_editor_property("enemy", enemy)
    entry.set_editor_property("weight", 1.0)
    entry.set_editor_property("spawn_cost", cost)
    return entry


def make_boss(spec, shared_sprite):
    key, display, health, power, armor, initial_phase, initial_mechanic, phase_specs = spec
    abilities = []
    for ability_spec in phase_specs[0][3]:
        abilities.append(make_boss_ability(*ability_spec))
    transitions = []
    for phase_id, threshold, mechanic_id, ability_specs in phase_specs[1:]:
        phase_abilities = [make_boss_ability(*ability_spec) for ability_spec in ability_specs]
        transitions.append(boss_phase(phase_id, threshold, mechanic_id, phase_abilities))
    boss = ensure_data_asset(
        f"DA_Enemy_{key}", f"{DATA}/Enemies/Bosses", unreal.W11EnemyDefinition
    )
    identity(boss, f"Boss.{key}", display)
    boss.set_editor_property("base_stats", stat_block(health, power, armor, 1.0))
    boss.set_editor_property("spawn_cost", 10)
    boss.set_editor_property("elite", False)
    boss.set_editor_property("boss", True)
    boss.set_editor_property("formal_boss", True)
    boss.set_editor_property("initial_boss_phase_id", unreal.Name(initial_phase))
    boss.set_editor_property("initial_boss_mechanic_id", unreal.Name(initial_mechanic))
    boss.set_editor_property("abilities", abilities)
    boss.set_editor_property("boss_phases", transitions)
    if shared_sprite:
        boss.set_editor_property("idle_sprite", shared_sprite)
    save(boss)

    encounter = ensure_data_asset(
        f"DA_Encounter_Boss_{key}", f"{DATA}/Encounters/Bosses",
        unreal.W11EncounterDefinition,
    )
    identity(encounter, f"Encounter.Boss.{key}", display)
    encounter.set_editor_property("encounter_type", unreal.W11EncounterType.BOSS)
    encounter.set_editor_property(
        "completion_rule", unreal.W11EncounterCompletionRule.ELIMINATE_ALL
    )
    encounter.set_editor_property("stage_index", 1)
    encounter.set_editor_property("spawn_budget", 10)
    encounter.set_editor_property("duration_seconds", 240.0)
    encounter.set_editor_property("wave_count", 1)
    encounter.set_editor_property("max_concurrent_enemies", 1)
    encounter.set_editor_property("spawn_safe_radius", 540.0)
    encounter.set_editor_property("spawn_outer_radius", 760.0)
    encounter.set_editor_property("enemy_base_health_scale", 1.0)
    encounter.set_editor_property("enemy_base_power_scale", 1.0)
    encounter.set_editor_property("health_scale_per_additional_player", 0.50)
    encounter.set_editor_property("enemy_pool", [enemy_pool_entry(boss)])
    encounter.set_editor_property("reward", reward(320, 260, True))
    save(encounter)
    return boss, encounter


def ability(key, name, shape, damage, windup, active, recovery, range_, radius, speed=0.0):
    return (key, name, shape, damage, windup, active, recovery, range_, radius, speed)


def make_bosses():
    prototype = load(f"{DATA}/Enemies/DA_Enemy_VoidWardenBoss", unreal.W11EnemyDefinition)
    shared_sprite = prototype.get_editor_property("idle_sprite")
    specs = (
        ("HeavenLawPatrol", "天律巡狩", 720.0, 1.30, 18.0,
         "Phase.HeavenLaw.Inspection", "Mechanic.LawLineInspection", (
             ("Phase.HeavenLaw.Inspection", 1.0, "Mechanic.LawLineInspection", [
                 ability("HeavenLawLine", "天律刻线", "line", 17, .70, .10, .90, 430, 52),
             ]),
             ("Phase.HeavenLaw.Sentence", .66, "Mechanic.SentenceCircles", [
                 ability("HeavenLawCircle", "天刑落印", "circle", 22, .92, .08, 1.05, 210, 185),
             ]),
             ("Phase.HeavenLaw.Pursuit", .33, "Mechanic.EdictVolley", [
                 ability("HeavenLawVolley", "巡狩律令", "projectile", 15, .55, .06, .72, 680, 32, 620),
                 ability("HeavenLawLine", "天律刻线", "line", 17, .70, .10, .90, 430, 52),
             ]))),
        ("LuGuanlan", "执律使陆观澜", 820.0, 1.38, 22.0,
         "Phase.Lu.Seal", "Mechanic.SealRush", (
             ("Phase.Lu.Seal", 1.0, "Mechanic.SealRush", [
                 ability("LuSealRush", "封脉剑令", "line", 19, .58, .12, .82, 470, 48),
             ]),
             ("Phase.Lu.Decree", .66, "Mechanic.SwordDecree", [
                 ability("LuSwordDecree", "执律剑诏", "projectile", 16, .62, .06, .78, 700, 34, 680),
             ]),
             ("Phase.Lu.Collapse", .33, "Mechanic.LawCollapse", [
                 ability("LuLawCollapse", "律域崩解", "circle", 26, 1.00, .08, 1.10, 200, 210),
                 ability("LuSealRush", "封脉剑令", "line", 19, .58, .12, .82, 470, 48),
             ]))),
        ("SixGeneralsWarMemory", "昔日六将战争执念", 900.0, 1.44, 24.0,
         "Phase.WarMemory.Charge", "Mechanic.PhantomCharge", (
             ("Phase.WarMemory.Charge", 1.0, "Mechanic.PhantomCharge", [
                 ability("WarPhantomCharge", "六将冲阵", "line", 21, .60, .12, .82, 520, 58),
             ]),
             ("Phase.WarMemory.Banners", .66, "Mechanic.BannerVolley", [
                 ability("WarBannerVolley", "军旗箭雨", "projectile", 17, .54, .06, .70, 720, 36, 700),
             ]),
             ("Phase.WarMemory.Quake", .33, "Mechanic.BattlefieldQuake", [
                 ability("WarBattlefieldQuake", "战场震裂", "circle", 28, 1.02, .09, 1.08, 210, 225),
                 ability("WarPhantomCharge", "六将冲阵", "line", 21, .60, .12, .82, 520, 58),
             ]))),
        ("WishlessHeavenOfficial", "无愿天官", 980.0, 1.50, 28.0,
         "Phase.Wishless.Drain", "Mechanic.DesireDrain", (
             ("Phase.Wishless.Drain", 1.0, "Mechanic.DesireDrain", [
                 ability("WishlessDrain", "无愿蚀心", "projectile", 18, .66, .06, .80, 710, 38, 610),
             ]),
             ("Phase.Wishless.Domain", .66, "Mechanic.EmptyDomain", [
                 ability("WishlessDomain", "空愿法域", "circle", 27, 1.05, .08, 1.10, 220, 235),
             ]),
             ("Phase.Wishless.Judgement", .33, "Mechanic.SilentJudgement", [
                 ability("WishlessJudgement", "寂灭裁断", "line", 24, .74, .12, .92, 540, 62),
                 ability("WishlessDrain", "无愿蚀心", "projectile", 18, .66, .06, .80, 710, 38, 610),
             ]))),
        ("IdealSelf", "理想自我", 1080.0, 1.58, 32.0,
         "Phase.Ideal.Mirror", "Mechanic.MirrorDash", (
             ("Phase.Ideal.Mirror", 1.0, "Mechanic.MirrorDash", [
                 ability("IdealMirrorDash", "镜身瞬斩", "line", 23, .52, .12, .76, 560, 54),
             ]),
             ("Phase.Ideal.Reflection", .66, "Mechanic.ReflectedVolley", [
                 ability("IdealReflectedVolley", "万法返照", "projectile", 19, .52, .06, .68, 760, 35, 740),
             ]),
             ("Phase.Ideal.Perfection", .33, "Mechanic.PerfectCircle", [
                 ability("IdealPerfectCircle", "圆满道域", "circle", 30, .96, .08, 1.00, 230, 245),
                 ability("IdealMirrorDash", "镜身瞬斩", "line", 23, .52, .12, .76, 560, 54),
             ]))),
        ("GuChangyuan", "顾长渊", 1450.0, 1.72, 36.0,
         "Phase.Gu.DemonSlayingHero", "Mechanic.DemonSlayingSword", (
             ("Phase.Gu.DemonSlayingHero", 1.0, "Mechanic.DemonSlayingSword", [
                 ability("GuHeroSword", "屠魔英雄剑", "line", 25, .56, .12, .78, 590, 58),
                 ability("GuHeroVolley", "镇妖剑雨", "projectile", 20, .58, .06, .72, 780, 38, 760),
             ]),
             ("Phase.Gu.WorldImmortal", .66, "Mechanic.WorldSealDomain", [
                 ability("GuWorldSeal", "镇世仙印", "circle", 32, 1.02, .08, 1.05, 230, 255),
                 ability("GuImmortalDecree", "仙君敕令", "projectile", 22, .52, .06, .66, 800, 40, 780),
             ]),
             ("Phase.Gu.WishlessDragon", .33, "Mechanic.DragonCataclysm", [
                 ability("GuDragonRush", "无愿龙冲", "line", 30, .48, .14, .70, 650, 72),
                 ability("GuDragonCataclysm", "龙劫灭世", "circle", 38, 1.12, .10, 1.08, 250, 285),
             ]))),
    )
    return [make_boss(spec, shared_sprite) for spec in specs]


def ensure_chapter_maps():
    names = (
        "L_W11_Chapter01_Prologue",
        "L_W11_Chapter02_HumanRealm",
        "L_W11_Chapter03_WarMemory",
        "L_W11_Chapter04_WishlessCourt",
        "L_W11_Chapter05_IdealMirror",
    )
    result = []
    for name in names:
        path = f"{MAPS}/{name}"
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not asset:
            asset = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, path)
        if not asset:
            raise RuntimeError(f"unable to create chapter map: {path}")
        save(asset)
        result.append(asset)
    return result


def route_node(node_id, display, node_type, encounter=None, safe=False, branch="Main"):
    node = unreal.W11ChapterRouteNode()
    node.set_editor_property("node_id", unreal.Name(node_id))
    node.set_editor_property("display_name", display)
    node.set_editor_property("node_type", node_type)
    node.set_editor_property("encounter", encounter)
    node.set_editor_property("safe_node", safe)
    node.set_editor_property("branch_id", unreal.Name(branch))
    return node


def make_route(chapter_maps, bosses):
    regular_encounters = [
        load(f"{DATA}/Encounters/DA_Encounter_{index:02d}", unreal.W11EncounterDefinition)
        for index in range(1, 6)
    ]
    chapter_names = (
        ("Prologue", "序章·天律巡狩"),
        ("HumanRealm", "第一章·人间执律"),
        ("WarMemory", "第二章·六将残梦"),
        ("WishlessCourt", "第三章·无愿天庭"),
        ("IdealMirror", "第四章·理想镜界"),
    )
    chapters = []
    for index, ((key, display), chapter_map, (_, boss_encounter)) in enumerate(
        zip(chapter_names, chapter_maps, bosses[:5]), start=1
    ):
        chapter = ensure_data_asset(
            f"DA_Chapter_{index:02d}_{key}", f"{DATA}/Chapters",
            unreal.W11ChapterDefinition,
        )
        identity(chapter, f"Chapter.{index:02d}.{key}", display)
        chapter.set_editor_property("chapter_index", index)
        chapter.set_editor_property("chapter_map", chapter_map)
        chapter.set_editor_property("nodes", [
            route_node(f"Node.C{index:02d}.Combat", "破界之战",
                       unreal.W11ChapterNodeType.COMBAT,
                       regular_encounters[(index - 1) % len(regular_encounters)]),
            route_node(f"Node.C{index:02d}.Event", "界痕抉择",
                       unreal.W11ChapterNodeType.EVENT, safe=True),
            route_node(f"Node.C{index:02d}.Rest", "灵脉休整",
                       unreal.W11ChapterNodeType.REST, safe=True),
            route_node(f"Node.C{index:02d}.Market", "云游仙坊",
                       unreal.W11ChapterNodeType.IMMORTAL_MARKET, safe=True),
            route_node(f"Node.C{index:02d}.Boss", "章节镇关",
                       unreal.W11ChapterNodeType.SMALL_BOSS, boss_encounter),
        ])
        save(chapter)
        chapters.append(chapter)

    route = ensure_data_asset(
        "DA_W11_FiveChapterRoute", f"{DATA}/Chapters", unreal.W11ChapterRouteDefinition
    )
    identity(route, "ChapterRoute.W11.FiveChapters", "太虚问道·五章路线")
    route.set_editor_property("chapters", chapters)
    route.set_editor_property("final_boss_encounter", bosses[5][1])
    save(route)
    return route


def main():
    for directory in (
        f"{DATA}/Abilities/Enemies/Bosses",
        f"{DATA}/Enemies/Bosses",
        f"{DATA}/Encounters/Bosses",
        f"{DATA}/Chapters",
    ):
        ensure_directory(directory)
    bosses = make_bosses()
    chapter_maps = ensure_chapter_maps()
    route = make_route(chapter_maps, bosses)
    rules = load(f"{DATA}/DA_W11_RunRules", unreal.W11RunRuleSet)
    rules.set_editor_property("content_version", CONTENT_VERSION)
    rules.set_editor_property("rule_version", RULE_VERSION)
    rules.set_editor_property("chapter_route", route)
    save(rules)
    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "W11_FORMAL_CHAPTER_CONTENT_READY: Chapters=5 Maps=5 SmallBosses=5 "
        "FinalBoss=GuChangyuan FinalBossPhases=3 MarketNodes=5 SafeNodes=15 "
        f"ContentVersion={CONTENT_VERSION} RuleVersion={RULE_VERSION} ContentFrozen=0"
    )


main()
