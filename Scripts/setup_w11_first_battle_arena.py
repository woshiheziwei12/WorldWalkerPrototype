"""Expand the runtime W11 arena and authored Chapter 1 floor.

The current route still enters through the shared runtime map, while Chapter 1
also owns a dedicated map asset. Keep both floors in sync until chapter travel
becomes a separate presentation pass.
"""

import unreal


MAPS = (
    "/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival",
    "/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_Chapter01_Prologue",
)
MIN_FLOOR_SCALE_XY = 32.0


def is_arena_floor(actor):
    if not isinstance(actor, unreal.StaticMeshActor):
        return False
    label = actor.get_actor_label().lower()
    component = actor.get_editor_property("static_mesh_component")
    mesh = component.get_editor_property("static_mesh") if component else None
    mesh_path = mesh.get_path_name().lower() if mesh else ""
    scale = actor.get_actor_scale3d()
    location = actor.get_actor_location()
    return label == "floor" or (
        "/engine/basicshapes/cube" in mesh_path
        and location.z <= 10.0
        and scale.x >= 8.0
        and scale.y >= 8.0
    )


def expand_map(map_path):
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_editor.load_level(map_path):
        raise RuntimeError(f"unable to load arena map: {map_path}")

    floors = [actor for actor in actor_editor.get_all_level_actors() if is_arena_floor(actor)]
    if not floors:
        raise RuntimeError(f"arena floor not found: {map_path}")
    for floor in floors:
        scale = floor.get_actor_scale3d()
        floor.set_actor_scale3d(
            unreal.Vector(
                max(scale.x, MIN_FLOOR_SCALE_XY),
                max(scale.y, MIN_FLOOR_SCALE_XY),
                scale.z,
            )
        )
        unreal.log(
            f"W11_FIRST_ARENA_FLOOR Map={map_path} Actor={floor.get_actor_label()} "
            f"Scale={floor.get_actor_scale3d()}"
        )
    if not level_editor.save_current_level():
        raise RuntimeError(f"unable to save expanded arena map: {map_path}")


def main():
    for map_path in MAPS:
        expand_map(map_path)
    unreal.log(
        "W11_FIRST_BATTLE_ARENA_EXPANDED: Maps=2 FloorScaleXY=32 "
        "CameraWidth=3000 SpawnSafeRadius=650 SpawnOuterRadius=1250"
    )


if __name__ == "__main__":
    main()
