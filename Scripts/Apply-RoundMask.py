"""Create the approved mask derivative and update its existing stable catalog ID."""
import json,pathlib,hashlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections'
W=pathlib.Path(r'C:\Users\diego\OneDrive\Documentos\ChatGPT\gamejam\Corrections20261114')
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
path='/Game/OneManStudio/Cosmetics/ProductionArt/Corrections/SM_RobberMaskRound'
if assets.does_asset_exist(path):
    if not (A/'MaskApplied.json').exists():raise RuntimeError('Refusing to overwrite an unowned derivative')
    mesh=assets.load_asset(path)
else:
    original=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/Face/RobberMask/SM_RobberMask')
    positions=[unreal.Vector(*p) for p in json.loads((W/'MaskCorrectedPositions.json').read_text())]
    mesh=unreal.StudioAssetCorrectionTools.create_corrected_mesh(original,positions,path)
    if not mesh:raise RuntimeError('Mask derivative could not be built')
    assets.save_loaded_asset(mesh,False)
catalog=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics')
entries=list(catalog.get_editor_property('cosmetics'))
profile=unreal.load_asset('/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character')
head=unreal.StudioCharacterAssetTools.reference_bone_transform(profile.get_editor_property('body_mesh'),'Head',profile.get_editor_property('mesh_transform'))
definition=next(e for e in entries if e.get_editor_property('id')=='Head.RobberMask')
definition.set_editor_properties({'static_mesh':mesh,'socket_name':'Head','fit_in_actor_space':False,'socket_scale_resolved':True,
    'actor_relative_transform':unreal.Transform(),'relative_transform':unreal.MathLibrary.make_relative_transform(unreal.Transform(),head)})
catalog.set_editor_property('cosmetics',entries);assets.save_loaded_asset(catalog,False)
levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='MovieManager':
        items=list(actor.get_editor_property('cosmetic_catalog'))
        for i,item in enumerate(items):
            if item.get_editor_property('id')=='Head.RobberMask':items[i]=definition
        actor.set_editor_property('cosmetic_catalog',items)
levels.save_current_level()
(A/'MaskApplied.json').write_text(json.dumps({'mesh':path,'catalog_id':'Head.RobberMask','relative_transform':str(definition.get_editor_property('relative_transform')),'bank_sha256':hashlib.sha256((P/'Content/OneManStudio/Maps/Lvl_Bank.umap').read_bytes()).hexdigest()},indent=2),encoding='utf8')
unreal.log('Round-head mask derivative assigned; original mesh preserved')
