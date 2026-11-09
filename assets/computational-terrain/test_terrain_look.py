#!/usr/bin/env python3
"""
Refined Art Direction Test for Solaris Computational Terrain
- Deep mineral carbon / titanium dark rock
- Sharp horizontal strata & crisp canyon ledges
- Restrained, elegant hairline glowing circuit channels
- Deep midnight space background with crisp rim lighting
- Monumental architectural compute monoliths & crystal towers
"""

import bpy
import bmesh
import math
import os
from mathutils import Vector, Euler

def build_refined_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    
    # Cycles OptiX setup
    try:
        cpref = bpy.context.preferences.addons['cycles'].preferences
        cpref.compute_device_type = 'OPTIX'
        cpref.get_devices_for_type('OPTIX')
        for d in cpref.devices:
            if d.type == 'OPTIX':
                d.use = True
        scene.cycles.device = 'GPU'
    except Exception as e:
        print("GPU note:", e)

    scene.render.fps = 30
    scene.frame_start = 1
    scene.frame_end = 450
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100

    scene.cycles.samples = 16
    scene.cycles.preview_samples = 8
    scene.cycles.use_denoising = True
    scene.cycles.denoiser = 'OPTIX'
    scene.cycles.max_bounces = 4
    scene.cycles.diffuse_bounces = 2
    scene.cycles.glossy_bounces = 3
    scene.cycles.transmission_bounces = 2
    scene.cycles.volume_bounces = 0

    scene.view_settings.view_transform = 'AgX' if 'AgX' in [v.name for v in scene.view_settings.bl_rna.properties['view_transform'].enum_items] else 'Filmic'
    scene.view_settings.look = 'High Contrast'
    scene.view_settings.exposure = 0.0

    # =========================================================================
    # WORLD / ENVIRONMENT: Deep Midnight Space (No blowout!)
    # =========================================================================
    world = bpy.data.worlds.new("MidnightSpace")
    scene.world = world
    world.use_nodes = True
    wn = world.node_tree.nodes
    wl = world.node_tree.links
    wn.clear()
    w_out = wn.new('ShaderNodeOutputWorld')
    bg = wn.new('ShaderNodeBackground')
    bg.inputs['Color'].default_value = (0.003, 0.006, 0.012, 1.0)
    bg.inputs['Strength'].default_value = 0.8
    wl.new(bg.outputs['Background'], w_out.inputs['Surface'])

    # =========================================================================
    # MATERIALS: Restrained, Premium, Dark Mineral & Controlled Cyan Accents
    # =========================================================================
    # 1. Dark Basalt Mineral / Carbon Substrate
    mat_terrain = bpy.data.materials.new(name="Mat_Terrain_Basalt")
    mat_terrain.use_nodes = True
    tn = mat_terrain.node_tree.nodes
    tl = mat_terrain.node_tree.links
    tn.clear()

    t_out = tn.new('ShaderNodeOutputMaterial')
    bsdf_t = tn.new('ShaderNodeBsdfPrincipled')
    bsdf_t.inputs['Base Color'].default_value = (0.012, 0.022, 0.038, 1.0)
    bsdf_t.inputs['Metallic'].default_value = 0.88
    bsdf_t.inputs['Roughness'].default_value = 0.48

    # Texture Coordinates & Multi-scale bump
    coord = tn.new('ShaderNodeTexCoord')
    mapping = tn.new('ShaderNodeMapping')
    mapping.inputs['Scale'].default_value = (0.2, 0.2, 0.2)

    # Geological strata banding (Wave texture for horizontal rock layers)
    wave_strata = tn.new('ShaderNodeTexWave')
    wave_strata.wave_type = 'BANDS'
    wave_strata.bands_direction = 'Z'
    wave_strata.inputs['Scale'].default_value = 0.85
    wave_strata.inputs['Distortion'].default_value = 1.2
    wave_strata.inputs['Detail'].default_value = 4.0

    # Micro tech grain
    noise_micro = tn.new('ShaderNodeTexNoise')
    noise_micro.inputs['Scale'].default_value = 24.0
    noise_micro.inputs['Detail'].default_value = 8.0

    # Combine bump
    math_add = tn.new('ShaderNodeMath')
    math_add.operation = 'ADD'

    bump_strata = tn.new('ShaderNodeBump')
    bump_strata.inputs['Strength'].default_value = 0.65

    # Sparse, elegant hairline circuit emission (masked to low valleys only)
    voronoi_lines = tn.new('ShaderNodeTexVoronoi')
    voronoi_lines.feature = 'DISTANCE_TO_EDGE'
    voronoi_lines.inputs['Scale'].default_value = 2.5

    ramp_hairline = tn.new('ShaderNodeValToRGB')
    ramp_hairline.color_ramp.elements[0].position = 0.015
    ramp_hairline.color_ramp.elements[0].color = (0.0, 0.95, 1.0, 1.0)
    ramp_hairline.color_ramp.elements[1].position = 0.035
    ramp_hairline.color_ramp.elements[1].color = (0.0, 0.0, 0.0, 1.0)

    # Gradient mask to keep emission subtle and grounded in canyon valleys
    math_emiss_mult = tn.new('ShaderNodeMath')
    math_emiss_mult.operation = 'MULTIPLY'
    math_emiss_mult.inputs[1].default_value = 4.0

    tl.new(coord.outputs['Object'], mapping.inputs['Vector'])
    tl.new(mapping.outputs['Vector'], wave_strata.inputs['Vector'])
    tl.new(mapping.outputs['Vector'], noise_micro.inputs['Vector'])
    tl.new(mapping.outputs['Vector'], voronoi_lines.inputs['Vector'])

    tl.new(wave_strata.outputs['Color'], math_add.inputs[0])
    tl.new(noise_micro.outputs['Fac'], math_add.inputs[1])
    tl.new(math_add.outputs['Value'], bump_strata.inputs['Height'])
    tl.new(bump_strata.outputs['Normal'], bsdf_t.inputs['Normal'])

    tl.new(voronoi_lines.outputs['Distance'], ramp_hairline.inputs['Fac'])
    tl.new(ramp_hairline.outputs['Color'], bsdf_t.inputs['Emission Color'])
    tl.new(ramp_hairline.outputs['Color'], math_emiss_mult.inputs[0])
    tl.new(math_emiss_mult.outputs['Value'], bsdf_t.inputs['Emission Strength'])

    tl.new(bsdf_t.outputs['BSDF'], t_out.inputs['Surface'])

    # 2. Brushed Dark Alloy Monoliths
    mat_alloy = bpy.data.materials.new(name="Mat_Monolith_Alloy")
    mat_alloy.use_nodes = True
    an = mat_alloy.node_tree.nodes
    al = mat_alloy.node_tree.links
    an.clear()
    a_out = an.new('ShaderNodeOutputMaterial')
    bsdf_a = an.new('ShaderNodeBsdfPrincipled')
    bsdf_a.inputs['Base Color'].default_value = (0.03, 0.06, 0.11, 1.0)
    bsdf_a.inputs['Metallic'].default_value = 0.95
    bsdf_a.inputs['Roughness'].default_value = 0.22
    al.new(bsdf_a.outputs['BSDF'], a_out.inputs['Surface'])

    # 3. Core Chip Die (Controlled Neon Cyan / Amber)
    mat_die = bpy.data.materials.new(name="Mat_Processor_Core")
    mat_die.use_nodes = True
    dn = mat_die.node_tree.nodes
    dl = mat_die.node_tree.links
    dn.clear()
    d_out = dn.new('ShaderNodeOutputMaterial')
    bsdf_d = dn.new('ShaderNodeBsdfPrincipled')
    bsdf_d.inputs['Base Color'].default_value = (0.01, 0.02, 0.03, 1.0)
    bsdf_d.inputs['Metallic'].default_value = 0.90
    bsdf_d.inputs['Roughness'].default_value = 0.15
    bsdf_d.inputs['Emission Color'].default_value = (0.0, 0.92, 1.0, 1.0)
    bsdf_d.inputs['Emission Strength'].default_value = 8.0
    dl.new(bsdf_d.outputs['BSDF'], d_out.inputs['Surface'])

    # 4. Translucent Memory Prism
    mat_crystal = bpy.data.materials.new(name="Mat_Memory_Prism")
    mat_crystal.use_nodes = True
    cn = mat_crystal.node_tree.nodes
    cl = mat_crystal.node_tree.links
    cn.clear()
    c_out = cn.new('ShaderNodeOutputMaterial')
    bsdf_c = cn.new('ShaderNodeBsdfPrincipled')
    bsdf_c.inputs['Base Color'].default_value = (0.0, 0.70, 0.85, 1.0)
    bsdf_c.inputs['Metallic'].default_value = 0.20
    bsdf_c.inputs['Roughness'].default_value = 0.10
    bsdf_c.inputs['IOR'].default_value = 1.52
    bsdf_c.inputs['Transmission Weight'].default_value = 0.85
    bsdf_c.inputs['Emission Color'].default_value = (0.0, 1.0, 0.85, 1.0)
    bsdf_c.inputs['Emission Strength'].default_value = 3.5
    cl.new(bsdf_c.outputs['BSDF'], c_out.inputs['Surface'])

    # 5. Glowing Conduit Core Line
    mat_conduit = bpy.data.materials.new(name="Mat_Conduit_Luminous")
    mat_conduit.use_nodes = True
    cnn = mat_conduit.node_tree.nodes
    cnl = mat_conduit.node_tree.links
    cnn.clear()
    co_out = cnn.new('ShaderNodeOutputMaterial')
    bsdf_co = cnn.new('ShaderNodeBsdfPrincipled')
    bsdf_co.inputs['Base Color'].default_value = (0.0, 0.8, 1.0, 1.0)
    bsdf_co.inputs['Emission Color'].default_value = (0.0, 0.95, 1.0, 1.0)
    bsdf_co.inputs['Emission Strength'].default_value = 6.0
    cnl.new(bsdf_co.outputs['BSDF'], co_out.inputs['Surface'])

    # 6. Laser Scanner (Fine Translucent Cyan Sheet)
    mat_laser = bpy.data.materials.new(name="Mat_Laser_Curtain")
    mat_laser.use_nodes = True
    ln = mat_laser.node_tree.nodes
    ll = mat_laser.node_tree.links
    ln.clear()
    l_out = ln.new('ShaderNodeOutputMaterial')
    emit_l = ln.new('ShaderNodeEmission')
    emit_l.inputs['Color'].default_value = (0.0, 0.95, 1.0, 1.0)
    emit_l.inputs['Strength'].default_value = 4.0
    trans_l = ln.new('ShaderNodeBsdfTransparent')
    mix_l = ln.new('ShaderNodeMixShader')
    mix_l.inputs['Fac'].default_value = 0.15
    ll.new(trans_l.outputs['BSDF'], mix_l.inputs[1])
    ll.new(emit_l.outputs['Emission'], mix_l.inputs[2])
    ll.new(mix_l.outputs['Shader'], l_out.inputs['Surface'])

    # 7. Horizon Beacon Core
    mat_beacon = bpy.data.materials.new(name="Mat_Beacon_Core")
    mat_beacon.use_nodes = True
    bn = mat_beacon.node_tree.nodes
    bl = mat_beacon.node_tree.links
    bn.clear()
    b_out = bn.new('ShaderNodeOutputMaterial')
    emit_b = bn.new('ShaderNodeEmission')
    emit_b.inputs['Color'].default_value = (0.6, 0.95, 1.0, 1.0)
    emit_b.inputs['Strength'].default_value = 25.0
    bl.new(emit_b.outputs['Emission'], b_out.inputs['Surface'])

    mats = {
        'terrain': mat_terrain,
        'alloy': mat_alloy,
        'die': mat_die,
        'crystal': mat_crystal,
        'conduit': mat_conduit,
        'laser': mat_laser,
        'beacon': mat_beacon
    }

    # =========================================================================
    # GEOMETRY: Expansive Grand Canyon with Stepped Stratum Ledges
    # =========================================================================
    mesh = bpy.data.meshes.new("GrandCanyonMesh")
    obj = bpy.data.objects.new("GrandCanyonTerrain", mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    nx = 160
    ny = 180
    lx = 120.0
    ly = 150.0

    verts = []
    for j in range(ny):
        y = -60.0 + (j / (ny - 1)) * ly
        row = []
        for i in range(nx):
            x = -60.0 + (i / (nx - 1)) * lx

            # Broad canyon floor: width ~24m, gently winding
            canyon_center_x = 4.0 * math.sin(y * 0.05)
            dx = abs(x - canyon_center_x)

            # Flat canyon bottom for |dx| < 12m
            if dx < 12.0:
                elev = -2.5 + (0.4 * math.sin(x * 0.2 + y * 0.1))
            else:
                # Stepped terraces ascending up the canyon walls
                wall_dist = dx - 12.0
                raw_h = math.pow(wall_dist / 40.0, 0.85) * 22.0
                # Quantize into 4 large geological strata steps
                step_h = 4.5
                step_level = math.floor(raw_h / step_h)
                fraction = (raw_h % step_h) / step_h
                # Steep risers with flat treads
                shelf_h = step_level * step_h + (fraction ** 4) * step_h
                elev = -2.5 + shelf_h

            # Distant horizon tech mountains (Y > 45)
            if y > 45.0:
                h_t = (y - 45.0) / 45.0
                mountain_ridge = h_t * (16.0 + 8.0 * math.sin(x * 0.12))
                elev += mountain_ridge

            v = bm.verts.new((x, y, elev))
            row.append(v)
        verts.append(row)

    bm.verts.ensure_lookup_table()
    for j in range(ny - 1):
        for i in range(nx - 1):
            bm.faces.new((verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i]))

    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(mats['terrain'])
    for poly in mesh.polygons:
        poly.use_smooth = True

    # =========================================================================
    # COMPUTE NEXUS: Stepped Mainframe Monoliths Embedded in Canyon Floor
    # =========================================================================
    cols = 5
    rows = 4
    col_w = 4.8
    row_d = 4.2
    for r in range(rows):
        for c in range(cols):
            cx = (c - (cols - 1) * 0.5) * col_w
            cy = -12.0 + (r * row_d)
            dist_c = math.sqrt(cx * cx + (cy - 0.0) * (cy - 0.0))
            h = max(2.8, 7.5 - dist_c * 0.38)
            w = 3.6
            d = 3.2

            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(cx, cy, h * 0.5 - 2.5))
            cube = bpy.context.active_object
            cube.name = f"ComputeMonolith_{c}_{r}"
            cube.scale = (w, d, h)
            bpy.ops.object.transform_apply(scale=True)
            cube.data.materials.append(mats['alloy'])

            bev = cube.modifiers.new("Bevel", type='BEVEL')
            bev.width = 0.14
            bev.segments = 2

            # Recessed processor die chip
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(cx, cy, h - 2.44))
            die = bpy.context.active_object
            die.name = f"ProcessorDie_{c}_{r}"
            die.scale = (w * 0.68, d * 0.68, 0.12)
            bpy.ops.object.transform_apply(scale=True)
            die.data.materials.append(mats['die'])

    # =========================================================================
    # MEMORY MEGALITHS (Port Flank Crystal Prism Towers)
    # =========================================================================
    for i in range(8):
        px = -22.0 + ((i % 2) * -6.0)
        py = -15.0 + (i * 5.2)
        ph = 9.0 + (i % 3) * 3.0
        pw = 2.0
        pd = 1.8
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(px, py, ph * 0.5 + 1.5))
        p = bpy.context.active_object
        p.name = f"MemoryPrism_{i}"
        p.scale = (pw, pd, ph)
        bpy.ops.object.transform_apply(scale=True)
        p.data.materials.append(mats['crystal'])

    # =========================================================================
    # HORIZON TRANSMISSION SPIRE & SKYWARD BEAM (At horizon, X=0, Y=70)
    # =========================================================================
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=4.0, radius2=0.5, depth=48.0, location=(0, 70.0, 24.0))
    spire = bpy.context.active_object
    spire.name = "TransmissionPylon"
    spire.data.materials.append(mats['alloy'])

    # Radiant Beacon Sphere
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=1.6, location=(0, 70.0, 48.5))
    beacon = bpy.context.active_object
    beacon.name = "BeaconSphere"
    beacon.data.materials.append(mats['beacon'])

    # Skyward Vertical Light Beam
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.8, depth=80.0, location=(0, 70.0, 88.0))
    beam = bpy.context.active_object
    beam.name = "SkywardBeam"
    beam.data.materials.append(mats['laser'])

    # =========================================================================
    # BRANCHING CONDUIT NETWORKS
    # =========================================================================
    c_data = bpy.data.curves.new('PrimaryBusCurve', type='CURVE')
    c_data.dimensions = '3D'
    c_data.bevel_depth = 0.26
    c_data.bevel_resolution = 4
    spline = c_data.splines.new('BEZIER')
    pts = [
        Vector(( 0.0, -45.0, -2.1)),
        Vector(( 0.0, -20.0, -2.4)),
        Vector((-3.0,  -2.0, -2.2)),
        Vector(( 3.0,  18.0, -1.8)),
        Vector(( 0.0,  42.0,  2.5)),
        Vector(( 0.0,  68.0, 20.0))
    ]
    spline.bezier_points.add(len(pts) - 1)
    for i, pt in enumerate(pts):
        bp = spline.bezier_points[i]
        bp.co = pt
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
    c_obj = bpy.data.objects.new('CentralBusConduit', c_data)
    c_obj.data.materials.append(mats['conduit'])
    bpy.context.collection.objects.link(c_obj)

    # =========================================================================
    # CINEMATIC CAMERA FLIGHT (Seamless Looping Path)
    # =========================================================================
    cam_data = bpy.data.cameras.new("CinematicCamera")
    cam_data.lens = 26.0
    cam_data.clip_start = 0.5
    cam_data.clip_end = 400.0
    cam_obj = bpy.data.objects.new("CinematicCamera", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    target = bpy.data.objects.new("CamTarget", None)
    bpy.context.collection.objects.link(target)
    track = cam_obj.constraints.new(type='TRACK_TO')
    track.target = target
    track.track_axis = 'TRACK_NEGATIVE_Z'
    track.up_axis = 'UP_Y'

    total_frames = 450
    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        forward_t = (1.0 - math.cos(theta)) * 0.5
        y_cam = -38.0 + forward_t * 24.0
        x_cam = 4.0 * math.sin(theta)
        z_cam = 7.5 + 2.0 * math.sin(2.0 * theta) - 0.8 * math.cos(theta)
        cam_obj.location = (x_cam, y_cam, z_cam)
        cam_obj.keyframe_insert("location", frame=f)

        t_y = y_cam + 35.0 + 6.0 * math.sin(theta)
        t_x = 2.0 * math.sin(theta)
        t_z = 3.5 + 1.5 * math.sin(theta)
        target.location = (t_x, t_y, t_z)
        target.keyframe_insert("location", frame=f)

    # =========================================================================
    # LIGHTING: Crisp Directional Sunlight & Chiaroscuro
    # =========================================================================
    # Sun Light: Cool ice cyan raking across the canyon
    sun_data = bpy.data.lights.new(name="SunKeyLight", type='SUN')
    sun_data.color = (0.70, 0.90, 1.0)
    sun_data.energy = 3.8
    sun_data.angle = math.radians(2.0)
    sun_obj = bpy.data.objects.new("SunKeyLight", sun_data)
    sun_obj.rotation_euler = Euler((math.radians(56.0), math.radians(22.0), math.radians(-38.0)), 'XYZ')
    bpy.context.collection.objects.link(sun_obj)

    # Fill Light: Soft dark navy
    fill_data = bpy.data.lights.new(name="FillLight", type='SUN')
    fill_data.color = (0.05, 0.12, 0.22)
    fill_data.energy = 0.8
    fill_obj = bpy.data.objects.new("FillLight", fill_data)
    fill_obj.rotation_euler = Euler((math.radians(45.0), math.radians(-25.0), math.radians(142.0)), 'XYZ')
    bpy.context.collection.objects.link(fill_obj)

    # Save to computational_terrain.blend
    out_blend = "/home/venkatengineer/solaris-os/assets/computational-terrain/computational_terrain.blend"
    bpy.ops.wm.save_as_mainfile(filepath=out_blend)
    print(f"Saved refined master scene: {out_blend}")

if __name__ == '__main__':
    build_refined_scene()
