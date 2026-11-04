"""
buildAstroworld.py - rebuild the ASTROWORLD park in Blender from our engine's export.

Side project (branch 'blender'). Our C++ engine (astroworldExport) stays the single
source of truth for geometry and animation; this script only adds what Blender does
best: production materials, a physical sky with a real sunset, volumetric glow,
camera moves, motion blur and high-quality rendering (EEVEE or Cycles).

Run headless (paths are the export folder and outputs, Windows-style if Blender is the
Windows build):

    blender -b -P buildAstroworld.py -- --data <export dir> [--engine eevee|cycles]
            [--render none|still|animation] [--frame 240] [--width 1280 --height 720]
            [--samples 64] [--output <file or folder>] [--save <scene.blend>]

Coordinates: the export uses y-up metres; everything is parented to one root empty
rotated +90 degrees about X, which maps our y-up world into Blender's z-up world.
"""
import argparse
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

# ---------------------------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------------------------
argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
parser = argparse.ArgumentParser()
parser.add_argument("--data", required=True)
parser.add_argument("--engine", default="eevee", choices=["eevee", "cycles"])
parser.add_argument("--render", default="none", choices=["none", "still", "animation"])
parser.add_argument("--frame", type=int, default=1)
parser.add_argument("--width", type=int, default=1280)
parser.add_argument("--height", type=int, default=720)
parser.add_argument("--samples", type=int, default=64)
parser.add_argument("--output", default="")
parser.add_argument("--save", default="")
args = parser.parse_args(argv)
DATA = args.data

with open(os.path.join(DATA, "meshes.json")) as f:
    MESHES = json.load(f)
with open(os.path.join(DATA, "objects.json")) as f:
    OBJECTS = json.load(f)
with open(os.path.join(DATA, "lighting.json")) as f:
    LIGHTING = json.load(f)
FRAME_COUNT = OBJECTS["frameCount"]
FPS = OBJECTS["fps"]
# float32 [object][frame][16]: 3x4 row-major model matrix, RGB tint, emission
FRAMES = np.memmap(os.path.join(DATA, "frames.bin"), dtype=np.float32, mode="r", shape=(len(OBJECTS["objects"]), FRAME_COUNT, 16))
MESH_BIN = np.fromfile(os.path.join(DATA, "meshes.bin"), dtype=np.uint8)

LIGHT_SCALE = 18.0       # our point-light radiance -> Blender watts
SUN_SCALE = 1.7          # our sun radiance -> Blender sun strength (W/m^2)
EMISSION_SCALE = 1.0     # our emission strength -> Blender emission strength

# ---------------------------------------------------------------------------------------------
# Scene reset
# ---------------------------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = FPS
scene.frame_start = 1
scene.frame_end = FRAME_COUNT
root = bpy.data.objects.new("ASTROWORLD", None)
root.rotation_euler = (math.pi / 2.0, 0.0, 0.0)  # our y-up -> Blender z-up
scene.collection.objects.link(root)


def srgb_to_linear(values):
    return np.power(np.clip(values, 0.0, 1.0), 2.2)


def array(offset, count, dtype):
    size = np.dtype(dtype).itemsize * count
    return np.frombuffer(MESH_BIN[offset:offset + size].tobytes(), dtype=dtype)


