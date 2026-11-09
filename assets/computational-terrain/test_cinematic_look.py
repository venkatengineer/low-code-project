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
bg.inputs['Color'].default_value = (0.001, 0.002, 0.004, 1.0)
bg.inputs['Strength'].default_value = 0.25
out_w = world.node_tree.nodes.new('ShaderNodeOutputWorld')
world.node_tree.links.new(bg.outputs['Background'], out_w.inputs['Surface'])

try:
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - High Contrast'
except Exception:
    pass
scene.view_settings.exposure = 0.10

# 1. High-Density Terrain Synthesis (360x360)
res = 360
size_x = 240.0
size_y = 260.0
x = np.linspace(-size_x*0.5, size_x*0.5, res, dtype=np.float32)
y = np.linspace(-size_y*0.5, size_y*0.5, res, dtype=np.float32)
X, Y = np.meshgrid(x, y)

# A grand canyon gorge cutting through an expansive plateau
canyon_x = 18.0 * np.sin(Y * 0.02) + 6.0 * np.cos(Y * 0.045)
d_canyon = np.abs(X - canyon_x)

# Canyon profile: 28m wide gorge, dropping 20m down to a flat floor
canyon_w = 26.0 + 5.0 * np.sin(Y * 0.03)
u = np.clip(d_canyon / canyon_w, 0.0, 1.0)
canyon_depth = -18.0 * (1.0 - (u ** 3.0))

# Recessed conduit channel along canyon bed
bed_trench = np.clip(1.0 - (d_canyon / 1.8), 0.0, 1.0)
canyon_depth -= 0.6 * (bed_trench ** 2.0)

# Layered basalt plateau on the flanks with distinct horizontal terraces
plateau = 8.0 + 5.0 * np.sin(X * 0.03 + 0.5) * np.cos(Y * 0.025 - 0.4)
terrace = np.floor(plateau / 4.0) * 4.0 + 2.0 * np.sin(np.pi * ((plateau / 4.0) % 1.0)) * 0.4

# Mountain backdrop: Jagged alpine ridges lining the northern horizon (Y > 30)
north_dist = np.clip((Y - 20.0) / 90.0, 0.0, 1.0)
flank_dist = np.clip((np.abs(X) - 50.0) / 60.0, 0.0, 1.0)
rim = np.maximum(north_dist, flank_dist)

# Multi-octave ridged noise for mountains
r1 = 1.0 - np.abs(np.sin(X * 0.045 + 0.3) * np.cos(Y * 0.045 + 1.2))
r2 = 1.0 - np.abs(np.sin(X * 0.09 - 0.4) * np.cos(Y * 0.09 + 0.7))
r3 = 1.0 - np.abs(np.sin(X * 0.18 + 0.8) * np.cos(Y * 0.18 - 0.5))
mountains = (r1 * 38.0 + r2 * 18.0 + r3 * 8.0) * (rim ** 1.5)

# Erosion flutes on cliff faces
flutes = 1.2 * np.cos(d_canyon * 0.6 + 2.5 * np.sin(Y * 0.12)) * (1.0 - u) * (u ** 0.5)

Z = (terrace * u) + canyon_depth + mountains + flutes

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

# Color attribute
col_attr = mesh.color_attributes.new(name="Col_Rock", type='FLOAT_COLOR', domain='POINT')
z_flat = Z.ravel()
norm_z = np.clip((z_flat + 18.0) / 75.0, 0.0, 1.0)
# Dark basalt tones: Charcoal (#090c10) to deep slate (#121922)
r_c = 0.035 + 0.04 * norm_z
g_c = 0.045 + 0.05 * norm_z
b_c = 0.060 + 0.08 * norm_z
colors = np.column_stack((r_c, g_c, b_c, np.ones_like(r_c)))
col_attr.data.foreach_set('color', colors.ravel())

terrain_obj = bpy.data.objects.new("Alien_Terrain", mesh)
bpy.context.collection.objects.link(terrain_obj)

