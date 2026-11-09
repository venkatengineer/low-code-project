import bpy
import numpy as np
import math

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

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
bg.inputs['Color'].default_value = (0.001, 0.002, 0.005, 1.0)
bg.inputs['Strength'].default_value = 0.3
out_w = world.node_tree.nodes.new('ShaderNodeOutputWorld')
world.node_tree.links.new(bg.outputs['Background'], out_w.inputs['Surface'])

try:
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - High Contrast'
except Exception:
    pass
scene.view_settings.exposure = 0.20

# 1. High-Density Photorealistic Fractal Terrain Synthesis (380x380)
res = 380
size_x = 240.0
size_y = 260.0
x = np.linspace(-size_x*0.5, size_x*0.5, res, dtype=np.float32)
y = np.linspace(-size_y*0.5, size_y*0.5, res, dtype=np.float32)
X, Y = np.meshgrid(x, y)

# Macro canyon chasm
canyon_x = 16.0 * np.sin(Y * 0.022) + 6.0 * np.cos(Y * 0.048)
d_canyon = np.abs(X - canyon_x)
canyon_w = 24.0 + 5.0 * np.sin(Y * 0.032)
u = np.clip(d_canyon / canyon_w, 0.0, 1.0)
canyon_depth = -19.0 * (1.0 - (u ** 2.8))

# Stepped basalt plateau terraces
plateau = 9.0 + 6.0 * np.sin(X * 0.028 + 0.4) * np.cos(Y * 0.024 - 0.5)
step_h = 4.2
terrace = np.floor(plateau / step_h) * step_h + (step_h * 0.5) * (1.0 + np.sin(np.pi * ((plateau / step_h) % 1.0 - 0.5))) * 0.5

# Mountain rim on northern horizon and far flanks
north_dist = np.clip((Y - 15.0) / 95.0, 0.0, 1.0)
flank_dist = np.clip((np.abs(X) - 45.0) / 65.0, 0.0, 1.0)
rim = np.maximum(north_dist, flank_dist)

# Multi-octave ridged noise for mountains
r1 = 1.0 - np.abs(np.sin(X * 0.042 + 0.4) * np.cos(Y * 0.042 + 1.1))
r2 = 1.0 - np.abs(np.sin(X * 0.088 - 0.3) * np.cos(Y * 0.088 + 0.6))
r3 = 1.0 - np.abs(np.sin(X * 0.175 + 0.9) * np.cos(Y * 0.175 - 0.4))
mountains = (r1 * 36.0 + r2 * 18.0 + r3 * 8.0) * (rim ** 1.6)

# Multi-octave fractal rock roughness across entire terrain
np.random.seed(42)
fractal_noise = np.zeros_like(X)
freqs = [0.12, 0.28, 0.65, 1.45, 3.2]
amps = [2.8, 1.4, 0.65, 0.28, 0.12]
angles = [0.35, 1.12, 2.05, 0.78, 2.85]
for f, a, ang in zip(freqs, amps, angles):
    cos_a = math.cos(ang)
    sin_a = math.sin(ang)
    xr = X * cos_a + Y * sin_a
    yr = -X * sin_a + Y * cos_a
    fractal_noise += a * (np.sin(xr * f + 0.5) * np.cos(yr * f + 1.2))

# Erosion flutes along canyon walls
flutes = 1.4 * np.cos(d_canyon * 0.75 + 3.0 * np.sin(Y * 0.14)) * (1.0 - u) * (u ** 0.5)

Z = (terrace * u) + canyon_depth + mountains + fractal_noise + flutes

# Build Mesh
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

# Color attribute: Natural Basalt & Mineral Tone Variation
col_attr = mesh.color_attributes.new(name="Col_Rock", type='FLOAT_COLOR', domain='POINT')
z_flat = Z.ravel()
norm_z = np.clip((z_flat + 19.0) / 75.0, 0.0, 1.0)

# Rock tones: Rich dark basalt charcoal (#080c10), midnight mineral blue, subtle warm iron oxide
r_c = 0.028 + 0.035 * norm_z
g_c = 0.038 + 0.040 * norm_z
b_c = 0.052 + 0.065 * norm_z

# Add slope differentiation: steep slopes reveal dark obsidian scree
dz_dx = np.gradient(Z, axis=1).ravel()
dz_dy = np.gradient(Z, axis=0).ravel()
slope = np.sqrt(dz_dx**2 + dz_dy**2)
slope_norm = np.clip(slope / 2.5, 0.0, 1.0)

# Darker on steep cliffs
r_c = r_c * (1.0 - 0.35 * slope_norm)
g_c = g_c * (1.0 - 0.35 * slope_norm)
b_c = b_c * (1.0 - 0.30 * slope_norm)

colors = np.column_stack((r_c, g_c, b_c, np.ones_like(r_c)))
col_attr.data.foreach_set('color', colors.ravel())

