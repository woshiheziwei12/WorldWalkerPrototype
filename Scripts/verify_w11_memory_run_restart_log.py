import argparse
import re
import sys
from pathlib import Path


def fail(message):
    print(f"W11_MEMORY_RUN_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def one(pattern, text, label):
    matches = re.findall(pattern, text)
    if len(matches) != 1:
        fail(f"expected exactly one {label}, found {len(matches)}")
    return matches[0]


def main():
    parser = argparse.ArgumentParser(
        description="Verify W11 defeat-to-new-memory-run reset and cross-run authority identity."
    )
    parser.add_argument("log", type=Path)
    args = parser.parse_args()

    text = args.log.read_text(encoding="utf-8", errors="replace")
    boundary = one(
        r"W11_MEMORY_RUN_RESET_BOUNDARY CompletedRun=(\d+) NextRun=(\d+) "
        r"Seed=(\d+) RuleSet=([^ ]+) EncounterInstance=(\d+) Outcome=(\d+) Listener=(\d+)",
        text,
        "reset boundary",
    )
    completed, next_run, seed, _, encounter, outcome, _ = boundary
    if (completed, next_run, seed, encounter, outcome) != ("1", "2", "424242", "1", "2"):
        fail(f"unexpected reset boundary: {boundary}")

    reset = one(
        r"W11_NEW_MEMORY_RUN_PLAYER_RESET Run=(\d+) Player=(\d+) Hero=([^ ]+) "
        r"Sect=([^ ]+) ActiveSlots=(\d+) Equipped=(\d+) Unlocked=(\d+) "
        r"SpiritStones=(\d+) CultivationLevel=(\d+) Manuals=(\d+) Treasures=(\d+) "
        r"Pending=(\d+) BasicCooldown=([\d.]+) DodgeCooldown=([\d.-]+) DamageImmune=(\d+)",
        text,
        "player reset payload",
    )
    expected_reset = ("2", "256", "None", "None", "4", "0", "0", "0", "1", "0", "0", "0")
    if reset[:12] != expected_reset:
        fail(f"new run retained character or economy construction: {reset}")
    if float(reset[12]) != 0.0 or float(reset[13]) > 0.0 or reset[14] != "0":
        fail(f"new run retained cooldown or immunity state: {reset[12:]}")

    ready = one(
        r"W11_NEW_MEMORY_RUN_READY Run=(\d+) Seed=(\d+) Phase=Lobby "
        r"EncounterSequence=(\d+) OutcomeSequence=(\d+)",
        text,
        "new-run ready payload",
    )
    if ready != ("2", "424242", "1", "1"):
        fail(f"unexpected ready identity: {ready}")

    stage_starts = re.findall(
        r"W11_COMBAT_STAGE_STARTED Stage=(\d+) EncounterInstance=(\d+)", text
    )
    if len(stage_starts) < 2 or stage_starts[:2] != [("4", "1"), ("4", "2")]:
        fail(f"second memory run did not advance the encounter identity: {stage_starts}")
    if text.index("W11_NEW_MEMORY_RUN_READY") > text.rindex("EncounterInstance=2"):
        fail("the second encounter appeared before the new-run ready boundary")

    for marker in ("Fatal error:", "Assertion failed:", "Ensure condition failed"):
        if marker in text:
            fail(f"runtime log contains {marker}")

    print(
        "W11_MEMORY_RUN_GATE_PASSED: "
        f"Run={completed}->{next_run} Encounter=1->2 ResetPlayer={reset[1]} "
        "Equipped=0 Economy=0 Cooldowns=0 Immunity=0"
    )


if __name__ == "__main__":
    main()
