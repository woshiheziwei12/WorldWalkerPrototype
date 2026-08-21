"""Build a low-poly W02 medieval climber on the exported Warrior skeleton.

Run with Blender 4.5 LTS in background mode.  The geometry follows the approved
three-view concept in Docs/ConceptArt/W02 and intentionally uses rigid,
single-bone weights per garment/body section for predictable UE animation.
"""

from __future__ import annotations

import math
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_FBX = PROJECT_ROOT / "Intermediate" / "W02CharacterModeling" / "W02_Warrior_RigTemplate.fbx"
OUTPUT_DIRECTORY = PROJECT_ROOT / "SourceAssets" / "W02_MedievalClimber"
OUTPUT_FBX = OUTPUT_DIRECTORY / "SK_W02_MedievalClimber_LowPoly.fbx"
OUTPUT_BLEND = OUTPUT_DIRECTORY / "SK_W02_MedievalClimber_LowPoly.blend"
RIGID_PART_DIRECTORY = OUTPUT_DIRECTORY / "RigidParts"
MATERIAL_PALETTE_FBX = OUTPUT_DIRECTORY / "SM_W02_Climber_MaterialPalette.fbx"
PREVIEW_DIRECTORY = PROJECT_ROOT / "Saved" / "CharacterPreviews"
PREVIEW_PNG = PREVIEW_DIRECTORY / "W02_MedievalClimber_BlenderPreview.png"

OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
RIGID_PART_DIRECTORY.mkdir(parents=True, exist_ok=True)
PREVIEW_DIRECTORY.mkdir(parents=True, exist_ok=True)


def material(name: str, color: tuple[float, float, float, float], roughness: float = 0.72, metallic: float = 0.0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = color
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metallic
    return mat


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(SOURCE_FBX), automatic_bone_orientation=False)

armature = next(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")
rig_root = armature.parent
if rig_root is None or rig_root.type != "EMPTY":
    raise RuntimeError("The exported UE rig template is missing its original FBX root node")
armature.data.name = "SK_W02_MedievalClimber_Skeleton"

for obj in list(bpy.context.scene.objects):
    if obj.type == "MESH":
        bpy.data.objects.remove(obj, do_unlink=True)

MATERIALS = {
    "skin": material("M_W02_Climber_Skin", (0.73, 0.45, 0.34, 1.0), 0.82),
    "hair": material("M_W02_Climber_Hair", (0.82, 0.74, 0.58, 1.0), 0.76),
    "linen": material("M_W02_Climber_Linen", (0.72, 0.67, 0.55, 1.0), 0.92),
    "rose": material("M_W02_Climber_DustyRose", (0.46, 0.19, 0.20, 1.0), 0.86),
    "leather": material("M_W02_Climber_Leather", (0.19, 0.075, 0.035, 1.0), 0.78),
    "cloth": material("M_W02_Climber_DarkCloth", (0.055, 0.050, 0.045, 1.0), 0.94),
    "metal": material("M_W02_Climber_Metal", (0.20, 0.17, 0.13, 1.0), 0.34, 0.72),
    "eye": material("M_W02_Climber_Eyes", (0.035, 0.055, 0.045, 1.0), 0.55),
}

created: list[bpy.types.Object] = []


def finish_piece(obj: bpy.types.Object, piece_name: str, bone_name: str, mat_key: str) -> bpy.types.Object:
    obj.name = piece_name
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    group = obj.vertex_groups.new(name=bone_name)
    group.add(range(len(obj.data.vertices)), 1.0, "REPLACE")
    modifier = obj.modifiers.new(name="W02_Armature", type="ARMATURE")
    modifier.object = armature
    obj.data.materials.append(MATERIALS[mat_key])
    obj["w02_bone"] = bone_name
    created.append(obj)
    return obj


def cube_piece(name: str, bone: str, mat: str, location, scale, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.scale = scale
    return finish_piece(obj, name, bone, mat)


def sphere_piece(name: str, bone: str, mat: str, location, scale, segments=12, rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=location)
    obj = bpy.context.object
    obj.scale = scale
    return finish_piece(obj, name, bone, mat)


def cone_piece(name: str, bone: str, mat: str, location, radius_bottom, radius_top, depth, y_scale=1.0, vertices=10):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius_bottom,
        radius2=radius_top,
        depth=depth,
        location=location,
    )
    obj = bpy.context.object
    obj.scale.y = y_scale
    return finish_piece(obj, name, bone, mat)


def cylinder_between(name: str, bone: str, mat: str, start, end, radius, y_scale=1.0, vertices=10):
    start_v = Vector(start)
    end_v = Vector(end)
    direction = end_v - start_v
    midpoint = (start_v + end_v) * 0.5
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=direction.length,
        location=midpoint,
    )
    obj = bpy.context.object
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = direction.to_track_quat("Z", "Y")
    obj.scale.y = y_scale
    return finish_piece(obj, name, bone, mat)


