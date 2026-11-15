"""Build the authorized bank level. Rebuild only our generated actors after a file-hash check."""
import hashlib
import json
import math
import pathlib
import unreal

PROJECT = pathlib.Path(unreal.Paths.project_dir()).resolve()
AUDIT = PROJECT / "Saved" / "BankIntegration"
ROOT = "/Game/OneManStudio/Environment/Bank"
MAP = "/Game/OneManStudio/Maps/Lvl_Bank"
MAP_FILE = PROJECT / "Content" / "OneManStudio" / "Maps" / "Lvl_Bank.umap"
RECEIPT = AUDIT / "LevelReceipt.json"
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
if not levels or not actors:
    raise RuntimeError("Required level/actor editor subsystems are unavailable")

def file_hash(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

if assets.does_asset_exist(MAP):
    if not RECEIPT.exists() or json.loads(RECEIPT.read_text())["hash"] != file_hash(MAP_FILE):
        raise RuntimeError("Bank level changed outside this script; refusing to overwrite it")
    if not levels.load_level(MAP):
        raise RuntimeError("Could not load our existing bank level")
    for actor in actors.get_all_level_actors():
        if actor.actor_has_tag("BankSetup"):
            actors.destroy_actor(actor)
else:
    assets.make_directory("/Game/OneManStudio/Maps")
    if not levels.new_level(MAP, False):
        raise RuntimeError("Could not create the new bank level")
    RECEIPT.write_text(json.dumps({"map": MAP, "hash": file_hash(MAP_FILE)}))

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_properties({"default_game_mode": unreal.load_class(None, "/Script/MyProject.StudioGameMode"),
    "force_no_precomputed_lighting": True})

def spawn(cls, label, location, rotation=None):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), rotation or unreal.Rotator())
    if not actor:
        raise RuntimeError("Failed to spawn " + label)
    actor.set_actor_label(label)
    actor.set_editor_property("tags", ["BankSetup"])
    return actor

mesh = assets.load_asset(ROOT + "/SM_MeshyBank")
box = mesh.get_bounding_box()
scale = 1800.0 / max(box.max.x - box.min.x, box.max.y - box.min.y)
center_x = (box.min.x + box.max.x) * .5
center_y = (box.min.y + box.max.y) * .5
bank = spawn(unreal.StaticMeshActor, "BANK - Meshy Diorama", (center_y * scale, -center_x * scale, -box.min.z * scale), unreal.Rotator(yaw=90))
bank.set_editor_property("tags", ["BankSetup", "BankShell"])
bank.static_mesh_component.set_static_mesh(mesh)
bank.set_actor_scale3d(unreal.Vector(scale, scale, scale))
width_x = (box.max.y - box.min.y) * scale
width_y = (box.max.x - box.min.x) * scale
height = (box.max.z - box.min.z) * scale

