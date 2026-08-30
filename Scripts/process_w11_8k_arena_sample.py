"""Build the W11 8192-square combat arena master from the approved detail source."""

from pathlib import Path

from PIL import Image, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
ART_ROOT = ROOT / "SourceArt" / "W11" / "Generated"
SOURCE = ART_ROOT / "W11_FirstArena_Continuous_v3_detail_source.png"
OUTPUT = ART_ROOT / "W11_FirstArena_Continuous_v3_8K.png"
TARGET_SIZE = 8192


def main() -> None:
    if not SOURCE.is_file():
        raise RuntimeError(f"Missing approved arena detail source: {SOURCE}")
    source = Image.open(SOURCE).convert("RGB")
    if source.width != source.height:
        raise RuntimeError(f"Arena detail source must be square, found {source.size}")
    master = source.resize(
        (TARGET_SIZE, TARGET_SIZE),
        Image.Resampling.LANCZOS,
        reducing_gap=3.0,
    )
    master = master.filter(ImageFilter.UnsharpMask(radius=1.1, percent=105, threshold=3))
    master.save(OUTPUT, compress_level=6)
    print(
        "W11_ARENA_8K_SAMPLE_READY "
        f"source={source.width}x{source.height} output={TARGET_SIZE}x{TARGET_SIZE} "
        f"path={OUTPUT}"
    )


if __name__ == "__main__":
    main()