# 2. Rock Material (No spiderweb wireframes!)
mat_rock = bpy.data.materials.new("Mat_AlienBasalt")
mat_rock.use_nodes = True
nodes = mat_rock.node_tree.nodes
links = mat_rock.node_tree.links
nodes.clear()

out = nodes.new('ShaderNodeOutputMaterial')
bsdf = nodes.new('ShaderNodeBsdfPrincipled')

tc = nodes.new('ShaderNodeTexCoord')
mapping = nodes.new('ShaderNodeMapping')
mapping.inputs['Scale'].default_value = (0.15, 0.15, 0.15)
links.new(tc.outputs['Object'], mapping.inputs['Vector'])

# Rock micro-bump
noise = nodes.new('ShaderNodeTexNoise')
noise.inputs['Scale'].default_value = 35.0
noise.inputs['Detail'].default_value = 10.0
noise.inputs['Roughness'].default_value = 0.70
links.new(mapping.outputs['Vector'], noise.inputs['Vector'])

bump = nodes.new('ShaderNodeBump')
bump.inputs['Strength'].default_value = 0.65
bump.inputs['Distance'].default_value = 0.40
links.new(noise.outputs['Fac'], bump.inputs['Height'])
links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])

vcol = nodes.new('ShaderNodeVertexColor')
vcol.layer_name = "Col_Rock"
links.new(vcol.outputs['Color'], bsdf.inputs['Base Color'])

bsdf.inputs['Metallic'].default_value = 0.12
bsdf.inputs['Roughness'].default_value = 0.78
links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
terrain_obj.data.materials.append(mat_rock)

# 3. Discrete Physical Optical Conduit Ribbon in Canyon Bed
num_pts = 260
y_ribbon = np.linspace(-95.0, 95.0, num_pts)
x_ribbon = 18.0 * np.sin(y_ribbon * 0.02) + 6.0 * np.cos(y_ribbon * 0.045)
# Sample Z height on trench floor
z_ribbon = []
for rx, ry in zip(x_ribbon, y_ribbon):
    ix = int(np.clip((rx + size_x * 0.5) / size_x * (res - 1), 0, res - 1))
    iy = int(np.clip((ry + size_y * 0.5) / size_y * (res - 1), 0, res - 1))
    z_ribbon.append(Z[iy, ix] + 0.15)

ribbon_verts = []
ribbon_faces = []
half_w = 0.40 # 0.8m wide discrete conduit
for i in range(num_pts):
    cx = x_ribbon[i]
    cy = y_ribbon[i]
    cz = z_ribbon[i]
    if i < num_pts - 1:
        tx = x_ribbon[i+1] - cx
        ty = y_ribbon[i+1] - cy
    else:
        tx = cx - x_ribbon[i-1]
        ty = cy - y_ribbon[i-1]
    mag = math.sqrt(tx*tx + ty*ty)
    nx = -ty / mag
    ny = tx / mag
    ribbon_verts.append((cx - nx * half_w, cy - ny * half_w, cz))
    ribbon_verts.append((cx + nx * half_w, cy + ny * half_w, cz))
    if i > 0:
        idx = i * 2
        ribbon_faces.append((idx - 2, idx - 1, idx + 1))
        ribbon_faces.append((idx - 2, idx + 1, idx))

ribbon_mesh = bpy.data.meshes.new("Mesh_ConduitRibbon")
ribbon_mesh.from_pydata(ribbon_verts, [], ribbon_faces)
ribbon_mesh.update()
ribbon_mesh.shade_smooth()

ribbon_obj = bpy.data.objects.new("Conduit_Ribbon", ribbon_mesh)
bpy.context.collection.objects.link(ribbon_obj)

