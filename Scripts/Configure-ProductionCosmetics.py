"""Bind supplied art to stable wardrobe IDs; keep the existing body/skeleton/UVs."""
import json,pathlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/ProductionArt'
receipt=json.loads((A/'ImportReceipt.json').read_text(encoding='utf8'))
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools()
ROOT='/Game/OneManStudio/Cosmetics/ProductionArt';PATH=ROOT+'/DA_ProductionCosmetics'
profile=unreal.load_asset('/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character')
body=profile.get_editor_property('body_mesh');mesh_transform=profile.get_editor_property('mesh_transform')
# A small, editable surface separation prevents the rigid facial overlay from
# flickering against the skinned cheek triangles during the idle animation.
profile_file=P/'Content/OneManStudio/Characters/Base3/Customization/DA_Base3Character.uasset'
backup=A/'Backups/DA_Base3Character.uasset'
if not backup.exists():
    import shutil
    shutil.copy2(profile_file,backup)
face=profile.get_editor_property('face');face_transform=face.get_editor_property('relative_transform')
face_transform.set_editor_property('scale3d',unreal.Vector(1.015,1.015,1.015));face.set_editor_property('relative_transform',face_transform)
profile.set_editor_property('face',face);assets.save_loaded_asset(profile,False)
entries=[];report=[]

def icon(key,folder):
    path=folder+'/T_'+key+'_Icon'
    if assets.does_asset_exist(path):return assets.load_asset(path)
    image=A/'Thumbnails'/(key+'.png')
    if not image.exists():return None
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':str(image),'destination_path':folder,'destination_name':'T_'+key+'_Icon','automated':True,'save':False,'replace_existing':False,'factory':unreal.TextureFactory()})
    tools.import_asset_tasks([task]);objects=task.get_objects()
    if not objects:raise RuntimeError('Icon failed: '+key)
    texture=objects[0];texture.set_editor_properties({'compression_settings':unreal.TextureCompressionSettings.TC_EDITOR_ICON,'lod_group':unreal.TextureGroup.TEXTUREGROUP_UI,'srgb':True,'never_stream':True,'mip_gen_settings':unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS})
    assets.save_loaded_asset(texture,False);receipt.setdefault('_created',[]).append(path);return texture

def entry(key,id,label,category):
    r=receipt[key]
    if not r.get('complete'):raise RuntimeError('Incomplete art: '+key)
    e=unreal.CosmeticDefinition();e.set_editor_properties({'id':id,'display_name':label,'category':category,'materials':[unreal.load_asset(r['material'])],'icon':icon(key,r['folder']),'use_tint':False,'tint':unreal.LinearColor.WHITE})
    if r['mesh']:e.set_editor_property('static_mesh',unreal.load_asset(r['mesh']))
    entries.append(e);report.append({'id':id,'label':label,'source_key':key,'mesh':r['mesh'],'material':r['material']})
    return e

def transform(point=(0,0,0),scale=1,yaw=-90):
    return unreal.Transform(location=unreal.Vector(*point),rotation=unreal.Rotator(yaw=yaw),scale=unreal.Vector(scale,scale,scale))

def head_fit(e,desired):
    # Author against the reference pose. Fitting against an already animated
    # idle pose would bury face accessories and push hats behind the head.
    parent=unreal.StudioCharacterAssetTools.reference_bone_transform(body,'Head',mesh_transform)
    e.set_editor_properties({'socket_name':'Head','fit_in_actor_space':False,'actor_relative_transform':desired,
                            'relative_transform':unreal.MathLibrary.make_relative_transform(desired,parent),
                            'socket_scale_resolved':True})

