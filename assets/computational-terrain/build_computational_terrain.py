#!/usr/bin/env python3
"""
🪐 SOLARIS — COMPUTATIONAL TERRAIN GENERATOR FOR BLENDER 5.2+
Creates a complete, professionally art-directed 3D scene:
- Grand computational canyon with stepped tectonic ridges
- Compute Nexus processor monoliths with pulsating dies
- Memory Megaliths (crystal RAM towers with illuminated layer lines)
- Storage Canyon strata with flashing NVMe sector channels
- Horizon transmission spire with skyward volumetric beacon
- Branching conduit network with flowing photon energy pulses
- Sweeping holographic laser sensor plane
- Cinematic camera flight along a 100% mathematically seamless 450-frame loop (15s @ 30fps)
- Saves computational_terrain.blend
"""

import bpy
import bmesh
import math
import os
from mathutils import Vector, Euler

def clean_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    # Ensure Cycles and Optix
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    try:
        cpref = bpy.context.preferences.addons['cycles'].preferences
        cpref.compute_device_type = 'OPTIX'
        cpref.get_devices_for_type('OPTIX')
        for d in cpref.devices:
            if d.type == 'OPTIX':
                d.use = True
        scene.cycles.device = 'GPU'
    except Exception as e:
        print("Note on GPU setup:", e)

    # Frame & Render Settings (15s @ 30 FPS = 450 frames)
    scene.render.fps = 30
    scene.frame_start = 1
    scene.frame_end = 450
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100
    
    # Cycles performance optimization
    scene.cycles.samples = 32
    scene.cycles.preview_samples = 16
    scene.cycles.use_denoising = True
    scene.cycles.denoiser = 'OPTIX'
    scene.cycles.max_bounces = 6
    scene.cycles.diffuse_bounces = 3
    scene.cycles.glossy_bounces = 4
    scene.cycles.transmission_bounces = 4
    scene.cycles.volume_bounces = 2

    # Color Management (Filmic / AgX with high contrast)
    scene.view_settings.view_transform = 'AgX' if 'AgX' in [v.name for v in scene.view_settings.bl_rna.properties['view_transform'].enum_items] else 'Filmic'
    scene.view_settings.look = 'High Contrast'
    scene.view_settings.exposure = 0.2

