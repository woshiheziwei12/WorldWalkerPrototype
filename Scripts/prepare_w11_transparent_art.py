"""Create runtime-ready W11 static sprites with alpha-safe edge colors."""

from pathlib import Path

from PIL import Image

from process_w11_combat_animation_art import bleed_transparent_edges


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "SourceArt" / "W11" / "Generated"
OUTPUT_ROOT = SOURCE_ROOT / "RuntimeReady"
SPRITES = (
    "W11_Player_v1.png",
    "W11_MiasmaWisp_v1.png",
    "W11_StoneFiend_v1.png",
    "W11_SwordWraith_v1.png",
)


def main() -> None:
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    for filename in SPRITES:
        source = SOURCE_ROOT / filename
        if not source.is_file():
            raise RuntimeError(f"Missing W11 transparent source: {source}")
        output = OUTPUT_ROOT / filename
        bleed_transparent_edges(Image.open(source).convert("RGBA")).save(
            output, optimize=True
        )
        print(f"W11_ALPHA_SAFE_SPRITE_READY source={source.name} output={output}")
    print(f"W11_ALPHA_SAFE_SPRITES_COMPLETE count={len(SPRITES)}")


if __name__ == "__main__":
    main()
