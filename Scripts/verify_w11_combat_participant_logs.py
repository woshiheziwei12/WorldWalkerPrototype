import argparse
import re
import sys
from pathlib import Path


PARTICIPANTS = re.compile(
    r"W11_COMBAT_NODE_PARTICIPANTS EncounterInstance=(\d+) Outcome=(\d+) "
    r"Defeated=\[([^\]]*)\] Surviving=\[([^\]]*)\]"
)


def fail(message):
    print(f"W11_COMBAT_PARTICIPANTS_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_ids(raw):
    if not raw:
        return []
    values = [int(value) for value in raw.split(",")]
    if values != sorted(set(values)):
        fail(f"participant ids are not unique and sorted: {values}")
    return values


def read_one(path, expected_outcome):
    text = path.read_text(encoding="utf-8", errors="replace")
    matches = PARTICIPANTS.findall(text)
    if len(matches) != 1:
        fail(f"{path} should contain exactly one participant payload, found {len(matches)}")
    _, outcome, defeated_raw, surviving_raw = matches[0]
    if int(outcome) != expected_outcome:
        fail(f"{path} has Outcome={outcome}, expected {expected_outcome}")
    defeated = parse_ids(defeated_raw)
    surviving = parse_ids(surviving_raw)
    if set(defeated) & set(surviving):
        fail(f"{path} places a player in both participant sets")
    if "DuplicateRewards=0 GhostDamage=0 NoWarningDamage=0" not in text:
        fail(f"{path} has no clean terminal anomaly summary")
    return defeated, surviving


def main():
    parser = argparse.ArgumentParser(
        description="Verify authority-generated W11 victory and defeat participant payloads."
    )
    parser.add_argument("victory_log", type=Path)
    parser.add_argument("defeat_log", type=Path)
    args = parser.parse_args()

    victory_defeated, victory_surviving = read_one(args.victory_log, 1)
    defeat_defeated, defeat_surviving = read_one(args.defeat_log, 2)
    if victory_defeated or not victory_surviving:
        fail("victory smoke must contain a survivor and no defeated player")
    if not defeat_defeated or defeat_surviving:
        fail("defeat smoke must contain a defeated player and no survivor")

    print(
        "W11_COMBAT_PARTICIPANTS_GATE_PASSED: "
        f"VictoryDefeated={victory_defeated} VictorySurviving={victory_surviving} "
        f"DefeatDefeated={defeat_defeated} DefeatSurviving={defeat_surviving}"
    )


if __name__ == "__main__":
    main()