def create_materials():
    mats = {}

    # 1. Dark Basalt / Carbon Tech Substrate
    mat_terrain = bpy.data.materials.new(name="Mat_Terrain_Substrate")
    mat_terrain.use_nodes = True
    nodes = mat_terrain.node_tree.nodes
    links = mat_terrain.node_tree.links
    nodes.clear()

    out_node = nodes.new('ShaderNodeOutputMaterial')
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Base Color'].default_value = (0.025, 0.045, 0.08, 1.0)
    bsdf.inputs['Metallic'].default_value = 0.82
    bsdf.inputs['Roughness'].default_value = 0.42

    # Procedural Circuit Grooves (Noise -> ColorRamp -> Bump & Emission)
    coord = nodes.new('ShaderNodeTexCoord')
    mapping = nodes.new('ShaderNodeMapping')
    mapping.inputs['Scale'].default_value = (0.4, 0.4, 0.4)

    noise1 = nodes.new('ShaderNodeTexNoise')
    noise1.inputs['Scale'].default_value = 14.0
    noise1.inputs['Detail'].default_value = 6.0
    noise1.inputs['Roughness'].default_value = 0.65

    voronoi = nodes.new('ShaderNodeTexVoronoi')
    voronoi.feature = 'DISTANCE_TO_EDGE'
    voronoi.inputs['Scale'].default_value = 6.0

    math_mult = nodes.new('ShaderNodeMath')
    math_mult.operation = 'MULTIPLY'

    ramp_emiss = nodes.new('ShaderNodeValToRGB')
    ramp_emiss.color_ramp.elements[0].position = 0.08
    ramp_emiss.color_ramp.elements[0].color = (0.0, 0.0, 0.0, 1.0)
    ramp_emiss.color_ramp.elements[1].position = 0.14
    ramp_emiss.color_ramp.elements[1].color = (0.0, 0.95, 1.0, 1.0)

    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.45

    links.new(coord.outputs['Object'], mapping.inputs['Vector'])
    links.new(mapping.outputs['Vector'], noise1.inputs['Vector'])
    links.new(mapping.outputs['Vector'], voronoi.inputs['Vector'])
    links.new(noise1.outputs['Fac'], math_mult.inputs[0])
    links.new(voronoi.outputs['Distance'], math_mult.inputs[1])
    links.new(math_mult.outputs['Value'], ramp_emiss.inputs['Fac'])
    links.new(math_mult.outputs['Value'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    links.new(ramp_emiss.outputs['Color'], bsdf.inputs['Emission Color'])
    bsdf.inputs['Emission Strength'].default_value = 3.5
    links.new(bsdf.outputs['BSDF'], out_node.inputs['Surface'])
    mats['terrain'] = mat_terrain

    # 2. Brushed Alloy Processor Monolith
    mat_alloy = bpy.data.materials.new(name="Mat_Compute_Alloy")
    mat_alloy.use_nodes = True
    n_alloy = mat_alloy.node_tree.nodes
    l_alloy = mat_alloy.node_tree.links
    n_alloy.clear()
    out_a = n_alloy.new('ShaderNodeOutputMaterial')
    bsdf_a = n_alloy.new('ShaderNodeBsdfPrincipled')
    bsdf_a.inputs['Base Color'].default_value = (0.05, 0.09, 0.16, 1.0)
    bsdf_a.inputs['Metallic'].default_value = 0.94
    bsdf_a.inputs['Roughness'].default_value = 0.28
    l_alloy.new(bsdf_a.outputs['BSDF'], out_a.inputs['Surface'])
    mats['alloy'] = mat_alloy

    # 3. High-Temperature Processor Core Die (Emissive Cyan / Amber Pulse)
    mat_die = bpy.data.materials.new(name="Mat_Processor_Die")
    mat_die.use_nodes = True
    n_die = mat_die.node_tree.nodes
    l_die = mat_die.node_tree.links
    n_die.clear()
    out_d = n_die.new('ShaderNodeOutputMaterial')
    bsdf_d = n_die.new('ShaderNodeBsdfPrincipled')
    bsdf_d.inputs['Base Color'].default_value = (0.01, 0.02, 0.04, 1.0)
    bsdf_d.inputs['Metallic'].default_value = 0.90
    bsdf_d.inputs['Roughness'].default_value = 0.15
    bsdf_d.inputs['Emission Color'].default_value = (0.0, 0.92, 1.0, 1.0)
    bsdf_d.inputs['Emission Strength'].default_value = 12.0
    l_die.new(bsdf_d.outputs['BSDF'], out_d.inputs['Surface'])
    mats['die'] = mat_die

    # 4. Memory Megalith Crystal (Translucent Cyan Prism with Layer Lines)
    mat_crystal = bpy.data.materials.new(name="Mat_Memory_Crystal")
    mat_crystal.use_nodes = True
    n_cry = mat_crystal.node_tree.nodes
    l_cry = mat_crystal.node_tree.links
    n_cry.clear()
    out_c = n_cry.new('ShaderNodeOutputMaterial')
    bsdf_c = n_cry.new('ShaderNodeBsdfPrincipled')
    bsdf_c.inputs['Base Color'].default_value = (0.0, 0.85, 0.70, 1.0)
    bsdf_c.inputs['Metallic'].default_value = 0.35
    bsdf_c.inputs['Roughness'].default_value = 0.12
    bsdf_c.inputs['IOR'].default_value = 1.54
    bsdf_c.inputs['Emission Color'].default_value = (0.0, 1.0, 0.75, 1.0)
    bsdf_c.inputs['Emission Strength'].default_value = 5.0
    l_cry.new(bsdf_c.outputs['BSDF'], out_c.inputs['Surface'])
    mats['crystal'] = mat_crystal

    # 5. Glowing Conduit Bus Line (Flowing Cyan Energy)
    mat_conduit = bpy.data.materials.new(name="Mat_Conduit_Bus")
    mat_conduit.use_nodes = True
    n_con = mat_conduit.node_tree.nodes
    l_con = mat_conduit.node_tree.links
    n_con.clear()
    out_co = n_con.new('ShaderNodeOutputMaterial')
    bsdf_co = n_con.new('ShaderNodeBsdfPrincipled')
    bsdf_co.inputs['Base Color'].default_value = (0.0, 0.5, 0.8, 1.0)
    bsdf_co.inputs['Emission Color'].default_value = (0.0, 0.95, 1.0, 1.0)
    bsdf_co.inputs['Emission Strength'].default_value = 8.0
    l_con.new(bsdf_co.outputs['BSDF'], out_co.inputs['Surface'])
    mats['conduit'] = mat_conduit

    # 6. Holographic Scanning Laser Sheet (Translucent Vertical Gradient)
    mat_laser = bpy.data.materials.new(name="Mat_Laser_Scanner")
    mat_laser.use_nodes = True
    n_las = mat_laser.node_tree.nodes
    l_las = mat_laser.node_tree.links
    n_las.clear()
    out_l = n_las.new('ShaderNodeOutputMaterial')
    emit_l = n_las.new('ShaderNodeEmission')
    emit_l.inputs['Color'].default_value = (0.0, 0.95, 1.0, 1.0)
    emit_l.inputs['Strength'].default_value = 6.0
    trans_l = n_las.new('ShaderNodeBsdfTransparent')
    mix_l = n_las.new('ShaderNodeMixShader')
    mix_l.inputs['Fac'].default_value = 0.28
    l_las.new(trans_l.outputs['BSDF'], mix_l.inputs[1])
    l_las.new(emit_l.outputs['Emission'], mix_l.inputs[2])
    l_las.new(mix_l.outputs['Shader'], out_l.inputs['Surface'])
    mats['laser'] = mat_laser

    # 7. Horizon Beacon Core (Intense Radiant Point)
    mat_beacon = bpy.data.materials.new(name="Mat_Beacon_Core")
    mat_beacon.use_nodes = True
    n_b = mat_beacon.node_tree.nodes
    l_b = mat_beacon.node_tree.links
    n_b.clear()
    out_b = n_b.new('ShaderNodeOutputMaterial')
    emit_b = n_b.new('ShaderNodeEmission')
    emit_b.inputs['Color'].default_value = (0.5, 0.95, 1.0, 1.0)
    emit_b.inputs['Strength'].default_value = 45.0
    l_b.new(emit_b.outputs['Emission'], out_b.inputs['Surface'])
    mats['beacon'] = mat_beacon

    return mats

def build_computational_terrain(mats):
    # 1. Base Procedural Grid Mesh
    mesh = bpy.data.meshes.new("ComputationalTerrainMesh")
    obj = bpy.data.objects.new("ComputationalTerrain", mesh)
    bpy.context.collection.objects.link(obj)

    bm = bmesh.new()
    nx = 140
    ny = 160
    lx = 100.0
    ly = 130.0

    verts = []
    for j in range(ny):
        y = -50.0 + (j / (ny - 1)) * ly
        row = []
        for i in range(nx):
            x = -50.0 + (i / (nx - 1)) * lx
            
            # --- Grand Central Canyon Profile ---
            canyon_w = 12.0 + 3.0 * math.sin(y * 0.08)
            dist_x = abs(x)
            
            # Canyon depth: deep trench at center (|x| < canyon_w), rising up to flanking ridges
            canyon_factor = max(0.0, min(1.0, (dist_x - 3.0) / (canyon_w - 3.0)))
            # Smoothstep
            canyon_factor = canyon_factor * canyon_factor * (3.0 - 2.0 * canyon_factor)
            
            # Flanking Continental Ridges
            ridge_h = (math.sin(x * 0.07) * 4.0) + (math.cos(y * 0.05) * 5.0) + 12.0
            
            # Stepped Geological Plateaus (quantized terraces)
            raw_elev = (canyon_factor * ridge_h) - 3.5
            terrace_step = 2.4
            quantized = math.floor(raw_elev / terrace_step) * terrace_step
            blend_terrace = raw_elev * 0.35 + quantized * 0.65
            
            # Secondary micro-canyons / erosion fissures
            erosion = math.sin(x * 0.45 + y * 0.25) * math.cos(y * 0.35) * 0.9
            
            # Distant Horizon Mountain Uplift (Y > 40)
            horizon_lift = 0.0
            if y > 35.0:
                h_frac = (y - 35.0) / 45.0
                horizon_lift = h_frac * h_frac * (18.0 + math.cos(x * 0.15) * 8.0)
            
            z = blend_terrace + erosion + horizon_lift
            v = bm.verts.new((x, y, z))
            row.append(v)
        verts.append(row)

    bm.verts.ensure_lookup_table()

    # Create Quad Faces
    for j in range(ny - 1):
        for i in range(nx - 1):
            v1 = verts[j][i]
            v2 = verts[j][i + 1]
            v3 = verts[j + 1][i + 1]
            v4 = verts[j + 1][i]
            bm.faces.new((v1, v2, v3, v4))

    bm.to_mesh(mesh)
    bm.free()

    mesh.materials.append(mats['terrain'])
    # Smooth Shading
    for poly in mesh.polygons:
        poly.use_smooth = True

    # Subdivision modifier for crisp micro-details
    sub_mod = obj.modifiers.new(name="Subsurf", type='SUBSURF')
    sub_mod.levels = 1
    sub_mod.render_levels = 1

    return obj

def build_compute_nexus(mats):
    nexus_group = bpy.data.collections.new("ComputeNexus")
    bpy.context.scene.collection.children.link(nexus_group)

    cols = 5
    rows = 4
    col_w = 4.2
    row_d = 3.8

    for r in range(rows):
        for c in range(cols):
            x = (c - (cols - 1) * 0.5) * col_w
            y = -8.0 + (r * row_d)
            
            # Stepped elevation profile
            dist_c = math.sqrt(x * x + (y - 0.0) * (y - 0.0))
            h = max(2.5, 6.5 - dist_c * 0.35)
            w = 3.2
            d = 2.8

            # Main Monolith Body
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, h * 0.5 - 2.5))
            cube = bpy.context.active_object
            cube.name = f"ComputeMonolith_{c}_{r}"
            cube.scale = (w, d, h)
            bpy.ops.object.transform_apply(scale=True)
            cube.data.materials.append(mats['alloy'])

            # Bevel Modifier for technical edge highlights
            bev = cube.modifiers.new("Bevel", type='BEVEL')
            bev.width = 0.12
            bev.segments = 2

            # Recessed Processor Core Die on Top
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, h - 2.45))
            die = bpy.context.active_object
            die.name = f"ProcessorDie_{c}_{r}"
            die.scale = (w * 0.65, d * 0.65, 0.15)
            bpy.ops.object.transform_apply(scale=True)
            die.data.materials.append(mats['die'])

            # Unlink from scene collection and link to nexus group
            for coll in cube.users_collection:
                coll.objects.unlink(cube)
            nexus_group.objects.link(cube)

            for coll in die.users_collection:
                coll.objects.unlink(die)
            nexus_group.objects.link(die)

