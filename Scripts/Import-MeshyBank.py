"""Import only the supplied bank into a new asset folder; never saves unrelated packages."""
import json
import pathlib
import unreal

PROJECT = pathlib.Path(unreal.Paths.project_dir()).resolve()
SOURCE = PROJECT / "SourceArt" / "MeshyBank"
AUDIT = PROJECT / "Saved" / "BankIntegration"
ROOT = "/Game/OneManStudio/Environment/Bank"
AUDIT.mkdir(parents=True, exist_ok=True)
tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
assets.make_directory(ROOT)

def import_one(path, name, factory, options=None):
    destination = ROOT + "/" + name
    if assets.does_asset_exist(destination):
        return assets.load_asset(destination)
    task = unreal.AssetImportTask()
    task.set_editor_properties({"filename": str(path), "destination_path": ROOT,
        "destination_name": name, "automated": True, "async_": False,
        "replace_existing": False, "save": True, "factory": factory})
    if options is not None:
        task.set_editor_property("options", options)
    tools.import_asset_tasks([task])
    imported = task.get_objects()
    if not imported:
        raise RuntimeError("Import produced no asset: " + str(path))
    asset = imported[0]
    if asset.get_path_name().split(".")[0] != destination:
        if not tools.rename_assets([unreal.AssetRenameData(asset, ROOT, name)]):
            raise RuntimeError("Could not give imported asset its stable name")
    return asset

options = unreal.FbxImportUI()
options.set_editor_properties({"automated_import_should_detect_type": False,
    "mesh_type_to_import": unreal.FBXImportType.FBXIT_STATIC_MESH,
    "import_as_skeletal": False, "import_mesh": True, "import_animations": False,
    "import_materials": False, "import_textures": False})
mesh_options = options.get_editor_property("static_mesh_import_data")
mesh_options.set_editor_properties({"combine_meshes": True, "auto_generate_collision": False,
    "generate_lightmap_u_vs": False, "convert_scene": True, "convert_scene_unit": True,
    "import_uniform_scale": 100.0,
    "normal_import_method": unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS})
if hasattr(mesh_options, "build_nanite"):
    mesh_options.set_editor_property("build_nanite", True)
mesh = import_one(next(SOURCE.glob("*.fbx")), "SM_MeshyBank", unreal.FbxFactory(), options)
if not isinstance(mesh, unreal.StaticMesh):
    raise RuntimeError("The supplied bank did not import as a static mesh")

textures = {}
for suffix, name, compression, group in [
    ("_texture.png", "T_Bank_BaseColor", unreal.TextureCompressionSettings.TC_DEFAULT, unreal.TextureGroup.TEXTUREGROUP_WORLD),
    ("_normal.png", "T_Bank_Normal", unreal.TextureCompressionSettings.TC_NORMALMAP, unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP),
    ("_metallic.png", "T_Bank_Metallic", unreal.TextureCompressionSettings.TC_MASKS, unreal.TextureGroup.TEXTUREGROUP_WORLD_SPECULAR),
    ("_roughness.png", "T_Bank_Roughness", unreal.TextureCompressionSettings.TC_MASKS, unreal.TextureGroup.TEXTUREGROUP_WORLD_SPECULAR),
]:
    filename = next(path for path in SOURCE.glob("*.png") if path.name.endswith(suffix))
    texture = import_one(filename, name, unreal.TextureFactory())
    texture.set_editor_properties({"srgb": name == "T_Bank_BaseColor", "compression_settings": compression,
        "lod_group": group, "max_texture_size": 4096})
    assets.save_loaded_asset(texture, False)
    textures[name] = texture

material_path = ROOT + "/M_MeshyBank"
if assets.does_asset_exist(material_path):
    material = assets.load_asset(material_path)
else:
    material = tools.create_asset("M_MeshyBank", ROOT, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    for index, (name, output, property_type, sampler) in enumerate([
        ("T_Bank_BaseColor", "RGB", unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
        ("T_Bank_Normal", "RGB", unreal.MaterialProperty.MP_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
        ("T_Bank_Metallic", "R", unreal.MaterialProperty.MP_METALLIC, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS),
        ("T_Bank_Roughness", "R", unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS),
    ]):
        sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -450, index * 200)
        sample.set_editor_properties({"texture": textures[name], "sampler_type": sampler})
        unreal.MaterialEditingLibrary.connect_material_property(sample, output, property_type)
    unreal.MaterialEditingLibrary.recompile_material(material)
    assets.save_loaded_asset(material, False)

nanite_usage = unreal.MaterialUsage.MATUSAGE_NANITE
if not unreal.MaterialEditingLibrary.has_material_usage(material, nanite_usage):
    unreal.MaterialEditingLibrary.set_material_usage(material, nanite_usage)
    unreal.MaterialEditingLibrary.recompile_material(material)
if not unreal.MaterialEditingLibrary.has_material_usage(material, nanite_usage):
    raise RuntimeError("The bank material must support Nanite before cooking")
assets.save_loaded_asset(material, False)

slots = mesh.get_editor_property("static_materials")
for index in range(len(slots)):
    mesh.set_material(index, material)
body = mesh.get_editor_property("body_setup")
body.set_editor_properties({"collision_trace_flag": unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
    "double_sided_geometry": True})
nanite = mesh.get_editor_property("nanite_settings")
nanite.set_editor_properties({"enabled": True,
    "fallback_target": unreal.NaniteFallbackTarget.PERCENT_TRIANGLES,
    "fallback_percent_triangles": 1.0, "fallback_relative_error": 0.0})
mesh.set_editor_property("nanite_settings", nanite)
assets.save_loaded_asset(mesh, False)
bounds = mesh.get_bounding_box()
report = {"mesh": mesh.get_path_name(), "material": material.get_path_name(),
    "textures": {key: value.get_path_name() for key, value in textures.items()},
    "bounds_min": [bounds.min.x, bounds.min.y, bounds.min.z],
    "bounds_max": [bounds.max.x, bounds.max.y, bounds.max.z],
    "material_slots": len(slots), "nanite": True, "complex_collision": True,
    "triangle_count": mesh.get_num_triangles(0), "runtime_max_texture_size": 4096,
    "material_used_with_nanite": unreal.MaterialEditingLibrary.has_material_usage(material, nanite_usage),
    "mesh_methods": [n for n in dir(mesh) if any(k in n for k in ("triangle", "vertex", "description"))]}
AUDIT.joinpath("ImportResult.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("BankImport: saved new bank assets and PBR material; existing maps and art unchanged")