# ---------------------------------------------------------------------------------------------
# Meshes: built with bulk foreach_set calls (fast even for the 130k-triangle head)
# ---------------------------------------------------------------------------------------------
def build_mesh(name, info):
    vertex_count, index_count = info["vertices"], info["indices"]
    positions = array(info["positions"], vertex_count * 3, np.float32)
    normals = array(info["normals"], vertex_count * 3, np.float32).reshape(-1, 3)
    colors = array(info["colors"], vertex_count * 3, np.float32).reshape(-1, 3)
    uvs = array(info["uvs"], vertex_count * 2, np.float32).reshape(-1, 2)
    triangles = array(info["triangles"], index_count, np.uint32).astype(np.int32)
    triangle_count = index_count // 3

    mesh = bpy.data.meshes.new(name)
    mesh.vertices.add(vertex_count)
    mesh.vertices.foreach_set("co", positions)
    mesh.loops.add(index_count)
    mesh.loops.foreach_set("vertex_index", triangles)
    mesh.polygons.add(triangle_count)
    mesh.polygons.foreach_set("loop_start", np.arange(0, index_count, 3, dtype=np.int32))
    try:
        mesh.polygons.foreach_set("loop_total", np.full(triangle_count, 3, dtype=np.int32))
    except (AttributeError, TypeError, RuntimeError):
        pass  # read-only and derived from loop_start in newer Blender versions
    mesh.update()

    # Vertex colours are sRGB in the export; Blender's float colour attributes are linear.
    rgba = np.ones((vertex_count, 4), dtype=np.float32)
    rgba[:, :3] = srgb_to_linear(colors)
    attribute = mesh.color_attributes.new("Col", "FLOAT_COLOR", "POINT")
    attribute.data.foreach_set("color", rgba.ravel())
    uv_layer = mesh.uv_layers.new(name="UVMap")
    uv_layer.data.foreach_set("uv", uvs[triangles].ravel())

    if hasattr(mesh, "shade_smooth"):
        mesh.shade_smooth()
    else:
        mesh.polygons.foreach_set("use_smooth", np.ones(triangle_count, dtype=bool))
    if hasattr(mesh, "use_auto_smooth"):
        mesh.use_auto_smooth = True  # Blender < 4.1 needs this for custom normals
    mesh.normals_split_custom_set_from_vertices(normals)

    if "swayPositions" in info:  # the dreads' wind bend as a shape key
        bend = array(info["swayPositions"], vertex_count * 3, np.float32)
        holder = bpy.data.objects.new("_shapeKeyHolder", mesh)
        scene.collection.objects.link(holder)
        holder.shape_key_add(name="Basis")
        sway = holder.shape_key_add(name="Sway")
        sway.data.foreach_set("co", bend)
        sway.slider_min = -2.0
        sway.slider_max = 2.0
        bpy.data.objects.remove(holder)
    return mesh


MESH_DATA = {name: build_mesh(name, info) for name, info in MESHES.items()}

