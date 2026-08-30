import argparse
import struct
import sys
from pathlib import Path


SESSION = (
    "W11_MANUAL_EVIDENCE_SESSION Role=Observe Seed=424242 Schema=3 InputScript=None "
    "Interactive=1 Rendered=1 Audio=1 AutoStart=0 NetMode=0 TestOverride=0"
)
ENTRY_GUIDE = (
    "W11_MANUAL_EVIDENCE_GUIDE_READY Surface=EntryMenu "
    "Role=Observe Accepted=1 Reason=None"
)
HUD_GUIDE = (
    "W11_MANUAL_EVIDENCE_GUIDE_READY Surface=CombatHUD "
    "Role=Observe Accepted=1 Reason=None"
)


def fail(message):
    print(f"W11_MANUAL_EVIDENCE_GUIDE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def read_png_size(path):
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        fail(f"{path} is not a valid PNG with an IHDR header")
    return struct.unpack(">II", data[16:24])


def main():
    parser = argparse.ArgumentParser(description="Verify the visible W11 manual-evidence guide capture.")
    parser.add_argument("log", type=Path)
    parser.add_argument("screenshot", type=Path)
    args = parser.parse_args()

    text = args.log.read_text(encoding="utf-8", errors="replace")
    for marker in (SESSION, ENTRY_GUIDE, HUD_GUIDE, "W11_ORNATE_MENU_SCREENSHOT_REQUESTED"):
        if text.count(marker) != 1:
            fail(f"expected exactly one marker: {marker}")
    if "W11_MANUAL_EVIDENCE_SESSION_REJECTED" in text:
        fail("capture session was rejected")
    width, height = read_png_size(args.screenshot)
    if (width, height) != (1280, 720):
        fail(f"expected 1280x720 capture, found {width}x{height}")

    print(
        "W11_MANUAL_EVIDENCE_GUIDE_PASSED: Role=Observe Surfaces=EntryMenu,CombatHUD "
        f"Capture={width}x{height}"
    )


if __name__ == "__main__":
    main()