# Conduit Mask Attribute: Integrated directly into the bedrock!
conduit_attr = mesh.color_attributes.new(name="Col_Conduit", type='FLOAT_COLOR', domain='POINT')
d_canyon_flat = d_canyon.ravel()
y_flat = Y.ravel()
# Narrow 0.8m wide channel along canyon floor, fading at ends
conduit_mask = np.clip(1.0 - (d_canyon_flat / 0.55), 0.0, 1.0) * np.clip(1.0 - (np.abs(y_flat) / 85.0), 0.0, 1.0)
conduit_mask = conduit_mask ** 2.0
# Junction nodes at y = -35, 10, 55
j_mask = np.zeros_like(conduit_mask)
for jy in [-35.0, 10.0, 55.0]:
    dist_j = np.sqrt(d_canyon_flat**2 + (y_flat - jy)**2)
    j_mask = np.maximum(j_mask, np.clip(1.0 - (dist_j / 1.8), 0.0, 1.0))
j_mask = j_mask ** 2.0

conduit_colors = np.column_stack((conduit_mask, j_mask, np.zeros_like(conduit_mask), np.ones_like(conduit_mask)))
conduit_attr.data.foreach_set('color', conduit_colors.ravel())

terrain_obj = bpy.data.objects.new("Alien_Terrain", mesh)
bpy.context.collection.objects.link(terrain_obj)

# 2. Material: Photorealistic Basalt Rock + Bedrock Integrated Cyan Conduit
mat_rock = bpy.data.materials.new("Mat_AlienBasalt")
mat_rock.use_nodes = True
nodes = mat_rock.node_tree.nodes
links = mat_rock.node_tree.links
nodes.clear()

out = nodes.new('ShaderNodeOutputMaterial')
bsdf = nodes.new('ShaderNodeBsdfPrincipled')

tc = nodes.new('ShaderNodeTexCoord')
mapping = nodes.new('ShaderNodeMapping')
mapping.inputs['Scale'].default_value = (0.25, 0.25, 0.25)
links.new(tc.outputs['Object'], mapping.inputs['Vector'])

# Rock micro-bump
noise = nodes.new('ShaderNodeTexNoise')
noise.inputs['Scale'].default_value = 45.0
noise.inputs['Detail'].default_value = 10.0
noise.inputs['Roughness'].default_value = 0.72
links.new(mapping.outputs['Vector'], noise.inputs['Vector'])

bump = nodes.new('ShaderNodeBump')
bump.inputs['Strength'].default_value = 0.75
bump.inputs['Distance'].default_value = 0.25
links.new(noise.outputs['Fac'], bump.inputs['Height'])
links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])

# Base rock color from vertex attribute
vcol_rock = nodes.new('ShaderNodeVertexColor')
vcol_rock.layer_name = "Col_Rock"
links.new(vcol_rock.outputs['Color'], bsdf.inputs['Base Color'])

bsdf.inputs['Metallic'].default_value = 0.12
bsdf.inputs['Roughness'].default_value = 0.75

# Conduit Emission from vertex attribute Col_Conduit
vcol_conduit = nodes.new('ShaderNodeVertexColor')
vcol_conduit.layer_name = "Col_Conduit"

# Separate R (conduit channel) and G (junction node)
sep_col = nodes.new('ShaderNodeSeparateColor')
links.new(vcol_conduit.outputs['Color'], sep_col.inputs['Color'])

# Mix Cyan (#00e5ff) for channel and Solar Amber (#ffaa00) for junction nodes
mix_emit = nodes.new('ShaderNodeMix')
mix_emit.data_type = 'RGBA'
mix_emit.inputs[6].default_value = (0.0, 0.88, 1.0, 1.0) # Cyan
mix_emit.inputs[7].default_value = (1.0, 0.60, 0.05, 1.0) # Amber
links.new(sep_col.outputs['Green'], mix_emit.inputs['Factor'])
links.new(mix_emit.outputs[2], bsdf.inputs['Emission Color'])

# Emission Strength = (channel * 4.0) + (junction * 6.5)
math_c = nodes.new('ShaderNodeMath')
math_c.operation = 'MULTIPLY'
math_c.inputs[1].default_value = 4.0
links.new(sep_col.outputs['Red'], math_c.inputs[0])

math_j = nodes.new('ShaderNodeMath')
math_j.operation = 'MULTIPLY'
math_j.inputs[1].default_value = 6.5
links.new(sep_col.outputs['Green'], math_j.inputs[0])

math_sum = nodes.new('ShaderNodeMath')
math_sum.operation = 'ADD'
links.new(math_c.outputs['Value'], math_sum.inputs[0])
links.new(math_j.outputs['Value'], math_sum.inputs[1])
links.new(math_sum.outputs['Value'], bsdf.inputs['Emission Strength'])

links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
terrain_obj.data.materials.append(mat_rock)

# 3. Transmission Pylon & Skyward Beam
spire_x, spire_y = 16.0, 95.0
bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=4.2, radius2=0.7, depth=58.0, location=(spire_x, spire_y, 30.0))
spire = bpy.context.active_object
spire.data.materials.append(mat_rock)

