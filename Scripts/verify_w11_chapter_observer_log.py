import argparse
import re
import sys
from pathlib import Path


def fail(message):
    print(f"W11_CHAPTER_OBSERVER_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Verify that W11 combat-node observers do not implicitly own chapter transitions."
    )
    parser.add_argument("log", type=Path)
    parser.add_argument("--expect-owner", action="store_true")
    args = parser.parse_args()
    text = args.log.read_text(encoding="utf-8", errors="replace")

    expected_once = [
        "W11_COMBAT_NODE_OBSERVER_SMOKE_BOUND OwnershipClaimed=0",
        "W11_COMBAT_NODE_OBSERVER_SMOKE_RECEIVED Run=1 EncounterInstance=1 Outcome=2",
    ]
    if args.expect_owner:
        expected_once.extend((
            "W11_CHAPTER_TRANSITION_OWNERSHIP_CLAIMED Owner=W11GameMode",
            "W11_COMBAT_NODE_TRANSITION_DECISION EncounterInstance=1 "
            "ExplicitOwner=1 Observers=1 OwnsTransition=1",
            "W11_COMBAT_NODE_DEFEAT_DESTINATION EncounterInstance=1 "
            "Destination=ChapterTransition",
        ))
    else:
        expected_once.extend((
            "W11_COMBAT_NODE_TRANSITION_DECISION EncounterInstance=1 "
            "ExplicitOwner=0 Observers=1 OwnsTransition=0",
            "W11_NEW_MEMORY_RUN_READY Run=2 Seed=424242 Phase=Lobby "
            "EncounterSequence=1 OutcomeSequence=1",
        ))
    for marker in expected_once:
        count = text.count(marker)
        if count != 1:
            fail(f"expected one '{marker}', found {count}")

    if args.expect_owner:
        if "W11_NEW_MEMORY_RUN_READY" in text:
            fail("an explicit chapter owner incorrectly fell through to the memory-run restart")
    else:
        if "W11_COMBAT_NODE_HANDED_OFF" in text:
            fail("an observation-only listener incorrectly captured the chapter transition")
        if re.search(r"W11_CHAPTER_TRANSITION_OWNERSHIP_(CLAIMED|REJECTED)", text):
            fail("observer smoke unexpectedly claimed transition ownership")
    for marker in ("Fatal error:", "Assertion failed:", "Ensure condition failed"):
        if marker in text:
            fail(f"runtime log contains {marker}")

    if args.expect_owner:
        print(
            "W11_CHAPTER_OBSERVER_GATE_PASSED: "
            "Observers=1 ExplicitOwner=1 OwnsTransition=1 Destination=ChapterTransition"
        )
    else:
        print(
            "W11_CHAPTER_OBSERVER_GATE_PASSED: "
            "Observers=1 ExplicitOwner=0 OwnsTransition=0 NewMemoryRun=2"
        )


if __name__ == "__main__":
    main()
