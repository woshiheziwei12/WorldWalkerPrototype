import argparse
import hashlib
import json
import re
import sys
from pathlib import Path


MATRIX = (
    ("Hero.Wei", "Sect.CanglanPalace"),
    ("Hero.Dong", "Sect.DanxiaValley"),
    ("Hero.Tian", "Sect.ThunderManor"),
    ("Hero.Xiang", "Sect.AzureCloudSword"),
    ("Hero.Pu", "Sect.TianjiFormation"),
    ("Hero.Ying", "Sect.DanxiaValley"),
    ("Hero.Li", "Sect.TianjiFormation"),
    ("Hero.Jia", "Sect.ThunderManor"),
    ("Hero.Yu", "Sect.ShadowMoonTower"),
    ("Hero.Meng", "Sect.AzureCloudSword"),
)
KEY_VALUE = re.compile(r"([A-Za-z][A-Za-z0-9]*)=([^ ]+)")
BLOCKING_ERRORS = (
    "Fatal error:",
    "Assertion failed:",
    "Ensure condition failed:",
    "LogWindows: Error:",
)


class GateError(RuntimeError):
    pass


def parse_payload(line: str, marker: str) -> dict[str, str]:
    return dict(KEY_VALUE.findall(line.split(marker, 1)[1]))


def exactly_one(lines: list[str], marker: str) -> dict[str, str]:
    matches = [parse_payload(line, marker) for line in lines if marker in line]
    if len(matches) != 1:
        raise GateError(f"expected exactly one {marker.strip()}, found {len(matches)}")
    return matches[0]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_log(path: Path, expected_hero: str, expected_sect: str, seed: int) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    for error in BLOCKING_ERRORS:
        if error.lower() in text.lower():
            raise GateError(f"{path.name}: blocking engine error marker {error!r}")

    fallback = exactly_one(lines, "W11_CHARACTER_SELECTION_FALLBACK ")
    initializations = [
        parse_payload(line, "W11_PLAYER_INITIALIZED ")
        for line in lines
        if "W11_PLAYER_INITIALIZED " in line
    ]
    animation = exactly_one(lines, "W11_CHARACTER_ANIMATION_LOADED ")
    player_summary = exactly_one(lines, "W11_COMBAT_SUMMARY_PLAYER ")
    summary = exactly_one(lines, "W11_COMBAT_SUMMARY ")
    auto_quit = exactly_one(lines, "W11_AUTO_QUIT_ON_OUTCOME ")

    for label, payload in (("fallback", fallback), ("player summary", player_summary)):
        if payload.get("Hero") != expected_hero or payload.get("Sect") != expected_sect:
            raise GateError(
                f"{path.name}: {label} selected {payload.get('Hero')}/{payload.get('Sect')}, "
                f"expected {expected_hero}/{expected_sect}"
            )
    selected_initializations = [
        payload
        for payload in initializations
        if payload.get("Hero") == expected_hero and payload.get("Sect") == expected_sect
    ]
    if len(selected_initializations) != 1:
        raise GateError(
            f"{path.name}: expected one selected-player initialization, "
            f"found {len(selected_initializations)}"
        )
    invalid_initializations = [
        payload
        for payload in initializations
        if payload.get("Hero") not in {"None", expected_hero}
        or payload.get("Sect") not in {"None", expected_sect}
    ]
    if invalid_initializations:
        raise GateError(f"{path.name}: unexpected player initialization identity")
    if animation.get("Hero") != expected_hero:
        raise GateError(
            f"{path.name}: animation loaded for {animation.get('Hero')}, expected {expected_hero}"
        )
    if summary.get("Seed") != str(seed):
        raise GateError(f"{path.name}: Seed={summary.get('Seed')}, expected {seed}")
    if summary.get("InputScript") != "PacingCombatLoop" or summary.get("Stage") != "1":
        raise GateError(f"{path.name}: expected Stage 1 PacingCombatLoop summary")
    if summary.get("Outcome") not in {"1", "2"}:
        raise GateError(f"{path.name}: non-terminal Outcome={summary.get('Outcome')}")
    if auto_quit.get("Outcome") != summary.get("Outcome"):
        raise GateError(f"{path.name}: auto-quit outcome does not match combat summary")

    duration = float(summary.get("Duration", "-1"))
    if duration <= 0.0:
        raise GateError(f"{path.name}: invalid Duration={duration}")
    if int(summary.get("Spawned", "0")) <= 0:
        raise GateError(f"{path.name}: no enemies were spawned")
    if int(summary.get("AbilityUses", "0")) <= 0:
        raise GateError(f"{path.name}: no authoritative player abilities executed")
    for key in (
        "DuplicateRewards",
        "GhostDamage",
        "NoWarningDamage",
        "UnmatchedWarnings",
        "Stuck",
    ):
        if int(summary.get(key, "-1")) != 0:
            raise GateError(f"{path.name}: {key}={summary.get(key)}")
    if summary["Outcome"] == "1" and int(summary.get("RewardGrants", "0")) != 1:
        raise GateError(f"{path.name}: victory must grant exactly one reward")
    if summary["Outcome"] == "2" and int(summary.get("RewardGrants", "0")) != 0:
        raise GateError(f"{path.name}: defeat must not grant a reward")

    return {
        "hero": expected_hero,
        "sect": expected_sect,
        "log": path.name,
        "sha256": sha256(path),
        "outcome": "Victory" if summary["Outcome"] == "1" else "Defeat",
        "durationSeconds": duration,
        "health": summary.get("Health"),
        "damageDealt": float(summary.get("DamageDealt", "0")),
        "damageTaken": float(summary.get("DamageTaken", "0")),
        "abilityUses": int(summary.get("AbilityUses", "0")),
        "abilityFailures": int(summary.get("AbilityFailures", "0")),
        "dodges": int(summary.get("Dodges", "0")),
        "dodgeImmunities": int(summary.get("DodgeImmunities", "0")),
    }


