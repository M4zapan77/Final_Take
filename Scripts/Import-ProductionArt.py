"""Import the supplied production packs into new folders, preserving source meshes and stable IDs."""
import gc,hashlib,json,pathlib,re,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve()
SOURCE=P/'SourceArt/ProductionArt20261113';AUDIT=P/'Saved/ProductionArt';THUMBS=AUDIT/'Thumbnails';THUMBS.mkdir(exist_ok=True)
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
receipt_path=AUDIT/'ImportReceipt.json'
receipt=json.loads(receipt_path.read_text(encoding='utf8')) if receipt_path.exists() else {}

def checkpoint():receipt_path.write_text(json.dumps(receipt,indent=2),encoding='utf8')
def name(s):return re.sub('[^A-Za-z0-9_]','_',s)
def create(n,root,cls,factory):
    path=root+'/'+n
    if assets.does_asset_exist(path):
        if path not in receipt.get('_created',[]):raise RuntimeError('Refusing to overwrite '+path)
        return assets.load_asset(path)
    a=tools.create_asset(n,root,cls,factory)
    if not a:raise RuntimeError('Could not create '+path)
    receipt.setdefault('_created',[]).append(path);checkpoint();return a
def node(m,c,**kw):
    n=mel.create_material_expression(m,c);n.set_editor_properties(kw);return n
def wire(a,b,pin='',out=''):
    if not mel.connect_material_expressions(a,out,b,pin):raise RuntimeError('Material connection failed '+pin)
def output(a,m,prop):
    if not mel.connect_material_property(a,'',prop):raise RuntimeError('Material output failed')
