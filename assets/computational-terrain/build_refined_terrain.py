import bpy
import numpy as np
import math

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

# EEVEE Next Settings
scene.render.engine = 'BLENDER_EEVEE'
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080
scene.render.fps = 30
scene.frame_start = 1
scene.frame_end = 450

eevee = scene.eevee
eevee.taa_render_samples = 32
eevee.use_raytracing = True
eevee.use_shadows = True
eevee.volumetric_tile_size = '2'
eevee.volumetric_samples = 64

# World
world = bpy.data.worlds.new("DeepSpace")
scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.new('ShaderNodeBackground')
bg.inputs['Color'].default_value = (0.002, 0.004, 0.008, 1.0)
bg.inputs['Strength'].default_value = 0.4
out_w = world.node_tree.nodes.new('ShaderNodeOutputWorld')
world.node_tree.links.new(bg.outputs['Background'], out_w.inputs['Surface'])

try:
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - High Contrast'
except Exception:
    pass
scene.view_settings.exposure = 0.20

# 1. Synthesize Terrain
res = 360
size = 260.0
x = np.linspace(-size*0.5, size*0.5, res, dtype=np.float32)
y = np.linspace(-size*0.5, size*0.5, res, dtype=np.float32)
X, Y = np.meshgrid(x, y)

# Canyon axis
canyon_axis = 20.0 * np.sin(Y * 0.022) + 10.0 * np.cos(Y * 0.048)
d_canyon = np.abs(X - canyon_axis)
canyon_depth = -18.0 / (1.0 + (d_canyon / 24.0)**2.6)

# Stepped basalt plateau terraces
plateau = 12.0 * np.sin(X * 0.026 + 0.3) * np.cos(Y * 0.022 - 0.7)
step_h = 4.5
terrace = np.floor(plateau / step_h) * step_h + (step_h * 0.5) * (1.0 + np.sin(np.pi * ((plateau / step_h) % 1.0 - 0.5))) * 0.5

# Mountain rim
r = np.sqrt(X**2 + Y**2)
rim = np.clip((r - 60.0) / 50.0, 0.0, 1.0)
m1 = 1.0 - np.abs(np.sin(X * 0.038 + 0.4) * np.cos(Y * 0.038 + 1.1))
m2 = 1.0 - np.abs(np.sin(X * 0.082 - 0.3) * np.cos(Y * 0.082 + 0.6))
mountains = (m1 * 34.0 + m2 * 18.0) * rim

# Fine erosion fluting & detail
flutes = 1.2 * np.cos(d_canyon * 0.45 + 3.0 * np.sin(Y * 0.12)) * (1.0 - np.clip(d_canyon / 35.0, 0.0, 1.0))
Z = canyon_depth + terrace * (1.0 - rim * 0.6) + mountains + flutes

# Build mesh
verts = np.column_stack((X.ravel(), Y.ravel(), Z.ravel())).tolist()
faces = []
for r_i in range(res - 1):
    r_off = r_i * res
    nr_off = (r_i + 1) * res
    for c_i in range(res - 1):
        p00 = r_off + c_i
        p10 = r_off + c_i + 1
        p01 = nr_off + c_i
        p11 = nr_off + c_i + 1
        faces.append((p00, p10, p11))
        faces.append((p00, p11, p01))

mesh = bpy.data.meshes.new("Mesh_AlienTerrain")
mesh.from_pydata(verts, [], faces)
mesh.update()
mesh.shade_smooth()

# Color attribute
col_attr = mesh.color_attributes.new(name="Col_Rock", type='FLOAT_COLOR', domain='POINT')
z_flat = Z.ravel()
norm_z = np.clip((z_flat + 18.0) / 65.0, 0.0, 1.0)
r_c = 0.03 + 0.03 * norm_z
g_c = 0.04 + 0.04 * norm_z
b_c = 0.06 + 0.07 * norm_z
colors = np.column_stack((r_c, g_c, b_c, np.ones_like(r_c)))
col_attr.data.foreach_set('color', colors.ravel())

terrain_obj = bpy.data.objects.new("Alien_Terrain", mesh)
bpy.context.collection.objects.link(terrain_obj)

# 2. Material: Procedural Basalt with Orthogonal Circuit Veins in Bedrock
mat = bpy.data.materials.new("Mat_AlienGeology")
mat.use_nodes = True
nodes = mat.node_tree.nodes
links = mat.node_tree.links
nodes.clear()

out = nodes.new('ShaderNodeOutputMaterial')
bsdf = nodes.new('ShaderNodeBsdfPrincipled')