# Apex Beacon
bpy.ops.mesh.primitive_cylinder_add(radius=1.1, depth=2.5, location=(spire_x, spire_y, 59.0))
beacon = bpy.context.active_object
b_mat = bpy.data.materials.new("Mat_Beacon")
b_mat.use_nodes = True
b_nodes = b_mat.node_tree.nodes
b_nodes.clear()
b_out = b_nodes.new('ShaderNodeOutputMaterial')
b_emit = b_nodes.new('ShaderNodeEmission')
b_emit.inputs['Color'].default_value = (1.0, 0.65, 0.1, 1.0)
b_emit.inputs['Strength'].default_value = 16.0
b_mat.node_tree.links.new(b_emit.outputs['Emission'], b_out.inputs['Surface'])
beacon.data.materials.append(b_mat)

# Skyward Beam
bpy.ops.mesh.primitive_cylinder_add(radius=0.8, depth=220.0, location=(spire_x, spire_y, 168.0))
beam = bpy.context.active_object
beam_mat = bpy.data.materials.new("Mat_Beam")
beam_mat.use_nodes = True
bm_nodes = beam_mat.node_tree.nodes
bm_nodes.clear()
bm_out = bm_nodes.new('ShaderNodeOutputMaterial')
bm_vol = bm_nodes.new('ShaderNodeVolumePrincipled')
bm_vol.inputs['Density'].default_value = 0.07
bm_vol.inputs['Emission Color'].default_value = (0.1, 0.85, 1.0, 1.0)
bm_vol.inputs['Emission Strength'].default_value = 6.0
beam_mat.node_tree.links.new(bm_vol.outputs['Volume'], bm_out.inputs['Volume'])
beam.data.materials.append(beam_mat)

# 4. Volumetric Atmosphere Box
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 32.0))
fog_box = bpy.context.active_object
fog_box.scale = (340.0, 340.0, 100.0)
fog_mat = bpy.data.materials.new("Mat_Haze")
fog_mat.use_nodes = True
f_nodes = fog_mat.node_tree.nodes
f_nodes.clear()
f_out = f_nodes.new('ShaderNodeOutputMaterial')
f_vol = f_nodes.new('ShaderNodeVolumePrincipled')
f_vol.inputs['Density'].default_value = 0.0024
f_vol.inputs['Color'].default_value = (0.05, 0.12, 0.22, 1.0)
f_vol.inputs['Anisotropy'].default_value = 0.45
fog_mat.node_tree.links.new(f_vol.outputs['Volume'], f_out.inputs['Volume'])
fog_box.data.materials.append(fog_mat)

# 5. Cinematic Lighting
sun_data = bpy.data.lights.new(name="Sun_Key", type='SUN')
sun_data.energy = 5.8
sun_data.color = (0.90, 0.95, 1.0)
sun_data.angle = math.radians(7.0)
sun = bpy.data.objects.new("Sun_Key", sun_data)
bpy.context.collection.objects.link(sun)
sun.rotation_euler = (math.radians(54.0), math.radians(16.0), math.radians(118.0))

fill_data = bpy.data.lights.new(name="Sun_Fill", type='SUN')
fill_data.energy = 1.8
fill_data.color = (0.05, 0.28, 0.50)
fill = bpy.data.objects.new("Sun_Fill", fill_data)
bpy.context.collection.objects.link(fill)
fill.rotation_euler = (math.radians(38.0), math.radians(-12.0), math.radians(-62.0))

# 6. Looping Camera Flight Path
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 28.0
cam_data.sensor_width = 36.0
cam_data.clip_start = 1.0
cam_data.clip_end = 650.0

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
    # Aerial circuit above the southern and eastern plateau rims looking across into the canyon
    cx = 18.0 + 22.0 * math.sin(t) + 6.0 * math.sin(2.0 * t)
    cy = -78.0 + 20.0 * (1.0 - math.cos(t)) + 4.0 * math.sin(t)
    cz = 32.0 + 6.0 * math.cos(t) + 2.5 * math.sin(2.0 * t)

    cam.location = (cx, cy, cz)
    cam.keyframe_insert(data_path="location", frame=f)

    # Target looks toward canyon chasm and horizon pass
    tx = 16.0 + 8.0 * math.sin(t)
    ty = 25.0 + 15.0 * math.cos(t)
    tz = 6.0 + 3.0 * math.cos(t)
    target.location = (tx, ty, tz)
    target.keyframe_insert(data_path="location", frame=f)

# Save master project
blend_path = "/home/venkatengineer/solaris-os/assets/computational-terrain/solaris_computational_terrain.blend"
bpy.ops.wm.save_as_mainfile(filepath=blend_path)
print("Saved master project to:", blend_path)

# Render frame 1 preview
scene.render.filepath = "/tmp/photoreal_test_0001.png"
bpy.ops.render.render(write_still=True)
print("Rendered frame 1 to /tmp/photoreal_test_0001.png")