def verify(directory: Path, output: Path, seed: int) -> dict:
    if not directory.is_dir():
        raise GateError(f"session directory does not exist: {directory}")
    entries = []
    expected_names = set()
    for hero, sect in MATRIX:
        short_hero = hero.split(".", 1)[1]
        name = f"{short_hero}.log"
        expected_names.add(name)
        path = directory / name
        if not path.is_file():
            raise GateError(f"missing hero log: {path}")
        entries.append(verify_log(path, hero, sect, seed))

    actual_names = {path.name for path in directory.glob("*.log")}
    extras = sorted(actual_names - expected_names)
    if extras:
        raise GateError(f"unexpected log files in session: {extras}")
    if len({entry["hero"] for entry in entries}) != len(MATRIX):
        raise GateError("hero identities are not unique")
    if len({entry["sha256"] for entry in entries}) != len(MATRIX):
        raise GateError("two or more hero logs are byte-identical")
    covered_sects = sorted({entry["sect"] for entry in entries})
    if len(covered_sects) != 6:
        raise GateError(f"expected all six sects, got {covered_sects}")

    victories = sum(entry["outcome"] == "Victory" for entry in entries)
    evidence = {
        "schemaVersion": 1,
        "evidenceType": "W11AllHeroesAutomatedCombatSmoke",
        "status": "PASSED",
        "manualAcceptance": False,
        "runSeed": seed,
        "inputScript": "PacingCombatLoop",
        "heroesExpected": len(MATRIX),
        "heroesPassed": len(entries),
        "sectsCovered": covered_sects,
        "victories": victories,
        "defeats": len(entries) - victories,
        "minimumDurationSeconds": min(entry["durationSeconds"] for entry in entries),
        "maximumDurationSeconds": max(entry["durationSeconds"] for entry in entries),
        "entries": entries,
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return evidence


def main() -> None:
    parser = argparse.ArgumentParser(description="Verify the ten-hero W11 automated combat matrix.")
    parser.add_argument("session_directory", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--seed", type=int, default=424242)
    args = parser.parse_args()
    output = args.output or args.session_directory / "W11AllHeroesEvidence.json"
    try:
        evidence = verify(args.session_directory.resolve(), output.resolve(), args.seed)
    except (GateError, OSError, ValueError) as error:
        print(f"W11_ALL_HEROES_GATE_FAILED: {error}", file=sys.stderr)
        raise SystemExit(1)
    print(
        "W11_ALL_HEROES_GATE_PASSED: "
        f"Heroes={evidence['heroesPassed']}/{evidence['heroesExpected']} "
        f"Sects={len(evidence['sectsCovered'])}/6 "
        f"Victories={evidence['victories']} Defeats={evidence['defeats']} "
        f"Duration={evidence['minimumDurationSeconds']:.3f}-"
        f"{evidence['maximumDurationSeconds']:.3f}s Output={output.resolve()}"
    )


if __name__ == "__main__":
    main()
