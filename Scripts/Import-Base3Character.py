"""Import only the character explicitly supplied by the director; keep the live character and maps intact."""
import hashlib
import json
import pathlib
import unreal

PROJECT = pathlib.Path(unreal.Paths.project_dir()).resolve()
SOURCE = PROJECT / "SourceArt" / "Base3" / "base (3).fbx"
ROOT = "/Game/OneManStudio/Characters/Base3"
AUDIT = PROJECT / "Saved" / "CharacterImport"
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
AUDIT.mkdir(parents=True, exist_ok=True)
if assets.does_directory_exist(ROOT) and assets.list_assets(ROOT, recursive=True, include_folder=False):
    raise RuntimeError("The Base3 asset folder is not empty; refusing to overwrite imported or artist-edited assets")
assets.make_directory(ROOT)
options = unreal.FbxImportUI()
options.set_editor_properties({"automated_import_should_detect_type": False,
    "mesh_type_to_import": unreal.FBXImportType.FBXIT_SKELETAL_MESH, "import_as_skeletal": True,
    "import_mesh": True, "import_animations": False, "import_materials": True, "import_textures": True,
    "create_physics_asset": True})
data = options.get_editor_property("skeletal_mesh_import_data")
data.set_editor_properties({"convert_scene": True, "convert_scene_unit": True, "import_uniform_scale": 1.0,
    "normal_import_method": unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS})
task = unreal.AssetImportTask()
task.set_editor_properties({"filename": str(SOURCE), "destination_path": ROOT, "destination_name": "SK_Base3",
    "automated": True, "replace_existing": False, "save": True, "factory": unreal.FbxFactory(), "options": options})
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
meshes = [item for item in task.get_objects() if isinstance(item, unreal.SkeletalMesh)]
if len(meshes) != 1:
    raise RuntimeError("Expected exactly one skinned character mesh; inspect the import log")
mesh = meshes[0]
component = unreal.new_object(unreal.SkeletalMeshComponent)
component.set_skeletal_mesh_asset(mesh)
bones = [str(component.get_bone_name(index)) for index in range(component.get_num_bones())]
skeleton = mesh.get_editor_property("skeleton")
current_class = unreal.load_class(None, "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")
current_character = unreal.get_default_object(current_class)
current_component = current_character.get_component_by_class(unreal.SkeletalMeshComponent)
current_mesh = current_component.get_skeletal_mesh_asset()
report = {"source": str(SOURCE), "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
    "mesh": mesh.get_path_name(), "skeleton": skeleton.get_path_name() if skeleton else None,
    "bones": bones, "bone_count": len(bones),
    "current_player_mesh": current_mesh.get_path_name() if current_mesh else None,
    "shares_current_player_skeleton": current_mesh is not None and skeleton == current_mesh.get_editor_property("skeleton"),
    "assets": []}
for path in assets.list_assets(ROOT, recursive=True, include_folder=False):
    asset = assets.load_asset(path)
    assets.save_loaded_asset(asset, False)
    report["assets"].append({"path": asset.get_path_name(), "class": asset.get_class().get_name()})
try:
    bounds = mesh.get_editor_property("imported_bounds")
    extent = bounds.get_editor_property("box_extent")
    report["size_cm"] = [extent.x*2, extent.y*2, extent.z*2]
except Exception as error:
    report["bounds_note"] = str(error)
AUDIT.joinpath("Base3Import.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("Base3Import: imported new skeletal mesh, skeleton, materials and physics asset; live player and maps unchanged")
