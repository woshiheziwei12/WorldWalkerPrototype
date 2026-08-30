import argparse
import re
import sys
from pathlib import Path


HIT_STOP = re.compile(
    r"W11_LOCAL_HIT_STOP Actor=(Player|Enemy).*?Duration=([0-9.]+) "
    r"GlobalTimeDilation=([0-9.]+)"
)
CAMERA = re.compile(r"W11_LOCAL_CAMERA_IMPACT Strength=([0-9.]+) Duration=([0-9.]+) Cap=([0-9.]+)")


def fail(message):
    print(f"W11_IMPACT_PRESENTATION_GATE_FAILED: {message}", file=sys.stderr)
    raise SystemExit(1)


def contains_close(values, expected, tolerance=0.0005):
    return any(abs(value - expected) <= tolerance for value in values)


def main():
    parser = argparse.ArgumentParser(
        description="Verify W11 local hit-stop and camera-impact runtime evidence."
    )
    parser.add_argument("log", type=Path)
    args = parser.parse_args()

    text = args.log.read_text(encoding="utf-8", errors="replace")
    hit_stops = [(actor, float(duration), float(dilation)) for actor, duration, dilation in HIT_STOP.findall(text)]
    camera_impacts = [(float(strength), float(duration), float(cap)) for strength, duration, cap in CAMERA.findall(text)]
    if not hit_stops:
        fail("no local Flipbook hit-stop evidence was recorded")
    if not any(actor == "Player" for actor, _, _ in hit_stops):
        fail("no player incoming-hit stop was recorded")
    if not any(actor == "Enemy" for actor, _, _ in hit_stops):
        fail("no enemy hit stop was recorded")
    if any(abs(dilation - 1.0) > 0.0005 for _, _, dilation in hit_stops):
        fail("local presentation changed global time dilation")

    durations = [duration for _, duration, _ in hit_stops]
    for expected, tier in ((0.025, "normal"), (0.045, "critical"), (0.065, "defeat")):
        if not contains_close(durations, expected):
            fail(f"missing {tier} hit-stop tier {expected:.3f}s")
    if any(duration > 0.1205 for duration in durations):
        fail("a hit-stop duration exceeded the 0.12s hard cap")

    if not camera_impacts:
        fail("no local camera-impact evidence was recorded")
    strengths = [strength for strength, _, _ in camera_impacts]
    for expected, tier in ((1.5, "normal"), (3.5, "critical"), (5.0, "defeat")):
        if not contains_close(strengths, expected):
            fail(f"missing {tier} camera-impact tier {expected:.2f}")
    if any(strength > cap + 0.0005 or cap > 8.0005 for strength, _, cap in camera_impacts):
        fail("a camera impact exceeded its declared 8uu cap")

    if "W11_COMBAT_SUMMARY " not in text or "Duration=103.000 Outcome=1" not in text:
        fail("the presentation smoke did not preserve the fixed-seed 103s victory")

    print(
        "W11_IMPACT_PRESENTATION_GATE_PASSED: "
        f"HitStops={len(hit_stops)} PlayerStops={sum(a == 'Player' for a, _, _ in hit_stops)} "
        f"EnemyStops={sum(a == 'Enemy' for a, _, _ in hit_stops)} "
        f"CameraImpacts={len(camera_impacts)} GlobalTimeDilation=1.000"
    )


if __name__ == "__main__":
    main()