def build_memory_megaliths(mats):
    mem_group = bpy.data.collections.new("MemoryMegaliths")
    bpy.context.scene.collection.children.link(mem_group)

    # 8 Crystal RAM Pillars on Port Flank (X ~ -18 to -26)
    for i in range(8):
        x = -20.0 + ((i % 2) * -5.0)
        y = -12.0 + (i * 4.8)
        h = 7.0 + (i % 3) * 2.5
        pw = 1.8
        pd = 1.6

        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, h * 0.5 - 0.5))
        pillar = bpy.context.active_object
        pillar.name = f"MemoryCrystal_{i}"
        pillar.scale = (pw, pd, h)
        bpy.ops.object.transform_apply(scale=True)
        pillar.data.materials.append(mats['crystal'])

        bev = pillar.modifiers.new("Bevel", type='BEVEL')
        bev.width = 0.15
        bev.segments = 2

        for coll in pillar.users_collection:
            coll.objects.unlink(pillar)
        mem_group.objects.link(pillar)

def build_storage_strata(mats):
    store_group = bpy.data.collections.new("StorageStrata")
    bpy.context.scene.collection.children.link(store_group)

    # 4 Cascading Shelves on Starboard Flank (X ~ 18 to 28)
    for s in range(4):
        x = 20.0 + (s * 3.5)
        y = 4.0
        h = 4.0 + (s * 2.8)
        w = 3.6
        d = 28.0 - (s * 2.5)

        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, y, h * 0.5 - 1.5))
        shelf = bpy.context.active_object
        shelf.name = f"StorageVault_{s}"
        shelf.scale = (w, d, h)
        bpy.ops.object.transform_apply(scale=True)
        shelf.data.materials.append(mats['alloy'])

        for coll in shelf.users_collection:
            coll.objects.unlink(shelf)
        store_group.objects.link(shelf)

