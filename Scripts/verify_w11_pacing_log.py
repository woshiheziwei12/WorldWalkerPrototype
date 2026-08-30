import argparse
import re
import sys
from pathlib import Path


SUMMARY_MARKER = "W11_COMBAT_SUMMARY "
KEY_VALUE = re.compile(r"([A-Za-z][A-Za-z0-9]*)=([^ ]+)")


def fail(message):
    print(f"W11_PACING_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(description="Verify the fixed-seed W11 first-combat pacing run.")
    parser.add_argument("log", type=Path)
    parser.add_argument("--minimum-seconds", type=float, default=45.0)
    parser.add_argument("--maximum-seconds", type=float, default=90.0)
    args = parser.parse_args()

    text = args.log.read_text(encoding="utf-8", errors="replace")
    summaries = []
    for line in text.splitlines():
        if SUMMARY_MARKER in line:
            summaries.append(dict(KEY_VALUE.findall(line.split(SUMMARY_MARKER, 1)[1])))
    pacing = [
        item for item in summaries
        if item.get("InputScript") == "PacingCombatLoop" and item.get("Stage") == "1"
    ]
    if len(pacing) != 1:
        fail(f"expected exactly one Stage 1 PacingCombatLoop summary, found {len(pacing)}")

    summary = pacing[0]
    duration = float(summary.get("Duration", "-1"))
    if not args.minimum_seconds <= duration <= args.maximum_seconds:
        fail(
            f"duration {duration:.3f}s is outside "
            f"[{args.minimum_seconds:.3f}, {args.maximum_seconds:.3f}]s"
        )
    if summary.get("Outcome") != "1":
        fail(f"expected victory Outcome=1, got {summary.get('Outcome')}")
    if float(summary.get("Health", "0/0").split("/", 1)[0]) <= 0.0:
        fail("victory has no remaining player health")
    if int(summary.get("Spawned", "0")) != 12:
        fail(f"fixed Seed 424242 should spawn 12 enemies, got {summary.get('Spawned')}")
    if int(summary.get("RewardGrants", "0")) != 1:
        fail(f"expected exactly one reward grant, got {summary.get('RewardGrants')}")

    for key in (
        "DuplicateRewards", "GhostDamage", "NoWarningDamage",
        "UnmatchedWarnings", "Stuck",
    ):
        if int(summary.get(key, "-1")) != 0:
            fail(f"{key} must be zero, got {summary.get(key)}")

    reinforcement_count = text.count("W11_WAVE_REINFORCEMENT_SPAWNED")
    if reinforcement_count != 7:
        fail(f"fixed Seed 424242 should refill seven slots, got {reinforcement_count}")
    if "WaveBudgetRemaining=0 RemainingBudget=0" not in text:
        fail("the current wave did not consume the complete encounter budget")

    print(
        "W11_PACING_GATE_PASSED: "
        f"Duration={duration:.3f}s Health={summary['Health']} Spawned={summary['Spawned']} "
        f"Reinforcements={reinforcement_count} RewardGrants={summary['RewardGrants']}"
    )


if __name__ == "__main__":
    main()
