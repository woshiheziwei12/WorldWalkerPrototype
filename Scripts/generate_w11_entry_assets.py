"""Build deterministic W11 title-screen source art and an original ambient loop."""

from pathlib import Path
import wave

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter, ImageOps


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "SourceArt" / "W11" / "Generated"
TITLE_SOURCE = SOURCE_ROOT / "W11_Title_Background_v1_source.png"
TITLE_OUTPUT = SOURCE_ROOT / "W11_Title_Background_v1.png"
FOG_OUTPUT = SOURCE_ROOT / "W11_Title_Fog_v1.png"
MUSIC_OUTPUT = SOURCE_ROOT / "W11_TaixuMoonGate_MenuTheme_v1.wav"


def build_title_background() -> None:
    image = Image.open(TITLE_SOURCE).convert("RGB")
    image = ImageOps.fit(image, (2560, 1440), method=Image.Resampling.LANCZOS)
    image = ImageEnhance.Sharpness(image).enhance(1.08)
    image.save(TITLE_OUTPUT, quality=96)


def build_fog_layer() -> None:
    rng = np.random.default_rng(110827)
    low_width, low_height = 128, 32
    noise = rng.random((low_height, low_width), dtype=np.float32)
    noise_image = Image.fromarray(np.uint8(noise * 255), mode="L")
    noise_image = noise_image.resize((2048, 512), Image.Resampling.BICUBIC)
    noise_image = noise_image.filter(ImageFilter.GaussianBlur(radius=34.0))
    alpha = np.asarray(noise_image, dtype=np.float32) / 255.0
    vertical = np.sin(np.linspace(0.0, np.pi, 512, dtype=np.float32))[:, None]
    alpha = np.clip((alpha - 0.30) * 0.54 * vertical, 0.0, 0.22)
    rgba = np.zeros((512, 2048, 4), dtype=np.uint8)
    rgba[..., 0] = 156
    rgba[..., 1] = 212
    rgba[..., 2] = 224
    rgba[..., 3] = np.uint8(alpha * 255)
    Image.fromarray(rgba, mode="RGBA").save(FOG_OUTPUT)


def add_note(track: np.ndarray, sample_rate: int, start: float, duration: float,
             frequency: float, amplitude: float, pan: float, bell: bool = False) -> None:
    first = int(start * sample_rate)
    count = min(int(duration * sample_rate), len(track) - first)
    if count <= 0:
        return
    local_time = np.arange(count, dtype=np.float64) / sample_rate
    if bell:
        envelope = np.exp(-1.15 * local_time) * np.minimum(local_time * 10.0, 1.0)
        signal = (
            np.sin(2.0 * np.pi * frequency * local_time)
            + 0.52 * np.sin(2.0 * np.pi * frequency * 2.01 * local_time)
            + 0.26 * np.sin(2.0 * np.pi * frequency * 3.98 * local_time)
        )
    else:
        envelope = np.exp(-0.72 * local_time) * np.minimum(local_time * 18.0, 1.0)
        signal = (
            np.sin(2.0 * np.pi * frequency * local_time)
            + 0.32 * np.sin(2.0 * np.pi * frequency * 2.0 * local_time)
            + 0.12 * np.sin(2.0 * np.pi * frequency * 3.0 * local_time)
        )
    left = np.sqrt((1.0 - pan) * 0.5)
    right = np.sqrt((1.0 + pan) * 0.5)
    track[first:first + count, 0] += amplitude * signal * envelope * left
    track[first:first + count, 1] += amplitude * signal * envelope * right


def build_menu_music() -> None:
    sample_rate = 44100
    duration = 64.0
    sample_count = int(sample_rate * duration)
    time = np.arange(sample_count, dtype=np.float64) / sample_rate
    track = np.zeros((sample_count, 2), dtype=np.float64)

    # A quiet D pentatonic cloud pad with integer-length phase ramps.
    pad_frequencies = (73.416, 110.000, 146.832, 220.000)
    for index, frequency in enumerate(pad_frequencies):
        phase = index * 0.71
        lfo = 0.72 + 0.28 * np.sin(2.0 * np.pi * time / (20.0 + index * 5.0) + phase)
        tone = np.sin(2.0 * np.pi * frequency * time + phase)
        tone += 0.18 * np.sin(2.0 * np.pi * frequency * 2.0 * time + phase * 0.4)
        pan = -0.48 + index * 0.32
        track[:, 0] += tone * lfo * 0.018 * np.sqrt((1.0 - pan) * 0.5)
        track[:, 1] += tone * lfo * 0.018 * np.sqrt((1.0 + pan) * 0.5)

    # Slowly moving filtered wind, deterministic and intentionally subtle.
    rng = np.random.default_rng(20260827)
    control_count = int(duration * 5) + 2
    control_time = np.linspace(0.0, duration, control_count)
    wind_left = np.interp(time, control_time, rng.normal(0.0, 1.0, control_count))
    wind_right = np.interp(time, control_time, rng.normal(0.0, 1.0, control_count))
    track[:, 0] += wind_left * 0.010
    track[:, 1] += wind_right * 0.010

    pentatonic = (293.665, 349.228, 391.995, 440.000, 523.251, 587.330)
    melody = (0, 2, 4, 3, 2, 1, 0, 2, 5, 4, 2, 3, 1, 2, 0)
    for index, degree in enumerate(melody):
        start = 2.0 + index * 4.0
        add_note(track, sample_rate, start, 3.4, pentatonic[degree], 0.052,
                 -0.38 if index % 2 == 0 else 0.34)
        if index % 4 == 3:
            add_note(track, sample_rate, start + 1.15, 5.0,
                     pentatonic[degree] * 2.0, 0.020, 0.52, bell=True)

    for start, frequency, pan in ((8.0, 146.832, -0.58), (24.0, 195.998, 0.55),
                                  (40.0, 164.814, -0.45), (56.0, 220.000, 0.48)):
        add_note(track, sample_rate, start, 7.0, frequency, 0.047, pan, bell=True)

    fade_seconds = 2.5
    fade_count = int(fade_seconds * sample_rate)
    fade = np.sin(np.linspace(0.0, np.pi * 0.5, fade_count)) ** 2
    track[:fade_count] *= fade[:, None]
    track[-fade_count:] *= fade[::-1, None]

    peak = float(np.max(np.abs(track)))
    if peak > 0.0:
        track *= 0.82 / peak
    pcm = np.int16(np.clip(track, -1.0, 1.0) * 32767)
    with wave.open(str(MUSIC_OUTPUT), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(sample_rate)
        output.writeframes(pcm.tobytes())


def main() -> None:
    if not TITLE_SOURCE.is_file():
        raise FileNotFoundError(TITLE_SOURCE)
    build_title_background()
    build_fog_layer()
    build_menu_music()
    print(
        "W11_ENTRY_SOURCE_READY "
        f"title={TITLE_OUTPUT.name} fog={FOG_OUTPUT.name} music={MUSIC_OUTPUT.name}"
    )


if __name__ == "__main__":
    main()
