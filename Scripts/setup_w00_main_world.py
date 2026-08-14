"""Create and configure the authored W00 night-world map.

Run with UnrealEditor-Cmd after the Editor target compiles. The source Fab map
is never modified; W00 receives its own duplicate and WorldDefinition reference.
"""

import unreal


SOURCE_MAP = "/Game/Asian_Village/maps/Asian_Village_Demo"
DESTINATION_MAP = (
    "/Game/WorldWalker/Worlds/W00_MainWorld/Maps/L_W00_MainWorld_Night"
)
WORLD_DEFINITION = (
    "/Game/WorldWalker/Worlds/W00_MainWorld/Data/DA_W00_MainWorld"
)
NIGHT_SKY_TAG = unreal.Name("W00NightSky")


def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Unable to load required asset: {path}")
    return asset


def ensure_map_copy():
    if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION_MAP):
        unreal.log(f"Keeping existing W00 map copy: {DESTINATION_MAP}")
        return

    duplicated = unreal.EditorAssetLibrary.duplicate_asset(
        SOURCE_MAP,
        DESTINATION_MAP,
    )
    if duplicated is None:
        raise RuntimeError(
            f"Unable to duplicate {SOURCE_MAP} to {DESTINATION_MAP}"
        )
    unreal.log(f"Created W00 authored map copy: {DESTINATION_MAP}")


def set_property_if_available(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception as exc:
        unreal.log_warning(
            f"W00 setup skipped {obj.get_name()}.{property_name}: {exc}"
        )
        return False


def hide_actor_visuals(actor):
    actor.set_actor_hidden_in_game(True)
    for component in actor.get_components_by_class(unreal.PrimitiveComponent):
        set_property_if_available(component, "visible", False)
        set_property_if_available(component, "hidden_in_game", True)


def configure_authored_level():
    loaded_world = unreal.EditorLoadingAndSavingUtils.load_map(DESTINATION_MAP)
    if loaded_world is None:
        raise RuntimeError(f"Unable to load W00 map copy: {DESTINATION_MAP}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())

    # The vendor demo contains a bright stationary sun and baked daytime data.
    # Runtime treatment is authoritative; these changes make the editor preview
    # agree and prevent the old build data from washing the night scene out.
    editor_world = unreal.EditorLevelLibrary.get_editor_world()
    world_settings = editor_world.get_world_settings()
    set_property_if_available(
        world_settings,
        "force_no_precomputed_lighting",
        True,
    )

    for actor in actors:
        class_name = actor.get_class().get_name()
        if class_name == "DirectionalLight":
            component = actor.get_component_by_class(
                unreal.DirectionalLightComponent
            )
            if component:
                set_property_if_available(
                    component,
                    "mobility",
                    unreal.ComponentMobility.MOVABLE,
                )
                set_property_if_available(component, "intensity", 0.60)
                set_property_if_available(
                    component,
                    "light_color",
                    unreal.Color(110, 148, 255, 255),
                )
                actor.set_actor_rotation(
                    unreal.Rotator(pitch=-34.0, yaw=-42.0, roll=0.0),
                    False,
                )
        elif class_name == "SkyLight":
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            if component:
                set_property_if_available(
                    component,
                    "mobility",
                    unreal.ComponentMobility.MOVABLE,
                )
                set_property_if_available(component, "intensity", 0.20)
                set_property_if_available(
                    component,
                    "light_color",
                    unreal.Color(100, 135, 255, 255),
                )
                source_type = None
                if hasattr(unreal, "SkyLightSourceType"):
                    source_type = getattr(
                        unreal.SkyLightSourceType,
                        "SLS_CAPTURED_SCENE",
                        None,
                    )
                    if source_type is None:
                        source_type = getattr(
                            unreal.SkyLightSourceType,
                            "CAPTURED_SCENE",
                            None,
                        )
                if source_type is not None:
                    set_property_if_available(
                        component,
                        "source_type",
                        source_type,
                    )
                set_property_if_available(component, "cubemap", None)
        elif class_name == "ExponentialHeightFog":
            component = actor.get_component_by_class(
                unreal.ExponentialHeightFogComponent
            )
            if component:
                set_property_if_available(component, "fog_density", 0.0065)
                set_property_if_available(component, "fog_height_falloff", 0.08)
                set_property_if_available(component, "start_distance", 1600.0)
                set_property_if_available(component, "enable_volumetric_fog", True)
        elif class_name == "PostProcessVolume":
            # W00 applies its own runtime night grade.  The vendor daylight
            # volume otherwise stacks a heavy radial blur/bloom over the portal.
            set_property_if_available(actor, "enabled", False)
            set_property_if_available(actor, "blend_weight", 0.0)

        if "BP_Sky_Sphere" in class_name:
            hide_actor_visuals(actor)
            continue

        for component in actor.get_components_by_class(
            unreal.StaticMeshComponent
        ):
            mesh = component.static_mesh
            if mesh and "/Game/Asian_Village/meshes/sky/SM_sky" in mesh.get_path_name():
                set_property_if_available(component, "visible", False)
                set_property_if_available(component, "hidden_in_game", True)

    ensure_editor_night_sky(actor_subsystem, actors)

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError(f"Unable to save configured W00 map: {DESTINATION_MAP}")


def ensure_editor_night_sky(actor_subsystem, actors):
    existing = next(
        (
            actor
            for actor in actors
            if NIGHT_SKY_TAG in list(actor.get_editor_property("tags"))
        ),
        None,
    )
    if existing:
        unreal.log("Keeping existing W00 editor-preview night sky")
        return

    sky_mesh = load_asset("/Engine/MapTemplates/Sky/SM_SkySphere")
    sky_material = load_asset(
        "/Engine/MapTemplates/Sky/M_BlackBackground"
    )
    night_sky = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(0.0, 0.0, -1000.0),
        unreal.Rotator(),
        False,
    )
    if night_sky is None:
        raise RuntimeError("Unable to spawn W00 editor-preview night sky")

    night_sky.set_actor_label("W00_NightSky_Preview")
    night_sky.set_editor_property(
        "tags",
        list(night_sky.get_editor_property("tags")) + [NIGHT_SKY_TAG],
    )
    night_sky.set_actor_scale3d(unreal.Vector(100.0, 100.0, 100.0))
    component = night_sky.get_component_by_class(unreal.StaticMeshComponent)
    if component is None:
        raise RuntimeError("W00 editor-preview sky has no StaticMeshComponent")
    component.set_static_mesh(sky_mesh)
    component.set_material(0, sky_material)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    set_property_if_available(component, "cast_shadow", False)
    unreal.log("Created W00 editor-preview procedural night sky")


def update_world_definition():
    definition = load_asset(WORLD_DEFINITION)
    map_asset = load_asset(DESTINATION_MAP)
    definition.set_editor_property("entry_map", map_asset)
    unreal.EditorAssetLibrary.save_loaded_asset(
        definition,
        only_if_is_dirty=False,
    )
    unreal.log(f"Updated W00 WorldDefinition entry map: {DESTINATION_MAP}")


def main():
    ensure_map_copy()
    configure_authored_level()
    update_world_definition()
    unreal.log(
        "W00_MAIN_WORLD_SETUP_COMPLETE "
        "map=L_W00_MainWorld_Night source=Asian_Village_Demo"
    )


main()
