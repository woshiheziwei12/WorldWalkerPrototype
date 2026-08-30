"""Inspect imported PlayerGirl AnimSequence data-model tracks in Unreal."""

from __future__ import annotations

import os

import unreal


ROOT = os.environ.get(
    "WORLDWALKER_PLAYERGIRL_INSPECT_ROOT",
    "/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/Animations",
)
NAMES = tuple(
    value.strip()
    for value in os.environ.get(
        "WORLDWALKER_PLAYERGIRL_INSPECT_NAMES",
        "Standby,WalkCycle,RunCycle,SprintCycle",
    ).split(",")
    if value.strip()
)


def safe_call(label: str, function) -> None:
    try:
        unreal.log(f"WW_ANIM_INSPECT {label}={function()}")
    except Exception as error:
        unreal.log_warning(f"WW_ANIM_INSPECT {label}_ERROR={error}")


for name in NAMES:
    path = f"{ROOT}/A_WW_PlayerGirl_{name}"
    sequence = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(sequence, unreal.AnimSequence):
        unreal.log_error(f"WW_ANIM_INSPECT missing={path}")
        continue
    skeleton = sequence.get_editor_property("skeleton")
    unreal.log(
        f"WW_ANIM_INSPECT asset={name} length={sequence.get_play_length():.6f} "
        f"skeleton={skeleton.get_path_name() if skeleton else 'None'}"
    )
    safe_call(
        f"{name}.sampled_keys_property",
        lambda: sequence.get_editor_property("number_of_sampled_keys"),
    )
    safe_call(
        f"{name}.sequence_methods",
        lambda: [
            value
            for value in dir(sequence)
            if any(token in value.lower() for token in ("track", "bone", "model", "sample"))
        ],
    )
    safe_call(
        f"{name}.track_names",
        lambda: unreal.AnimationLibrary.get_animation_track_names(sequence),
    )
    safe_call(
        f"{name}.track_count",
        lambda: len(unreal.AnimationLibrary.get_animation_track_names(sequence)),
    )
    track_names = unreal.AnimationLibrary.get_animation_track_names(sequence)
    normalised_tracks = {str(track).lower(): track for track in track_names}
    for diagnostic_bone in (
        "bip001",
        "bip001-pelvis",
        "bip001-l-thigh",
        "bip001-l-upperarm",
        "bip001-r-upperarm",
    ):
        track_name = normalised_tracks.get(diagnostic_bone)
        if track_name is None:
            continue

        def rotation_summary(track_name=track_name) -> str:
            rotations = unreal.AnimationLibrary.get_raw_track_rotation_data(
                sequence, track_name
            )
            if not rotations:
                return "keys=0"
            first = rotations[0]
            maximum_delta = max(
                first.angular_distance(rotation) for rotation in rotations
            )
            return f"keys={len(rotations)} max_delta_rad={maximum_delta:.6f}"

        safe_call(f"{name}.rotation.{diagnostic_bone}", rotation_summary)
    if name in ("Standby", "WalkCycle"):
        for candidate in (
            "Bip001",
            "Bip001 Pelvis",
            "Bip001-Pelvis",
            "Bip001_Pelvis",
            "Bip001 L Thigh",
            "Bip001-L-Thigh",
            "Bip001_L_Thigh",
            "bip001lthigh",
            "bip001_l_thigh",
        ):
            safe_call(
                f"skeleton_has.{candidate}",
                lambda candidate=candidate: unreal.AnimationLibrary.does_bone_name_exist(
                    sequence, candidate
                ),
            )

safe_call(
    "animation_library_methods",
    lambda: [
        value
        for value in dir(unreal.AnimationLibrary)
        if any(token in value.lower() for token in ("track", "bone", "pose", "sample"))
    ],
)

unreal.log("WW_ANIM_INSPECT_COMPLETE")
