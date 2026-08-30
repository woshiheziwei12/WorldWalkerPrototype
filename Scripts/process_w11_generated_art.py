"""Split the generated W11 prototype atlas into normalized transparent sprites."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "SourceArt" / "W11" / "Generated"
ATLAS = ART / "W11_Prototype_SpriteAtlas_v1.png"
OUTPUTS = (
    ("W11_Player_v1.png", 0, 0),
    ("W11_MiasmaWisp_v1.png", 1, 0),
    ("W11_StoneFiend_v1.png", 0, 1),
    ("W11_SwordWraith_v1.png", 1, 1),
)
CANVAS_SIZE = 512
MAX_SUBJECT_SIZE = 440


def normalized_sprite(source: Image.Image) -> Image.Image:
    alpha = source.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise RuntimeError("Atlas quadrant does not contain visible pixels")
    subject = source.crop(bounds)
    scale = min(
        MAX_SUBJECT_SIZE / subject.width,
        MAX_SUBJECT_SIZE / subject.height,
        1.0,
    )
    size = (
        max(1, round(subject.width * scale)),
        max(1, round(subject.height * scale)),
    )
    subject = subject.resize(size, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (CANVAS_SIZE, CANVAS_SIZE), (0, 0, 0, 0))
    offset = (
        (CANVAS_SIZE - subject.width) // 2,
        (CANVAS_SIZE - subject.height) // 2,
    )
    canvas.alpha_composite(subject, offset)
    return canvas


def main() -> None:
    atlas = Image.open(ATLAS).convert("RGBA")
    half_width = atlas.width // 2
    half_height = atlas.height // 2
    for filename, column, row in OUTPUTS:
        left = column * half_width
        top = row * half_height
        right = atlas.width if column else half_width
        bottom = atlas.height if row else half_height
        quadrant = atlas.crop((left, top, right, bottom))
        output = ART / filename
        normalized_sprite(quadrant).save(output, optimize=True)
        print(f"W11_SPRITE_READY {filename}")


if __name__ == "__main__":
    main()
