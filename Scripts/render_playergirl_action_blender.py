"""Render a diagnostic solid preview of a merged PlayerGirl action FBX."""

from __future__ import annotations

import os
from pathlib import Path

import bpy
from mathutils import Vector


MODEL_FBX = Path(os.environ["WW_PLAYERGIRL_RENDER_MODEL"])
ACTION_FBX = Path(os.environ["WW_PLAYERGIRL_RENDER_ACTION"])
OUTPUT_PNG = Path(os.environ["WW_PLAYERGIRL_RENDER_OUTPUT"])
FRAME = int(os.environ.get("WW_PLAYERGIRL_RENDER_FRAME", "60"))

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

bpy.ops.import_scene.fbx(filepath=str(MODEL_FBX), automatic_bone_orientation=False)
model_objects = set(bpy.context.scene.objects)
model_armatures = [obj for obj in model_objects if obj.type == "ARMATURE"]
if not model_armatures:
    raise RuntimeError("PlayerGirl model FBX contains no armature")
model_armature = max(model_armatures, key=lambda obj: len(obj.data.bones))

bpy.ops.import_scene.fbx(filepath=str(ACTION_FBX), automatic_bone_orientation=False)
action_objects = [obj for obj in bpy.context.scene.objects if obj not in model_objects]
action_armatures = [obj for obj in action_objects if obj.type == "ARMATURE"]
if not action_armatures:
    raise RuntimeError("Action FBX contains no armature")
action_armature = max(action_armatures, key=lambda obj: len(obj.data.bones))
if not action_armature.animation_data or not action_armature.animation_data.action:
    raise RuntimeError("Action FBX armature has no action")

model_armature.animation_data_create()
model_armature.animation_data.action = action_armature.animation_data.action
for obj in action_objects:
    obj.hide_render = True
    obj.hide_viewport = True

bpy.context.scene.frame_set(FRAME)
visible_meshes = [obj for obj in model_objects if obj.type == "MESH"]
if not visible_meshes:
    raise RuntimeError("PlayerGirl model FBX contains no mesh")

depsgraph = bpy.context.evaluated_depsgraph_get()
world_corners: list[Vector] = []
for mesh in visible_meshes:
    evaluated = mesh.evaluated_get(depsgraph)
    world_corners.extend(evaluated.matrix_world @ Vector(corner) for corner in evaluated.bound_box)
minimum = Vector((min(v.x for v in world_corners), min(v.y for v in world_corners), min(v.z for v in world_corners)))
maximum = Vector((max(v.x for v in world_corners), max(v.y for v in world_corners), max(v.z for v in world_corners)))
center = (minimum + maximum) * 0.5
extent = maximum - minimum

camera_data = bpy.data.cameras.new("WW_ActionDiagnosticCamera")
camera = bpy.data.objects.new("WW_ActionDiagnosticCamera", camera_data)
bpy.context.scene.collection.objects.link(camera)
distance = max(extent.x, extent.y, extent.z) * 2.2
camera.location = center + Vector((0.0, -distance, extent.z * 0.15))
camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
camera_data.lens = 55.0
bpy.context.scene.camera = camera

scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "MATERIAL"
scene.display.shading.show_shadows = True
scene.render.resolution_x = 900
scene.render.resolution_y = 1100
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.filepath = str(OUTPUT_PNG)
scene.render.film_transparent = False
OUTPUT_PNG.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.render.render(write_still=True)
print(
    "WW_PLAYERGIRL_BLENDER_RENDER "
    f"frame={FRAME} action={model_armature.animation_data.action.name} "
    f"bounds_min={tuple(round(v, 3) for v in minimum)} "
    f"bounds_max={tuple(round(v, 3) for v in maximum)} output={OUTPUT_PNG}"
)