ground_material = assets.load_asset(ROOT + "/MI_BankGround") if assets.does_asset_exist(ROOT + "/MI_BankGround") else None
if not ground_material:
    ground_material = tools.create_asset("MI_BankGround", ROOT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(ground_material, assets.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(ground_material, "Color", unreal.LinearColor(.20, .19, .17, 1))
    assets.save_loaded_asset(ground_material, False)
ground = spawn(unreal.StaticMeshActor, "Bank exterior ground", (0, 0, -15))
ground.static_mesh_component.set_static_mesh(assets.load_asset("/Engine/BasicShapes/Cube"))
ground.static_mesh_component.set_material(0, ground_material)
ground.set_actor_scale3d(unreal.Vector(100, 100, .3))

hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(0,0,height+500), unreal.Vector(0,0,-100),
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE)
if not hit:
    raise RuntimeError("Could not find a collision floor for the bank")
floor_z = hit.to_tuple()[5].z
if floor_z > 5:
    ramp_length = 350.0
    for side in [-1,1]:
        ramp = spawn(unreal.StaticMeshActor, "Bank access ramp", (side*(width_x*.5+125),0,floor_z*.5-5),
            unreal.Rotator(pitch=-side*math.degrees(math.atan2(floor_z,ramp_length))))
        ramp.static_mesh_component.set_static_mesh(assets.load_asset("/Engine/BasicShapes/Cube"))
        ramp.static_mesh_component.set_material(0,ground_material)
        ramp.set_actor_scale3d(unreal.Vector(ramp_length/100,2.4,.1))

counter_material = assets.load_asset(ROOT + "/MI_BankCounter") if assets.does_asset_exist(ROOT + "/MI_BankCounter") else None
if not counter_material:
    counter_material = tools.create_asset("MI_BankCounter", ROOT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(counter_material, assets.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(counter_material, "Color", unreal.LinearColor(.08,.13,.24,1))
    assets.save_loaded_asset(counter_material, False)
for label, z, cube_scale in [("Bank counter",50,(.8,4.8,1)),("Counter top",105,(1,5.2,.1))]:
    counter = spawn(unreal.StaticMeshActor, label, (200,0,floor_z+z))
    counter.static_mesh_component.set_static_mesh(assets.load_asset("/Engine/BasicShapes/Cube"))
    counter.static_mesh_component.set_material(0,counter_material)
    counter.set_actor_scale3d(unreal.Vector(*cube_scale))

def zone(label, tag, point, extent):
    actor = spawn(unreal.load_class(None,"/Script/MyProject.ObjectiveZone"),label,point)
    actor.get_editor_property("bounds").set_box_extent(unreal.Vector(*extent),False)
    actor.set_editor_properties({"zone_tag":tag,"show_floor_marker":False})
    actor.get_editor_property("floor_marker").set_visibility(False)
    return actor

zone("Bank entry objective", "Bank.Entry", (-450,0,floor_z+110), (110,260,180))
zone("Bank counter objective", "Bank.Counter", (40,150,floor_z+110), (110,200,180))
zone("Bank escape objective", "Bank.Escape", (1180,0,110), (180,240,160))
for class_name,label,pickup_id,y,z,prop_scale in [
    ("MoneyBagProp","Money bag pickup","Bank.MoneyBag.01",0,130,(.30,.24,.38)),
    ("DonutProp","Donut pickup","Bank.Donut.01",160,114,(.20,.20,.06)),
    ("BananaProp","Banana pickup","Bank.Banana.01",-160,126,(.08,.10,.32)),
]:
    prop = spawn(unreal.load_class(None,"/Script/MyProject."+class_name),label,(200,y,floor_z+z))
    prop.set_editor_property("pickup_id",pickup_id)
    prop.set_actor_scale3d(unreal.Vector(*prop_scale))

sun = spawn(unreal.DirectionalLight, "Bank daylight", (0, 0, 2500), unreal.Rotator(pitch=-42, yaw=-32))
sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun_component.set_editor_properties({"intensity": 75000.0, "atmosphere_sun_light": True,
    "forward_shading_priority": 1})
fill = spawn(unreal.DirectionalLight, "Soft interior fill", (0, 0, 1800), unreal.Rotator(pitch=-35, yaw=180))
fill_component = fill.get_component_by_class(unreal.DirectionalLightComponent)
fill_component.set_mobility(unreal.ComponentMobility.MOVABLE)
fill_component.set_editor_properties({"intensity": 8000.0, "cast_shadows": False,
    "atmosphere_sun_light": False})
spawn(unreal.SkyAtmosphere, "Bank sky", (0, 0, 0))
sky = spawn(unreal.SkyLight, "Bank ambient sky", (0, 0, 1500))
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_component.set_editor_properties({"real_time_capture": True, "intensity": 1.0})
post_volume = spawn(unreal.PostProcessVolume, "Bank exposure and clear motion", (0,0,0))
post_volume.set_editor_property("unbound",True)
post_settings = post_volume.get_editor_property("settings")
post_settings.set_editor_properties({"override_auto_exposure_min_brightness":True,
    "override_auto_exposure_max_brightness":True,"auto_exposure_min_brightness":15.0,
    "auto_exposure_max_brightness":15.0,"override_auto_exposure_bias":True,"auto_exposure_bias":0.0,
    "override_motion_blur_amount":True,"motion_blur_amount":0.0})
post_volume.set_editor_property("settings",post_settings)

start_x = -width_x * .5 + 250
start = spawn(unreal.PlayerStart, "Movie player start", (start_x, 0, floor_z+110), unreal.Rotator(yaw=0))
camera_location = unreal.Vector(100, -500, 950)
camera_target = unreal.Vector(100, 50, 120)
camera = spawn(unreal.CameraActor, "Movie premiere camera", (camera_location.x, camera_location.y, camera_location.z),
    unreal.MathLibrary.find_look_at_rotation(camera_location, camera_target))
camera.get_component_by_class(unreal.CameraComponent).set_field_of_view(100.0)
manager = spawn(unreal.load_class(None, "/Script/MyProject.MovieManager"), "ONE MAN STUDIO - Movie Manager", (start_x, 0, 0))
manager.set_editor_properties({"premiere_camera": camera, "auto_start": True, "show_debug_overlay": False})
role_definitions = manager.get_editor_property("roles")
instructions = ["Step into the lobby, take the money, dance, laugh and escape through the rear door.",
    "Enter the bank, approach the recorded robber and stage an arrest. Use a donut and your siren.",
    "Visit the counter, react to the robber, approach the officer and dance through the chaos."]
for index in range(len(role_definitions)):
    role_definition = role_definitions[index]
    role_definition.set_editor_property("initial_transform",unreal.Transform(location=unreal.Vector(0,[0,120,-120][index],0)))
    role_definition.set_editor_property("script_instruction",instructions[index])
    role_definitions[index] = role_definition
manager.set_editor_property("roles", role_definitions)
sign = spawn(unreal.TextRenderActor,"Bank sign",(-width_x*.5-15,0,height*.82),unreal.Rotator(yaw=180))
sign_text = sign.get_component_by_class(unreal.TextRenderComponent)
sign_text.set_text("ONE MAN BANK")
sign_text.set_world_size(52)
sign_text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
sign_text.set_text_render_color(unreal.Color(245,200,65,255))

if not levels.save_current_level():
    raise RuntimeError("Could not save the generated bank level")
receipt = {"map": MAP, "hash": file_hash(MAP_FILE), "scale": scale,
    "size": [width_x, width_y, height], "player_start": [start_x, 0, floor_z+110], "floor_z":floor_z,
    "camera": [camera_location.x, camera_location.y, camera_location.z],
    "stage": "playable", "triangle_count": mesh.get_num_triangles(0)}
RECEIPT.write_text(json.dumps(receipt, indent=2), encoding="utf-8")
unreal.log("BankLevel: saved generated playable level; default maps and previous level unchanged")
