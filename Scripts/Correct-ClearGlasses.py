"""Clear lenses on a separate material; preserve the imported opaque source."""
import pathlib,json,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections'
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
root='/Game/OneManStudio/Cosmetics/ProductionArt/Corrections';path=root+'/M_ClearRoundGlasses'
receipt=A/'ClearGlasses.json'
if assets.does_asset_exist(path):
    if not receipt.exists():raise RuntimeError('Unowned glasses correction')
    material=assets.load_asset(path)
else:
    material=tools.create_asset('M_ClearRoundGlasses',root,unreal.Material,unreal.MaterialFactoryNew())
    texture=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/Face/Glasses/T_Glasses_Color')
    if not texture:raise RuntimeError('Glasses color missing')
    material.set_editor_properties({'blend_mode':unreal.BlendMode.BLEND_TRANSLUCENT,'two_sided':True})
    t=mel.create_material_expression(material,unreal.MaterialExpressionTextureSample);t.set_editor_property('texture',texture)
    opacity=mel.create_material_expression(material,unreal.MaterialExpressionCustom);opacity.set_editor_properties({'code':'return 1-smoothstep(.04,.12,dot(C.rgb,float3(.2126,.7152,.0722)));','output_type':unreal.CustomMaterialOutputType.CMOT_FLOAT1})
    i=unreal.CustomInput();i.set_editor_property('input_name','C');opacity.set_editor_property('inputs',[i]);mel.connect_material_expressions(t,'RGB',opacity,'C')
    mel.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);mel.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
    rough=mel.create_material_expression(material,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.6);mel.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material);assets.save_loaded_asset(material,False)
    receipt.write_text(json.dumps({'material':path,'original_preserved':True}),encoding='utf8')
catalog=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics');entries=list(catalog.get_editor_property('cosmetics'))
entry=next(e for e in entries if str(e.get_editor_property('id'))=='Face.Glasses');entry.set_editor_property('materials',[material]);catalog.set_editor_property('cosmetics',entries);assets.save_loaded_asset(catalog,False)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name()=='MovieManager':a.set_editor_property('cosmetic_catalog',[entry if str(e.get_editor_property('id'))=='Face.Glasses' else e for e in a.get_editor_property('cosmetic_catalog')])
levels.save_current_level();unreal.log('Clear glasses assigned without changing the facial material')