def torus_piece(name: str, bone: str, mat: str, location, major_radius, minor_radius, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=12,
        minor_segments=5,
        location=location,
        rotation=rotation,
    )
    return finish_piece(bpy.context.object, name, bone, mat)


# Head, face and long pale hair from the concept sheet.
cylinder_between("Neck", "Neck", "skin", (-0.001, 0.0, 1.93), (-0.001, 0.0, 2.13), 0.115, 0.88)
sphere_piece("Head", "Head", "skin", (0.0, -0.015, 2.36), (0.245, 0.205, 0.305), 14, 10)
sphere_piece("HairCap", "Head", "hair", (0.0, 0.035, 2.48), (0.265, 0.225, 0.225), 12, 8)
sphere_piece("LongHair", "Head", "hair", (0.0, 0.155, 1.78), (0.315, 0.135, 0.74), 12, 10)
for side in (-1.0, 1.0):
    cylinder_between(
        f"FrontHair_{'L' if side > 0 else 'R'}",
        "Head",
        "hair",
        (0.145 * side, -0.185, 2.46),
        (0.19 * side, -0.16, 1.78),
        0.055,
        0.72,
        8,
    )
    sphere_piece(
        f"Eye_{'L' if side > 0 else 'R'}",
        "Head",
        "eye",
        (0.085 * side, -0.211, 2.40),
        (0.028, 0.014, 0.022),
        8,
        5,
    )

# Layered linen, dusty-rose tunic and short leather brigandine.
cone_piece("LinenTorso", "Torso", "linen", (0.0, 0.0, 1.61), 0.34, 0.30, 0.72, 0.58, 10)
cone_piece("RoseTunic", "Abdomen", "rose", (0.0, 0.0, 1.39), 0.36, 0.31, 0.56, 0.61, 10)
cone_piece("LeatherVest", "Torso", "leather", (0.0, -0.006, 1.69), 0.35, 0.38, 0.52, 0.62, 10)
cone_piece("RoseSkirt", "Hips", "rose", (0.0, 0.01, 1.16), 0.42, 0.33, 0.40, 0.58, 10)
torus_piece("Belt", "Hips", "leather", (0.0, 0.0, 1.34), 0.325, 0.042)
cube_piece("BeltBuckle", "Hips", "metal", (0.0, -0.224, 1.34), (0.09, 0.028, 0.075))
for side in (-1.0, 1.0):
    cube_piece(
        f"UtilityPouch_{'L' if side > 0 else 'R'}",
        "Hips",
        "leather",
        (0.34 * side, -0.075, 1.20),
        (0.115, 0.095, 0.145),
        rotation=(0.0, 0.0, -0.10 * side),
    )
torus_piece("RopeLoopOuter", "Hips", "linen", (0.42, 0.01, 1.20), 0.155, 0.027, (math.pi * 0.5, 0.0, 0.0))
torus_piece("RopeLoopInner", "Hips", "linen", (0.42, 0.015, 1.20), 0.105, 0.023, (math.pi * 0.5, 0.0, 0.0))