mat_conduit = bpy.data.materials.new("Mat_Conduit")
mat_conduit.use_nodes = True
c_nodes = mat_conduit.node_tree.nodes
c_nodes.clear()
c_out = c_nodes.new('ShaderNodeOutputMaterial')
c_bsdf = c_nodes.new('ShaderNodeBsdfPrincipled')
c_bsdf.inputs['Base Color'].default_value = (0.01, 0.03, 0.05, 1.0)
c_bsdf.inputs['Metallic'].default_value = 0.80
c_bsdf.inputs['Roughness'].default_value = 0.20
c_bsdf.inputs['Emission Color'].default_value = (0.0, 0.85, 1.0, 1.0)
c_bsdf.inputs['Emission Strength'].default_value = 3.5
mat_conduit.node_tree.links.new(c_bsdf.outputs['BSDF'], c_out.inputs['Surface'])
ribbon_obj.data.materials.append(mat_conduit)

# 4. Transmission Pylon & Skyward Beam
spire_x, spire_y = 18.0, 95.0
bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=3.8, radius2=0.6, depth=55.0, location=(spire_x, spire_y, 28.0))
spire = bpy.context.active_object
spire.data.materials.append(mat_rock)

# Beacon
bpy.ops.mesh.primitive_cylinder_add(radius=0.9, depth=2.0, location=(spire_x, spire_y, 56.0))
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
bpy.ops.mesh.primitive_cylinder_add(radius=0.7, depth=180.0, location=(spire_x, spire_y, 146.0))
beam = bpy.context.active_object
beam_mat = bpy.data.materials.new("Mat_Beam")
beam_mat.use_nodes = True
bm_nodes = beam_mat.node_tree.nodes
bm_nodes.clear()
bm_out = bm_nodes.new('ShaderNodeOutputMaterial')
bm_vol = bm_nodes.new('ShaderNodeVolumePrincipled')
bm_vol.inputs['Density'].default_value = 0.08
bm_vol.inputs['Emission Color'].default_value = (0.1, 0.8, 1.0, 1.0)
bm_vol.inputs['Emission Strength'].default_value = 5.0
beam_mat.node_tree.links.new(bm_vol.outputs['Volume'], bm_out.inputs['Volume'])
beam.data.materials.append(beam_mat)

# 5. Volumetric Atmosphere Box
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 30.0))
fog_box = bpy.context.active_object
fog_box.scale = (320.0, 320.0, 95.0)
fog_mat = bpy.data.materials.new("Mat_Haze")
fog_mat.use_nodes = True
f_nodes = fog_mat.node_tree.nodes
f_nodes.clear()
f_out = f_nodes.new('ShaderNodeOutputMaterial')
f_vol = f_nodes.new('ShaderNodeVolumePrincipled')
f_vol.inputs['Density'].default_value = 0.0022
f_vol.inputs['Color'].default_value = (0.04, 0.11, 0.20, 1.0)
f_vol.inputs['Anisotropy'].default_value = 0.40
fog_mat.node_tree.links.new(f_vol.outputs['Volume'], f_out.inputs['Volume'])
fog_box.data.materials.append(fog_mat)

# 6. Lighting
sun_data = bpy.data.lights.new(name="Sun_Key", type='SUN')
sun_data.energy = 5.6
sun_data.color = (0.88, 0.94, 1.0)
sun_data.angle = math.radians(6.5)
sun = bpy.data.objects.new("Sun_Key", sun_data)
bpy.context.collection.objects.link(sun)
sun.rotation_euler = (math.radians(56.0), math.radians(14.0), math.radians(115.0))

fill_data = bpy.data.lights.new(name="Sun_Fill", type='SUN')
fill_data.energy = 1.4
fill_data.color = (0.04, 0.22, 0.42)
fill = bpy.data.objects.new("Sun_Fill", fill_data)
bpy.context.collection.objects.link(fill)
fill.rotation_euler = (math.radians(35.0), math.radians(-10.0), math.radians(-65.0))

# 7. Cinematic Camera: Majestic Horizon Aerial View
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 28.0
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

# Frame 1 camera position: Elevated above the southern canyon scarp looking north toward the horizon!
cam.location = (12.0, -82.0, 32.0)
target.location = (16.0, 35.0, 6.0)

# Render frame 1 preview
scene.render.filepath = "/tmp/cinematic_test_0001.png"
bpy.ops.render.render(write_still=True)
print("Rendered cinematic preview to /tmp/cinematic_test_0001.png")
