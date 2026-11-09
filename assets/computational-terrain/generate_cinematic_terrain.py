#!/usr/bin/env python3
"""
🪐 SOLARIS — MASTER CINEMATIC COMPUTATIONAL TERRAIN GENERATOR
=============================================================
High-budget sci-fi 3D environment for Solaris OS SERVER CORE:
- Open Grand Canyon central avenue flanked by monolithic citadels
- Stepped obsidian / basalt canyon with fine orthogonal microprocessor circuit traces
- Angled Compute Nexus Towers with heatsinks, vents, and LED arrays facing inward
- Memory Crystal Megaliths with internal illuminated quantum cores
- Storage Array Vaults with longitudinal data indicator stripes
- Multi-lane glowing bus conduit highway with flowing data pulses
- Horizon Transmission Spire with needle-fine vertical laser
- Scanning holographic laser sweep
- Perfectly seamless 15s (450 frames @ 30 FPS) cinematic camera loop
- Optimized for Blender EEVEE Next rendering
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

def build_computational_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene

    # -------------------------------------------------------------------------
    # Render Settings: EEVEE Next @ 1080p
    # -------------------------------------------------------------------------
    scene.render.engine = 'BLENDER_EEVEE'
    scene.render.fps = 30
    scene.frame_start = 1
    scene.frame_end = 450
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100

    scene.view_settings.view_transform = 'Filmic'
    scene.view_settings.look = 'High Contrast'
    scene.view_settings.exposure = 0.10

    try:
        eevee = scene.eevee
        eevee.taa_render_samples = 24
        eevee.use_raytracing = True
        eevee.use_fast_gi = True
        eevee.use_shadows = True
        eevee.use_volumetric_shadows = True
    except Exception as e:
        print("EEVEE setup note:", e)

    # -------------------------------------------------------------------------
    # Deep Midnight Space World (Pure black abyss with deep navy atmospheric rim)
    # -------------------------------------------------------------------------
    world = bpy.data.worlds.new("DeepSpaceWorld")
    scene.world = world
    world.use_nodes = True
    wn = world.node_tree.nodes
    wl = world.node_tree.links
    wn.clear()
    w_out = wn.new('ShaderNodeOutputWorld')
    bg = wn.new('ShaderNodeBackground')
    bg.inputs['Color'].default_value = (0.001, 0.002, 0.005, 1.0)
    bg.inputs['Strength'].default_value = 0.25
    wl.new(bg.outputs['Background'], w_out.inputs['Surface'])

    # -------------------------------------------------------------------------
    # Master Sci-Fi Materials System
    # -------------------------------------------------------------------------
    # 1. Dark Obsidian Terrain with Fine Orthogonal Microprocessor Circuit Traces
    mat_terrain = bpy.data.materials.new(name="Mat_Terrain_Obsidian")
    mat_terrain.use_nodes = True
    tn = mat_terrain.node_tree.nodes
    tl = mat_terrain.node_tree.links
    tn.clear()
    t_out = tn.new('ShaderNodeOutputMaterial')
    bsdf_t = tn.new('ShaderNodeBsdfPrincipled')
    bsdf_t.inputs['Base Color'].default_value = (0.012, 0.016, 0.024, 1.0)
    bsdf_t.inputs['Metallic'].default_value = 0.88
    bsdf_t.inputs['Roughness'].default_value = 0.32

    coord_t = tn.new('ShaderNodeTexCoord')
    map_t = tn.new('ShaderNodeMapping')
    map_t.inputs['Scale'].default_value = (0.45, 0.45, 0.45)

    wave_x = tn.new('ShaderNodeTexWave')
    wave_x.wave_type = 'BANDS'
    wave_x.bands_direction = 'X'
    wave_x.inputs['Scale'].default_value = 0.45
    wave_x.inputs['Distortion'].default_value = 0.0

    wave_y = tn.new('ShaderNodeTexWave')
    wave_y.wave_type = 'BANDS'
    wave_y.bands_direction = 'Y'
    wave_y.inputs['Scale'].default_value = 0.45
    wave_y.inputs['Distortion'].default_value = 0.0

    math_grid_add = tn.new('ShaderNodeMath')
    math_grid_add.operation = 'MAXIMUM'

    # Ultra-fine crisp glowing circuit trace lines
    ramp_circuit_lines = tn.new('ShaderNodeValToRGB')
    ramp_circuit_lines.color_ramp.elements[0].position = 0.965
    ramp_circuit_lines.color_ramp.elements[0].color = (0.0, 0.0, 0.0, 1.0)
    ramp_circuit_lines.color_ramp.elements[1].position = 0.995
    ramp_circuit_lines.color_ramp.elements[1].color = (0.0, 0.85, 1.0, 1.0)

    noise_micro = tn.new('ShaderNodeTexNoise')
    noise_micro.inputs['Scale'].default_value = 24.0
    noise_micro.inputs['Detail'].default_value = 4.0
    bump_rock = tn.new('ShaderNodeBump')
    bump_rock.inputs['Strength'].default_value = 0.18

    tl.new(coord_t.outputs['Object'], map_t.inputs['Vector'])
    tl.new(map_t.outputs['Vector'], wave_x.inputs['Vector'])
    tl.new(map_t.outputs['Vector'], wave_y.inputs['Vector'])
    tl.new(wave_x.outputs['Color'], math_grid_add.inputs[0])
    tl.new(wave_y.outputs['Color'], math_grid_add.inputs[1])
    tl.new(math_grid_add.outputs['Value'], ramp_circuit_lines.inputs['Fac'])
    tl.new(ramp_circuit_lines.outputs['Color'], bsdf_t.inputs['Emission Color'])
    bsdf_t.inputs['Emission Strength'].default_value = 1.6

    tl.new(map_t.outputs['Vector'], noise_micro.inputs['Vector'])
    tl.new(noise_micro.outputs['Fac'], bump_rock.inputs['Height'])
    tl.new(bump_rock.outputs['Normal'], bsdf_t.inputs['Normal'])
    tl.new(bsdf_t.outputs['BSDF'], t_out.inputs['Surface'])

    # 2. Brushed Dark Alloy Server Chassis
    mat_alloy = bpy.data.materials.new(name="Mat_Server_Alloy")
    mat_alloy.use_nodes = True
    an = mat_alloy.node_tree.nodes
    al = mat_alloy.node_tree.links
    an.clear()
    a_out = an.new('ShaderNodeOutputMaterial')
    bsdf_a = an.new('ShaderNodeBsdfPrincipled')
    bsdf_a.inputs['Base Color'].default_value = (0.016, 0.024, 0.038, 1.0)
    bsdf_a.inputs['Metallic'].default_value = 0.95
    bsdf_a.inputs['Roughness'].default_value = 0.20
    al.new(bsdf_a.outputs['BSDF'], a_out.inputs['Surface'])

    # 3. High-Tech Server Heatsink & Vent Fins (Deep Matte Charcoal)
    mat_heatsink = bpy.data.materials.new(name="Mat_Server_Heatsink")
    mat_heatsink.use_nodes = True
    hn = mat_heatsink.node_tree.nodes
    hl = mat_heatsink.node_tree.links
    hn.clear()
    h_out = hn.new('ShaderNodeOutputMaterial')
    bsdf_h = hn.new('ShaderNodeBsdfPrincipled')
    bsdf_h.inputs['Base Color'].default_value = (0.006, 0.010, 0.014, 1.0)
    bsdf_h.inputs['Metallic'].default_value = 0.90
    bsdf_h.inputs['Roughness'].default_value = 0.45
    hl.new(bsdf_h.outputs['BSDF'], h_out.inputs['Surface'])

    # 4. Monolith LED Telemetry Matrices (Glowing Activity Dots)
    mat_led_cyan = bpy.data.materials.new(name="Mat_LED_Cyan")
    mat_led_cyan.use_nodes = True
    ln = mat_led_cyan.node_tree.nodes
    ll = mat_led_cyan.node_tree.links
    ln.clear()
    l_out = ln.new('ShaderNodeOutputMaterial')
    emit_cyan = ln.new('ShaderNodeEmission')
    emit_cyan.inputs['Color'].default_value = (0.0, 0.95, 1.0, 1.0)
    emit_cyan.inputs['Strength'].default_value = 6.0
    ll.new(emit_cyan.outputs['Emission'], l_out.inputs['Surface'])

    mat_led_amber = bpy.data.materials.new(name="Mat_LED_Amber")
    mat_led_amber.use_nodes = True
    lan = mat_led_amber.node_tree.nodes
    lal = mat_led_amber.node_tree.links
    lan.clear()
    la_out = lan.new('ShaderNodeOutputMaterial')
    emit_amber = lan.new('ShaderNodeEmission')
    emit_amber.inputs['Color'].default_value = (1.0, 0.65, 0.05, 1.0)
    emit_amber.inputs['Strength'].default_value = 5.0
    lal.new(emit_amber.outputs['Emission'], la_out.inputs['Surface'])

    # 5. Quantum Memory Crystal (Translucent Smoky Emerald/Teal Glass)
    mat_crystal = bpy.data.materials.new(name="Mat_Memory_Crystal")
    mat_crystal.use_nodes = True
    cn = mat_crystal.node_tree.nodes
    cl = mat_crystal.node_tree.links
    cn.clear()
    c_out = cn.new('ShaderNodeOutputMaterial')
    bsdf_c = cn.new('ShaderNodeBsdfPrincipled')
    bsdf_c.inputs['Base Color'].default_value = (0.01, 0.06, 0.10, 1.0)
    bsdf_c.inputs['Metallic'].default_value = 0.05
    bsdf_c.inputs['Roughness'].default_value = 0.06
    bsdf_c.inputs['IOR'].default_value = 1.54
    bsdf_c.inputs['Transmission Weight'].default_value = 0.92
    bsdf_c.inputs['Emission Strength'].default_value = 0.0
    cl.new(bsdf_c.outputs['BSDF'], c_out.inputs['Surface'])

    # 6. Conduit Bus (Flowing Luminous Data Waves)
    mat_conduit = bpy.data.materials.new(name="Mat_Conduit_Bus")
    mat_conduit.use_nodes = True
    cnn = mat_conduit.node_tree.nodes
    cnl = mat_conduit.node_tree.links
    cnn.clear()
    co_out = cnn.new('ShaderNodeOutputMaterial')
    bsdf_co = cnn.new('ShaderNodeBsdfPrincipled')
    bsdf_co.inputs['Base Color'].default_value = (0.01, 0.03, 0.05, 1.0)
    bsdf_co.inputs['Metallic'].default_value = 0.92
    bsdf_co.inputs['Roughness'].default_value = 0.18

    coord_co = cnn.new('ShaderNodeTexCoord')
    map_co = cnn.new('ShaderNodeMapping')
    map_co.name = 'ConduitMapping'
    map_co.inputs['Scale'].default_value = (1.0, 1.0, 16.0)

    wave_pulse = cnn.new('ShaderNodeTexWave')
    wave_pulse.wave_type = 'BANDS'
    wave_pulse.bands_direction = 'Z'
    wave_pulse.inputs['Scale'].default_value = 1.0

    ramp_pulse = cnn.new('ShaderNodeValToRGB')
    ramp_pulse.color_ramp.elements[0].position = 0.35
    ramp_pulse.color_ramp.elements[0].color = (0.0, 0.12, 0.22, 1.0)
    ramp_pulse.color_ramp.elements[1].position = 0.75
    ramp_pulse.color_ramp.elements[1].color = (0.0, 0.95, 1.0, 1.0)

    math_pulse_mult = cnn.new('ShaderNodeMath')
    math_pulse_mult.operation = 'MULTIPLY'
    math_pulse_mult.inputs[1].default_value = 7.0

    cnl.new(coord_co.outputs['Generated'], map_co.inputs['Vector'])
    cnl.new(map_co.outputs['Vector'], wave_pulse.inputs['Vector'])
    cnl.new(wave_pulse.outputs['Color'], ramp_pulse.inputs['Fac'])
    cnl.new(ramp_pulse.outputs['Color'], bsdf_co.inputs['Emission Color'])
    cnl.new(wave_pulse.outputs['Fac'], math_pulse_mult.inputs[0])
    cnl.new(math_pulse_mult.outputs['Value'], bsdf_co.inputs['Emission Strength'])
    cnl.new(bsdf_co.outputs['BSDF'], co_out.inputs['Surface'])

    # Animate conduit pulse offset
    total_frames = 450
    for f in range(1, total_frames + 2):
        frac = (f - 1) / total_frames
        offset_z = -frac * 8.0
        map_co.inputs['Location'].default_value[2] = offset_z
        map_co.inputs['Location'].keyframe_insert(data_path="default_value", index=2, frame=f)

    # 7. Holographic Laser Scanning Line (Fine glowing ground scan line)
    mat_laser = bpy.data.materials.new(name="Mat_Laser_Scan")
    mat_laser.use_nodes = True
    lnn = mat_laser.node_tree.nodes
    lnl = mat_laser.node_tree.links
    lnn.clear()
    laser_out = lnn.new('ShaderNodeOutputMaterial')
    emit_laser = lnn.new('ShaderNodeEmission')
    emit_laser.inputs['Color'].default_value = (0.0, 1.0, 0.90, 1.0)
    emit_laser.inputs['Strength'].default_value = 5.0
    lnl.new(emit_laser.outputs['Emission'], laser_out.inputs['Surface'])

    # 8. Horizon Beacon Star
    mat_beacon = bpy.data.materials.new(name="Mat_Beacon_Star")
    mat_beacon.use_nodes = True
    bnn = mat_beacon.node_tree.nodes
    bnl = mat_beacon.node_tree.links
    bnn.clear()
    beacon_out = bnn.new('ShaderNodeOutputMaterial')
    emit_beacon = bnn.new('ShaderNodeEmission')
    emit_beacon.inputs['Color'].default_value = (0.7, 0.95, 1.0, 1.0)
    emit_beacon.inputs['Strength'].default_value = 16.0
    bnl.new(emit_beacon.outputs['Emission'], beacon_out.inputs['Surface'])

    mats = {
        'terrain': mat_terrain,
        'alloy': mat_alloy,
        'heatsink': mat_heatsink,
        'led_cyan': mat_led_cyan,
        'led_amber': mat_led_amber,
        'crystal': mat_crystal,
        'conduit': mat_conduit,
        'laser': mat_laser,
        'beacon': mat_beacon
    }

    # -------------------------------------------------------------------------
    # GEOMETRY: Open Grand Canyon with Flanking Stepped Terraces
    # -------------------------------------------------------------------------
    mesh = bpy.data.meshes.new("GrandCanyonMesh")
    obj = bpy.data.objects.new("GrandCanyonTerrain", mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    nx = 140
    ny = 160
    lx = 130.0
    ly = 150.0

    verts = []
    for j in range(ny):
        y = -60.0 + (j / (ny - 1)) * ly
        row = []
        for i in range(nx):
            x = -65.0 + (i / (nx - 1)) * lx

            canyon_center_x = 3.0 * math.sin(y * 0.04)
            dx = abs(x - canyon_center_x)

            if dx < 10.0:
                elev = -3.2 + (0.15 * math.sin(x * 0.2 + y * 0.15))
            else:
                wall_dist = dx - 10.0
                raw_h = math.pow(wall_dist / 38.0, 0.8) * 22.0
                step_h = 4.8
                step_level = math.floor(raw_h / step_h)
                fraction = (raw_h % step_h) / step_h
                elev = -3.2 + (step_level * step_h) + (fraction ** 5) * step_h

            if y > 42.0:
                dist_factor = (y - 42.0) / 48.0
                elev = elev * (1.0 - dist_factor * 0.85) - (dist_factor * 14.0)

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

    # -------------------------------------------------------------------------
    # COMPUTE CITADELS: Properly Positioned & Angled Monoliths
    # -------------------------------------------------------------------------
    def spawn_monolith(name, x, y, yaw_deg, w, d, h):
        yaw_rad = math.radians(yaw_deg)
        cos_y = math.cos(yaw_rad)
        sin_y = math.sin(yaw_rad)

        # Main Chassis Tower
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, h * 0.5 - 3.2))
        tower = bpy.context.active_object
        tower.name = name
        tower.scale = (w, d, h)
        tower.rotation_euler = Euler((0, 0, yaw_rad), 'XYZ')
        bpy.ops.object.transform_apply(scale=True, rotation=False)
        tower.data.materials.append(mats['alloy'])

        bev = tower.modifiers.new("Bevel", type='BEVEL')
        bev.width = 0.14
        bev.segments = 2

        # Front Face Grill Fins offset along rotated -Y
        dy_loc = -(d * 0.5 + 0.06)
        gx = x + (dy_loc * -sin_y)
        gy = y + (dy_loc * cos_y)
        gz = h * 0.5 - 3.2

        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(gx, gy, gz))
        grill = bpy.context.active_object
        grill.name = f"{name}_Grill"
        grill.scale = (w * 0.78, 0.10, h * 0.80)
        grill.rotation_euler = Euler((0, 0, yaw_rad), 'XYZ')
        bpy.ops.object.transform_apply(scale=True, rotation=False)
        grill.data.materials.append(mats['heatsink'])

        # Multi-Blade LEDs
        for l_idx in range(3):
            lz = gz - (h * 0.25) + (l_idx * (h * 0.25))
            lx_loc = -0.3 + (l_idx * 0.3)
            ly_loc = -(d * 0.5 + 0.12)
            lx = x + (lx_loc * cos_y - ly_loc * sin_y)
            ly = y + (lx_loc * sin_y + ly_loc * cos_y)
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(lx, ly, lz))
            led = bpy.context.active_object
            led.name = f"{name}_LED_{l_idx}"
            led.scale = (0.22, 0.06, 0.10)
            led.rotation_euler = Euler((0, 0, yaw_rad), 'XYZ')
            bpy.ops.object.transform_apply(scale=True, rotation=False)
            led.data.materials.append(mats['led_cyan'] if (l_idx % 2 == 0) else mats['led_amber'])

        # Top Accent Ring
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, h - 3.08))
        accent_ring = bpy.context.active_object
        accent_ring.name = f"{name}_Accent"
        accent_ring.scale = (w * 0.72, d * 0.72, 0.05)
        accent_ring.rotation_euler = Euler((0, 0, yaw_rad), 'XYZ')
        bpy.ops.object.transform_apply(scale=True, rotation=False)
        accent_ring.data.materials.append(mats['led_cyan'])

    # Spawn Left Flank Monolith Citadels (Angled +24° inward toward canyon road)
    for i in range(5):
        mx = -14.0 - (i % 2) * 3.5
        my = -24.0 + (i * 9.5)
        mh = 10.0 + (i % 3) * 3.0
        spawn_monolith(f"LeftTower_{i}", mx, my, 24.0, 4.2, 4.0, mh)

    # Spawn Right Flank Monolith Citadels (Angled -24° inward toward canyon road)
    for i in range(5):
        mx = 14.0 + (i % 2) * 3.5
        my = -24.0 + (i * 9.5)
        mh = 10.0 + ((i + 1) % 3) * 3.0
        spawn_monolith(f"RightTower_{i}", mx, my, -24.0, 4.2, 4.0, mh)

    # -------------------------------------------------------------------------
    # MEMORY MEGALITHS (Port Outer Terrace)
    # -------------------------------------------------------------------------
    for i in range(6):
        px = -25.0 - ((i % 2) * 4.5)
        py = -20.0 + (i * 7.5)
        ph = 11.0 + (i % 3) * 3.5
        pw = 2.6
        pd = 2.4
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(px, py, ph * 0.5 + 1.5))
        p = bpy.context.active_object
        p.name = f"MemoryCrystal_{i}"
        p.scale = (pw, pd, ph)
        bpy.ops.object.transform_apply(scale=True)
        p.data.materials.append(mats['crystal'])

        bev = p.modifiers.new("Bevel", type='BEVEL')
        bev.width = 0.22
        bev.segments = 2

        bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=0.16, depth=ph * 0.75, location=(px, py, ph * 0.5 + 1.5))
        core_rod = bpy.context.active_object
        core_rod.name = f"CrystalCore_{i}"
        core_rod.data.materials.append(mats['led_cyan'])

    # -------------------------------------------------------------------------
    # STORAGE CANYONS (Starboard Outer Terrace)
    # -------------------------------------------------------------------------
    for s in range(4):
        sx = 25.0 + (s * 4.2)
        sy = 0.0
        sh = 5.5 + (s * 3.2)
        sw = 3.6
        sd = 36.0 - (s * 3.5)
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(sx, sy, sh * 0.5 + 0.8))
        sv = bpy.context.active_object
        sv.name = f"StorageArray_{s}"
        sv.scale = (sw, sd, sh)
        bpy.ops.object.transform_apply(scale=True)
        sv.data.materials.append(mats['alloy'])

        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(sx - (sw * 0.5) - 0.04, sy, sh * 0.5 + 0.8))
        stripe = bpy.context.active_object
        stripe.name = f"StorageStripe_{s}"
        stripe.scale = (0.06, sd * 0.92, 0.16)
        bpy.ops.object.transform_apply(scale=True)
        stripe.data.materials.append(mats['led_cyan'])

    # -------------------------------------------------------------------------
    # HORIZON TRANSMISSION SPIRE & SKYWARD BEAM (At horizon, X=0, Y=56)
    # -------------------------------------------------------------------------
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=3.2, radius2=0.35, depth=46.0, location=(0, 56.0, 23.0))
    spire = bpy.context.active_object
    spire.name = "TransmissionPylon"
    spire.data.materials.append(mats['alloy'])

    for r in range(4):
        rz = 8.0 + (r * 9.5)
        rw = 2.8 - (r * 0.55)
        bpy.ops.mesh.primitive_torus_add(major_radius=rw, minor_radius=0.16, location=(0, 56.0, rz))
        ring = bpy.context.active_object
        ring.name = f"SpireRing_{r}"
        ring.data.materials.append(mats['led_cyan'])

    # Radiant Beacon Sphere
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=1.3, location=(0, 56.0, 46.5))
    beacon = bpy.context.active_object
    beacon.name = "BeaconSphere"
    beacon.data.materials.append(mats['beacon'])

    # Needle-fine Skyward Vertical Laser
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.14, depth=120.0, location=(0, 56.0, 106.0))
    beam = bpy.context.active_object
    beam.name = "SkywardBeam"
    beam.data.materials.append(mats['laser'])

    # -------------------------------------------------------------------------
    # CENTRAL MULTI-LANE CONDUIT HIGHWAY (Running straight down the canyon)
    # -------------------------------------------------------------------------
    def spawn_conduit(name, offset_x, scale_depth=0.22):
        c_data = bpy.data.curves.new(f'{name}Curve', type='CURVE')
        c_data.dimensions = '3D'
        c_data.bevel_depth = scale_depth
        c_data.bevel_resolution = 4
        spline = c_data.splines.new('BEZIER')
        pts = [
            Vector((offset_x, -55.0, -2.8)),
            Vector((offset_x, -25.0, -2.9)),
            Vector((offset_x * 0.8, -4.0, -2.8)),
            Vector((offset_x * 0.5, 18.0, -2.2)),
            Vector((offset_x * 0.2, 38.0, -0.5)),
            Vector((0.0, 54.0, 14.0))
        ]
        spline.bezier_points.add(len(pts) - 1)
        for i, pt in enumerate(pts):
            bp = spline.bezier_points[i]
            bp.co = pt
            bp.handle_left_type = 'AUTO'
            bp.handle_right_type = 'AUTO'
        c_obj = bpy.data.objects.new(name, c_data)
        c_obj.data.materials.append(mats['conduit'])
        bpy.context.collection.objects.link(c_obj)

    spawn_conduit('CenterBus_Main', 0.0, 0.26)
    spawn_conduit('CenterBus_Port', -2.2, 0.18)
    spawn_conduit('CenterBus_Starboard', 2.2, 0.18)

    # -------------------------------------------------------------------------
    # HOLOGRAPHIC SCANNING LASER LINE (Fine Razor-Thin Ground Scan Line)
    # -------------------------------------------------------------------------
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.08, depth=32.0, location=(0, 0, -2.65))
    laser_beam = bpy.context.active_object
    laser_beam.name = "ScanningLaserBeam"
    laser_beam.rotation_euler = Euler((0, math.radians(90), 0), 'XYZ')
    laser_beam.data.materials.append(mats['laser'])

    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        y_pos = 2.0 + 22.0 * math.sin(2.0 * theta)
        laser_beam.location.y = y_pos
        laser_beam.keyframe_insert(data_path="location", index=1, frame=f)

    if laser_beam.animation_data:
        set_action_interpolation_bezier(laser_beam.animation_data.action)

    # -------------------------------------------------------------------------
    # CINEMATIC CAMERA FLIGHT (Gliding Over the Central Highway with Grand Depth)
    # -------------------------------------------------------------------------
    cam_data = bpy.data.cameras.new("CinematicCamera")
    cam_data.lens = 28.0
    cam_data.clip_start = 0.5
    cam_data.clip_end = 350.0
    cam_obj = bpy.data.objects.new("CinematicCamera", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    scene.camera = cam_obj

    target = bpy.data.objects.new("CamTarget", None)
    bpy.context.collection.objects.link(target)
    track = cam_obj.constraints.new(type='TRACK_TO')
    track.target = target
    track.track_axis = 'TRACK_NEGATIVE_Z'
    track.up_axis = 'UP_Y'

    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        forward_t = (1.0 - math.cos(theta)) * 0.5

        # Flight path gliding along the canyon corridor
        y_cam = -42.0 + forward_t * 22.0
        x_cam = 3.6 * math.sin(theta)
        z_cam = 14.0 + 1.8 * math.sin(2.0 * theta) - 0.8 * math.cos(theta)

        cam_obj.location = (x_cam, y_cam, z_cam)
        cam_obj.keyframe_insert("location", frame=f)

        # Dynamic tracking focus looking forward toward the horizon spire
        t_y = y_cam + 40.0 + 3.0 * math.sin(theta)
        t_x = 1.2 * math.sin(theta)
        t_z = 2.5 + 1.0 * math.sin(theta)
        target.location = (t_x, t_y, t_z)
        target.keyframe_insert("location", frame=f)

    for obj in (cam_obj, target):
        if obj.animation_data:
            set_action_interpolation_bezier(obj.animation_data.action)

    # -------------------------------------------------------------------------
    # LIGHTING: Dramatic Chiaroscuro (Cool Ice Cyan Key & Rim, Deep Navy Shadows)
    # -------------------------------------------------------------------------
    sun_data = bpy.data.lights.new(name="SunKeyLight", type='SUN')
    sun_data.color = (0.75, 0.94, 1.0)
    sun_data.energy = 2.2
    sun_data.angle = math.radians(2.0)
    sun_obj = bpy.data.objects.new("SunKeyLight", sun_data)
    sun_obj.rotation_euler = Euler((math.radians(64.0), math.radians(20.0), math.radians(-40.0)), 'XYZ')
    bpy.context.collection.objects.link(sun_obj)

    fill_data = bpy.data.lights.new(name="FillLight", type='SUN')
    fill_data.color = (0.02, 0.05, 0.12)
    fill_data.energy = 0.35
    fill_obj = bpy.data.objects.new("FillLight", fill_data)
    fill_obj.rotation_euler = Euler((math.radians(45.0), math.radians(-30.0), math.radians(135.0)), 'XYZ')
    bpy.context.collection.objects.link(fill_obj)

    out_blend = "/home/venkatengineer/solaris-os/assets/computational-terrain/computational_terrain.blend"
    bpy.ops.wm.save_as_mainfile(filepath=out_blend)
    print(f"✓ Master scene generated and saved: {out_blend}")

if __name__ == '__main__':
    build_computational_scene()
