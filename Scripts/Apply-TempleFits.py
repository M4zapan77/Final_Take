"""Assign non-destructive, round-head temple derivatives using the same catalog offsets."""
import json,pathlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections';assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
catalog=assets.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics');entries=list(catalog.get_editor_property('cosmetics'));receipt_file=A/'TempleMeshes.json';receipt=json.loads(receipt_file.read_text()) if receipt_file.exists() else {}
for name in ('Glasses','Sunglasses'):
    path='/Game/OneManStudio/Cosmetics/ProductionArt/Corrections/SM_'+name+'RoundHead'
    if assets.does_asset_exist(path):
        if name not in receipt:raise RuntimeError('Unowned temple derivative '+path)
        mesh=assets.load_asset(path)
    else:
        source=assets.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/Face/'+name+'/SM_'+name)
        positions=[unreal.Vector(*p) for p in json.loads((P/'SourceArt/FinalCorrections20261114/TempleFits'/(name+'.json')).read_text())]
        mesh=unreal.StudioAssetCorrectionTools.create_corrected_mesh(source,positions,path)
        if not mesh:raise RuntimeError('Could not create temple correction '+name)
        assets.save_loaded_asset(mesh,False);receipt[name]=path;receipt_file.write_text(json.dumps(receipt,indent=2))
    next(e for e in entries if str(e.get_editor_property('id'))=='Face.'+name).set_editor_property('static_mesh',mesh)
catalog.set_editor_property('cosmetics',entries);assets.save_loaded_asset(catalog,False)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank');by_id={str(e.get_editor_property('id')):e for e in entries}
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='MovieManager':actor.set_editor_property('cosmetic_catalog',[by_id.get(str(e.get_editor_property('id')),e) for e in actor.get_editor_property('cosmetic_catalog')])
levels.save_current_level();unreal.log('Round-head temple fits assigned; original meshes preserved')
