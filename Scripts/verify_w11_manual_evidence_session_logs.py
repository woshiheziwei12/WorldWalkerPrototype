import argparse
import re
import sys
from pathlib import Path


ACCEPTED = re.compile(
    r"W11_MANUAL_EVIDENCE_SESSION Role=Observe Seed=424242 Schema=3 InputScript=None "
    r"Interactive=1 Rendered=1 Audio=1 AutoStart=0 NetMode=0 TestOverride=0"
)
SCRIPTED_REJECTED = re.compile(
    r"W11_MANUAL_EVIDENCE_SESSION_REJECTED Role=Recovery Seed=424242 Schema=3 "
    r"InputScript=RecoveryPunish Interactive=1 Rendered=1 Audio=1 AutoStart=0 NetMode=0 "
    r"TestOverride=0 Reason=ScriptedInput"
)
HEADLESS_REJECTED = re.compile(
    r"W11_MANUAL_EVIDENCE_SESSION_REJECTED Role=Observe Seed=424242 Schema=3 "
    r"InputScript=None Interactive=0 Rendered=0 Audio=1 AutoStart=0 NetMode=0 "
    r"TestOverride=0 Reason=Unattended"
)


def fail(message):
    print(f"W11_MANUAL_EVIDENCE_SESSION_SMOKE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Verify interactive acceptance plus scripted and headless manual-evidence rejection."
    )
    parser.add_argument("accepted", type=Path)
    parser.add_argument("scripted_rejected", type=Path)
    parser.add_argument("headless_rejected", type=Path)
    args = parser.parse_args()

    accepted_text = args.accepted.read_text(encoding="utf-8", errors="replace")
    scripted_text = args.scripted_rejected.read_text(encoding="utf-8", errors="replace")
    headless_text = args.headless_rejected.read_text(encoding="utf-8", errors="replace")
    if len(ACCEPTED.findall(accepted_text)) != 1:
        fail("accepted log does not contain exactly one Observe session marker")
    if "W11_MANUAL_EVIDENCE_SESSION_REJECTED" in accepted_text:
        fail("accepted log contains a rejection marker")
    if len(SCRIPTED_REJECTED.findall(scripted_text)) != 1:
        fail("scripted log does not contain exactly one scripted-input rejection marker")
    if len(HEADLESS_REJECTED.findall(headless_text)) != 1:
        fail("headless log does not contain exactly one unattended/NullRHI rejection marker")
    for label, text in (("scripted", scripted_text), ("headless", headless_text)):
        if "W11_MANUAL_EVIDENCE_SESSION Role=" in text:
            fail(f"{label} rejected log also contains an accepted marker")
    for label, text in (
        ("accepted", accepted_text), ("scripted", scripted_text), ("headless", headless_text)
    ):
        if "W11_ARCHITECTURE_READY Schema=3 Seed=424242" not in text:
            fail(f"{label} log does not use Schema=3 and Seed=424242")

    print(
        "W11_MANUAL_EVIDENCE_SESSION_SMOKE_PASSED: Accepted=InteractiveObserve "
        "Rejected=ScriptedInput,UnattendedNullRHI Seed=424242 Schema=3"
    )


if __name__ == "__main__":
    main()