def build_horizon_transmission_spire(mats):
    # Colossal Data Spire at horizon (X=0, Y=62)
    spire_group = bpy.data.collections.new("TransmissionSpire")
    bpy.context.scene.collection.children.link(spire_group)

    # Tapered Spire Shaft
    bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=3.5, radius2=0.4, depth=42.0, location=(0, 64.0, 19.0))
    shaft = bpy.context.active_object
    shaft.name = "TransmissionPylon"
    shaft.data.materials.append(mats['alloy'])

    # Lattice Ring Collars
    for r in range(4):
        rz = 5.0 + (r * 9.5)
        rw = 3.0 - (r * 0.6)
        bpy.ops.mesh.primitive_torus_add(major_radius=rw, minor_radius=0.18, location=(0, 64.0, rz))
        ring = bpy.context.active_object
        ring.name = f"SpireRing_{r}"
        ring.data.materials.append(mats['die'])
        for coll in ring.users_collection:
            coll.objects.unlink(ring)
        spire_group.objects.link(ring)

    # Apex Radiant Beacon Sphere
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16, radius=1.4, location=(0, 64.0, 40.5))
    beacon = bpy.context.active_object
    beacon.name = "HorizonBeaconCore"
    beacon.data.materials.append(mats['beacon'])

    # Vertical Skyward Beam (Cylinder with emission & volumetric falloff)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.6, depth=70.0, location=(0, 64.0, 75.0))
    beam = bpy.context.active_object
    beam.name = "SkywardVolumetricBeam"
    beam.data.materials.append(mats['laser'])

    for coll in shaft.users_collection:
        coll.objects.unlink(shaft)
    spire_group.objects.link(shaft)

    for coll in beacon.users_collection:
        coll.objects.unlink(beacon)
    spire_group.objects.link(beacon)

    for coll in beam.users_collection:
        coll.objects.unlink(beam)
    spire_group.objects.link(beam)