# T-pose arm sections.  Each part is rigidly assigned to its matching source bone.
for side, suffix in ((1.0, "L"), (-1.0, "R")):
    cylinder_between(
        f"UpperSleeve_{suffix}",
        f"UpperArm_{suffix}",
        "rose",
        (0.31 * side, 0.01, 1.87),
        (0.71 * side, 0.035, 1.87),
        0.145,
        0.82,
        10,
    )
    cylinder_between(
        f"LinenForearm_{suffix}",
        f"LowerArm_{suffix}",
        "linen",
        (0.71 * side, 0.035, 1.87),
        (1.16 * side, 0.043, 1.86),
        0.115,
        0.82,
        10,
    )
    cylinder_between(
        f"Glove_{suffix}",
        f"Fist_{suffix}",
        "leather",
        (1.15 * side, 0.043, 1.86),
        (1.34 * side, 0.043, 1.86),
        0.105,
        0.78,
        8,
    )
    sphere_piece(
        f"Hand_{suffix}",
        f"Fist1_{suffix}",
        "skin",
        (1.37 * side, 0.04, 1.86),
        (0.12, 0.085, 0.095),
        10,
        6,
    )

# Dark fitted trousers, leather knees and flexible climbing boots.
for side, suffix in ((1.0, "L"), (-1.0, "R")):
    cylinder_between(
        f"UpperLeg_{suffix}",
        f"UpperLeg_{suffix}",
        "cloth",
        (0.20 * side, 0.035, 1.16),
        (0.205 * side, -0.01, 0.61),
        0.19,
        0.82,
        10,
    )
    cylinder_between(
        f"LowerLeg_{suffix}",
        f"LowerLeg_{suffix}",
        "cloth",
        (0.205 * side, -0.01, 0.61),
        (0.205 * side, 0.04, 0.16),
        0.16,
        0.82,
        10,
    )
    sphere_piece(
        f"KneePad_{suffix}",
        f"LowerLeg_{suffix}",
        "leather",
        (0.205 * side, -0.145, 0.61),
        (0.155, 0.065, 0.18),
        10,
        6,
    )
    cylinder_between(
        f"BootShaft_{suffix}",
        f"LowerLeg_{suffix}",
        "leather",
        (0.205 * side, 0.04, 0.34),
        (0.205 * side, 0.04, 0.08),
        0.18,
        0.86,
        9,
    )
    cube_piece(
        f"BootFoot_{suffix}",
        f"Foot_{suffix}",
        "leather",
        (0.205 * side, -0.105, 0.07),
        (0.18, 0.31, 0.115),
    )


def duplicate_static_piece(source: bpy.types.Object, transform: Matrix) -> bpy.types.Object:
    duplicate = source.copy()
    duplicate.data = source.data.copy()
    bpy.context.collection.objects.link(duplicate)
    source_world = source.matrix_world.copy()
    for vertex in duplicate.data.vertices:
        vertex.co = transform @ source_world @ vertex.co
    duplicate.parent = None
    duplicate.matrix_world = Matrix.Identity(4)
    duplicate.vertex_groups.clear()
    for modifier in list(duplicate.modifiers):
        duplicate.modifiers.remove(modifier)
    return duplicate


def join_meshes(meshes: list[bpy.types.Object], name: str) -> bpy.types.Object:
    bpy.ops.object.select_all(action="DESELECT")
    for mesh in meshes:
        mesh.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = name
    joined.data.name = f"{name}_Mesh"
    for polygon in joined.data.polygons:
        polygon.use_smooth = True
    return joined


def export_static_mesh(mesh: bpy.types.Object, output_path: Path) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    mesh.select_set(True)
    bpy.context.view_layer.objects.active = mesh
    bpy.ops.export_scene.fbx(
        filepath=str(output_path),
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Y",
        axis_up="Z",
        mesh_smooth_type="FACE",
        bake_anim=False,
        path_mode="AUTO",
    )