# Tex coords
tc = nodes.new('ShaderNodeTexCoord')
mapping = nodes.new('ShaderNodeMapping')
mapping.inputs['Scale'].default_value = (0.2, 0.2, 0.2)
links.new(tc.outputs['Object'], mapping.inputs['Vector'])

# Noise for rock bump
noise = nodes.new('ShaderNodeTexNoise')
noise.inputs['Scale'].default_value = 24.0
noise.inputs['Detail'].default_value = 8.0
noise.inputs['Roughness'].default_value = 0.65
links.new(mapping.outputs['Vector'], noise.inputs['Vector'])

bump = nodes.new('ShaderNodeBump')
bump.inputs['Strength'].default_value = 0.55
bump.inputs['Distance'].default_value = 0.35
links.new(noise.outputs['Fac'], bump.inputs['Height'])
links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])

# Vertex color
vcol = nodes.new('ShaderNodeVertexColor')
vcol.layer_name = "Col_Rock"

# Strata wave
wave = nodes.new('ShaderNodeTexWave')
wave.wave_type = 'BANDS'
wave.bands_direction = 'Z'
wave.inputs['Scale'].default_value = 0.35
wave.inputs['Distortion'].default_value = 2.5
links.new(tc.outputs['Object'], wave.inputs['Vector'])

# Mix base color
mix = nodes.new('ShaderNodeMix')
mix.data_type = 'RGBA'
mix.inputs['Factor'].default_value = 0.25
links.new(vcol.outputs['Color'], mix.inputs[6])
links.new(wave.outputs['Color'], mix.inputs[7])
links.new(mix.outputs[2], bsdf.inputs['Base Color'])

bsdf.inputs['Metallic'].default_value = 0.15
bsdf.inputs['Roughness'].default_value = 0.75

# Subsurface glowing circuit veins (Orthogonal Voronoi in canyon depths)
voronoi = nodes.new('ShaderNodeTexVoronoi')
voronoi.distance = 'MANHATTAN'
voronoi.feature = 'DISTANCE_TO_EDGE'
voronoi.inputs['Scale'].default_value = 1.2
links.new(mapping.outputs['Vector'], voronoi.inputs['Vector'])

# Color ramp for narrow razor vein
cramp = nodes.new('ShaderNodeValToRGB')
cramp.color_ramp.elements[0].position = 0.02
cramp.color_ramp.elements[0].color = (0.0, 0.85, 1.0, 1.0) # Neon cyan
cramp.color_ramp.elements[1].position = 0.05
cramp.color_ramp.elements[1].color = (0.0, 0.0, 0.0, 1.0)
links.new(voronoi.outputs['Distance'], cramp.inputs['Fac'])

# Separate Z to restrict glowing veins only to canyon floor (Z < -4.0)
sep_xyz = nodes.new('ShaderNodeSeparateXYZ')
links.new(tc.outputs['Object'], sep_xyz.inputs['Vector'])
math_z = nodes.new('ShaderNodeMath')
math_z.operation = 'LESS_THAN'
math_z.inputs[1].default_value = -2.0
links.new(sep_xyz.outputs['Z'], math_z.inputs[0])

# Multiply vein mask by canyon depth
math_mult = nodes.new('ShaderNodeMath')
math_mult.operation = 'MULTIPLY'
links.new(cramp.outputs['Color'], math_mult.inputs[0])
links.new(math_z.outputs['Value'], math_mult.inputs[1])

# Emission
bsdf.inputs['Emission Color'].default_value = (0.0, 0.85, 1.0, 1.0)
links.new(math_mult.outputs['Value'], bsdf.inputs['Emission Strength'])

links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
terrain_obj.data.materials.append(mat)

# 3. Transmission Spire
spire_x, spire_y = 22.0, 85.0
bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=3.5, radius2=0.5, depth=52.0, location=(spire_x, spire_y, 26.0))
spire = bpy.context.active_object
spire.name = "Transmission_Pylon"
spire.data.materials.append(mat)

# Spire apex beacon
bpy.ops.mesh.primitive_cylinder_add(radius=0.9, depth=2.0, location=(spire_x, spire_y, 52.0))
beacon = bpy.context.active_object
b_mat = bpy.data.materials.new("Mat_Beacon")
b_mat.use_nodes = True
b_nodes = b_mat.node_tree.nodes
b_nodes.clear()
b_out = b_nodes.new('ShaderNodeOutputMaterial')
b_emit = b_nodes.new('ShaderNodeEmission')
b_emit.inputs['Color'].default_value = (1.0, 0.6, 0.1, 1.0)
b_emit.inputs['Strength'].default_value = 8.0
b_mat.node_tree.links.new(b_emit.outputs['Emission'], b_out.inputs['Surface'])
beacon.data.materials.append(b_mat)