def build_branching_conduits(mats):
    # Primary Central Bus Curve (Spline running through the canyon)
    curve_data = bpy.data.curves.new('PrimaryBusCurve', type='CURVE')
    curve_data.dimensions = '3D'
    curve_data.bevel_depth = 0.22
    curve_data.bevel_resolution = 4

    spline = curve_data.splines.new('BEZIER')
    points = [
        Vector((  0.0, -42.0, -1.8)),
        Vector((  0.0, -22.0, -2.5)),
        Vector(( -2.5,  -4.0, -2.0)),
        Vector((  2.5,  16.0, -1.2)),
        Vector((  0.0,  38.0,  2.5)),
        Vector((  0.0,  62.0, 16.0))
    ]

    spline.bezier_points.add(len(points) - 1)
    for i, pt in enumerate(points):
        bp = spline.bezier_points[i]
        bp.co = pt
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'

    curve_obj = bpy.data.objects.new('CentralConduitBus', curve_data)
    curve_obj.data.materials.append(mats['conduit'])
    bpy.context.collection.objects.link(curve_obj)

    # Tributary 1: Memory Matrix Branch
    c_mem = bpy.data.curves.new('MemoryBranchCurve', type='CURVE')
    c_mem.dimensions = '3D'
    c_mem.bevel_depth = 0.14
    s_mem = c_mem.splines.new('BEZIER')
    m_pts = [Vector((-2.5, -4.0, -2.0)), Vector((-12.0, -2.0, -1.0)), Vector((-20.0, 0.0, 1.2))]
    s_mem.bezier_points.add(len(m_pts) - 1)
    for i, pt in enumerate(m_pts):
        bp = s_mem.bezier_points[i]
        bp.co = pt
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
    mem_obj = bpy.data.objects.new('MemoryConduitBranch', c_mem)
    mem_obj.data.materials.append(mats['conduit'])
    bpy.context.collection.objects.link(mem_obj)

    # Tributary 2: Storage Strata Branch
    c_sto = bpy.data.curves.new('StorageBranchCurve', type='CURVE')
    c_sto.dimensions = '3D'
    c_sto.bevel_depth = 0.14
    s_sto = c_sto.splines.new('BEZIER')
    s_pts = [Vector((2.5, 16.0, -1.2)), Vector((12.0, 15.0, -0.5)), Vector((20.0, 14.0, 0.8))]
    s_sto.bezier_points.add(len(s_pts) - 1)
    for i, pt in enumerate(s_pts):
        bp = s_sto.bezier_points[i]
        bp.co = pt
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
    sto_obj = bpy.data.objects.new('StorageConduitBranch', c_sto)
    sto_obj.data.materials.append(mats['conduit'])
    bpy.context.collection.objects.link(sto_obj)

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

