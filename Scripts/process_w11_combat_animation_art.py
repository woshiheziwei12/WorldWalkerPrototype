"""Build W11 character/skill atlases, frame PNGs, GIF previews and a manifest.

Character source sheets are AI-generated RGB images with a baked light checkerboard.
The processor removes only checkerboard pixels connected to each cell border, so pale
costume details remain intact. Skill source sheets already contain a real alpha channel.
"""

from __future__ import annotations

from collections import deque
import json
from pathlib import Path
import shutil

from PIL import Image, ImageChops, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
ANIMATION_ROOT = ROOT / "SourceArt" / "W11" / "Animations"
CHARACTER_ROOT = ANIMATION_ROOT / "Characters"
SKILL_ROOT = ANIMATION_ROOT / "Skills"
SHOWCASE_ROOT = ANIMATION_ROOT / "Showcases"
FRAME_SIZE = 320
GRID_SIZE = 4
CHARACTER_BOTTOM_ANCHOR = 302
SAMPLE_FRAME_SIZE = 512
SAMPLE_COLUMNS = 4
SAMPLE_ROWS = 3
SAMPLE_BOTTOM_ANCHOR = 484

CHARACTERS = (
    "Wei", "Dong", "Tian", "Xiang", "Pu",
    "Ying", "Li", "Jia", "Yu", "Meng",
)
CHARACTER_ACTIONS = (
    ("Idle", 6.0),
    ("Move", 10.0),
    ("Cast", 8.0),
    ("Hit", 7.0),
)
SKILLS = {
    "FlowingCloudSword": {"fps": 12.0, "directional": True},
    "FiveThunder": {"fps": 14.0, "directional": False},
    "DarkWaterWard": {"fps": 10.0, "directional": False},
    "SpringRenewal": {"fps": 10.0, "directional": False},
    "MoonChasingStrike": {"fps": 16.0, "directional": True},
    "TwoPolesFormation": {"fps": 12.0, "directional": False},
    "CommonHitSpark": {"fps": 18.0, "directional": False},
}