# ---------------------------------------------------------------------------------------------
# Materials (Principled BSDF). Object colour carries each instance's tint (RGB) and its
# emission strength (alpha x 20), so one material serves thousands of instances.
# ---------------------------------------------------------------------------------------------
def principled_material(name, metallic, roughness, texture=None, coat=0.0, emission_from_object=True):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    bsdf = nodes["Principled BSDF"]
    vertex_color = nodes.new("ShaderNodeVertexColor")
    vertex_color.layer_name = "Col"
    object_info = nodes.new("ShaderNodeObjectInfo")
    base = nodes.new("ShaderNodeMix")
    base.data_type = "RGBA"
    base.blend_type = "MULTIPLY"
    base.inputs["Factor"].default_value = 1.0
    links.new(vertex_color.outputs["Color"], base.inputs[6])
    links.new(object_info.outputs["Color"], base.inputs[7])
    color_out = base.outputs[2]
    if texture is not None:
        image_node = nodes.new("ShaderNodeTexImage")
        image_node.image = texture
        uv = nodes.new("ShaderNodeUVMap")
        uv.uv_map = "UVMap"
        links.new(uv.outputs["UV"], image_node.inputs["Vector"])
        textured = nodes.new("ShaderNodeMix")
        textured.data_type = "RGBA"
        textured.blend_type = "MULTIPLY"
        textured.inputs["Factor"].default_value = 1.0
        links.new(color_out, textured.inputs[6])
        links.new(image_node.outputs["Color"], textured.inputs[7])
        color_out = textured.outputs[2]
        # Fine surface relief from the texture's brightness (concrete grain, joints).
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.25
        links.new(image_node.outputs["Color"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    links.new(color_out, bsdf.inputs["Base Color"])
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if coat > 0.0 and "Coat Weight" in bsdf.inputs:
        bsdf.inputs["Coat Weight"].default_value = coat
        bsdf.inputs["Coat Roughness"].default_value = 0.08
    if emission_from_object:
        links.new(color_out, bsdf.inputs["Emission Color"])
        strength = nodes.new("ShaderNodeMath")
        strength.operation = "MULTIPLY"
        strength.inputs[1].default_value = 20.0 * EMISSION_SCALE
        links.new(object_info.outputs["Alpha"], strength.inputs[0])
        links.new(strength.outputs[0], bsdf.inputs["Emission Strength"])
    return material


def load_texture(name):
    path = os.path.join(DATA, name)
    image = bpy.data.images.load(path)
    image.pack()  # keep the saved .blend self-contained
    return image


TEXTURES = {"ground": load_texture("ground.bmp"), "sign": load_texture("sign.bmp")}
MATERIALS = {}


def material_for(entry):
    mesh = entry["mesh"]
    if mesh in ("head", "dreads"):
        key = "goldVinyl"  # glossy inflatable: metallic gold under a clear coat
        if key not in MATERIALS:
            MATERIALS[key] = principled_material(key, 0.85, 0.28, coat=0.6, emission_from_object=False)
        return MATERIALS[key]
    if mesh == "mirrorBall":
        key = "chrome"
        if key not in MATERIALS:
            MATERIALS[key] = principled_material(key, 1.0, 0.02, emission_from_object=False)
        return MATERIALS[key]
    texture = entry["texture"] or None
    key = "m%.2f_r%.2f_%s" % (entry["metallic"], entry["roughness"], texture or "")
    if key not in MATERIALS:
        MATERIALS[key] = principled_material(key, entry["metallic"], entry["roughness"], TEXTURES.get(texture) if texture else None)
    return MATERIALS[key]


# ---------------------------------------------------------------------------------------------
# Objects
# ---------------------------------------------------------------------------------------------
park = bpy.data.collections.new("Park")
scene.collection.children.link(park)
ANIMATED = []        # (object, index) pairs moved by the frame handler
SEARCHLIGHTS = []    # (light object, index): beams become volumetric spot lights


def to_blender_matrix(row):
    return Matrix(((row[0], row[1], row[2], row[3]), (row[4], row[5], row[6], row[7]), (row[8], row[9], row[10], row[11]), (0, 0, 0, 1)))


for index, entry in enumerate(OBJECTS["objects"]):
    first = FRAMES[index, 0]
    if entry["lightBeam"]:
        # A searchlight: a spot light shining along the beam's local +y axis.
        light = bpy.data.lights.new("searchlight%d" % index, "SPOT")
        light.spot_size = math.radians(9.0)
        light.spot_blend = 0.4
        light.energy = 0.0
        light_object = bpy.data.objects.new(light.name, light)
        light_object.parent = root
        park.objects.link(light_object)
        SEARCHLIGHTS.append((light_object, index))
        continue
    mesh = MESH_DATA.get(entry["mesh"])
    if mesh is None:
        continue
    blender_object = bpy.data.objects.new("%s.%03d" % (entry["mesh"], entry["occurrence"]), mesh)
    blender_object.parent = root
    # Instances SHARE mesh data, so the material lives on the object's own slot.
    if len(mesh.materials) == 0:
        mesh.materials.append(None)
    blender_object.material_slots[0].link = "OBJECT"
    blender_object.material_slots[0].material = material_for(entry)
    blender_object.matrix_basis = to_blender_matrix(first)
    blender_object.color = (float(first[12]), float(first[13]), float(first[14]), float(first[15]) / 20.0)
    if entry["mesh"] in ("dreads", "personTorso", "personHead", "rider", "riderHead", "balloon", "horse"):
        modifier = blender_object.modifiers.new("Smooth", "SUBSURF")
        modifier.levels = 1
        modifier.render_levels = 2 if entry["mesh"] == "dreads" else 1
    park.objects.link(blender_object)
    if entry["animated"]:
        ANIMATED.append((blender_object, index))

ANIMATED_INDICES = np.array([index for _, index in ANIMATED], dtype=np.int64)

# Volumetric glow pouring out of the mouth (the album cover's golden haze).
haze_mesh = bpy.data.meshes.new("mouthHaze")
bpy_vertices = [(x, y, z) for x in (-3.2, 3.2) for y in (0.0, 4.5) for z in (-6.0, 9.0)]
haze_faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
haze_mesh.from_pydata(bpy_vertices, [], haze_faces)
haze = bpy.data.objects.new("mouthHaze", haze_mesh)
haze.parent = root
park.objects.link(haze)
haze_material = bpy.data.materials.new("mouthHaze")
haze_material.use_nodes = True
nodes = haze_material.node_tree.nodes
nodes.remove(nodes["Principled BSDF"])
volume = nodes.new("ShaderNodePrincipledVolume")
volume.inputs["Density"].default_value = 0.06
volume.inputs["Color"].default_value = (1.0, 0.75, 0.4, 1.0)
volume.inputs["Emission Color"].default_value = (1.0, 0.55, 0.18, 1.0)
volume.inputs["Emission Strength"].default_value = 0.6
haze_material.node_tree.links.new(volume.outputs[0], nodes["Material Output"].inputs["Volume"])
haze.active_material = haze_material

# ---------------------------------------------------------------------------------------------
# Lights: the sun (with a real sunset), a moon, and the park's point lights
# ---------------------------------------------------------------------------------------------
sun_data = bpy.data.lights.new("Sun", "SUN")
sun_data.angle = math.radians(0.6)  # soft-edged shadows like the real sun's disc
sun = bpy.data.objects.new("Sun", sun_data)
scene.collection.objects.link(sun)
moon_data = bpy.data.lights.new("Moon", "SUN")
moon_data.color = (0.6, 0.7, 1.0)
moon = bpy.data.objects.new("Moon", moon_data)
scene.collection.objects.link(moon)
POINT_LIGHTS = []
for slot in range(LIGHTING["maxLights"]):
    data = bpy.data.lights.new("parkLight%02d" % slot, "POINT")
    data.shadow_soft_size = 0.25
    light_object = bpy.data.objects.new(data.name, data)
    light_object.parent = root
    scene.collection.objects.link(light_object)
    POINT_LIGHTS.append(light_object)

# ---------------------------------------------------------------------------------------------
# World: physical (Nishita) sky by day, our purple starry sky at night
# ---------------------------------------------------------------------------------------------
world = bpy.data.worlds.new("Sky")
scene.world = world
world.use_nodes = True
wn, wl = world.node_tree.nodes, world.node_tree.links
background = wn["Background"]
sky = wn.new("ShaderNodeTexSky")
if hasattr(sky, "sky_type"):
    sky.sky_type = "NISHITA"
day_background = wn.new("ShaderNodeBackground")
wl.new(sky.outputs["Color"], day_background.inputs["Color"])
night_gradient = wn.new("ShaderNodeValToRGB")
texture_coordinates = wn.new("ShaderNodeTexCoord")
separate = wn.new("ShaderNodeSeparateXYZ")
wl.new(texture_coordinates.outputs["Generated"], separate.inputs[0])
wl.new(separate.outputs["Z"], night_gradient.inputs["Fac"])
stars = wn.new("ShaderNodeTexVoronoi")
stars.inputs["Scale"].default_value = 420.0
star_ramp = wn.new("ShaderNodeValToRGB")
star_ramp.color_ramp.elements[0].position = 0.0
star_ramp.color_ramp.elements[0].color = (1, 1, 1, 1)
star_ramp.color_ramp.elements[1].position = 0.025
star_ramp.color_ramp.elements[1].color = (0, 0, 0, 1)
wl.new(stars.outputs["Distance"], star_ramp.inputs["Fac"])
night_mix = wn.new("ShaderNodeMix")
night_mix.data_type = "RGBA"
night_mix.blend_type = "ADD"
night_mix.inputs["Factor"].default_value = 0.6
wl.new(night_gradient.outputs["Color"], night_mix.inputs[6])
wl.new(star_ramp.outputs["Color"], night_mix.inputs[7])
night_background = wn.new("ShaderNodeBackground")
wl.new(night_mix.outputs[2], night_background.inputs["Color"])
world_mix = wn.new("ShaderNodeMixShader")
wl.new(day_background.outputs[0], world_mix.inputs[1])
wl.new(night_background.outputs[0], world_mix.inputs[2])
wl.new(world_mix.outputs[0], wn["World Output"].inputs["Surface"])
wn.remove(background)
# Night-time air: a faint volume so searchlight beams become visible shafts of light.
world_volume = wn.new("ShaderNodeVolumeScatter")
world_volume.inputs["Density"].default_value = 0.0
wl.new(world_volume.outputs[0], wn["World Output"].inputs["Volume"])


def linear_color(rgb):
    return tuple(float(c) for c in srgb_to_linear(np.array(rgb, dtype=np.float32) / 255.0)) + (1.0,)


def set_night_gradient(frame_lighting):
    elements = night_gradient.color_ramp.elements
    elements[0].position, elements[0].color = 0.45, linear_color(frame_lighting["skyHorizon"])
    elements[1].position, elements[1].color = 0.75, linear_color(frame_lighting["skyTop"])


# ---------------------------------------------------------------------------------------------
# Per-frame update: transforms, tints/emission, lights, sky - all from the export
# ---------------------------------------------------------------------------------------------
dread_objects = [o for o in park.objects if o.data is not None and getattr(o.data, "name", "") == "dreads"]


def our_direction_to_blender(direction):
    return Vector((direction[0], -direction[2], direction[1]))


def apply_frame(frame_float):
    position = min(max(frame_float - 1.0, 0.0), FRAME_COUNT - 1.0)
    low = int(math.floor(position))
    high = min(low + 1, FRAME_COUNT - 1)
    t = position - low

    rows = FRAMES[ANIMATED_INDICES, low] * (1.0 - t) + FRAMES[ANIMATED_INDICES, high] * t
    for (blender_object, _), row in zip(ANIMATED, rows):
        blender_object.matrix_basis = to_blender_matrix(row)
        blender_object.color = (float(row[12]), float(row[13]), float(row[14]), float(row[15]) / 20.0)
    for light_object, index in SEARCHLIGHTS:
        row = FRAMES[index, low]
        matrix = to_blender_matrix(row)
        # Spot lights shine down their local -z; the beam mesh points along its local +y.
        light_object.matrix_basis = matrix @ Matrix.Rotation(-math.pi / 2.0, 4, "X")
        light_object.data.energy = 2.5e5 * float(np.mean(row[12:15]))
        light_object.data.color = tuple(float(c) / max(float(np.max(row[12:15])), 1e-3) for c in row[12:15])

    lighting = LIGHTING["frames"][low]
    night = float(lighting["night"])
    # A real sunset: the sun sinks below the horizon as night falls.
    day_direction = our_direction_to_blender(LIGHTING["frames"][0]["sunDirection"])
    azimuth = math.atan2(day_direction.y, day_direction.x)
    elevation = math.radians(34.0) * (1.0 - night) + math.radians(-8.0) * night
    sun_direction = Vector((math.cos(elevation) * math.cos(azimuth), math.cos(elevation) * math.sin(azimuth), math.sin(elevation)))
    # A sun lamp shines along its local -Z: point that axis away from the sun, i.e. along -sun_direction.
    sun.rotation_euler = (-sun_direction).to_track_quat("-Z", "Y").to_euler()
    sun_data.energy = max(0.0, SUN_SCALE * 3.0 * math.sin(max(elevation, 0.0)) / math.sin(math.radians(34.0)))
    moon.rotation_euler = (-our_direction_to_blender(lighting["sunDirection"])).to_track_quat("-Z", "Y").to_euler()
    moon_data.energy = 0.25 * night
    if hasattr(sky, "sun_elevation"):
        sky.sun_elevation = elevation
        sky.sun_rotation = math.pi / 2.0 - azimuth
    world_mix.inputs[0].default_value = min(1.0, night * 1.4)
    world_volume.inputs["Density"].default_value = 0.004 * night
    set_night_gradient(lighting)

    for slot, light_object in enumerate(POINT_LIGHTS):
        if slot < len(lighting["lights"]):
            light_position, radiance = lighting["lights"][slot]
            light_object.location = light_position
            strength = max(radiance)
            light_object.data.energy = LIGHT_SCALE * strength
            light_object.data.color = tuple(c / max(strength, 1e-6) for c in radiance)
        else:
            light_object.data.energy = 0.0

    for dread in dread_objects:
        if dread.data.shape_keys:
            dread.data.shape_keys.key_blocks["Sway"].value = float(lighting["dreadSway"])


def on_frame_change(scene_value, depsgraph=None):
    apply_frame(scene_value.frame_current + scene_value.frame_subframe)


bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(on_frame_change)

# ---------------------------------------------------------------------------------------------
# Camera: a designed fly-through (times in seconds, positions in OUR coordinates)
# ---------------------------------------------------------------------------------------------
camera_data = bpy.data.cameras.new("Camera")
camera = bpy.data.objects.new("Camera", camera_data)
camera.parent = root
scene.collection.objects.link(camera)
scene.camera = camera
target = bpy.data.objects.new("CameraTarget", None)
target.parent = root
scene.collection.objects.link(target)
tracking = camera.constraints.new("TRACK_TO")
tracking.target = target
tracking.track_axis = "TRACK_NEGATIVE_Z"
tracking.up_axis = "UP_Y"
camera_data.dof.use_dof = True
camera_data.dof.focus_object = target
camera_data.dof.aperture_fstop = 5.6

SHOTS = [  # (seconds, eye, look-at, lens mm)
    (0.0, (0.0, 6.0, 62.0), (0.0, 7.0, 0.0), 35),
    (6.0, (7.0, 3.2, 22.0), (0.0, 3.5, 2.0), 32),
    (10.0, (0.6, 1.9, 7.5), (0.0, 2.0, -12.0), 24),
    (13.0, (0.0, 2.2, -9.0), (6.0, 4.0, -40.0), 24),
    (18.0, (12.0, 13.0, -20.0), (14.0, 3.0, -44.0), 32),
    (23.0, (-4.0, 9.0, -36.0), (-22.0, 8.0, -50.0), 32),
    (27.0, (22.0, 19.0, -28.0), (38.0, 15.0, -52.0), 30),
    (30.0, (-30.0, 46.0, 34.0), (0.0, 5.0, -45.0), 28),
]
camera.rotation_mode = "XYZ"
for seconds, eye, look, lens in SHOTS:
    frame = 1 + int(round(seconds * FPS))
    if frame > FRAME_COUNT:
        continue
    camera.location = eye
    camera.keyframe_insert("location", frame=frame)
    target.location = look
    target.keyframe_insert("location", frame=frame)
    camera_data.lens = lens
    camera_data.keyframe_insert("lens", frame=frame)

# ---------------------------------------------------------------------------------------------
# Render settings: AgX colour, bloom, motion blur
# ---------------------------------------------------------------------------------------------
render = scene.render
render.resolution_x, render.resolution_y = args.width, args.height
render.use_motion_blur = True
scene.view_settings.view_transform = "AgX"
try:
    scene.view_settings.look = "AgX - Punchy"
except TypeError:
    pass
if args.engine == "cycles":
    render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for backend in ("OPTIX", "CUDA", "HIP", "ONEAPI", "METAL"):
        try:
            prefs.compute_device_type = backend
            prefs.get_devices()
            if any(device.type == backend for device in prefs.devices):
                for device in prefs.devices:
                    device.use = True
                scene.cycles.device = "GPU"
                break
        except TypeError:
            continue
else:
    engines = [item.identifier for item in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items]
    render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in engines else "BLENDER_EEVEE"
    eevee = scene.eevee
    eevee.taa_render_samples = args.samples
    for attribute, value in (("use_raytracing", True), ("use_shadows", True), ("use_volumetric_shadows", True), ("use_gtao", True), ("use_bloom", True)):
        if hasattr(eevee, attribute):
            setattr(eevee, attribute, value)

# Lens glare (bloom) in the compositor - EEVEE Next and Cycles have no built-in bloom.
scene.use_nodes = True
compositor = scene.node_tree
render_layers = compositor.nodes.get("Render Layers") or compositor.nodes.new("CompositorNodeRLayers")
composite = compositor.nodes.get("Composite") or compositor.nodes.new("CompositorNodeComposite")
glare = compositor.nodes.new("CompositorNodeGlare")
glare.glare_type = "BLOOM" if "BLOOM" in [item.identifier for item in glare.bl_rna.properties["glare_type"].enum_items] else "FOG_GLOW"
glare.threshold = 1.0
if hasattr(glare, "mix"):
    glare.mix = -0.6
compositor.links.new(render_layers.outputs["Image"], glare.inputs["Image"])
compositor.links.new(glare.outputs["Image"], composite.inputs["Image"])

apply_frame(float(args.frame))
scene.frame_set(args.frame)
print("ASTROWORLD: %d objects (%d animated), %d frames at %d fps, %d point lights, engine %s"
      % (len(OBJECTS["objects"]), len(ANIMATED), FRAME_COUNT, FPS, len(POINT_LIGHTS), render.engine))

if args.save:
    bpy.ops.wm.save_as_mainfile(filepath=args.save)
if args.render == "still":
    render.filepath = args.output or os.path.join(DATA, "still.png")
    bpy.ops.render.render(write_still=True)
elif args.render == "animation":
    render.filepath = args.output or os.path.join(DATA, "frames", "frame_")
    render.image_settings.file_format = "PNG"
    bpy.ops.render.render(animation=True)
