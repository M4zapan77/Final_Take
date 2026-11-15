"""Import all 34 supplied, prepared recordings; configure existing gameplay presentation."""
import json,pathlib,re,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections';ROOT='/Game/OneManStudio/Audio'
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools()
receipt_path=A/'AudioImportReceipt.json';receipt=json.loads(receipt_path.read_text()) if receipt_path.exists() else {}
def checkpoint():receipt_path.write_text(json.dumps(receipt,indent=2,ensure_ascii=False),encoding='utf8')
def owned_create(name,folder,cls,factory):
    path=folder+'/'+name
    if assets.does_asset_exist(path):
        if path not in receipt.get('_created',[]):raise RuntimeError('Refusing to overwrite '+path)
        return assets.load_asset(path)
    result=tools.create_asset(name,folder,cls,factory)
    if not result:raise RuntimeError('Could not create '+path)
    receipt.setdefault('_created',[]).append(path);checkpoint();return result
entries=json.loads((A/'AudioPrepared.json').read_text(encoding='utf8'));sounds={}
names=[
('Cartoon_police','PoliceSiren'),('Comedic_cartoon_fail','Failure'),('Exaggerated_cartoon_','Kiss'),('Exaggerated_theatric','EvilLaugh'),('Over-the-top_fake','Cry'),('Short_comedic_cartoo','Scream'),
('action.mp3','DirectorAction'),('cut.mp3','DirectorCut'),('next actor.mp3','DirectorNextActor'),('pick custome.mp3','DirectorCostume'),('wrap.mp3','DirectorWrap'),
('Brief_polite','Applause'),('Comedic_game_show_re','ResultsSting'),('Comedic_soft_body_th_#1','FaintImpact1'),('Comedic_soft_body_th_#2','FaintImpact2'),('Crisp_bite','DonutBite'),
('Picking_up_a_heavy_c_#1','MoneyPickup1'),('Picking_up_a_heavy_c_#4','MoneyPickup2'),('Quiet_movie','CinemaRoomTone'),('Short_pleasant','InterfaceClick'),('Short_triumphant','PremiereSting'),
('Single_dress_shoe_fo_#1','FootstepDress1'),('Single_dress_shoe_fo_#2','FootstepDress2'),('Single_dress_shoe_fo_#4','FootstepDress3'),('Single_leather','FootstepLeather'),('Single_light','FootstepLight'),('Single_sharp_wooden','Clapper'),('Subtle_quiet_bank','BankRoomTone'),
('Backstage Bounce','BackstageBounce'),('Bassline Getaway','BasslineGetaway'),('Brassy Buzzer Blast','BrassyBuzzerBlast'),('Dressing Room Frolic','DressingRoomFrolic'),('Comedic_game_show_ri','AlternateResultsSting'),('Short_cinematic_movi','AlternatePremiereSting')]
for item in entries:
    source=pathlib.Path(item['source']).name
    matches=[name for prefix,name in names if source.startswith(prefix)]
    if len(matches)!=1:raise RuntimeError('Ambiguous audio source '+source)
    key=matches[0];category=item['category'];folder=ROOT+'/'+category;path=folder+'/SW_'+key
    prepared=P/'SourceArt/FinalAudio20261114/Prepared'/category/pathlib.Path(item['prepared']).name
    if not prepared.exists():raise RuntimeError('Prepared file missing '+str(prepared))
    if assets.does_asset_exist(path):
        if path not in receipt.get('_created',[]):raise RuntimeError('Unowned audio asset '+path)
        sound=assets.load_asset(path)
    else:
        task=unreal.AssetImportTask();task.set_editor_properties({'filename':str(prepared),'destination_path':folder,'destination_name':'SW_'+key,'automated':True,'replace_existing':False,'save':False,'factory':unreal.SoundFactory()})
        tools.import_asset_tasks([task]);objects=list(task.get_objects())
        if not objects:raise RuntimeError('Import failed '+source)
        sound=objects[0];receipt.setdefault('_created',[]).append(path);checkpoint()
    sound.set_editor_properties({'looping':category in ('Music','Ambience'),'volume':1.0,'compression_quality':75})
    if category in ('Music','Ambience'):sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    assets.save_loaded_asset(sound,False);sounds[key]=sound
    receipt[key]={'asset':path,'source':item['source'],'source_sha256':item['sha256'],'category':category,'prepared_duration':item['prepared_duration'],'channels':item['prepared_channels'],'gain_db':item['gain_db']};checkpoint()
