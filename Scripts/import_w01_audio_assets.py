"""Download, verify, normalize and import the CC0 ambience selected for W01.

Run inside Unreal Editor Python after the Editor target has been built.  The
source WAV files use 24-bit and 32-bit PCM, while Unreal's most portable import
path is 16-bit PCM.  This script performs a deterministic conversion before
creating stable SoundWave assets under the W01-owned ThirdParty directory.

Set ``WORLDWALKER_W01_AUDIO_ROOT`` before Unreal starts to override the source
cache.  The default is ``%TEMP%/WorldWalker_W01_Audio``.
"""

from __future__ import annotations

import array
import hashlib
import os
import shutil
import sys
import tempfile
import urllib.request
import wave
import zipfile
from pathlib import Path

import unreal


SOURCE_ROOT_ENV = "WORLDWALKER_W01_AUDIO_ROOT"
DEFAULT_SOURCE_ROOT = Path(tempfile.gettempdir()) / "WorldWalker_W01_Audio"
DESTINATION_ROOT = (
    "/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/"
    "OpenGameArt/Audio"
)

FIRE_SOURCE = {
    "url": "https://opengameart.org/sites/default/files/fire.wav",
    "file_name": "fire.wav",
    "sha256": "85CA0CC60D0C037FFF8B185E31AD1FCDBDA6CE45EEE17C3EE1318D1B8F59E330",
}

AMBIENCE_ARCHIVE = {
    "url": "https://opengameart.org/sites/default/files/dark_ambiences.zip",
    "file_name": "dark_ambiences.zip",
    "sha256": "4E95C28A468CBDE21D6300AA0BB9A5AEDBEE01B393E712418C49D6B4B8A758BB",
}

AMBIENCE_SOURCES = (
    ("ambience-1.wav", "23C9438B65835F8E493A00DE244387BE33E0303E774E15022692CABB1B707DF0"),
    ("ambience-2.wav", "B03D50AA11DF1FD7D859A5776299D73F9D9444565AC6501720D0035DF0AB3424"),
    ("ambience-3.wav", "CEEFB1041A70357F8D5A9D4A22C200992C38E34468D5717663402435290D8AA6"),
    ("ambience-4.wav", "C5AA2E32B681654DBF510EE4A5E85C5A7968A9A8D3C329CF78108542381D2D2D"),
    ("ambience-5.wav", "2A07090EB244E42445F7286DC68811BCB79D2F236825F770058B02669A11828F"),
)


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def _verify_hash(path: Path, expected: str, label: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"W01 audio source missing: {label}: {path}")
    actual = _sha256(path)
    if actual != expected.upper():
        raise RuntimeError(
            f"W01 audio hash mismatch: {label}: "
            f"expected={expected.upper()} actual={actual}"
        )


def _download_verified(spec: dict[str, str], source_root: Path) -> Path:
    destination = source_root / spec["file_name"]
    if destination.is_file():
        _verify_hash(destination, spec["sha256"], "cached download")
        return destination

    temporary = destination.with_suffix(destination.suffix + ".download")
    temporary.unlink(missing_ok=True)
    request = urllib.request.Request(
        spec["url"],
        headers={"User-Agent": "WorldWalkerPrototype-W01-Audio/1.0"},
    )
    unreal.log(f"W01 downloading CC0 audio: {spec['url']}")
    with urllib.request.urlopen(request, timeout=120) as response:
        with temporary.open("wb") as output:
            shutil.copyfileobj(response, output)
    _verify_hash(temporary, spec["sha256"], "downloaded source")
    temporary.replace(destination)
    return destination