# Export one static mesh per driven bone. Vertices are converted into that
# bone's rest-local coordinates, allowing UE to attach each piece at identity.
rigid_part_paths: list[Path] = []
bone_names = sorted({str(obj["w02_bone"]) for obj in created})
for bone_name in bone_names:
    bone_world = armature.matrix_world @ armature.data.bones[bone_name].matrix_local
    bone_inverse = bone_world.inverted()
    sources = [obj for obj in created if str(obj["w02_bone"]) == bone_name]
    duplicates = [duplicate_static_piece(obj, bone_inverse) for obj in sources]
    asset_name = f"SM_W02_Climber_{bone_name}"
    rigid_part = join_meshes(duplicates, asset_name)
    output_path = RIGID_PART_DIRECTORY / f"{asset_name}.fbx"
    export_static_mesh(rigid_part, output_path)
    rigid_part_paths.append(output_path)
    bpy.data.objects.remove(rigid_part, do_unlink=True)

# Export a full rest-pose palette mesh once so Unreal can create the eight
# shared color materials before the rigid pieces are imported without materials.
palette_duplicates = [duplicate_static_piece(obj, Matrix.Identity(4)) for obj in created]
palette = join_meshes(palette_duplicates, "SM_W02_Climber_MaterialPalette")
export_static_mesh(palette, MATERIAL_PALETTE_FBX)
bpy.data.objects.remove(palette, do_unlink=True)

# Join all geometry into one skinned mesh while preserving material slots and groups.
character = join_meshes(created, "SK_W02_MedievalClimber_LowPoly")
character.name = "SK_W02_MedievalClimber_LowPoly"
character.data.name = "SK_W02_MedievalClimber_LowPoly_Mesh"
character_world = character.matrix_world.copy()
character.parent = armature
character.matrix_world = character_world
for polygon in character.data.polygons:
    polygon.use_smooth = True

triangle_count = sum(len(polygon.vertices) - 2 for polygon in character.data.polygons)
vertex_count = len(character.data.vertices)
if triangle_count > 12000:
    raise RuntimeError(f"Low-poly budget exceeded: triangles={triangle_count}")

bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))

# Export the exact imported bone hierarchy and the generated skinned mesh.
bpy.ops.object.select_all(action="DESELECT")
armature.select_set(True)
rig_root.select_set(True)
character.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.export_scene.fbx(
    filepath=str(OUTPUT_FBX),
    use_selection=True,
    object_types={"EMPTY", "ARMATURE", "MESH"},
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_ALL",
    axis_forward="-Y",
    axis_up="Z",
    mesh_smooth_type="FACE",
    add_leaf_bones=False,
    use_armature_deform_only=False,
    bake_anim=False,
    path_mode="AUTO",
)

# Produce a neutral Blender preview for fast inspection before the UE import.
world = bpy.context.scene.world or bpy.data.worlds.new("W02_PreviewWorld")
bpy.context.scene.world = world
world.color = (0.035, 0.04, 0.05)
bpy.ops.object.light_add(type="AREA", location=(-3.5, -4.0, 5.0))
key = bpy.context.object
key.data.energy = 850.0
key.data.shape = "DISK"
key.data.size = 4.0
bpy.ops.object.light_add(type="AREA", location=(3.0, 1.5, 3.5))
fill = bpy.context.object
fill.data.energy = 500.0
fill.data.size = 3.0
bpy.ops.object.camera_add(location=(4.4, -7.2, 3.15))
camera = bpy.context.object
direction = Vector((0.0, 0.0, 1.38)) - camera.location
camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
camera.data.lens = 58.0
bpy.context.scene.camera = camera
bpy.context.scene.render.engine = "BLENDER_EEVEE_NEXT"
bpy.context.scene.render.resolution_x = 768
bpy.context.scene.render.resolution_y = 1024
bpy.context.scene.render.resolution_percentage = 100
bpy.context.scene.render.image_settings.file_format = "PNG"
bpy.context.scene.render.filepath = str(PREVIEW_PNG)
bpy.context.scene.render.film_transparent = False
bpy.ops.render.render(write_still=True)

print(
    "W02_MEDIEVAL_CLIMBER_BUILD_COMPLETE",
    f"vertices={vertex_count}",
    f"triangles={triangle_count}",
    f"bones={len(armature.data.bones)}",
    f"materials={len(character.data.materials)}",
    f"rigid_parts={len(rigid_part_paths)}",
    f"fbx={OUTPUT_FBX}",
    f"preview={PREVIEW_PNG}",
)