atten=owned_create('ATT_ActorPerformance',ROOT+'/SFX',unreal.SoundAttenuation,unreal.SoundAttenuationFactory())
settings=atten.get_editor_property('attenuation');settings.set_editor_properties({'attenuate':True,'spatialize':True,'attenuation_shape_extents':unreal.Vector(400,0,0),'falloff_distance':2200.0})
atten.set_editor_property('attenuation',settings);assets.save_loaded_asset(atten,False)
factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.StudioAudioProfile)
profile=owned_create('DA_StudioAudio',ROOT,unreal.StudioAudioProfile,factory)
flow=profile.get_editor_property('flow_audio')
flow.set_editor_properties({'menu_music':sounds['BackstageBounce'],'introduction_music':sounds['BackstageBounce'],'wardrobe_music':sounds['DressingRoomFrolic'],'recording_music':sounds['BasslineGetaway'],'premiere_music':sounds['BasslineGetaway'],'results_music':sounds['BrassyBuzzerBlast'],'bank_ambience':sounds['BankRoomTone'],'cinema_ambience':sounds['CinemaRoomTone'],'clapper_sound':sounds['Clapper'],'button_sound':sounds['InterfaceClick']})
announcements=list(flow.get_editor_property('announcements'))
for cue,key in zip(announcements,['DirectorCostume','DirectorAction','DirectorCut','DirectorNextActor','DirectorWrap']):
    cue.set_editor_property('sound',sounds[key]);cue.set_editor_property('duration_seconds',max(1.5,receipt[key]['prepared_duration']+.15))
flow.set_editor_property('announcements',announcements);profile.set_editor_property('flow_audio',flow)
premiere=profile.get_editor_property('premiere_audio');premiere.set_editor_properties({'premiere_music':sounds['BasslineGetaway'],'results_music':sounds['BrassyBuzzerBlast'],'premiere_sting':sounds['PremiereSting'],'results_sting':sounds['ResultsSting'],'button_sound':sounds['InterfaceClick']});profile.set_editor_property('premiere_audio',premiere)
def definition(id,key,volume=1):
    d=unreal.ActingSoundDefinition();d.set_editor_properties({'identifier':id,'sound':sounds[key],'volume_multiplier':volume,'attenuation':atten,'max_duration_seconds':receipt[key]['prepared_duration']+.1});return d
profile.set_editor_properties({'soundboard':[definition(k,k,.9 if k=='PoliceSiren' else 1) for k in ['EvilLaugh','Scream','Cry','PoliceSiren','Kiss','Failure']],
 'footsteps':[definition('Footstep.'+str(i+1),k,.55) for i,k in enumerate(['FootstepDress1','FootstepDress2','FootstepDress3','FootstepLeather','FootstepLight'])],
 'actor_attenuation':atten,'faint_impacts':[sounds['FaintImpact1'],sounds['FaintImpact2']],'money_pickups':[sounds['MoneyPickup1'],sounds['MoneyPickup2']],'donut_bite':sounds['DonutBite'],'applause':sounds['Applause'],'alternate_stings':[sounds['AlternateResultsSting'],sounds['AlternatePremiereSting']]})
assets.save_loaded_asset(profile,False)
# Persist explicit references on the existing level actors as well as the native defaults.
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
catalog=assets.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics');cosmetics=list(catalog.get_editor_property('cosmetics'))
for e in cosmetics:
    if str(e.get_editor_property('id'))=='Prop.MoneyBag':e.set_editor_property('pickup_sound',sounds['MoneyPickup1'])
    if str(e.get_editor_property('id'))=='Prop.Donut':e.set_editor_property('use_sound',sounds['DonutBite'])
catalog.set_editor_property('cosmetics',cosmetics);assets.save_loaded_asset(catalog,False)
by_id={str(e.get_editor_property('id')):e for e in cosmetics}
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='MovieManager':
        actor.set_editor_property('audio_profile',profile)
        actor.set_editor_property('cosmetic_catalog',[by_id.get(str(e.get_editor_property('id')),e) for e in actor.get_editor_property('cosmetic_catalog')])
    if actor.get_class().get_name() in ('MoneyBagProp','DonutProp','BananaProp'):actor.set_editor_property('equipped_visual',by_id[str(actor.get_editor_property('prop_id'))])
levels.save_current_level();receipt['_complete']=len(sounds);checkpoint()
unreal.log('Final audio imported and assigned: '+str(len(sounds)))
