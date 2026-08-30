"""Print sampled PlayerGirl pose transforms from a Blender-readable FBX."""

from __future__ import annotations

import sys

import bpy


fbx_path = sys.argv[sys.argv.index("--") + 1]
primary_only = "--primary-only" in sys.argv
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=fbx_path)

armatures = [obj for obj in bpy.data.objects if obj.type == "ARMATURE"]
if len(armatures) != 1:
    raise RuntimeError(f"Expected one armature, found {len(armatures)}")
armature = armatures[0]
interesting = [
    bone.name
    for bone in armature.pose.bones
    if any(
        token in bone.name.lower()
        for token in (
            "thigh", "calf", "foot", "spine", "clavicle",
            "upperarm", "forearm", "hand",
        )
    )
]
actions = list(bpy.data.actions)
if not actions:
    raise RuntimeError("Imported FBX has no actions")

if armature.animation_data is None:
    armature.animation_data_create()

for action in actions:
    armature.animation_data.action = action
    if getattr(action, "slots", None):
        compatible_slots = [slot for slot in action.slots if slot.target_id_type == "OBJECT"]
        if compatible_slots:
            armature.animation_data.action_slot = compatible_slots[0]

    action_fcurves = []
    action_slot_curves = []
    if getattr(action, "is_action_layered", False):
        for layer in action.layers:
            for strip in layer.strips:
                for slot in action.slots:
                    channelbag = strip.channelbag(slot, ensure=False)
                    if channelbag is not None:
                        action_fcurves.extend(channelbag.fcurves)
                        action_slot_curves.extend((slot, curve) for curve in channelbag.fcurves)
    else:
        action_fcurves.extend(action.fcurves)
        action_slot_curves.extend((None, curve) for curve in action.fcurves)

    if primary_only and len(action_fcurves) < 100:
        continue

    moving_leg_curves = []
    for curve in action_fcurves:
        if any(
            token in curve.data_path.lower()
            for token in (
                "thigh", "calf", "foot", "spine", "clavicle",
                "upperarm", "forearm", "hand",
            )
        ):
            values = [point.co[1] for point in curve.keyframe_points]
            if values:
                moving_leg_curves.append(
                    (
                        curve.data_path,
                        curve.array_index,
                        len(values),
                        min(values),
                        max(values),
                    )
                )
    frames = sorted(
        {
            int(action.frame_range[0]),
            int(action.frame_range[0] + (action.frame_range[1] - action.frame_range[0]) * 0.25),
            int(action.frame_range[0] + (action.frame_range[1] - action.frame_range[0]) * 0.50),
            int(action.frame_range[0] + (action.frame_range[1] - action.frame_range[0]) * 0.75),
            int(action.frame_range[1]),
        }
    )

    print(
        f"WW_SAMPLE_ACTION name={action.name} "
        f"range={action.frame_range[0]:.3f}-{action.frame_range[1]:.3f} "
        f"fcurves={len(action_fcurves)} leg_curves={len(moving_leg_curves)} "
        f"bones={interesting}"
    )
    for path, index, key_count, value_min, value_max in moving_leg_curves[:24]:
        print(
            f"WW_SAMPLE_CURVE path={path} index={index} keys={key_count} "
            f"min={value_min:.6f} max={value_max:.6f} span={value_max - value_min:.6f}"
        )
    for slot, curve in action_slot_curves[:20]:
        slot_name = slot.identifier if slot is not None else "LEGACY"
        values = [point.co[1] for point in curve.keyframe_points]
        value_min = min(values) if values else 0.0
        value_max = max(values) if values else 0.0
        print(
            f"WW_SAMPLE_RAW slot={slot_name} path={curve.data_path} index={curve.array_index} "
            f"keys={len(values)} span={value_max - value_min:.6f}"
        )
    matching_slots = sorted(
        {
            slot.identifier
            for slot, _curve in action_slot_curves
            if slot is not None
            and any(token in slot.identifier.lower() for token in ("thigh", "calf", "foot"))
        }
    )
    print(f"WW_SAMPLE_LEG_SLOTS slots={matching_slots}")
    for frame in frames:
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        values = []
        for bone_name in interesting:
            bone = armature.pose.bones[bone_name]
            rotation = bone.matrix.to_euler("XYZ")
            values.append(
                f"{bone_name}:"
                f"R({rotation.x:.3f},{rotation.y:.3f},{rotation.z:.3f})"
            )
        print(f"WW_SAMPLE_FRAME frame={frame} {' '.join(values)}")
