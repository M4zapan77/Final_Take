"""Import only the director's approved UI. Existing media, maps and character art stay untouched."""
import hashlib,json,pathlib,struct,unreal
project=pathlib.Path(unreal.Paths.project_dir()).resolve()
source=project/'SourceArt/FinalUI20261115'
manifest=json.loads((source/'ImportManifest.json').read_text(encoding='utf8'))
directory=project/'Saved/FinalUI';directory.mkdir(parents=True,exist_ok=True)
receipt_file=directory/'ImportReceipt.json'
receipt=json.loads(receipt_file.read_text()) if receipt_file.exists() else {'textures':[],'created':[]}
library=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools=unreal.AssetToolsHelpers.get_asset_tools()
def checkpoint():receipt_file.write_text(json.dumps(receipt,indent=2),encoding='utf8')
art={}
for row in manifest['textures']:
    file=source/row['source'];path=row['asset']
    if hashlib.sha256(file.read_bytes()).hexdigest()!=row['sha256']:raise RuntimeError('Original source changed: '+str(file))
    previous=next((r for r in receipt['textures'] if r['asset']==path),None)
    if library.does_asset_exist(path):
        if not previous or previous['sha256']!=row['sha256']:raise RuntimeError('Refusing to overwrite unowned UI '+path)
        texture=library.load_asset(path)
    else:
        task=unreal.AssetImportTask()
        task.set_editor_properties({'filename':str(file),'destination_path':path.rsplit('/',1)[0],
            'destination_name':path.rsplit('/',1)[1],'automated':True,'save':False,'replace_existing':False,'factory':unreal.TextureFactory()})
        tools.import_asset_tasks([task]);objects=task.get_objects()
        if len(objects)!=1 or not isinstance(objects[0],unreal.Texture2D):raise RuntimeError('Texture import failed '+path)
        texture=objects[0]
        texture.set_editor_properties({'srgb':True,'compression_settings':unreal.TextureCompressionSettings.TC_EDITOR_ICON,
            'compression_no_alpha':False,'lod_group':unreal.TextureGroup.TEXTUREGROUP_UI,
            'mip_gen_settings':unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,'filter':unreal.TextureFilter.TF_BILINEAR,
            'address_x':unreal.TextureAddress.TA_CLAMP,'address_y':unreal.TextureAddress.TA_CLAMP,'never_stream':True})
        library.save_loaded_asset(texture,False)
        width,height=struct.unpack('>II',file.read_bytes()[16:24])
        receipt['textures'].append({**row,'width':width,'height':height,'alpha_preserved':True,'ui_compression':'UserInterface2D / TC_EDITOR_ICON'})
        receipt['created'].append(path);checkpoint()
    for key in row['keys']:art[key]=texture

font_path='/Game/OneManStudio/UI/Common/FF_Bungee'
if library.does_asset_exist(font_path):
    if font_path not in receipt['created']:raise RuntimeError('Refusing to overwrite font')
    face=library.load_asset(font_path)
else:
    task=unreal.AssetImportTask();task.set_editor_properties({'filename':str(source/manifest['font']),
        'destination_path':font_path.rsplit('/',1)[0],'destination_name':'FF_Bungee','automated':True,'save':False,
        'replace_existing':False,'factory':unreal.FontFileImportFactory()})
    tools.import_asset_tasks([task]);objects=task.get_objects()
    face=next((obj for obj in objects if isinstance(obj,unreal.FontFace)),None)
    if not face:raise RuntimeError('Bungee FontFace import failed')
    face.set_editor_property('loading_policy',unreal.FontLoadingPolicy.INLINE)
    library.save_loaded_asset(face,False);receipt['created'].append(font_path);checkpoint()

theme_path='/Game/OneManStudio/UI/Common/DA_FinalTakeUITheme'
if library.does_asset_exist(theme_path):
    if theme_path not in receipt['created']:raise RuntimeError('Refusing to overwrite theme')
    theme=library.load_asset(theme_path)
else:
    factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.StudioUITheme)
    theme=tools.create_asset('DA_FinalTakeUITheme',theme_path.rsplit('/',1)[0],unreal.StudioUITheme,factory)
    if not theme:raise RuntimeError('Could not create configurable theme')
    receipt['created'].append(theme_path)
theme.set_editor_properties({'art':art,'heading_face':face});library.save_loaded_asset(theme,False)
receipt.update(theme=theme_path,font=font_path,font_license=manifest['font_license'],spanish_font_glyphs_verified=manifest['spanish_cmap_check'],
    missing_art=manifest['missing_art'],texture_count=len(receipt['textures']),media_assets_modified=[],maps_modified=[],character_assets_modified=[])
checkpoint()
report=project/'GameJam 2026/UI';report.mkdir(parents=True,exist_ok=True)
(report/'ImportReceipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf8')
unreal.log('FINAL TAKE UI: %d supplied textures, licensed Bungee font and configurable theme. No unrelated assets saved.'%len(receipt['textures']))
if '-FinalUIFullEditor' in unreal.SystemLibrary.get_command_line():unreal.SystemLibrary.quit_editor()