def clean_directory(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def split_grid(
    image: Image.Image,
    columns: int = GRID_SIZE,
    rows_count: int = GRID_SIZE,
) -> list[list[Image.Image]]:
    rows: list[list[Image.Image]] = []
    for row in range(rows_count):
        cells: list[Image.Image] = []
        for column in range(columns):
            left = round(column * image.width / columns)
            top = round(row * image.height / rows_count)
            right = round((column + 1) * image.width / columns)
            bottom = round((row + 1) * image.height / rows_count)
            cells.append(image.crop((left, top, right, bottom)))
        rows.append(cells)
    return rows


def is_checker_candidate(rgb: tuple[int, int, int]) -> bool:
    low = min(rgb)
    high = max(rgb)
    return low >= 198 and high - low <= 18


def remove_connected_checkerboard(cell: Image.Image) -> Image.Image:
    rgb = cell.convert("RGB")
    width, height = rgb.size
    source = rgb.load()
    candidate = bytearray(width * height)
    for y in range(height):
        row_offset = y * width
        for x in range(width):
            candidate[row_offset + x] = is_checker_candidate(source[x, y])

    background = bytearray(width * height)
    queue: deque[tuple[int, int]] = deque()

    def enqueue(x: int, y: int) -> None:
        index = y * width + x
        if candidate[index] and not background[index]:
            background[index] = 1
            queue.append((x, y))

    for x in range(width):
        enqueue(x, 0)
        enqueue(x, height - 1)
    for y in range(height):
        enqueue(0, y)
        enqueue(width - 1, y)

    while queue:
        x, y = queue.popleft()
        if x > 0:
            enqueue(x - 1, y)
        if x + 1 < width:
            enqueue(x + 1, y)
        if y > 0:
            enqueue(x, y - 1)
        if y + 1 < height:
            enqueue(x, y + 1)

    output = rgb.convert("RGBA")
    pixels = output.load()
    for y in range(height):
        row_offset = y * width
        for x in range(width):
            if background[row_offset + x]:
                red, green, blue, _ = pixels[x, y]
                pixels[x, y] = (red, green, blue, 0)
    return output


def remove_distant_fragments(image: Image.Image) -> Image.Image:
    """Remove neighboring-cell spill while preserving nearby ribbons or rocks."""
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    width, height = rgba.size
    source = alpha.load()
    visited = bytearray(width * height)
    components: list[tuple[list[tuple[int, int]], tuple[int, int, int, int]]] = []

    for start_y in range(height):
        for start_x in range(width):
            start_index = start_y * width + start_x
            if visited[start_index] or source[start_x, start_y] <= 8:
                continue
            visited[start_index] = 1
            queue: deque[tuple[int, int]] = deque([(start_x, start_y)])
            points: list[tuple[int, int]] = []
            left = right = start_x
            top = bottom = start_y
            while queue:
                x, y = queue.popleft()
                points.append((x, y))
                left = min(left, x)
                right = max(right, x)
                top = min(top, y)
                bottom = max(bottom, y)
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if nx < 0 or nx >= width or ny < 0 or ny >= height:
                        continue
                    index = ny * width + nx
                    if not visited[index] and source[nx, ny] > 8:
                        visited[index] = 1
                        queue.append((nx, ny))
            components.append((points, (left, top, right + 1, bottom + 1)))

    if not components:
        return rgba
    main_points, main_box = max(components, key=lambda item: len(item[0]))
    del main_points
    margin = max(8, round(min(width, height) * 0.08))

    def box_distance(box: tuple[int, int, int, int]) -> int:
        left, top, right, bottom = box
        main_left, main_top, main_right, main_bottom = main_box
        dx = max(main_left - right, left - main_right, 0)
        dy = max(main_top - bottom, top - main_bottom, 0)
        return max(dx, dy)

    pixels = rgba.load()
    for points, box in components:
        if box == main_box or (len(points) >= 6 and box_distance(box) <= margin):
            continue
        for x, y in points:
            red, green, blue, _ = pixels[x, y]
            pixels[x, y] = (red, green, blue, 0)
    return rgba


def alpha_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    alpha = image.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise RuntimeError("Animation frame does not contain visible pixels")
    return bounds


def normalize_character_frames(
    frames: list[Image.Image],
    frame_size: int = FRAME_SIZE,
    bottom_anchor: int = CHARACTER_BOTTOM_ANCHOR,
) -> list[Image.Image]:
    bounds = [alpha_bounds(frame) for frame in frames]
    max_width = max(right - left for left, _, right, _ in bounds)
    max_height = max(bottom - top for _, top, _, bottom in bounds)
    scale = min((frame_size - 30) / max_width, (frame_size - 24) / max_height)
    normalized: list[Image.Image] = []
    for frame, box in zip(frames, bounds):
        subject = frame.crop(box)
        size = (
            max(1, round(subject.width * scale)),
            max(1, round(subject.height * scale)),
        )
        subject = subject.resize(size, Image.Resampling.LANCZOS)
        canvas = Image.new("RGBA", (frame_size, frame_size), (0, 0, 0, 0))
        x = (frame_size - subject.width) // 2
        y = bottom_anchor - subject.height
        canvas.alpha_composite(subject, (x, y))
        normalized.append(canvas)
    return normalized


def bleed_transparent_edges(image: Image.Image, padding: int = 8) -> Image.Image:
    """Fill RGB beneath transparent edge pixels while preserving the alpha channel.

    Unreal's bilinear sampler can otherwise pull the PNG's default black RGB into
    translucent sprite edges.  Repeated color dilation gives filtered/overlapping
    sprites safe neighboring colors without changing a single visible alpha value.
    """
    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A")
    color = rgba.convert("RGB")
    coverage = alpha
    for _ in range(max(0, padding)):
        expanded = coverage.filter(ImageFilter.MaxFilter(3))
        expanded_mask = expanded.point(lambda value: 255 if value > 0 else 0)
        old_coverage = coverage.point(lambda value: 255 if value > 0 else 0)
        newly_covered = ImageChops.subtract(expanded_mask, old_coverage)
        color = Image.composite(color.filter(ImageFilter.MaxFilter(3)), color, newly_covered)
        coverage = expanded
    result = color.convert("RGBA")
    result.putalpha(alpha)
    return result


def save_frame_sequence(
    frames: list[Image.Image],
    directory: Path,
    filename_prefix: str,
) -> list[str]:
    clean_directory(directory)
    paths: list[str] = []
    for index, frame in enumerate(frames):
        frame = bleed_transparent_edges(frame)
        path = directory / f"{filename_prefix}_{index:02d}.png"
        frame.save(path, optimize=True)
        paths.append(path.relative_to(ROOT).as_posix())
    return paths


def compose_sequence_atlas(
    frames: list[Image.Image], columns: int, frame_size: int
) -> Image.Image:
    rows_count = (len(frames) + columns - 1) // columns
    atlas = Image.new(
        "RGBA", (columns * frame_size, rows_count * frame_size), (0, 0, 0, 0)
    )
    for index, frame in enumerate(frames):
        atlas.alpha_composite(
            frame, ((index % columns) * frame_size, (index // columns) * frame_size)
        )
    return atlas


def process_move24_source(
    source: Path,
    inbetween_source: Path,
    right_directory: Path,
    left_directory: Path,
    right_prefix: str,
    left_prefix: str,
    preview_root: Path,
    preview_prefix: str,
    pixels_per_unreal_unit: float,
) -> dict:
    if not source.is_file():
        raise RuntimeError(f"Missing 12-frame movement sample: {source}")
    if not inbetween_source.is_file():
        raise RuntimeError(f"Missing 12-frame movement in-betweens: {inbetween_source}")
    source_rows = split_grid(Image.open(source), SAMPLE_COLUMNS, SAMPLE_ROWS)
    keyframes = [
        remove_distant_fragments(remove_connected_checkerboard(cell))
        for row in source_rows
        for cell in row
    ]
    inbetween_rows = split_grid(
        Image.open(inbetween_source), SAMPLE_COLUMNS, SAMPLE_ROWS
    )
    inbetweens = [
        remove_distant_fragments(remove_connected_checkerboard(cell))
        for row in inbetween_rows
        for cell in row
    ]
    cleaned = [
        frame
        for pair in zip(keyframes, inbetweens)
        for frame in pair
    ]
    right_frames = normalize_character_frames(
        cleaned, SAMPLE_FRAME_SIZE, SAMPLE_BOTTOM_ANCHOR
    )
    left_frames = [
        frame.transpose(Image.Transpose.FLIP_LEFT_RIGHT) for frame in right_frames
    ]
    right_paths = save_frame_sequence(right_frames, right_directory, right_prefix)
    left_paths = save_frame_sequence(left_frames, left_directory, left_prefix)
    save_gif(right_frames, preview_root / f"{preview_prefix}_Right_v3.gif", 24.0)
    save_gif(left_frames, preview_root / f"{preview_prefix}_Left_v3.gif", 24.0)
    sheet_root = preview_root.parent / "Sheets"
    sheet_root.mkdir(parents=True, exist_ok=True)
    compose_sequence_atlas(
        right_frames, SAMPLE_COLUMNS, SAMPLE_FRAME_SIZE
    ).save(sheet_root / f"{preview_prefix}_Right_v3.png", optimize=True)
    compose_sequence_atlas(
        left_frames, SAMPLE_COLUMNS, SAMPLE_FRAME_SIZE
    ).save(sheet_root / f"{preview_prefix}_Left_v3.png", optimize=True)
    return {
        "fps": 24.0,
        "right": right_paths,
        "left": left_paths,
        "frame_size": [SAMPLE_FRAME_SIZE, SAMPLE_FRAME_SIZE],
        "pixels_per_unreal_unit": pixels_per_unreal_unit,
        "sample": True,
    }


def compose_atlas(rows: list[list[Image.Image]]) -> Image.Image:
    atlas = Image.new(
        "RGBA", (FRAME_SIZE * GRID_SIZE, FRAME_SIZE * GRID_SIZE), (0, 0, 0, 0)
    )
    for row, frames in enumerate(rows):
        for column, frame in enumerate(frames):
            atlas.alpha_composite(frame, (column * FRAME_SIZE, row * FRAME_SIZE))
    return atlas


def save_gif(frames: list[Image.Image], output: Path, fps: float) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    duration = max(20, round(1000.0 / fps))
    frames[0].save(
        output,
        save_all=True,
        append_images=frames[1:],
        duration=duration,
        loop=0,
        disposal=2,
        transparency=0,
        optimize=False,
    )


def process_character(key: str) -> dict:
    base = CHARACTER_ROOT / key
    source = base / "Sheets" / f"W11_Hero_{key}_CombatSheet_v1_source.png"
    if not source.is_file():
        raise RuntimeError(f"Missing character source sheet: {source}")

    source_rows = split_grid(Image.open(source))
    cleaned = [remove_connected_checkerboard(cell) for row in source_rows for cell in row]
    normalized = normalize_character_frames(cleaned)
    right_rows = [normalized[index:index + GRID_SIZE] for index in range(0, 16, 4)]
    left_rows = [[frame.transpose(Image.Transpose.FLIP_LEFT_RIGHT) for frame in row] for row in right_rows]

    frame_root = base / "Frames"
    preview_root = base / "Previews"
    clean_directory(frame_root)
    clean_directory(preview_root)
    actions: dict[str, dict] = {}

    for row, (action, fps) in enumerate(CHARACTER_ACTIONS):
        action_data = {
            "fps": fps,
            "right": [],
            "left": [],
            "frame_size": [FRAME_SIZE, FRAME_SIZE],
            "pixels_per_unreal_unit": 1.0,
        }
        for facing, frames in (("Right", right_rows[row]), ("Left", left_rows[row])):
            directory = frame_root / facing / action
            directory.mkdir(parents=True, exist_ok=True)
            for index, frame in enumerate(frames):
                filename = f"W11_Hero_{key}_{facing}_{action}_{index:02d}.png"
                path = directory / filename
                bleed_transparent_edges(frame).save(path, optimize=True)
                action_data[facing.lower()].append(path.relative_to(ROOT).as_posix())
            save_gif(
                frames,
                preview_root / f"W11_Hero_{key}_{facing}_{action}_v1.gif",
                fps,
            )
        actions[action] = action_data

    if key == "Yu":
        actions["Move"] = process_move24_source(
            base / "Samples" / "W11_Hero_Yu_Move12_Right_v2_source.png",
            base / "Samples" / "W11_Hero_Yu_Move24_Inbetweens_v3_source.png",
            frame_root / "Right" / "Move",
            frame_root / "Left" / "Move",
            "W11_Hero_Yu_Right_Move",
            "W11_Hero_Yu_Left_Move",
            preview_root,
            "W11_Hero_Yu_Move24",
            SAMPLE_FRAME_SIZE / FRAME_SIZE,
        )
        print("W11_CHARACTER_MOVE_SAMPLE_READY hero=Yu frames=24 fps=24")

    right_atlas = compose_atlas(right_rows)
    left_atlas = compose_atlas(left_rows)
    right_atlas_path = base / "Sheets" / f"W11_Hero_{key}_CombatSheet_Right_v1.png"
    left_atlas_path = base / "Sheets" / f"W11_Hero_{key}_CombatSheet_Left_v1.png"
    right_atlas.save(right_atlas_path, optimize=True)
    left_atlas.save(left_atlas_path, optimize=True)
    save_gif(
        [frame for row in right_rows for frame in row],
        preview_root / f"W11_Hero_{key}_CombatSequence_Right_v1.gif",
        8.0,
    )
    print(f"W11_CHARACTER_ANIMATION_READY hero={key} frames=32 previews=9")
    return {
        "hero_id": f"Hero.{key}",
        "source_sheet": source.relative_to(ROOT).as_posix(),
        "right_atlas": right_atlas_path.relative_to(ROOT).as_posix(),
        "left_atlas": left_atlas_path.relative_to(ROOT).as_posix(),
        "frame_size": [FRAME_SIZE, FRAME_SIZE],
        "pivot": [0.5, CHARACTER_BOTTOM_ANCHOR / FRAME_SIZE],
        "actions": actions,
        "vertical_facing_policy": "keep_last_horizontal_facing",
    }


def process_skill(key: str, settings: dict) -> dict:
    base = SKILL_ROOT / key
    source = base / "Sheets" / f"W11_Skill_{key}_Sheet_v1_source.png"
    if not source.is_file():
        raise RuntimeError(f"Missing skill source sheet: {source}")
    source_rows = split_grid(Image.open(source).convert("RGBA"))
    rows = [
        [cell.resize((FRAME_SIZE, FRAME_SIZE), Image.Resampling.LANCZOS) for cell in row]
        for row in source_rows
    ]
    frames = [frame for row in rows for frame in row]
    frame_root = base / "Frames"
    preview_root = base / "Previews"
    clean_directory(frame_root)
    clean_directory(preview_root)
    paths: list[str] = []
    for index, frame in enumerate(frames):
        filename = f"W11_Skill_{key}_{index:02d}.png"
        path = frame_root / filename
        bleed_transparent_edges(frame).save(path, optimize=True)
        paths.append(path.relative_to(ROOT).as_posix())
    atlas_path = base / "Sheets" / f"W11_Skill_{key}_Sheet_v1.png"
    compose_atlas(rows).save(atlas_path, optimize=True)
    preview_path = preview_root / f"W11_Skill_{key}_v1.gif"
    save_gif(frames, preview_path, settings["fps"])
    print(f"W11_SKILL_ANIMATION_READY skill={key} frames=16 previews=1")
    return {
        "visual_id": f"SkillVisual.{key}",
        "ability_id": None if key == "CommonHitSpark" else f"Ability.{key}",
        "source_sheet": source.relative_to(ROOT).as_posix(),
        "atlas": atlas_path.relative_to(ROOT).as_posix(),
        "frames": paths,
        "frame_size": [FRAME_SIZE, FRAME_SIZE],
        "pivot": [0.5, 0.5],
        "fps": settings["fps"],
        "directional": settings["directional"],
        "authored_direction": "Right" if settings["directional"] else "None",
    }


def process_enemy_movement_sample() -> dict:
    key = "StoneFiend"
    base = ANIMATION_ROOT / "Enemies" / key
    preview_root = base / "Previews"
    preview_root.mkdir(parents=True, exist_ok=True)
    action = process_move24_source(
        base / "Sheets" / "W11_Enemy_StoneFiend_Move12_Right_v1_source.png",
        base / "Sheets" / "W11_Enemy_StoneFiend_Move24_Inbetweens_v2_source.png",
        base / "Frames" / "Right" / "Move",
        base / "Frames" / "Left" / "Move",
        "W11_Enemy_StoneFiend_Right_Move",
        "W11_Enemy_StoneFiend_Left_Move",
        preview_root,
        "W11_Enemy_StoneFiend_Move24",
        1.0,
    )
    print("W11_ENEMY_MOVE_SAMPLE_READY enemy=StoneFiend frames=24 fps=24")
    return {
        "enemy_id": "Enemy.StoneFiend",
        "source_sheet": (
            base / "Sheets" / "W11_Enemy_StoneFiend_Move12_Right_v1_source.png"
        ).relative_to(ROOT).as_posix(),
        "inbetween_source_sheet": (
            base / "Sheets" / "W11_Enemy_StoneFiend_Move24_Inbetweens_v2_source.png"
        ).relative_to(ROOT).as_posix(),
        "move": action,
    }


def fit_sprite(image: Image.Image, max_size: tuple[int, int]) -> Image.Image:
    rgba = image.convert("RGBA")
    bounds = alpha_bounds(rgba)
    subject = rgba.crop(bounds)
    scale = min(max_size[0] / subject.width, max_size[1] / subject.height)
    return subject.resize(
        (max(1, round(subject.width * scale)), max(1, round(subject.height * scale))),
        Image.Resampling.LANCZOS,
    )


def make_showcase_background() -> Image.Image:
    source = ROOT / "SourceArt" / "W11" / "Generated" / "W11_Arena_Background_v1.png"
    if not source.is_file():
        return Image.new("RGBA", (640, 360), (20, 24, 32, 255))
    image = Image.open(source).convert("RGB")
    scale = max(640 / image.width, 360 / image.height)
    image = image.resize(
        (round(image.width * scale), round(image.height * scale)),
        Image.Resampling.LANCZOS,
    )
    left = (image.width - 640) // 2
    top = (image.height - 360) // 2
    image = image.crop((left, top, left + 640, top + 360))
    shade = Image.new("RGBA", image.size, (8, 12, 22, 92))
    result = image.convert("RGBA")
    result.alpha_composite(shade)
    return result


def build_shared_skill_showcases() -> list[str]:
    """Compose review-only GIFs; runtime character and skill sources stay separate."""
    clean_directory(SHOWCASE_ROOT)
    background = make_showcase_background()
    enemy_source = ROOT / "SourceArt" / "W11" / "Generated" / "W11_StoneFiend_v1.png"
    enemy = fit_sprite(Image.open(enemy_source), (145, 170)) if enemy_source.is_file() else None
    skill_paths = sorted((SKILL_ROOT / "FlowingCloudSword" / "Frames").glob("*.png"))
    impact_paths = sorted((SKILL_ROOT / "CommonHitSpark" / "Frames").glob("*.png"))
    if len(skill_paths) != 16 or len(impact_paths) != 16:
        raise RuntimeError("Shared showcase requires 16 FlowingCloudSword and hit frames")
    skill_frames = [Image.open(path).convert("RGBA") for path in skill_paths]
    impact_frames = [Image.open(path).convert("RGBA") for path in impact_paths]
    outputs: list[str] = []

    for key in CHARACTERS:
        cast_paths = sorted((CHARACTER_ROOT / key / "Frames" / "Right" / "Cast").glob("*.png"))
        idle_paths = sorted((CHARACTER_ROOT / key / "Frames" / "Right" / "Idle").glob("*.png"))
        cast = [fit_sprite(Image.open(path), (205, 260)) for path in cast_paths]
        idle = [fit_sprite(Image.open(path), (205, 260)) for path in idle_paths]
        frames: list[Image.Image] = []
        for index in range(20):
            frame = background.copy()
            hero = cast[index] if index < 4 else idle[(index - 4) % len(idle)]
            frame.alpha_composite(hero, (62, 330 - hero.height))
            if enemy is not None:
                frame.alpha_composite(enemy, (500, 330 - enemy.height))
            if index >= 4:
                effect = skill_frames[index - 4].resize((300, 300), Image.Resampling.LANCZOS)
                frame.alpha_composite(effect, (215, 30))
                if index >= 12:
                    impact = impact_frames[min(15, (index - 12) * 2)].resize(
                        (150, 150), Image.Resampling.LANCZOS
                    )
                    frame.alpha_composite(impact, (475, 125))
            frames.append(frame)
        output = SHOWCASE_ROOT / f"W11_Hero_{key}_SharedFlowingCloud_BattleDemo_v1.gif"
        save_gif(frames, output, 10.0)
        outputs.append(output.relative_to(ROOT).as_posix())
        print(f"W11_SHARED_SKILL_SHOWCASE_READY hero={key} skill=FlowingCloudSword")
    return outputs


def main() -> None:
    manifest = {
        "schema_version": 1,
        "content_version": "W11-ANIM-SAMPLE-v3",
        "separation_contract": {
            "characters_contain_skill_vfx": False,
            "skills_contain_character_art": False,
            "character_actions": [action for action, _ in CHARACTER_ACTIONS],
            "cast_anchor": "character_root",
            "directional_skills_authored_facing": "Right",
        },
        "characters": [process_character(key) for key in CHARACTERS],
        "enemy_samples": [process_enemy_movement_sample()],
        "skills": [process_skill(key, settings) for key, settings in SKILLS.items()],
    }
    manifest["showcases"] = {
        "preview_only": True,
        "shared_skill": "Ability.FlowingCloudSword",
        "files": build_shared_skill_showcases(),
    }
    output = ANIMATION_ROOT / "W11_AnimationManifest_v1.json"
    output.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(
        "W11_ANIMATION_PROCESS_COMPLETE "
        f"characters={len(CHARACTERS)} enemy_samples=1 "
        f"skills={len(SKILLS)} manifest={output}"
    )


if __name__ == "__main__":
    main()
