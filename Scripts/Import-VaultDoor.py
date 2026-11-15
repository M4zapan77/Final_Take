"""Integrate the supplied door on the existing decorative actor; gameplay actors remain unchanged."""
import pathlib,json,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections';SRC=P/'SourceArt/FinalCorrections20261114/Vault';ROOT='/Game/OneManStudio/Environment/Bank/ProductionArt/VaultDoor'
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
receipt_file=A/'VaultDoor.json';receipt=json.loads(receipt_file.read_text()) if receipt_file.exists() else {'created':[]}
def save_receipt():receipt_file.write_text(json.dumps(receipt,indent=2),encoding='utf8')
def import_asset(file,name,factory,options=None):
    path=ROOT+'/'+name
    if assets.does_asset_exist(path):
        if path not in receipt['created']:raise RuntimeError('Unowned vault asset '+path)
        return assets.load_asset(path)
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':str(file),'destination_path':ROOT,'destination_name':name,'factory':factory,'automated':True,'replace_existing':False,'save':False})
    if options:task.set_editor_property('options',options)
    tools.import_asset_tasks([task]);objects=list(task.get_objects())
    if not objects:raise RuntimeError('Vault import failed '+str(file))
    result=objects[0];receipt['created'].append(path);save_receipt();return result
color=import_asset(SRC/'bank_vault_interior_3d_model.fbm/bank_vault_interior_3d_model_basecolor.jpg','T_VaultDoor_Color',unreal.TextureFactory())
normal=import_asset(SRC/'bank_vault_interior_3d_model.fbm/bank_vault_interior_3d_model_normal.png','T_VaultDoor_Normal',unreal.TextureFactory())
color.set_editor_properties({'srgb':True,'max_texture_size':2048});normal.set_editor_properties({'srgb':False,'max_texture_size':2048,'compression_settings':unreal.TextureCompressionSettings.TC_NORMALMAP})
for a in (color,normal):assets.save_loaded_asset(a,False)
material_path=ROOT+'/M_VaultDoor'
if assets.does_asset_exist(material_path):
    if material_path not in receipt['created']:raise RuntimeError('Unowned vault material')
    material=assets.load_asset(material_path)
else:
    material=tools.create_asset('M_VaultDoor',ROOT,unreal.Material,unreal.MaterialFactoryNew());receipt['created'].append(material_path);save_receipt()
    t=mel.create_material_expression(material,unreal.MaterialExpressionTextureSample);t.set_editor_property('texture',color);mel.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    n=mel.create_material_expression(material,unreal.MaterialExpressionTextureSample);n.set_editor_properties({'texture':normal,'sampler_type':unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL});mel.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_NORMAL)
    for value,prop in ((.68,unreal.MaterialProperty.MP_ROUGHNESS),(.35,unreal.MaterialProperty.MP_METALLIC),(.25,unreal.MaterialProperty.MP_SPECULAR)):
        node=mel.create_material_expression(material,unreal.MaterialExpressionConstant);node.set_editor_property('r',value);mel.connect_material_property(node,'',prop)
    mel.recompile_material(material);assets.save_loaded_asset(material,False)
options=unreal.FbxImportUI();options.set_editor_properties({'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH,'import_as_skeletal':False,'import_mesh':True,'import_animations':False,'import_materials':False,'import_textures':False})
options.static_mesh_import_data.set_editor_properties({'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,'convert_scene':True,'convert_scene_unit':True,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS})
mesh=import_asset(SRC/'bank_vault_interior_3d_model.fbx','SM_VaultDoor',unreal.FbxFactory(),options)
if not receipt.get('normalized'):
    receipt['source_triangles']=mesh.get_num_triangles(0)
    if not unreal.StudioCharacterAssetTools.normalize_art_mesh(mesh,100,60000):raise RuntimeError('Vault mesh normalization failed')
    receipt['normalized']=True;save_receipt()
for i in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(i,material)
unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).remove_collisions(mesh);assets.save_loaded_asset(mesh,False)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors();by_label={a.get_actor_label():a for a in actors if '/Lvl_Bank.' in a.get_path_name()}
door=by_label['Vault door open steel leaf'];receipt.setdefault('original_actor',door.get_path_name());receipt.setdefault('original_mesh',door.static_mesh_component.static_mesh.get_path_name())
door.static_mesh_component.set_static_mesh(mesh);door.static_mesh_component.set_material(0,material);door.static_mesh_component.set_collision_profile_name('NoCollision')
door.set_actor_rotation(unreal.Rotator(yaw=145),False);door.set_actor_scale3d(unreal.Vector(3.3,3.3,3.3));door.set_actor_location(unreal.Vector(665,-280,190),False,False)
shell=by_label['BANK - Meshy Diorama'].static_mesh_component
floor=unreal.StudioAssetCorrectionTools.trace_surface(shell,unreal.Vector(665,-280,180),unreal.Vector(665,-280,-100)).z
if floor < -1000:raise RuntimeError('Vault support not found')
center,extent=door.get_actor_bounds(False,False);p=door.get_actor_location();door.set_actor_location(unreal.Vector(p.x,p.y,p.z+floor+.5-(center.z-extent.z)),False,False)
# The imported model includes its own handle; retain the old placeholder actor for rollback.
handle=by_label['Vault golden handle'];handle.set_actor_hidden_in_game(True);handle.static_mesh_component.set_visibility(False,True)
if not levels.save_current_level():raise RuntimeError('Vault placement save failed')
receipt.update({'runtime_triangles':mesh.get_num_triangles(0),'mesh':mesh.get_path_name(),'transform':str(door.get_actor_transform()),'floor':floor,'supplied_model':'Solid vault door panel; no walkable interior in this archive','retained_gameplay':True});save_receipt()
unreal.log('Supplied vault door integrated on original set actor')
