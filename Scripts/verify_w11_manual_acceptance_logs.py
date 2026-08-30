import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


SUMMARY_MARKER = "W11_COMBAT_SUMMARY "
KEY_VALUE = re.compile(r"([A-Za-z][A-Za-z0-9]*)=([^ ]+)")
PLAYER = re.compile(r"W11_COMBAT_SUMMARY_PLAYER .*? Sect=([^ ]+)")
ENEMY = re.compile(
    r"W11_COMBAT_SUMMARY_ENEMY .*? Enemy=([^ ]+) .*? Attacks=(\d+) Hits=(\d+)"
)
MANUAL_SESSION = re.compile(
    r"W11_MANUAL_EVIDENCE_SESSION Role=(Observe|Evasion|Dodge|Recovery|Feedback) "
    r"Seed=(\d+) Schema=(\d+) InputScript=None Interactive=1 Rendered=1 "
    r"Audio=1 AutoStart=0 NetMode=0 TestOverride=0"
)
REJECTED_SESSION_MARKER = "W11_MANUAL_EVIDENCE_SESSION_REJECTED"
EXPECTED_SEED = 424242
EXPECTED_SCHEMA = 3


def fail(message):
    print(f"W11_MANUAL_OBJECTIVE_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


@dataclass
class RunLog:
    label: str
    text: str
    summary: dict
    sects: set
    enemies: dict
    role: str
    seed: int
    schema: int


def parse_log(label, text, expected_role):
    if REJECTED_SESSION_MARKER in text:
        fail(f"{label} contains a rejected manual evidence session")
    sessions = MANUAL_SESSION.findall(text)
    if len(sessions) != 1:
        fail(f"{label} must contain exactly one accepted manual evidence session, found {len(sessions)}")
    role, seed_text, schema_text = sessions[0]
    seed = int(seed_text)
    schema = int(schema_text)
    if role != expected_role:
        fail(f"{label} declares Role={role}, expected Role={expected_role}")
    if seed != EXPECTED_SEED:
        fail(f"{label} uses Seed={seed}, expected fixed Seed={EXPECTED_SEED}")
    if schema != EXPECTED_SCHEMA:
        fail(f"{label} uses Schema={schema}, expected Schema={EXPECTED_SCHEMA}")

    summaries = [
        dict(KEY_VALUE.findall(line.split(SUMMARY_MARKER, 1)[1]))
        for line in text.splitlines()
        if SUMMARY_MARKER in line
    ]
    if len(summaries) != 1:
        fail(f"{label} must contain exactly one terminal summary, found {len(summaries)}")
    summary = summaries[0]
    if summary.get("InputScript") != "None":
        fail(f"{label} used InputScript={summary.get('InputScript')}; scripted input is not manual evidence")
    if int(summary.get("Schema", "-1")) != schema:
        fail(f"{label} summary Schema={summary.get('Schema')} does not match session Schema={schema}")
    if int(summary.get("Seed", "-1")) != seed:
        fail(f"{label} summary Seed={summary.get('Seed')} does not match session Seed={seed}")
    for key in (
        "DuplicateRewards", "GhostDamage", "NoWarningDamage", "UnmatchedWarnings", "Stuck"
    ):
        if int(summary.get(key, "-1")) != 0:
            fail(f"{label} has {key}={summary.get(key)}")
    enemies = {
        enemy_id: (int(attacks), int(hits))
        for enemy_id, attacks, hits in ENEMY.findall(text)
    }
    return RunLog(label, text, summary, set(PLAYER.findall(text)), enemies, role, seed, schema)


def require_roles(observe, evasion, dodge, recovery, all_runs):
    required_enemy_abilities = (
        "EnemyAbility.MiasmaBolt",
        "EnemyAbility.SwordWraithDash",
        "EnemyAbility.StoneFiendSlam",
    )
    for ability in required_enemy_abilities:
        if f"W11_ENEMY_ATTACK_ACTIVE" not in observe.text or f"Ability={ability}" not in observe.text:
            fail(f"{observe.label} did not observe an active {ability} attack")
    if int(observe.summary.get("AbilityUses", "-1")) != 0 or int(observe.summary.get("Dodges", "-1")) != 0:
        fail(f"{observe.label} must observe without attacking or dodging")

    miasma = evasion.enemies.get("Enemy.MiasmaWisp")
    if not miasma or miasma[0] < 2 or miasma[1] >= miasma[0]:
        fail(f"{evasion.label} did not evade at least one of two MiasmaWisp attacks: {miasma}")
    if "W11_ENEMY_PROJECTILE_EVADED" not in evasion.text:
        fail(f"{evasion.label} has no authoritative projectile-expiry evasion event")
    if int(evasion.summary.get("AbilityUses", "-1")) != 0 or int(evasion.summary.get("Dodges", "-1")) != 0:
        fail(f"{evasion.label} must use movement only")

    if int(dodge.summary.get("Dodges", "0")) <= 0 or not re.search(
        r"Ability=EnemyAbility\.SwordWraithDash[^\n]*Immune=1", dodge.text
    ):
        fail(f"{dodge.label} has no immune SwordWraithDash dodge")

    if int(recovery.summary.get("RecoveryPunishHits", "0")) <= 0:
        fail(f"{recovery.label} has no authoritative recovery punish hit")
    if "W11_RECOVERY_PUNISH_HIT" not in recovery.text:
        fail(f"{recovery.label} has no recovery punish event detail")

    feedback_runs = [run for run in all_runs if run.role == "Feedback"]
    if not feedback_runs:
        fail("manual evidence must include at least one dedicated Feedback role log")
    feedback_text = "\n".join(run.text for run in feedback_runs)
    sects = set().union(*(run.sects for run in all_runs))
    outcomes = {run.summary.get("Outcome") for run in all_runs}
    if None in outcomes:
        fail("every manual evidence summary must declare Outcome")
    if len(sects) < 3:
        fail(f"manual evidence must cover at least three sects, found {sorted(sects)}")
    if not {"1", "2"}.issubset(outcomes):
        fail(f"manual evidence must cover victory and defeat, found outcomes {sorted(outcomes)}")

    required_feedback = {
        "basic attack": r"W11_ABILITY_EFFECT[^\n]*Ability=Ability\.BasicAttack",
        "multi-target area ability": r"W11_ABILITY_EXECUTED[^\n]*Slot=[1-4][^\n]*Targets=([2-9]|[1-9][0-9]+)",
        "actual healing": r"W11_ABILITY_EFFECT[^\n]*Type=1[^\n]*Actual=(?!0\.0)([0-9.]+)",
        "actual barrier grant": r"W11_ABILITY_EFFECT[^\n]*Type=2[^\n]*Actual=(?!0\.0)([0-9.]+)",
        "critical hit": r"W11_ABILITY_EFFECT[^\n]*Critical=1",
        "barrier absorption": r"W11_ENEMY_(?:ATTACK|PROJECTILE)_HIT[^\n]*Barrier=(?!0\.0)([0-9.]+)",
        "enemy defeat": r"W11_ABILITY_EFFECT[^\n]*Defeated=1",
    }
    for label, pattern in required_feedback.items():
        if not re.search(pattern, feedback_text):
            fail(f"Feedback role evidence has no {label}")

    return sects, outcomes


def synthetic_self_test():
    clean = (
        " DuplicateRewards=0 GhostDamage=0 NoWarningDamage=0 "
        "UnmatchedWarnings=0 Stuck=0"
    )
    player = lambda sect: f"W11_COMBAT_SUMMARY_PLAYER EncounterInstance=1 Player=256 Sect={sect}\n"
    session = lambda role: (
        f"W11_MANUAL_EVIDENCE_SESSION Role={role} Seed={EXPECTED_SEED} "
        f"Schema={EXPECTED_SCHEMA} InputScript=None Interactive=1 Rendered=1 "
        "Audio=1 AutoStart=0 NetMode=0 TestOverride=0\n"
    )
    summary = lambda outcome, uses, dodges, recovery=0: (
        f"W11_COMBAT_SUMMARY Schema={EXPECTED_SCHEMA} Seed={EXPECTED_SEED} InputScript=None Outcome=" + str(outcome)
        + f" AbilityUses={uses} Dodges={dodges} RecoveryPunishHits={recovery}" + clean + "\n"
    )
    observe_text = session("Observe") + player("Sect.ThunderManor") + "".join(
        f"W11_ENEMY_ATTACK_ACTIVE Ability={ability}\n"
        for ability in ("EnemyAbility.MiasmaBolt", "EnemyAbility.SwordWraithDash", "EnemyAbility.StoneFiendSlam")
    ) + summary(2, 0, 0)
    evasion_text = session("Evasion") + player("Sect.ShadowMoonTower") + (
        "W11_ENEMY_PROJECTILE_EVADED EncounterInstance=1 AttackInstance=1 Ability=EnemyAbility.MiasmaBolt\n"
        "W11_COMBAT_SUMMARY_ENEMY EncounterInstance=1 Enemy=Enemy.MiasmaWisp "
        "Spawned=1 Attacks=3 Hits=1 Cancelled=0\n"
    ) + summary(2, 0, 0)
    dodge_text = session("Dodge") + player("Sect.CanglanPalace") + (
        "W11_ENEMY_ATTACK_HIT Ability=EnemyAbility.SwordWraithDash Barrier=10.0 Immune=1\n"
        "W11_ABILITY_EFFECT Type=2 Ability=Ability.DarkWaterWard Actual=35.0\n"
    ) + summary(2, 1, 1)
    recovery_text = session("Recovery") + player("Sect.DanxiaValley") + (
        "W11_RECOVERY_PUNISH_HIT HealthDamage=20.0\n"
        "W11_ABILITY_EFFECT Type=0 Ability=Ability.BasicAttack Actual=20.0 Critical=0 Defeated=0\n"
    ) + summary(1, 1, 0, 1)
    feedback_text = session("Feedback") + player("Sect.ThunderManor") + (
        "W11_ABILITY_EFFECT Type=0 Ability=Ability.BasicAttack Actual=20.0 Critical=1 Defeated=1\n"
        "W11_ABILITY_EFFECT Type=1 Ability=Ability.SpringRenewal Actual=30.0\n"
        "W11_ABILITY_EFFECT Type=2 Ability=Ability.DarkWaterWard Actual=35.0\n"
        "W11_ABILITY_EXECUTED Slot=1 Ability=Ability.FiveThunder Targets=3\n"
        "W11_ENEMY_ATTACK_HIT Ability=EnemyAbility.StoneFiendSlam Barrier=10.0 Immune=0\n"
    ) + summary(1, 4, 0)
    runs = [
        parse_log("self-observe", observe_text, "Observe"),
        parse_log("self-evasion", evasion_text, "Evasion"),
        parse_log("self-dodge", dodge_text, "Dodge"),
        parse_log("self-recovery", recovery_text, "Recovery"),
        parse_log("self-feedback", feedback_text, "Feedback"),
    ]
    sects, outcomes = require_roles(runs[0], runs[1], runs[2], runs[3], runs)
    print("W11_MANUAL_OBJECTIVE_GATE_SELF_TEST_PASSED")
    return runs, sects, outcomes


def load_and_validate_evidence(observe, evasion, dodge, recovery, extras):
    if not all((observe, evasion, dodge, recovery)):
        fail("--observe, --evasion, --dodge and --recovery are all required")
    if not extras:
        fail("at least one --extra Feedback role log is required")
    paths = [observe, evasion, dodge, recovery, *extras]
    resolved_paths = [path.resolve() for path in paths]
    if len(set(resolved_paths)) != len(resolved_paths):
        fail("each manual evidence role must use a unique log file")
    expected_roles = ["Observe", "Evasion", "Dodge", "Recovery"] + ["Feedback"] * len(extras)
    runs = [
        parse_log(str(path), path.read_text(encoding="utf-8", errors="replace"), role)
        for path, role in zip(paths, expected_roles)
    ]
    sects, outcomes = require_roles(runs[0], runs[1], runs[2], runs[3], runs)
    return runs, sects, outcomes


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Aggregate objective W11 manual-play evidence. This gate rejects internal input scripts "
            "and does not claim subjective comfort, readability, audio, or feel approval."
        )
    )
    parser.add_argument("--observe", type=Path)
    parser.add_argument("--evasion", type=Path)
    parser.add_argument("--dodge", type=Path)
    parser.add_argument("--recovery", type=Path)
    parser.add_argument("--extra", action="append", type=Path, default=[])
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        synthetic_self_test()
        return
    runs, sects, outcomes = load_and_validate_evidence(
        args.observe, args.evasion, args.dodge, args.recovery, args.extra
    )
    print(
        "W11_MANUAL_OBJECTIVE_GATE_PASSED: "
        f"Runs={len(runs)} Sects={','.join(sorted(sects))} Outcomes={','.join(sorted(outcomes))} "
        f"Seed={EXPECTED_SEED} Schema={EXPECTED_SCHEMA} InputScript=None "
        "ObjectiveCoverage=Observe,Evasion,Dodge,Recovery,Feedback"
    )


if __name__ == "__main__":
    main()
