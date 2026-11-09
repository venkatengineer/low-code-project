#!/usr/bin/env python3
"""
🪐 SOLARIS — CINEMATIC COMPUTATIONAL TERRAIN GENERATOR (Blender 5.2)
Master Technical Art Pipeline:
1. Deep mineral basalt/obsidian rock strata with geological shelves
2. High-tech mainframe monolith citadels (Compute Nexus & Memory Megaliths)
3. Restrained, premium lighting: chiaroscuro rim lights & dark midnight space
4. Luminous data conduits with flowing energy pulses along canyon channels
5. Sweeping holographic laser scanner plane
6. 100% mathematically seamless 450-frame loop (15s @ 30 FPS)
7. Optimized Cycles OptiX rendering for fast, stunning output
"""

import bpy
import bmesh
import math
import os
from mathutils import Vector, Euler

def set_action_interpolation_bezier(action):
    if not action:
        return
    if hasattr(action, 'fcurves'):
        for fc in action.fcurves:
            for kf in fc.keyframe_points:
                kf.interpolation = 'BEZIER'
    elif hasattr(action, 'layers'):
        for layer in action.layers:
            for strip in layer.strips:
                for cb in getattr(strip, 'channelbags', []):
                    for fc in cb.fcurves:
                        for kf in fc.keyframe_points:
                            kf.interpolation = 'BEZIER'

