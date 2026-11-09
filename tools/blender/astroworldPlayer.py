"""
astroworldPlayer.py - plays our engine's exported animation inside Blender.

Side project (branch 'blender'). The park's motion (rides, riders, ~140 visitors), the
day/night cycle and the lights come from astroworldExport as data files, NOT as Blender
keyframes (that would be millions of keys). This module applies one frame of that data to
the scene and registers itself as a frame-change handler, so scrubbing the timeline, playing
it back or rendering all move the park.

It is used twice:
  * buildAstroworld.py imports it while building and rendering headless, and
  * the same code is embedded in the saved .blend as the text block "astroworldPlayer.py"
    with "Register" ticked, so it runs when the file is opened in Blender (Blender asks
    whether to allow Python scripts: allow it, or the park stands still).

Everything is found BY NAME (objects "mesh.occurrence", lights "parkLightNN" and
"searchlightN", world nodes "daySky", "nightGradient", "dayNightMix"), so the player
needs no state from the build. The export folder is the .blend's own folder ("//") when
the file is saved next to the data, else the path stored in scene["astroworldData"].
"""
import json
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector

# ---- Look-development constants (tuned by eye on preview sheets) ---------------------------
LIGHT_SCALE = 60.0                   # our point-light radiance -> Blender watts
SUN_SCALE = 1.7                      # our sun radiance -> Blender sun strength (W/m^2)
EXPOSURE = -0.7                      # film exposure in stops (AgX) by day; opens up at night
EMISSION_BY_MESH = {"tunnel": 0.15}  # per-mesh emission trims


# ---- Helpers shared with the build script -----------------------------------------------------
def srgb_to_linear(values):
    return np.power(np.clip(values, 0.0, 1.0), 2.2)


def linear_color(rgb):
    return tuple(float(c) for c in srgb_to_linear(np.array(rgb, dtype=np.float32) / 255.0)) + (1.0,)


def object_color(row, mesh=""):
    """Object colour = (linear tint RGB, emission / 20). Our tints are sRGB, Blender shades in linear."""
    tint = srgb_to_linear(np.asarray(row[12:15], dtype=np.float32))
    return (float(tint[0]), float(tint[1]), float(tint[2]), float(row[15]) / 20.0 * EMISSION_BY_MESH.get(mesh, 1.0))


def to_blender_matrix(row):
    return Matrix(((row[0], row[1], row[2], row[3]), (row[4], row[5], row[6], row[7]), (row[8], row[9], row[10], row[11]), (0, 0, 0, 1)))


def our_direction_to_blender(direction):
    return Vector((direction[0], -direction[2], direction[1]))


def object_name(entry):
    return "%s.%03d" % (entry["mesh"], entry["occurrence"])


