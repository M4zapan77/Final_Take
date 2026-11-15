"""Small saved camera/material refinements after rendered gameplay review."""
import pathlib,json,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank');camera=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='Movie premiere camera')
location=unreal.Vector(250,1140,850);target=unreal.Vector(250,-70,80)
camera.set_actor_location(location,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False)
component=camera.get_component_by_class(unreal.CameraComponent);component.set_relative_transform(unreal.Transform(),False,False);component.set_field_of_view(90)
levels.save_current_level();(A/'FilmFraming.json').write_text(json.dumps({'camera':camera.get_path_name(),'location':[250,1140,850],'look_at':[250,-70,80],'fov':90,'component_transform':str(component.get_relative_transform())},indent=2),encoding='utf8')
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
root='/Game/OneManStudio/Environment/Bank/ProductionArt/Corrections';receipt_file=A/'PresentationMaterials.json'
receipt=json.loads(receipt_file.read_text()) if receipt_file.exists() else {'created':[]}
def material(name):
    folder=root.replace('/Bank/','/Cinema/') if name=='M_WarmCinemaRed' else root
    path=folder+'/'+name
    if assets.does_asset_exist(path):
        if path not in receipt['created']:raise RuntimeError('Unowned presentation material '+path)
        return assets.load_asset(path),False
    m=tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew());receipt['created'].append(path);receipt_file.write_text(json.dumps(receipt,indent=2));return m,True
def finish(m):
    for value,prop in ((.82,unreal.MaterialProperty.MP_ROUGHNESS),(.22,unreal.MaterialProperty.MP_SPECULAR)):
        n=mel.create_material_expression(m,unreal.MaterialExpressionConstant);n.set_editor_property('r',value);mel.connect_material_property(n,'',prop)
    mel.recompile_material(m);assets.save_loaded_asset(m,False)
teal,new=material('M_TealWaitingBench')
if new:
    source=assets.load_asset('/Game/OneManStudio/Environment/Bank/ProductionArt/Sillon_Azul/MI_Sillon_Azul')
    tex=mel.get_material_instance_texture_parameter_value(source,'BaseColorTexture')
    t=mel.create_material_expression(teal,unreal.MaterialExpressionTextureSample);t.set_editor_property('texture',tex)
    c=mel.create_material_expression(teal,unreal.MaterialExpressionCustom);c.set_editor_properties({'code':'float b=saturate((T.b-max(T.r,T.g))*8);float3 mint=float3(max(T.r*.8,.04),max(T.g*1.7,.28),max(T.b*.8,.23));return lerp(T.rgb,mint,b);','output_type':unreal.CustomMaterialOutputType.CMOT_FLOAT3})
    i=unreal.CustomInput();i.set_editor_property('input_name','T');c.set_editor_property('inputs',[i]);mel.connect_material_expressions(t,'RGB',c,'T');mel.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR);finish(teal)
bench=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='Front-right waiting bench');bench.static_mesh_component.set_material(0,teal)
levels.save_current_level()
red,new=material('M_WarmCinemaRed')
if new or not mel.get_material_property_input_node(red,unreal.MaterialProperty.MP_BASE_COLOR):
    c=mel.create_material_expression(red,unreal.MaterialExpressionVectorParameter);c.set_editor_properties({'parameter_name':'Color','default_value':unreal.LinearColor(.3,.004,.009,1)})
    assert mel.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    finish(red)
assert mel.get_material_property_input_node(red,unreal.MaterialProperty.MP_BASE_COLOR)
levels.load_level('/Game/OneManStudio/Maps/Lvl_Cinema')
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith('Red velvet curtain') or a.get_actor_label() in ('Cinema screen wall','Cinema left wall','Cinema right wall','Cinema rear wall'):
        a.static_mesh_component.set_material(0,red)
    if a.get_actor_label().startswith('Warm cinema light'):
        c=a.get_component_by_class(unreal.PointLightComponent);c.set_editor_properties({'light_color':unreal.Color(r=255,g=203,b=152,a=255),'intensity':9000.0})
levels.save_current_level();receipt_file.write_text(json.dumps(receipt,indent=2))
unreal.log('Same film camera reframed with a margin around the complete objective route')
