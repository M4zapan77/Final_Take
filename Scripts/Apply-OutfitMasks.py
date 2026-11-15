"""Create corrected material variants; original imported textures/materials remain intact."""
import pathlib,json,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections';ROOT='/Game/OneManStudio/Cosmetics/ProductionArt/Corrections/Outfits'
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
receipt_file=A/'OutfitMasks.json';receipt=json.loads(receipt_file.read_text()) if receipt_file.exists() else {'created':[]}
def checkpoint():receipt_file.write_text(json.dumps(receipt,indent=2),encoding='utf8')
def create(name,cls,factory):
    path=ROOT+'/'+name
    if assets.does_asset_exist(path):
        if path not in receipt['created']:raise RuntimeError('Unowned material '+path)
        return assets.load_asset(path)
    a=tools.create_asset(name,ROOT,cls,factory);receipt['created'].append(path);checkpoint();return a
master=create('M_MaskedOutfit',unreal.Material,unreal.MaterialFactoryNew())
if receipt.get('material_version',0)<2:
    unreal.StudioCharacterAssetTools.reset_customization_material(master)
    mel.set_material_usage(master,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    def node(cls,**properties):
        n=mel.create_material_expression(master,cls);n.set_editor_properties(properties);return n
    white=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
    color=node(unreal.MaterialExpressionTextureSampleParameter2D,parameter_name='BaseColorTexture',texture=white)
    default_mask=assets.load_asset(ROOT+'/T_ClothingMask_Robber')
    if not default_mask:raise RuntimeError('Import the six masks before rebuilding the material')
    default_mask.set_editor_properties({'srgb':False,'compression_settings':unreal.TextureCompressionSettings.TC_MASKS});assets.save_loaded_asset(default_mask,False)
    mask=node(unreal.MaterialExpressionTextureSampleParameter2D,parameter_name='ClothingMask',texture=default_mask,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    skin=node(unreal.MaterialExpressionVectorParameter,parameter_name='SkinColor',default_value=unreal.LinearColor.WHITE)
    tint=node(unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor.WHITE)
    mix=node(unreal.MaterialExpressionCustom,code='return lerp(Skin.rgb,Cloth.rgb*Tint.rgb,saturate(Mask.r));',output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for key,value in {'Skin':skin,'Cloth':color,'Tint':tint,'Mask':mask}.items():
        item=unreal.CustomInput();item.set_editor_property('input_name',key);inputs.append(item)
    mix.set_editor_property('inputs',inputs)
    for key,value in {'Skin':skin,'Cloth':color,'Tint':tint,'Mask':mask}.items():assert mel.connect_material_expressions(value,'',mix,key)
    assert mel.connect_material_property(mix,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=node(unreal.MaterialExpressionConstant,r=.75);spec=node(unreal.MaterialExpressionConstant,r=.23)
    mel.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS);mel.connect_material_property(spec,'',unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(master);assets.save_loaded_asset(master,False);receipt['material_ready']=True;receipt['material_version']=2;checkpoint()
source=json.loads((P/'Saved/ProductionArt/ImportReceipt.json').read_text());catalog=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics');entries=list(catalog.get_editor_property('cosmetics'))
mapping={'Robber':'Outfit_02fb7247','Uniform':'Outfit_0629c304','Dress':'Outfit_4f3481c9','Casual':'Outfit_6d76132a','Pajamas':'Outfit_b29c29ff','Suit':'Outfit_b37fff0d'}
for name,key in mapping.items():
    path=ROOT+'/T_ClothingMask_'+name
    if assets.does_asset_exist(path):
        if path not in receipt['created']:raise RuntimeError('Unowned mask '+path)
        texture=assets.load_asset(path)
    else:
        task=unreal.AssetImportTask();task.set_editor_properties({'filename':str(P/'SourceArt/FinalCorrections20261114/OutfitMasks'/('T_ClothingMask_'+name+'.png')),'destination_path':ROOT,'destination_name':'T_ClothingMask_'+name,'automated':True,'save':False,'replace_existing':False,'factory':unreal.TextureFactory()});tools.import_asset_tasks([task]);texture=task.get_objects()[0];receipt['created'].append(path);checkpoint()
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':str(P/'SourceArt/FinalCorrections20261114/OutfitMasks'/('T_ClothingMask_'+name+'.png')),'destination_path':ROOT,'destination_name':'T_ClothingMask_'+name,'automated':True,'save':False,'replace_existing':True,'factory':unreal.TextureFactory()});tools.import_asset_tasks([task]);texture=task.get_objects()[0]
    texture.set_editor_properties({'srgb':False,'compression_settings':unreal.TextureCompressionSettings.TC_MASKS,'max_texture_size':2048});assets.save_loaded_asset(texture,False)
    material=create('MI_Outfit_'+name,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew());mel.set_material_instance_parent(material,master)
    mel.set_material_instance_texture_parameter_value(material,'BaseColorTexture',unreal.load_asset(source[key]['textures']['Color']))
    mel.set_material_instance_texture_parameter_value(material,'ClothingMask',texture);assets.save_loaded_asset(material,False)
    entry=next(e for e in entries if str(e.get_editor_property('id'))=='Body.'+name);entry.set_editor_properties({'clothing_mask':texture,'materials':[material]})
catalog.set_editor_property('cosmetics',entries);assets.save_loaded_asset(catalog,False)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank');by_id={str(e.get_editor_property('id')):e for e in entries}
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='MovieManager':actor.set_editor_property('cosmetic_catalog',[by_id.get(str(e.get_editor_property('id')),e) for e in actor.get_editor_property('cosmetic_catalog')])
levels.save_current_level();receipt['outfits']=list(mapping);checkpoint();unreal.log('Six independent garment/skin masks assigned on the original UV0 and skeleton')