# ---- Loaded data and the scene objects it drives ---------------------------------------------
class Player:
    def __init__(self, data_directory):
        with open(os.path.join(data_directory, "objects.json")) as f:
            objects = json.load(f)
        with open(os.path.join(data_directory, "lighting.json")) as f:
            self.lighting = json.load(f)
        self.frame_count = objects["frameCount"]
        # float32 [object][frame][16]: 3x4 row-major model matrix, RGB tint, emission. Read in one
        # go: a Windows Blender sees WSL files over a network share, where a memory map is slow.
        self.frames = np.fromfile(os.path.join(data_directory, "frames.bin"), dtype=np.float32).reshape(
            len(objects["objects"]), self.frame_count, 16)

        self.animated, indices, self.searchlights = [], [], []
        for index, entry in enumerate(objects["objects"]):
            if entry["lightBeam"]:
                light = bpy.data.objects.get("searchlight%d" % index)
                if light is not None:
                    self.searchlights.append((light, index))
            blender_object = bpy.data.objects.get(object_name(entry))
            if entry["animated"] and blender_object is not None:
                self.animated.append((blender_object, entry["mesh"]))
                indices.append(index)
        self.animated_indices = np.array(indices, dtype=np.int64)
        self.point_lights = [bpy.data.objects["parkLight%02d" % slot] for slot in range(self.lighting["maxLights"])
                             if "parkLight%02d" % slot in bpy.data.objects]
        self.dreads = [o for o in bpy.data.objects if o.name.startswith("dreads.")]

    def apply(self, scene, frame_float):
        position = min(max(frame_float - 1.0, 0.0), self.frame_count - 1.0)
        low = int(math.floor(position))
        high = min(low + 1, self.frame_count - 1)
        t = position - low  # sub-frame interpolation keeps motion blur smooth

        rows = self.frames[self.animated_indices, low] * (1.0 - t) + self.frames[self.animated_indices, high] * t
        for (blender_object, mesh), row in zip(self.animated, rows):
            blender_object.matrix_basis = to_blender_matrix(row)
            blender_object.color = object_color(row, mesh)
        for light_object, index in self.searchlights:
            row = self.frames[index, low]
            # Spot lights shine down their local -z; the beam mesh points along its local +y.
            light_object.matrix_basis = to_blender_matrix(row) @ Matrix.Rotation(-math.pi / 2.0, 4, "X")
            light_object.data.energy = 2.5e5 * float(np.mean(row[12:15]))
            light_object.data.color = tuple(float(c) / max(float(np.max(row[12:15])), 1e-3) for c in row[12:15])

        lighting = self.lighting["frames"][low]
        night = float(lighting["night"])
        # A real sunset: the sun sinks below the horizon as night falls.
        day_direction = our_direction_to_blender(self.lighting["frames"][0]["sunDirection"])
        azimuth = math.atan2(day_direction.y, day_direction.x)
        elevation = math.radians(34.0) * (1.0 - night) + math.radians(-8.0) * night
        sun_direction = Vector((math.cos(elevation) * math.cos(azimuth), math.cos(elevation) * math.sin(azimuth), math.sin(elevation)))
        sun, moon = bpy.data.objects.get("Sun"), bpy.data.objects.get("Moon")
        if sun is not None:
            # A sun lamp shines along its local -Z: point that axis away from the sun.
            sun.rotation_euler = (-sun_direction).to_track_quat("-Z", "Y").to_euler()
            sun.data.energy = max(0.0, SUN_SCALE * 3.0 * math.sin(max(elevation, 0.0)) / math.sin(math.radians(34.0)))
        if moon is not None:
            # The moon stays high, opposite the setting sun, lighting the park in cool blue.
            moon_elevation, moon_azimuth = math.radians(42.0), azimuth + math.radians(150.0)
            moon_direction = Vector((math.cos(moon_elevation) * math.cos(moon_azimuth),
                                     math.cos(moon_elevation) * math.sin(moon_azimuth), math.sin(moon_elevation)))
            moon.rotation_euler = (-moon_direction).to_track_quat("-Z", "Y").to_euler()
            moon.data.energy = 1.0 * night
        # Like a real camera, the exposure opens up at night (follows our engine's auto-exposure).
        scene.view_settings.exposure = EXPOSURE + math.log2(float(lighting["exposure"]) / float(self.lighting["frames"][0]["exposure"]))

        world_nodes = scene.world.node_tree.nodes if scene.world and scene.world.node_tree else {}
        sky = world_nodes.get("daySky")
        if sky is not None and hasattr(sky, "sun_elevation"):
            sky.sun_elevation = elevation
            sky.sun_rotation = math.pi / 2.0 - azimuth
        mix = world_nodes.get("dayNightMix")
        if mix is not None:
            mix.inputs[0].default_value = min(1.0, night * 1.4)
        gradient = world_nodes.get("nightGradient")
        if gradient is not None:
            # Our engine's night sky is tuned for its own tone mapper; lift it so the sky reads as a deep
            # purple-blue that also lights the park a little, above a magenta horizon glow.
            horizon = np.array(linear_color(lighting["skyHorizon"])[:3]) * 1.2
            top = np.maximum(np.array(linear_color(lighting["skyTop"])[:3]) * 12.0, (0.012, 0.009, 0.05))
            elements = gradient.color_ramp.elements
            elements[0].position, elements[0].color = 0.5, tuple(horizon) + (1.0,)
            elements[1].position, elements[1].color = 0.7, tuple(top) + (1.0,)

        for slot, light_object in enumerate(self.point_lights):
            if slot < len(lighting["lights"]):
                light_position, radiance = lighting["lights"][slot]
                light_object.location = light_position
                strength = max(radiance)
                light_object.data.energy = LIGHT_SCALE * strength
                light_object.data.color = tuple(c / max(strength, 1e-6) for c in radiance)
            else:
                light_object.data.energy = 0.0

        for dread in self.dreads:
            if dread.data.shape_keys:
                dread.data.shape_keys.key_blocks["Sway"].value = float(lighting["dreadSway"])


# ---- Registration -------------------------------------------------------------------------------
_player = None


def data_directory():
    """The export folder: next to the saved .blend if the data is there, else the stored path."""
    here = bpy.path.abspath("//") if bpy.data.filepath else ""
    if here and os.path.exists(os.path.join(here, "frames.bin")):
        return here
    return bpy.context.scene.get("astroworldData", "")


def on_frame_change(scene, depsgraph=None):
    global _player
    if _player is None:
        directory = data_directory()
        if not directory or not os.path.exists(os.path.join(directory, "frames.bin")):
            return  # no export data next to this file: leave the park as saved
        _player = Player(directory)
    _player.apply(scene, scene.frame_current + scene.frame_subframe)


def register(player=None):
    """Install the frame handler (replacing any older copy), optionally with already-loaded data."""
    global _player
    _player = player
    handlers = bpy.app.handlers.frame_change_pre
    for handler in list(handlers):
        if getattr(handler, "__name__", "") == "on_frame_change":
            handlers.remove(handler)
    handlers.append(on_frame_change)


# Runs on import AND when Blender executes the registered text block as the .blend opens.
register()
