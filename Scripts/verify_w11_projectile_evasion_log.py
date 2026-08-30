import argparse
import re
import sys
from pathlib import Path


ATTACK_ACTIVE = re.compile(
    r"W11_ENEMY_ATTACK_ACTIVE EncounterInstance=(\d+) SpawnId=(\d+) "
    r"AttackInstance=(\d+) Ability=EnemyAbility\.MiasmaBolt Delivery=1"
)
PROJECTILE_HIT = re.compile(
    r"W11_ENEMY_PROJECTILE_HIT EncounterInstance=(\d+) AttackInstance=(\d+) "
    r"Ability=EnemyAbility\.MiasmaBolt"
)
PROJECTILE_EVADED = re.compile(
    r"W11_ENEMY_PROJECTILE_EVADED EncounterInstance=(\d+) AttackInstance=(\d+) "
    r"Ability=EnemyAbility\.MiasmaBolt"
)
MOVE_LEG = re.compile(
    r"W11_SCRIPT_MOVE_EVASION_LEG Step=(\d+) Direction=([-\d.]+),([-\d.]+) "
    r"Location=([-\d.]+),([-\d.]+) Speed=([-\d.]+) FrameDelta=([-\d.]+)"
)


def fail(message: str) -> None:
    print(f"W11_PROJECTILE_EVASION_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Verify atomic W11 movement-only projectile evasion evidence."
    )
    parser.add_argument("log", type=Path)
    args = parser.parse_args()

    if not args.log.is_file():
        fail(f"log not found: {args.log}")
    text = args.log.read_text(encoding="utf-8", errors="replace")

    if "W11_ARCHITECTURE_READY Schema=3 Seed=424242" not in text:
        fail("missing fixed-seed schema-3 architecture marker")
    if "W11_INPUT_SCRIPT_SELECTED Script=MoveEvasionLoop Seed=424242" not in text:
        fail("MoveEvasionLoop was not selected with the fixed seed")
    if "-BENCHMARK" not in text or not re.search(r"-FPS=60(?:\s|\"|$)", text):
        fail("headless movement evidence must use a fixed 60 FPS benchmark step")

    active_pairs = {(int(encounter), int(attack)) for encounter, _, attack in ATTACK_ACTIVE.findall(text)}
    hit_pairs = {(int(encounter), int(attack)) for encounter, attack in PROJECTILE_HIT.findall(text)}
    evaded_sequence = [
        (int(encounter), int(attack)) for encounter, attack in PROJECTILE_EVADED.findall(text)
    ]
    evaded_pairs = set(evaded_sequence)
    if len(active_pairs) < 2:
        fail(f"expected at least two active miasma projectiles, found {len(active_pairs)}")
    if not evaded_pairs:
        fail("no projectile completed its lifetime without hitting")
    if len(evaded_pairs) != len(evaded_sequence):
        fail("a projectile resolution was recorded as evaded more than once")
    if not evaded_pairs.issubset(active_pairs):
        fail(f"evaded projectile lacks an active attack record: {sorted(evaded_pairs - active_pairs)}")
    overlap = evaded_pairs & hit_pairs
    if overlap:
        fail(f"projectile attack ids were resolved as both hit and evaded: {sorted(overlap)}")

    move_legs = MOVE_LEG.findall(text)
    if len(move_legs) < 2:
        fail(f"expected at least two movement trajectory samples, found {len(move_legs)}")
    positions = {(round(float(x), 1), round(float(y), 1)) for _, _, _, x, y, _, _ in move_legs}
    if len(positions) < 2:
        fail("movement trajectory did not change position")
    for _, _, _, _, _, speed, frame_delta in move_legs:
        if float(speed) <= 0.0:
            fail("movement trajectory contains a zero-speed sample")
        if not 0.015 <= float(frame_delta) <= 0.018:
            fail(f"movement trajectory used non-60-FPS frame delta {frame_delta}")

    forbidden_markers = (
        "W11_DODGE_EXECUTED",
        "W11_DODGE_IMMUNITY",
        "W11_ABILITY_EXECUTED",
        "W11_BASIC_ATTACK_EXECUTED",
        "Fatal error:",
        "Assertion failed:",
        "Ensure condition failed",
    )
    for marker in forbidden_markers:
        if marker in text:
            fail(f"forbidden marker present: {marker}")

    print(
        "W11_PROJECTILE_EVASION_GATE_PASSED: "
        f"active={len(active_pairs)} hit={len(hit_pairs)} evaded={len(evaded_pairs)} "
        f"move_samples={len(move_legs)}"
    )


if __name__ == "__main__":
    main()