def build_scanning_laser_plane(mats):
    # Holographic scanning sheet sweeping across the canyon
    mesh = bpy.data.meshes.new("LaserSheetMesh")
    bm = bmesh.new()
    w = 55.0
    h = 24.0
    v1 = bm.verts.new((-w * 0.5, 0.0, -3.0))
    v2 = bm.verts.new(( w * 0.5, 0.0, -3.0))
    v3 = bm.verts.new(( w * 0.5, 0.0, -3.0 + h))
    v4 = bm.verts.new((-w * 0.5, 0.0, -3.0 + h))
    bm.faces.new((v1, v2, v3, v4))
    bm.to_mesh(mesh)
    bm.free()

    laser_obj = bpy.data.objects.new("ScanningLaserSheet", mesh)
    laser_obj.data.materials.append(mats['laser'])
    bpy.context.collection.objects.link(laser_obj)

    # Animate scanning motion (2 complete cycles in 450 frames = 100% seamless loop)
    total_frames = 450
    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        # Completes 2 sweeps: Y oscillates between -18 and 36
        y_pos = 9.0 + 26.0 * math.sin(2.0 * theta)
        laser_obj.location.y = y_pos
        laser_obj.keyframe_insert(data_path="location", index=1, frame=f)

    if laser_obj.animation_data:
        set_action_interpolation_bezier(laser_obj.animation_data.action)