def build_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    
    # Cycles OptiX GPU Setup
    try:
        cpref = bpy.context.preferences.addons['cycles'].preferences
        cpref.compute_device_type = 'OPTIX'
        cpref.get_devices_for_type('OPTIX')
        for d in cpref.devices:
            if d.type == 'OPTIX':
                d.use = True
        scene.cycles.device = 'GPU'
    except Exception as e:
        print("GPU OptiX setup:", e)

    scene.render.fps = 30
    scene.frame_start = 1
    scene.frame_end = 450
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100

    scene.cycles.samples = 24
    scene.cycles.preview_samples = 12
    scene.cycles.use_denoising = True
    scene.cycles.denoiser = 'OPTIX'
    scene.cycles.max_bounces = 4
    scene.cycles.diffuse_bounces = 2
    scene.cycles.glossy_bounces = 3
    scene.cycles.transmission_bounces = 3
    scene.cycles.volume_bounces = 0

    scene.view_settings.view_transform = 'AgX' if 'AgX' in [v.name for v in scene.view_settings.bl_rna.properties['view_transform'].enum_items] else 'Filmic'
    scene.view_settings.look = 'High Contrast'
    scene.view_settings.exposure = 0.15

    # =========================================================================
    # WORLD: Deep Space Atmosphere
    # =========================================================================
    world = bpy.data.worlds.new("DeepSpaceWorld")
    scene.world = world
    world.use_nodes = True
    wn = world.node_tree.nodes
    wl = world.node_tree.links
    wn.clear()
    w_out = wn.new('ShaderNodeOutputWorld')
    bg = wn.new('ShaderNodeBackground')
    bg.inputs['Color'].default_value = (0.002, 0.005, 0.010, 1.0)
    bg.inputs['Strength'].default_value = 0.75
    wl.new(bg.outputs['Background'], w_out.inputs['Surface'])

    # =========================================================================
    # SHADER SYSTEM: Restrained, Premium Mineral & Controlled Cyan/Teal Accents
    # =========================================================================
    # 1. Dark Basalt / Obsidian Geological Terrain
    mat_terrain = bpy.data.materials.new(name="Mat_Terrain_Obsidian")
    mat_terrain.use_nodes = True
    tn = mat_terrain.node_tree.nodes
    tl = mat_terrain.node_tree.links
    tn.clear()

    t_out = tn.new('ShaderNodeOutputMaterial')
    bsdf_t = tn.new('ShaderNodeBsdfPrincipled')
    bsdf_t.inputs['Base Color'].default_value = (0.012, 0.018, 0.028, 1.0)
    bsdf_t.inputs['Metallic'].default_value = 0.85
    bsdf_t.inputs['Roughness'].default_value = 0.38

    coord_t = tn.new('ShaderNodeTexCoord')
    map_t = tn.new('ShaderNodeMapping')
    map_t.inputs['Scale'].default_value = (0.15, 0.15, 0.15)

    # Geological horizontal strata banding
    wave_strata = tn.new('ShaderNodeTexWave')
    wave_strata.wave_type = 'BANDS'
    wave_strata.bands_direction = 'Z'
    wave_strata.inputs['Scale'].default_value = 0.6
    wave_strata.inputs['Distortion'].default_value = 2.5
    wave_strata.inputs['Detail'].default_value = 6.0

    # High-frequency crystalline surface roughness
    noise_rock = tn.new('ShaderNodeTexNoise')
    noise_rock.inputs['Scale'].default_value = 18.0
    noise_rock.inputs['Detail'].default_value = 6.0

    bump_rock = tn.new('ShaderNodeBump')
    bump_rock.inputs['Strength'].default_value = 0.55

    math_bump_mix = tn.new('ShaderNodeMath')
    math_bump_mix.operation = 'ADD'

    tl.new(coord_t.outputs['Object'], map_t.inputs['Vector'])
    tl.new(map_t.outputs['Vector'], wave_strata.inputs['Vector'])
    tl.new(map_t.outputs['Vector'], noise_rock.inputs['Vector'])
    tl.new(wave_strata.outputs['Color'], math_bump_mix.inputs[0])
    tl.new(noise_rock.outputs['Fac'], math_bump_mix.inputs[1])
    tl.new(math_bump_mix.outputs['Value'], bump_rock.inputs['Height'])
    tl.new(bump_rock.outputs['Normal'], bsdf_t.inputs['Normal'])
    tl.new(bsdf_t.outputs['BSDF'], t_out.inputs['Surface'])

    # 2. Brushed Alloy Server Monoliths
    mat_alloy = bpy.data.materials.new(name="Mat_Monolith_Alloy")
    mat_alloy.use_nodes = True
    an = mat_alloy.node_tree.nodes
    al = mat_alloy.node_tree.links
    an.clear()
    a_out = an.new('ShaderNodeOutputMaterial')
    bsdf_a = an.new('ShaderNodeBsdfPrincipled')
    bsdf_a.inputs['Base Color'].default_value = (0.025, 0.05, 0.09, 1.0)
    bsdf_a.inputs['Metallic'].default_value = 0.95
    bsdf_a.inputs['Roughness'].default_value = 0.18
    al.new(bsdf_a.outputs['BSDF'], a_out.inputs['Surface'])

    # 3. High-Intensity Processor Core Die (Cyan / Amber)
    mat_die = bpy.data.materials.new(name="Mat_Processor_Die")
    mat_die.use_nodes = True
    dn = mat_die.node_tree.nodes
    dl = mat_die.node_tree.links
    dn.clear()
    d_out = dn.new('ShaderNodeOutputMaterial')
    bsdf_d = dn.new('ShaderNodeBsdfPrincipled')
    bsdf_d.inputs['Base Color'].default_value = (0.005, 0.015, 0.025, 1.0)
    bsdf_d.inputs['Metallic'].default_value = 0.90
    bsdf_d.inputs['Roughness'].default_value = 0.12
    bsdf_d.inputs['Emission Color'].default_value = (0.0, 0.95, 1.0, 1.0)
    bsdf_d.inputs['Emission Strength'].default_value = 14.0
    dl.new(bsdf_d.outputs['BSDF'], d_out.inputs['Surface'])

    # 4. Translucent Memory Crystal Prism
    mat_crystal = bpy.data.materials.new(name="Mat_Memory_Crystal")
    mat_crystal.use_nodes = True
    cn = mat_crystal.node_tree.nodes
    cl = mat_crystal.node_tree.links
    cn.clear()
    c_out = cn.new('ShaderNodeOutputMaterial')
    bsdf_c = cn.new('ShaderNodeBsdfPrincipled')
    bsdf_c.inputs['Base Color'].default_value = (0.0, 0.75, 0.85, 1.0)
    bsdf_c.inputs['Metallic'].default_value = 0.15
    bsdf_c.inputs['Roughness'].default_value = 0.08
    bsdf_c.inputs['IOR'].default_value = 1.54
    bsdf_c.inputs['Transmission Weight'].default_value = 0.88
    bsdf_c.inputs['Emission Color'].default_value = (0.0, 1.0, 0.80, 1.0)
    bsdf_c.inputs['Emission Strength'].default_value = 4.5
    cl.new(bsdf_c.outputs['BSDF'], c_out.inputs['Surface'])

    # 5. Glowing Conduit Bus Line (Flowing Cyan Energy Pulses)
    mat_conduit = bpy.data.materials.new(name="Mat_Conduit_Bus")
    mat_conduit.use_nodes = True
    cnn = mat_conduit.node_tree.nodes
    cnl = mat_conduit.node_tree.links
    cnn.clear()
    co_out = cnn.new('ShaderNodeOutputMaterial')
    bsdf_co = cnn.new('ShaderNodeBsdfPrincipled')
    bsdf_co.inputs['Base Color'].default_value = (0.01, 0.04, 0.08, 1.0)
    bsdf_co.inputs['Metallic'].default_value = 0.90
    bsdf_co.inputs['Roughness'].default_value = 0.20

    coord_c = cnn.new('ShaderNodeTexCoord')
    map_c = cnn.new('ShaderNodeMapping')
    map_c.inputs['Scale'].default_value = (1.0, 1.0, 0.25)

    wave_pulse = cnn.new('ShaderNodeTexWave')
    wave_pulse.wave_type = 'BANDS'
    wave_pulse.bands_direction = 'Z'
    wave_pulse.inputs['Scale'].default_value = 1.2
    wave_pulse.inputs['Distortion'].default_value = 0.0

    ramp_pulse = cnn.new('ShaderNodeValToRGB')
    ramp_pulse.color_ramp.elements[0].position = 0.25
    ramp_pulse.color_ramp.elements[0].color = (0.0, 0.2, 0.35, 1.0)
    ramp_pulse.color_ramp.elements[1].position = 0.70
    ramp_pulse.color_ramp.elements[1].color = (0.0, 1.0, 0.95, 1.0)

    math_pulse_mult = cnn.new('ShaderNodeMath')
    math_pulse_mult.operation = 'MULTIPLY'
    math_pulse_mult.inputs[1].default_value = 12.0

    cnl.new(coord_c.outputs['Generated'], map_c.inputs['Vector'])
    cnl.new(map_c.outputs['Vector'], wave_pulse.inputs['Vector'])
    cnl.new(wave_pulse.outputs['Color'], ramp_pulse.inputs['Fac'])
    cnl.new(ramp_pulse.outputs['Color'], bsdf_co.inputs['Emission Color'])
    cnl.new(wave_pulse.outputs['Fac'], math_pulse_mult.inputs[0])
    cnl.new(math_pulse_mult.outputs['Value'], bsdf_co.inputs['Emission Strength'])
    cnl.new(bsdf_co.outputs['BSDF'], co_out.inputs['Surface'])

    # 6. Holographic Laser Scanning Beam (Fine Translucent Cyan Line)
    mat_laser = bpy.data.materials.new(name="Mat_Laser_Curtain")
    mat_laser.use_nodes = True
    ln = mat_laser.node_tree.nodes
    ll = mat_laser.node_tree.links
    ln.clear()
    l_out = ln.new('ShaderNodeOutputMaterial')
    emit_l = ln.new('ShaderNodeEmission')
    emit_l.inputs['Color'].default_value = (0.0, 1.0, 0.85, 1.0)
    emit_l.inputs['Strength'].default_value = 8.0
    ll.new(emit_l.outputs['Emission'], l_out.inputs['Surface'])

    # 7. Horizon Beacon Core (Brilliant Star)
    mat_beacon = bpy.data.materials.new(name="Mat_Beacon_Core")
    mat_beacon.use_nodes = True
    bn = mat_beacon.node_tree.nodes
    bl = mat_beacon.node_tree.links
    bn.clear()
    b_out = bn.new('ShaderNodeOutputMaterial')
    emit_b = bn.new('ShaderNodeEmission')
    emit_b.inputs['Color'].default_value = (0.7, 0.95, 1.0, 1.0)
    emit_b.inputs['Strength'].default_value = 35.0
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
    # GEOMETRY: Sweeping Computational Grand Canyon with Stepped Stratum Ledges
    # =========================================================================
    mesh = bpy.data.meshes.new("GrandCanyonMesh")
    obj = bpy.data.objects.new("GrandCanyonTerrain", mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    nx = 160
    ny = 180
    lx = 140.0
    ly = 160.0

    verts = []
    for j in range(ny):
        y = -65.0 + (j / (ny - 1)) * ly
        row = []
        for i in range(nx):
            x = -70.0 + (i / (nx - 1)) * lx

            # Broad canyon floor: width ~32m, gently winding
            canyon_center_x = 5.0 * math.sin(y * 0.04)
            dx = abs(x - canyon_center_x)

            # Flat canyon bottom for |dx| < 16m
            if dx < 16.0:
                elev = -2.8 + (0.35 * math.sin(x * 0.15 + y * 0.1))
            else:
                # Stepped terraces ascending up the canyon walls
                wall_dist = dx - 16.0
                raw_h = math.pow(wall_dist / 45.0, 0.85) * 26.0
                # Quantize into 4 large geological strata steps
                step_h = 5.5
                step_level = math.floor(raw_h / step_h)
                fraction = (raw_h % step_h) / step_h
                # Steep risers with flat treads
                shelf_h = step_level * step_h + (fraction ** 4) * step_h
                elev = -2.8 + shelf_h

            # Distant horizon tech mountains (Y > 48)
            if y > 48.0:
                h_t = (y - 48.0) / 45.0
                mountain_ridge = h_t * (18.0 + 9.0 * math.sin(x * 0.10))
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
    # COMPUTE NEXUS: Monolith Mainframe Citadels Along the Canyon
    # =========================================================================
    cols = 5
    rows = 4
    col_w = 5.4
    row_d = 4.8
    for r in range(rows):
        for c in range(cols):
            cx = (c - (cols - 1) * 0.5) * col_w
            cy = -14.0 + (r * row_d)
            dist_c = math.sqrt(cx * cx + (cy - 0.0) * (cy - 0.0))
            h = max(2.6, 6.8 - dist_c * 0.32)
            w = 4.0
            d = 3.6

            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(cx, cy, h * 0.5 - 2.8))
            cube = bpy.context.active_object
            cube.name = f"ComputeMonolith_{c}_{r}"
            cube.scale = (w, d, h)
            bpy.ops.object.transform_apply(scale=True)
            cube.data.materials.append(mats['alloy'])

            bev = cube.modifiers.new("Bevel", type='BEVEL')
            bev.width = 0.16
            bev.segments = 2

            # Recessed processor die chip on top face
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(cx, cy, h - 2.74))
            die = bpy.context.active_object
            die.name = f"ProcessorDie_{c}_{r}"
            die.scale = (w * 0.70, d * 0.70, 0.12)
            bpy.ops.object.transform_apply(scale=True)
            die.data.materials.append(mats['die'])

    # =========================================================================
    # MEMORY MEGALITHS (Port Flank Crystal Prisms on Elevated Shelf)
    # =========================================================================
    for i in range(8):
        px = -25.0 + ((i % 2) * -6.5)
        py = -18.0 + (i * 5.5)
        ph = 10.0 + (i % 3) * 3.5
        pw = 2.4
        pd = 2.0
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(px, py, ph * 0.5 + 2.0))
        p = bpy.context.active_object
        p.name = f"MemoryPrism_{i}"
        p.scale = (pw, pd, ph)
        bpy.ops.object.transform_apply(scale=True)
        p.data.materials.append(mats['crystal'])

        bev = p.modifiers.new("Bevel", type='BEVEL')
        bev.width = 0.18
        bev.segments = 2

    # =========================================================================
    # STORAGE CANYONS (Starboard Flank Stepped Strata Shelves)
    # =========================================================================
    for s in range(4):
        sx = 25.0 + (s * 4.5)
        sy = 2.0
        sh = 5.0 + (s * 3.2)
        sw = 4.2
        sd = 34.0 - (s * 3.0)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(sx, sy, sh * 0.5 + 1.0))
        sv = bpy.context.active_object
        sv.name = f"StorageVault_{s}"
        sv.scale = (sw, sd, sh)
        bpy.ops.object.transform_apply(scale=True)
        sv.data.materials.append(mats['alloy'])

    # =========================================================================
    # HORIZON TRANSMISSION SPIRE & SKYWARD BEAM (At horizon, X=0, Y=72)
    # =========================================================================
    # Lattice Spire Tower
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=4.5, radius2=0.6, depth=52.0, location=(0, 72.0, 26.0))
    spire = bpy.context.active_object
    spire.name = "TransmissionPylon"
    spire.data.materials.append(mats['alloy'])

    # Structural cross-rings
    for r in range(4):
        rz = 8.0 + (r * 11.0)
        rw = 4.0 - (r * 0.8)
        bpy.ops.mesh.primitive_torus_add(major_radius=rw, minor_radius=0.22, location=(0, 72.0, rz))
        ring = bpy.context.active_object
        ring.name = f"SpireRing_{r}"
        ring.data.materials.append(mats['die'])

    # Radiant Beacon Sphere
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=1.8, location=(0, 72.0, 52.5))
    beacon = bpy.context.active_object
    beacon.name = "BeaconSphere"
    beacon.data.materials.append(mats['beacon'])

    # Skyward Vertical Light Beam
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.9, depth=90.0, location=(0, 72.0, 98.0))
    beam = bpy.context.active_object
    beam.name = "SkywardBeam"
    beam.data.materials.append(mats['laser'])

    # =========================================================================
    # BRANCHING CONDUIT NETWORKS (Luminous bus lines)
    # =========================================================================
    # Central Bus Trunk
    c_data = bpy.data.curves.new('PrimaryBusCurve', type='CURVE')
    c_data.dimensions = '3D'
    c_data.bevel_depth = 0.28
    c_data.bevel_resolution = 4
    spline = c_data.splines.new('BEZIER')
    pts = [
        Vector(( 0.0, -50.0, -2.4)),
        Vector(( 0.0, -24.0, -2.6)),
        Vector((-3.5,  -4.0, -2.4)),
        Vector(( 3.5,  18.0, -2.0)),
        Vector(( 0.0,  44.0,  2.8)),
        Vector(( 0.0,  70.0, 20.0))
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

    # Memory Tributary Branch
    m_data = bpy.data.curves.new('MemoryBusCurve', type='CURVE')
    m_data.dimensions = '3D'
    m_data.bevel_depth = 0.18
    s_m = m_data.splines.new('BEZIER')
    m_pts = [Vector((-3.5, -4.0, -2.4)), Vector((-14.0, -3.0, -1.0)), Vector((-23.0, -2.0, 2.0))]
    s_m.bezier_points.add(len(m_pts) - 1)
    for i, pt in enumerate(m_pts):
        bp = s_m.bezier_points[i]
        bp.co = pt
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
    m_obj = bpy.data.objects.new('MemoryBusConduit', m_data)
    m_obj.data.materials.append(mats['conduit'])
    bpy.context.collection.objects.link(m_obj)

    # Storage Tributary Branch
    s_data = bpy.data.curves.new('StorageBusCurve', type='CURVE')
    s_data.dimensions = '3D'
    s_data.bevel_depth = 0.18
    s_s = s_data.splines.new('BEZIER')
    sto_pts = [Vector((3.5, 18.0, -2.0)), Vector((15.0, 16.0, -0.5)), Vector((24.0, 15.0, 1.5))]
    s_s.bezier_points.add(len(sto_pts) - 1)
    for i, pt in enumerate(sto_pts):
        bp = s_s.bezier_points[i]
        bp.co = pt
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
    s_obj = bpy.data.objects.new('StorageBusConduit', s_data)
    s_obj.data.materials.append(mats['conduit'])
    bpy.context.collection.objects.link(s_obj)

    # =========================================================================
    # HOLOGRAPHIC SCANNING LASER SWEEP (Fine 3.5m Elegant Laser Sheet)
    # =========================================================================
    l_mesh = bpy.data.meshes.new("LaserSheetMesh")
    bm_l = bmesh.new()
    lw = 65.0
    lh = 3.5
    v1 = bm_l.verts.new((-lw * 0.5, 0.0, -2.6))
    v2 = bm_l.verts.new(( lw * 0.5, 0.0, -2.6))
    v3 = bm_l.verts.new(( lw * 0.5, 0.0, -2.6 + lh))
    v4 = bm_l.verts.new((-lw * 0.5, 0.0, -2.6 + lh))
    bm_l.faces.new((v1, v2, v3, v4))
    bm_l.to_mesh(l_mesh)
    bm_l.free()

    laser_obj = bpy.data.objects.new("ScanningLaserSheet", l_mesh)
    laser_obj.data.materials.append(mats['laser'])
    bpy.context.collection.objects.link(laser_obj)

    total_frames = 450
    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        y_pos = 10.0 + 26.0 * math.sin(2.0 * theta)
        laser_obj.location.y = y_pos
        laser_obj.keyframe_insert(data_path="location", index=1, frame=f)

    if laser_obj.animation_data:
        set_action_interpolation_bezier(laser_obj.animation_data.action)

    # =========================================================================
    # CINEMATIC CAMERA FLIGHT (Elevated vantage looking down the canyon)
    # =========================================================================
    cam_data = bpy.data.cameras.new("CinematicCamera")
    cam_data.lens = 28.0
    cam_data.clip_start = 0.5
    cam_data.clip_end = 450.0
    cam_obj = bpy.data.objects.new("CinematicCamera", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    target = bpy.data.objects.new("CamTarget", None)
    bpy.context.collection.objects.link(target)
    track = cam_obj.constraints.new(type='TRACK_TO')
    track.target = target
    track.track_axis = 'TRACK_NEGATIVE_Z'
    track.up_axis = 'UP_Y'

    # Camera flies along an elevated cinematic trajectory
    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        forward_t = (1.0 - math.cos(theta)) * 0.5
        
        # Advances down the canyon from Y=-46 to Y=-24 and back
        y_cam = -46.0 + forward_t * 22.0
        # Lateral sweeping motion
        x_cam = 4.8 * math.sin(theta)
        # Elevated vantage giving full grand overview of the terrain
        z_cam = 15.0 + 2.0 * math.sin(2.0 * theta) - 1.0 * math.cos(theta)

        cam_obj.location = (x_cam, y_cam, z_cam)
        cam_obj.keyframe_insert("location", frame=f)

        # Target tracks along canyon floor toward the horizon spire
        t_y = y_cam + 42.0 + 5.0 * math.sin(theta)
        t_x = 2.0 * math.sin(theta)
        t_z = 2.5 + 1.5 * math.sin(theta)
        target.location = (t_x, t_y, t_z)
        target.keyframe_insert("location", frame=f)

    for obj in (cam_obj, target):
        if obj.animation_data:
            set_action_interpolation_bezier(obj.animation_data.action)

    # =========================================================================
    # LIGHTING: Crisp Directional Sun & Ambient Rim (Chiaroscuro)
    # =========================================================================
    # Sun Light: Cool ice cyan raking across the canyon
    sun_data = bpy.data.lights.new(name="SunKeyLight", type='SUN')
    sun_data.color = (0.72, 0.92, 1.0)
    sun_data.energy = 4.0
    sun_data.angle = math.radians(2.2)
    sun_obj = bpy.data.objects.new("SunKeyLight", sun_data)
    sun_obj.rotation_euler = Euler((math.radians(58.0), math.radians(24.0), math.radians(-35.0)), 'XYZ')
    bpy.context.collection.objects.link(sun_obj)

    # Fill Light: Soft dark navy
    fill_data = bpy.data.lights.new(name="FillLight", type='SUN')
    fill_data.color = (0.04, 0.10, 0.20)
    fill_data.energy = 0.9
    fill_obj = bpy.data.objects.new("FillLight", fill_data)
    fill_obj.rotation_euler = Euler((math.radians(45.0), math.radians(-25.0), math.radians(145.0)), 'XYZ')
    bpy.context.collection.objects.link(fill_obj)

    # Save Master .blend File
    out_blend = "/home/venkatengineer/solaris-os/assets/computational-terrain/computational_terrain.blend"
    bpy.ops.wm.save_as_mainfile(filepath=out_blend)
    print(f"✓ Saved master editable project: {out_blend}")

if __name__ == '__main__':
    build_scene()
