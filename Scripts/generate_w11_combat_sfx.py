"""Generate deterministic, original prototype combat SFX for the W11 vertical slice."""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "SourceArt" / "W11" / "CombatAudio"
RATE = 48_000


def envelope(t: float, duration: float, attack: float = 0.008) -> float:
    return min(1.0, t / attack) * max(0.0, 1.0 - t / duration) ** 2


def write_sound(name: str, duration: float, sample_fn) -> None:
    rng = random.Random(f"W11-M4-{name}")
    samples = []
    for index in range(int(RATE * duration)):
        t = index / RATE
        samples.append(sample_fn(t, duration, rng) * envelope(t, duration))
    peak = max(0.001, max(abs(value) for value in samples))
    scale = 0.88 / peak
    pcm = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, value * scale)) * 32767)) for value in samples)
    path = OUTPUT / name
    with wave.open(str(path), "wb") as stream:
        stream.setnchannels(1)
        stream.setsampwidth(2)
        stream.setframerate(RATE)
        stream.writeframes(pcm)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    write_sound("W11_BasicAttack.wav", 0.22, lambda t, d, r:
                0.55 * math.sin(2 * math.pi * (780 - 560 * t / d) * t) + 0.24 * r.uniform(-1, 1))
    write_sound("W11_AbilityCast.wav", 0.48, lambda t, d, r:
                0.45 * math.sin(2 * math.pi * (330 + 520 * t / d) * t)
                + 0.28 * math.sin(2 * math.pi * 660 * t) + 0.05 * r.uniform(-1, 1))
    write_sound("W11_EnemyAttack.wav", 0.34, lambda t, d, r:
                0.42 * math.sin(2 * math.pi * (210 - 80 * t / d) * t) + 0.32 * r.uniform(-1, 1))
    # Three enemy signatures deliberately occupy different duration/frequency regions:
    # airy high miasma, short metallic sword sweep, and a long low stone impact.
    write_sound("W11_EnemyMiasma.wav", 0.58, lambda t, d, r:
                0.22 * math.sin(2 * math.pi * (520 + 760 * t / d) * t)
                + 0.18 * math.sin(2 * math.pi * 920 * t)
                + 0.34 * r.uniform(-1, 1) * (0.35 + 0.65 * t / d))
    write_sound("W11_EnemySwordDash.wav", 0.27, lambda t, d, r:
                0.48 * math.sin(2 * math.pi * (1820 - 1180 * t / d) * t)
                + 0.30 * r.uniform(-1, 1) * (1.0 - 0.55 * t / d))
    write_sound("W11_EnemyStoneSlam.wav", 0.64, lambda t, d, r:
                0.58 * math.sin(2 * math.pi * (92 - 34 * t / d) * t)
                + 0.24 * math.sin(2 * math.pi * 184 * t)
                + 0.18 * r.uniform(-1, 1) * max(0.0, 1.0 - 2.5 * t / d))
    write_sound("W11_Hit.wav", 0.18, lambda t, d, r:
                0.60 * r.uniform(-1, 1) + 0.42 * math.sin(2 * math.pi * 105 * t))
    write_sound("W11_Critical.wav", 0.42, lambda t, d, r:
                0.42 * math.sin(2 * math.pi * 880 * t) + 0.34 * math.sin(2 * math.pi * 1320 * t))
    write_sound("W11_Defeat.wav", 0.85, lambda t, d, r:
                0.50 * math.sin(2 * math.pi * (112 - 42 * t / d) * t)
                + 0.25 * math.sin(2 * math.pi * 224 * t) + 0.04 * r.uniform(-1, 1))
    print(f"W11_COMBAT_SFX_GENERATED count=9 rate={RATE} output={OUTPUT}")


if __name__ == "__main__":
    main()