def custom(m,code,inputs):
    n=node(m,unreal.MaterialExpressionCustom,code=code,output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    items=[]
    for k in inputs:
        i=unreal.CustomInput();i.set_editor_property('input_name',k);items.append(i)
    n.set_editor_property('inputs',items)
    for k,v in inputs.items():wire(v,n,k)
    return n
def save(a):assets.save_loaded_asset(a,False)

MASTER='/Game/OneManStudio/Environment/ProductionArt/Shared'
surface_path=MASTER+'/M_ProductionSurface'
body_path='/Game/OneManStudio/Cosmetics/ProductionArt/Body/M_ProductionOutfit'
white=assets.load_asset('/Engine/EngineResources/WhiteSquareTexture')

def master(path,body=False):
    if assets.does_asset_exist(path) and not body:return assets.load_asset(path)
    m=create(path.rsplit('/',1)[1],path.rsplit('/',1)[0],unreal.Material,unreal.MaterialFactoryNew())
    if body and not unreal.StudioCharacterAssetTools.reset_customization_material(m):raise RuntimeError('Cannot rebuild the owned outfit material')
    mel.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    m.set_editor_property('two_sided',not body)
    tex=node(m,unreal.MaterialExpressionTextureSampleParameter2D,parameter_name='BaseColorTexture',texture=white)
    if body:
        pos=node(m,unreal.MaterialExpressionPreSkinnedPosition);v=node(m,unreal.MaterialExpressionVertexInterpolator);wire(pos,v)
        skin=node(m,unreal.MaterialExpressionVectorParameter,parameter_name='SkinColor',default_value=unreal.LinearColor.WHITE)
        tint=node(m,unreal.MaterialExpressionVectorParameter,parameter_name='Color',default_value=unreal.LinearColor.WHITE)
        style=node(m,unreal.MaterialExpressionScalarParameter,parameter_name='OutfitStyle',default_value=2)
        rgb=custom(m,"""
float torso=step(40,P.z)*(1-step(104,P.z))*(1-step(29,abs(P.x)));
float legs=step(14,P.z)*(1-step(58,P.z));
float luma=dot(T.rgb,float3(.2126,.7152,.0722));
float painted=max(step(.10,max(T.r,max(T.g,T.b))-min(T.r,min(T.g,T.b))),1-step(.48,luma));
float cloth=max(torso,Style>=1.5?legs:0);
cloth=max(cloth,painted*step(14,P.z)*(1-step(104,P.z)));
cloth*=step(.5,Style);
return lerp(Skin.rgb,T.rgb*OutfitTint.rgb,saturate(cloth));
""",{'P':v,'T':tex,'Skin':skin,'Style':style,'OutfitTint':tint})
        output(rgb,m,unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        output(tex,m,unreal.MaterialProperty.MP_BASE_COLOR)
    rough=node(m,unreal.MaterialExpressionScalarParameter,parameter_name='Roughness',default_value=.72)
    output(rough,m,unreal.MaterialProperty.MP_ROUGHNESS)
    spec=node(m,unreal.MaterialExpressionConstant,r=.25);output(spec,m,unreal.MaterialProperty.MP_SPECULAR)
    mel.layout_material_expressions(m);mel.recompile_material(m);save(m);return m

surface=master(surface_path);body_master=master(body_path,True)
fbx_options=unreal.FbxImportUI()
fbx_options.set_editor_properties({'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH,
    'import_as_skeletal':False,'import_mesh':True,'import_animations':False,'import_materials':False,'import_textures':False})
fbx_options.static_mesh_import_data.set_editor_properties({'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,
    'convert_scene':True,'convert_scene_unit':True,'import_uniform_scale':1.0,'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS})

def import_file(file,n,folder,factory,options=None):
    path=folder+'/'+n
    if assets.does_asset_exist(path):
        if path not in receipt.get('_created',[]):raise RuntimeError('Existing asset not owned by this import: '+path)
        return assets.load_asset(path)
    t=unreal.AssetImportTask();t.set_editor_properties({'filename':str(file),'destination_path':folder,'destination_name':n,'factory':factory,'automated':True,'replace_existing':False,'save':False,'async_':False})
    if options:t.set_editor_property('options',options)
    tools.import_asset_tasks([t]);objects=list(t.get_objects())
    if not objects:raise RuntimeError('No import result: '+str(file))
    result=objects[0]
    if result.get_path_name().split('.')[0]!=path:
        if not tools.rename_assets([unreal.AssetRenameData(result,folder,n)]):raise RuntimeError('Asset rename failed')
    receipt.setdefault('_created',[]).append(path);checkpoint();return result

entries=[]
for file in sorted(SOURCE.rglob('*.fbx')):
    rel=file.relative_to(SOURCE).parts
    if rel[0]=='Derived':continue
    if rel[0]=='ropas':
        if file.name!='base.fbx':continue
        uid=rel[2];key='Outfit_'+uid[:8];folder='/Game/OneManStudio/Cosmetics/ProductionArt/Body/'+key;kind='Body';texture_root=file.parent.parent
    elif rel[0]=='objetos':
        base=next(file.parent.rglob('*basecolor.jpg'));key=''.join(w.title() for w in base.stem.split('_3d_model')[0].split('_'));folder='/Game/OneManStudio/Cosmetics/ProductionArt/Props/'+key;kind='Prop';texture_root=file.parent
    elif rel[0]=='gorros':
        lookup={'Black_Ribbed_Beanie':'Beanie','Blue_Police_Cap':'PoliceCap','Pink_Princess_Crown':'Crown','Sleepy_Rubber_Rooster':'Chicken','Starband_Cowboy_Hat':'Cowboy'}
        key=next(v for k,v in lookup.items() if k in file.name);folder='/Game/OneManStudio/Cosmetics/ProductionArt/Head/'+key;kind='Head';texture_root=file.parent
    elif rel[0]=='accesorios_cara':
        lookup={'Black_Masquerade_Mask':'RobberMask','Curled_Black_Mustache':'Mustache','Horizon_Shades':'Sunglasses','Round_Black_Glasses':'Glasses'}
        key=next(v for k,v in lookup.items() if k in file.name);folder='/Game/OneManStudio/Cosmetics/ProductionArt/Face/'+key;kind='Face';texture_root=file.parent
    else:
        key=rel[3];film=key in ('Camara1_Alta','Camara2_Baja','CamaraMan','Clapperboard','Silla_Director')
        kind='FilmEquipment' if film else 'Bank' if rel[2]=='Sala_Banco' else 'Cinema'
        folder='/Game/OneManStudio/Environment/'+kind+'/ProductionArt/'+key;texture_root=file.parent
    entries.append({'key':key,'kind':kind,'folder':folder,'source':str(file),'texture_root':str(texture_root)})

batch_limit_match=re.search(r'-ProductionImportLimit=(\d+)',unreal.SystemLibrary.get_command_line())
batch_limit=int(batch_limit_match.group(1)) if batch_limit_match else 4
kind_match=re.search(r'-ProductionImportKind=(\w+)',unreal.SystemLibrary.get_command_line())
processed=0
for e in entries:
    key=e['key'];folder=e['folder'];kind=e['kind'];file=pathlib.Path(e['source'])
    if kind_match and kind!=kind_match.group(1):continue
    if receipt.get(key,{}).get('complete'):
        if kind=='Body' and kind_match:
            unreal.StudioCharacterAssetTools.export_outfit_thumbnail(unreal.load_asset('/Game/OneManStudio/Characters/Base3/SK_Base3'),unreal.load_asset(receipt[key]['material']),str(THUMBS/(key+'.png')))
        continue
    unreal.log('ProductionArt importing '+key)
    texs={}
    for image in sorted(pathlib.Path(e['texture_root']).rglob('*')):
        if image.suffix.lower() not in ('.png','.jpg','.jpeg'):continue
        lower=image.stem.lower()
        role='Color' if lower==file.stem.lower() or lower.endswith(('_basecolor','_diffuse','_texture')) else 'Normal' if lower.endswith('_normal') else 'Packed' if lower.endswith(('_metallic_roughness','_pbr','_rm')) else 'Roughness' if lower.endswith('_roughness') else 'Metallic' if lower.endswith('_metallic') else 'Emissive' if lower.endswith('_emit') else 'Shaded' if lower=='shaded' else name(image.stem)
        texture=import_file(image,'T_'+key+'_'+role,folder,unreal.TextureFactory())
        texture.set_editor_properties({'srgb':role in ('Color','Shaded'),'max_texture_size':4096 if kind=='Body' and role=='Color' else 2048,
            'compression_settings':unreal.TextureCompressionSettings.TC_NORMALMAP if role=='Normal' else unreal.TextureCompressionSettings.TC_DEFAULT if role in ('Color','Shaded') else unreal.TextureCompressionSettings.TC_MASKS})
        save(texture);texs[role]=texture
    if 'Color' not in texs:raise RuntimeError('No diffuse texture for '+key)
    material=create('MI_'+key,folder,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(material,body_master if kind=='Body' else surface)
    mel.set_material_instance_texture_parameter_value(material,'BaseColorTexture',texs['Color'])
    mel.set_material_instance_scalar_parameter_value(material,'Roughness',.72);save(material)
    mesh=None;before=None
    if kind!='Body':
        mesh_source=SOURCE/'Derived'/(key+'.fbx') if kind=='Prop' else file
        if not mesh_source.exists():raise RuntimeError('Create the optimized prop first: '+str(mesh_source))
        e['imported_source']=str(mesh_source)
        mesh=import_file(mesh_source,'SM_'+key,folder,unreal.FbxFactory(),fbx_options)
        before=mesh.get_num_triangles(0)
        if not unreal.StudioCharacterAssetTools.normalize_art_mesh(mesh,100,24000 if kind in ('Head','Face','Prop') else 60000):raise RuntimeError('Mesh normalization failed '+key)
        for index in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(index,material)
        editor_mesh=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        editor_mesh.remove_collisions(mesh)
        if kind=='Bank':editor_mesh.add_simple_collisions(mesh,unreal.ScriptingCollisionShapeType.BOX)
        save(mesh)
        thumb=THUMBS/(key+'.png')
        if not unreal.StudioCharacterAssetTools.export_art_thumbnail(mesh,str(thumb),-45):raise RuntimeError('Thumbnail failed '+key)
        save(mesh)
    else:
        # Temporary presentation copy only. The game keeps the original mesh and skeleton.
        if not unreal.StudioCharacterAssetTools.export_outfit_thumbnail(unreal.load_asset('/Game/OneManStudio/Characters/Base3/SK_Base3'),material,str(THUMBS/(key+'.png'))):raise RuntimeError('Body thumbnail failed '+key)
    box=mesh.get_bounding_box() if mesh else None
    receipt[key]={**e,'complete':True,'material':material.get_path_name(),'mesh':mesh.get_path_name() if mesh else None,
        'textures':{k:v.get_path_name() for k,v in texs.items()},'source_triangles':before,'runtime_triangles':mesh.get_num_triangles(0) if mesh else 2000,
        'bounds_min':[box.min.x,box.min.y,box.min.z] if box else None,'bounds_max':[box.max.x,box.max.y,box.max.z] if box else None}
    checkpoint();unreal.SystemLibrary.collect_garbage();gc.collect()
    processed+=1
    if processed>=batch_limit:break
completed=sum(bool(receipt.get(e['key'],{}).get('complete')) for e in entries)
(AUDIT/('ImportComplete.json' if completed==len(entries) else 'ImportProgress.json')).write_text(json.dumps({'entries':len(entries),'complete':completed},indent=2))
unreal.log('ProductionArt batch complete: '+str(completed)+'/'+str(len(entries)))