def _safe_extract_zip(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    destination_resolved = destination.resolve()
    with zipfile.ZipFile(archive) as source_zip:
        for member in source_zip.infolist():
            member_path = (destination / member.filename).resolve()
            try:
                member_path.relative_to(destination_resolved)
            except ValueError as exc:
                raise RuntimeError(
                    f"Unsafe W01 audio archive member: {member.filename}"
                ) from exc
        source_zip.extractall(destination)


def _convert_pcm_to_16bit(source: Path, destination: Path) -> Path:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(source), "rb") as reader:
        channels = reader.getnchannels()
        sample_width = reader.getsampwidth()
        sample_rate = reader.getframerate()
        frame_count = reader.getnframes()
        raw_samples = reader.readframes(frame_count)

    if sample_width not in (2, 3, 4):
        raise RuntimeError(
            f"Unsupported W01 source sample width: {source}: {sample_width * 8}-bit"
        )

    if sample_width == 2:
        converted = raw_samples
    else:
        output_samples = array.array("h")
        shift = 8 if sample_width == 3 else 16
        for offset in range(0, len(raw_samples), sample_width):
            chunk = raw_samples[offset : offset + sample_width]
            sign_byte = b"\xff" if chunk[-1] & 0x80 else b"\x00"
            signed_value = int.from_bytes(
                chunk + sign_byte * (4 - sample_width),
                byteorder="little",
                signed=True,
            )
            output_samples.append(max(-32768, min(32767, signed_value >> shift)))
        if sys.byteorder != "little":
            output_samples.byteswap()
        converted = output_samples.tobytes()

    with wave.open(str(destination), "wb") as writer:
        writer.setnchannels(channels)
        writer.setsampwidth(2)
        writer.setframerate(sample_rate)
        writer.writeframes(converted)
    return destination


def _find_ambience_source(extraction_root: Path, file_name: str) -> Path:
    matches = [
        candidate
        for candidate in extraction_root.rglob(file_name)
        if "__MACOSX" not in candidate.parts
    ]
    if len(matches) != 1:
        raise RuntimeError(
            f"Expected one W01 ambience source named {file_name}, found {len(matches)}"
        )
    return matches[0]


def _import_sound(source: Path, asset_name: str, looping: bool) -> str:
    asset_path = f"{DESTINATION_ROOT}/{asset_name}"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION_ROOT)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    sound_wave = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not sound_wave or not isinstance(sound_wave, unreal.SoundWave):
        raise RuntimeError(f"W01 SoundWave import failed: {asset_path}")
    sound_wave.set_editor_property("looping", looping)
    try:
        sound_wave.set_editor_property("compression_quality", 72)
    except Exception as exc:  # Property differs on a few UE minor versions.
        unreal.log_warning(f"W01 audio compression quality unchanged: {asset_name}: {exc}")
    unreal.EditorAssetLibrary.save_loaded_asset(sound_wave, only_if_is_dirty=False)
    return asset_path


def main() -> None:
    source_root = Path(os.environ.get(SOURCE_ROOT_ENV, DEFAULT_SOURCE_ROOT))
    source_root.mkdir(parents=True, exist_ok=True)
    converted_root = source_root / "converted_16bit"

    fire_source = _download_verified(FIRE_SOURCE, source_root)
    ambience_archive = _download_verified(AMBIENCE_ARCHIVE, source_root)
    ambience_root = source_root / "dark_ambiences"
    if not ambience_root.is_dir():
        _safe_extract_zip(ambience_archive, ambience_root)

    converted_fire = _convert_pcm_to_16bit(
        fire_source, converted_root / "SW_W01_FireLoop.wav"
    )
    imported_fire = _import_sound(converted_fire, "SW_W01_FireLoop", True)

    imported_ambience: list[str] = []
    for index, (file_name, expected_hash) in enumerate(AMBIENCE_SOURCES, start=1):
        source = _find_ambience_source(ambience_root, file_name)
        _verify_hash(source, expected_hash, file_name)
        asset_name = f"SW_W01_DarkAmbience_{index:02d}"
        converted = _convert_pcm_to_16bit(
            source, converted_root / f"{asset_name}.wav"
        )
        imported_ambience.append(_import_sound(converted, asset_name, False))

    unreal.EditorAssetLibrary.save_directory(
        DESTINATION_ROOT,
        only_if_is_dirty=False,
        recursive=True,
    )
    unreal.log(
        "W01_AUDIO_IMPORT_COMPLETE "
        f"fire={1 if imported_fire else 0}/1 "
        f"ambience={len(imported_ambience)}/{len(AMBIENCE_SOURCES)}"
    )


if __name__ == "__main__":
    main()