# Skyward Beam
bpy.ops.mesh.primitive_cylinder_add(radius=0.6, depth=160.0, location=(spire_x, spire_y, 132.0))
beam = bpy.context.active_object
beam_mat = bpy.data.materials.new("Mat_Beam")
beam_mat.use_nodes = True
bm_nodes = beam_mat.node_tree.nodes
bm_nodes.clear()
bm_out = bm_nodes.new('ShaderNodeOutputMaterial')
bm_vol = bm_nodes.new('ShaderNodeVolumePrincipled')
bm_vol.inputs['Density'].default_value = 0.09
bm_vol.inputs['Emission Color'].default_value = (0.1, 0.8, 1.0, 1.0)
bm_vol.inputs['Emission Strength'].default_value = 5.0
beam_mat.node_tree.links.new(bm_vol.outputs['Volume'], bm_out.inputs['Volume'])
beam.data.materials.append(beam_mat)

# 4. Volumetric Atmosphere Box
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 25.0))
fog_box = bpy.context.active_object
fog_box.scale = (300.0, 300.0, 90.0)
fog_mat = bpy.data.materials.new("Mat_Haze")
fog_mat.use_nodes = True
f_nodes = fog_mat.node_tree.nodes
f_nodes.clear()
f_out = f_nodes.new('ShaderNodeOutputMaterial')
f_vol = f_nodes.new('ShaderNodeVolumePrincipled')
f_vol.inputs['Density'].default_value = 0.003
f_vol.inputs['Color'].default_value = (0.04, 0.11, 0.20, 1.0)
f_vol.inputs['Anisotropy'].default_value = 0.45
fog_mat.node_tree.links.new(f_vol.outputs['Volume'], f_out.inputs['Volume'])
fog_box.data.materials.append(fog_mat)

# 5. Cinematic Lighting
sun_data = bpy.data.lights.new(name="Sun_Key", type='SUN')
sun_data.energy = 5.2
sun_data.color = (0.88, 0.94, 1.0)
sun_data.angle = math.radians(6.5)
sun = bpy.data.objects.new("Sun_Key", sun_data)
bpy.context.collection.objects.link(sun)
sun.rotation_euler = (math.radians(58.0), math.radians(15.0), math.radians(125.0))

fill_data = bpy.data.lights.new(name="Sun_Fill", type='SUN')
fill_data.energy = 1.6
fill_data.color = (0.05, 0.25, 0.45)
fill = bpy.data.objects.new("Sun_Fill", fill_data)
bpy.context.collection.objects.link(fill)
fill.rotation_euler = (math.radians(40.0), math.radians(-10.0), math.radians(-55.0))

# 6. Looping Camera Flight Path
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 26.0
cam_data.sensor_width = 36.0
cam_data.clip_start = 1.0
cam_data.clip_end = 600.0

cam = bpy.data.objects.new("Cam", cam_data)
bpy.context.collection.objects.link(cam)
scene.camera = cam

target = bpy.data.objects.new("LookTarget", None)
bpy.context.collection.objects.link(target)
track = cam.constraints.new(type='TRACK_TO')
track.target = target
track.track_axis = 'TRACK_NEGATIVE_Z'
track.up_axis = 'UP_Y'

# Animate 450 frames
for f in range(1, 451):
    t = 2.0 * math.pi * float(f - 1) / 450.0
    # Aerial flight high above the canyon: radius 74m, altitude 55m
    cx = 68.0 * math.sin(t) + 14.0 * math.sin(2.0 * t)
    cy = -72.0 * math.cos(t) + 10.0 * math.sin(t)
    cz = 56.0 + 8.0 * math.cos(t) + 3.0 * math.sin(2.0 * t)

    cam.location = (cx, cy, cz)
    cam.keyframe_insert(data_path="location", frame=f)

    # Look target points slightly into canyon floor
    tx = 6.0 * math.sin(t)
    ty = 10.0 * math.cos(t)
    tz = -2.0 + 2.0 * math.cos(t)
    target.location = (tx, ty, tz)
    target.keyframe_insert(data_path="location", frame=f)

blend_path = "/home/venkatengineer/solaris-os/assets/computational-terrain/solaris_computational_terrain.blend"
bpy.ops.wm.save_as_mainfile(filepath=blend_path)
print("Saved refined terrain blend to:", blend_path)