# Bounds are centered, normalized in centimeters. +X is the actor's forward axis.
for key,label,width,bottom in [('PoliceCap','Gorra de policia',68,78),('Crown','Corona rosa',64,78),('Chicken','Gorro de pollo',73,63),('Beanie','Gorro tejido',67,66),('Cowboy','Sombrero vaquero',92,76)]:
    e=entry(key,'Head.'+key,label,unreal.CosmeticCategory.HEAD);r=receipt[key]
    scale=width/(r['bounds_max'][0]-r['bounds_min'][0]);height=(r['bounds_max'][2]-r['bounds_min'][2])*scale
    head_fit(e,transform((0,0,bottom+height*.5),scale))
    if key=='Chicken':e.set_editor_property('cosmetic_tags',['Costume.AnimalHead'])

for key,id,label,width,z in [('RobberMask','Head.RobberMask','Antifaz de ladron',63,54),('Glasses','Face.Glasses','Lentes redondos',61,58),('Mustache','Face.Mustache','Bigote',34,41),('Sunglasses','Face.Sunglasses','Lentes oscuros',64,58)]:
    e=entry(key,id,label,unreal.CosmeticCategory.HEAD if id.startswith('Head.') else unreal.CosmeticCategory.FACE);r=receipt[key]
    scale=width/(r['bounds_max'][0]-r['bounds_min'][0]);depth=(r['bounds_max'][1]-r['bounds_min'][1])*scale
    head_fit(e,transform((35-depth*.5,0,z),scale,yaw=90 if key=='RobberMask' else -90))

for key,id,label in [('Outfit_02fb7247','Body.Robber','Rayas de ladron'),('Outfit_0629c304','Body.Uniform','Uniforme'),('Outfit_4f3481c9','Body.Dress','Vestido rosa'),('Outfit_6d76132a','Body.Casual','Camisa casual'),('Outfit_b29c29ff','Body.Pajamas','Pijama azul'),('Outfit_b37fff0d','Body.Suit','Traje elegante')]:
    e=entry(key,id,label,unreal.CosmeticCategory.BODY)
    e.set_editor_properties({'clothing_style':2,'tint_parameter':'Color'})
    if id=='Body.Uniform':e.set_editor_property('cosmetic_tags',['Costume.OfficialUniform'])

for key,id,label,height,z in [('Banana','Prop.Banana','Platano',30,0),('HandheldMicrophone','Prop.Microphone','Microfono',32,9),('Donut','Prop.Donut','Dona',22,2),('MoneyBag','Prop.MoneyBag','Bolsa de dinero',39,-13),('RubberDuck','Prop.RubberDuck','Patito de hule',23,3),('Megaphone','Prop.Megaphone','Megafono',35,7)]:
    e=entry(key,id,label,unreal.CosmeticCategory.PROP);r=receipt[key]
    extent=[r['bounds_max'][i]-r['bounds_min'][i] for i in range(3)]
    scale=height/max(extent)
    fits=[]
    for bone in ('RightHand','LeftHand'):
        parent=unreal.StudioCharacterAssetTools.reference_bone_transform(body,bone,mesh_transform)
        desired=transform((parent.translation.x+7,parent.translation.y,parent.translation.z+z),scale)
        fits.append(unreal.MathLibrary.make_relative_transform(desired,parent))
    e.set_editor_properties({'socket_name':'RightHand','relative_transform':fits[0],'use_left_hand_transform':True,'left_hand_relative_transform':fits[1],'socket_scale_resolved':True,'actor_relative_transform':transform((30,35,-10),scale)})

if assets.does_asset_exist(PATH):
    if not (A/'CosmeticCatalog.json').exists():raise RuntimeError('Refusing to replace an unowned catalog')
    catalog=assets.load_asset(PATH)
else:
    factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.ProductionArtCatalog)
    catalog=tools.create_asset('DA_ProductionCosmetics',ROOT,unreal.ProductionArtCatalog,factory)
catalog.set_editor_property('cosmetics',entries);assets.save_loaded_asset(catalog,False)
(A/'CosmeticCatalog.json').write_text(json.dumps({'catalog':PATH,'entries':report},indent=2),encoding='utf8')
(A/'ImportReceipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf8')
unreal.log('Production cosmetics configured: '+str(len(entries)))