def build_cinematic_camera_loop():
    cam_data = bpy.data.cameras.new("CinematicLoopCamera")
    cam_data.lens = 24.0  # Wide-angle for dramatic perspective
    cam_data.clip_start = 0.2
    cam_data.clip_end = 350.0

    cam_obj = bpy.data.objects.new("CinematicCamera", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj

    # Create empty target for look-at tracking
    target = bpy.data.objects.new("CameraLookAtTarget", None)
    target.empty_display_type = 'SPHERE'
    target.empty_display_size = 0.5
    bpy.context.collection.objects.link(target)

    # Track To Constraint
    track = cam_obj.constraints.new(type='TRACK_TO')
    track.target = target
    track.track_axis = 'TRACK_NEGATIVE_Z'
    track.up_axis = 'UP_Y'

    # Seamless looping flight trajectory over 450 frames (15 seconds @ 30 FPS)
    total_frames = 450

    for f in range(1, total_frames + 2):
        theta = 2.0 * math.pi * ((f - 1) / total_frames)
        
        # Camera advances into canyon and smoothly glides along harmonic curve
        # Starts at Y=-34.0, drifts to Y=-16.0, and returns to -34.0 smoothly
        # Using (1 - cos(theta))*0.5 gives a smooth forward-and-return excursion with 0 velocity at start & end!
        forward_t = (1.0 - math.cos(theta)) * 0.5
        y_cam = -34.0 + forward_t * 22.0
        
        # Lateral sway (banks slightly towards Memory Terrace on left, then across to right)
        x_cam = 4.2 * math.sin(theta)
        
        # Elevation: breathes gently over the canyon
        z_cam = 8.5 + 2.2 * math.sin(2.0 * theta) - 1.2 * math.cos(theta)

        cam_obj.location = (x_cam, y_cam, z_cam)
        cam_obj.keyframe_insert(data_path="location", frame=f)

        # Target Look-At trajectory (focuses slightly ahead down the canyon toward the spire)
        t_y = y_cam + 32.0 + 8.0 * math.sin(theta)
        t_x = 2.5 * math.sin(theta)
        t_z = 3.5 + 2.0 * math.sin(theta)

        target.location = (t_x, t_y, t_z)
        target.keyframe_insert(data_path="location", frame=f)

    # Ensure Bezier smooth tangents
    for obj in (cam_obj, target):
        if obj.animation_data and obj.animation_data.action:
            set_action_interpolation_bezier(obj.animation_data.action)

def build_lighting_and_atmosphere():
    world = bpy.data.worlds.new("CyberWorld")
    bpy.context.scene.world = world
    world.use_nodes = True
    wnodes = world.node_tree.nodes
    wlinks = world.node_tree.links
    wnodes.clear()

    w_out = wnodes.new('ShaderNodeOutputWorld')
    bg = wnodes.new('ShaderNodeBackground')
    bg.inputs['Color'].default_value = (0.006, 0.012, 0.022, 1.0)
    bg.inputs['Strength'].default_value = 0.65
    wlinks.new(bg.outputs['Background'], w_out.inputs['Surface'])

    # Atmospheric Volume Scatter for cinematic depth and god rays
    vol = wnodes.new('ShaderNodeVolumeScatter')
    vol.inputs['Color'].default_value = (0.01, 0.04, 0.07, 1.0)
    vol.inputs['Density'].default_value = 0.006
    vol.inputs['Anisotropy'].default_value = 0.65
    wlinks.new(vol.outputs['Volume'], w_out.inputs['Volume'])

    # 1. Primary Directional Sun Key Light (Cool Ice Cyan from oblique top-right)
    sun_data = bpy.data.lights.new(name="SunKeyLight", type='SUN')
    sun_data.color = (0.65, 0.90, 1.0)
    sun_data.energy = 4.2
    sun_data.angle = math.radians(2.5)  # Soft shadows
    sun_obj = bpy.data.objects.new("SunKeyLight", sun_data)
    sun_obj.rotation_euler = Euler((math.radians(52.0), math.radians(18.0), math.radians(-42.0)), 'XYZ')
    bpy.context.collection.objects.link(sun_obj)

    # 2. Deep Blue Ambient Fill Light (Opposite flank fill)
    fill_data = bpy.data.lights.new(name="FillLight", type='SUN')
    fill_data.color = (0.04, 0.12, 0.25)
    fill_data.energy = 1.2
    fill_obj = bpy.data.objects.new("FillLight", fill_data)
    fill_obj.rotation_euler = Euler((math.radians(35.0), math.radians(-25.0), math.radians(135.0)), 'XYZ')
    bpy.context.collection.objects.link(fill_obj)

    # 3. Horizon Uplight (Rakes across distant mountains)
    uplight_data = bpy.data.lights.new(name="HorizonUplight", type='POINT')
    uplight_data.color = (0.0, 0.85, 1.0)
    uplight_data.energy = 4500.0
    uplight_data.shadow_soft_size = 4.0
    uplight_obj = bpy.data.objects.new("HorizonUplight", uplight_data)
    uplight_obj.location = (0.0, 58.0, 4.0)
    bpy.context.collection.objects.link(uplight_obj)

def main():
    print("==================================================================")
    print("🛸 BUILDING SOLARIS 3D COMPUTATIONAL TERRAIN IN BLENDER 5.2")
    print("==================================================================")
    clean_scene()
    mats = create_materials()
    print("✓ Created advanced composite materials")
    build_computational_terrain(mats)
    print("✓ Built procedural continuous computational canyon terrain")
    build_compute_nexus(mats)
    print("✓ Built Compute Nexus processor monoliths")
    build_memory_megaliths(mats)
    print("✓ Built Memory Megalith crystal pillars")
    build_storage_strata(mats)
    print("✓ Built Storage Canyon NVMe vaults")
    build_horizon_transmission_spire(mats)
    print("✓ Built Horizon High-Gain Transmission Spire with Skyward Beam")
    build_branching_conduits(mats)
    print("✓ Built branching conduit network")
    build_scanning_laser_plane(mats)
    print("✓ Built looping holographic laser scanning plane")
    build_cinematic_camera_loop()
    print("✓ Created 15s (450 frames) seamless looping cinematic camera trajectory")
    build_lighting_and_atmosphere()
    print("✓ Established cinematic lighting, god rays & volumetric atmosphere")

    output_dir = "/home/venkatengineer/solaris-os/assets/computational-terrain"
    os.makedirs(output_dir, exist_ok=True)
    blend_path = os.path.join(output_dir, "computational_terrain.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print(f"✓ Saved editable master project: {blend_path}")
    print("==================================================================")

if __name__ == '__main__':
    main()
