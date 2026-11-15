"""Per-item reference-pose fits. No live-pose fitting or source mesh mutation."""
import json,pathlib,copy,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections'
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
catalog=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics')
profile=unreal.load_asset('/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character')
entries=list(catalog.get_editor_property('cosmetics'))
mesh=profile.get_editor_property('body_mesh');mesh_to_actor=profile.get_editor_property('mesh_transform')
ref={bone:unreal.StudioCharacterAssetTools.reference_bone_transform(mesh,bone,mesh_to_actor) for bone in ('Head','RightHand','LeftHand')}
report=[]
def fit(entry,desired,bone='Head'):
    entry.set_editor_properties({'socket_name':bone,'relative_transform':unreal.MathLibrary.make_relative_transform(desired,ref[bone]),'actor_relative_transform':desired,'fit_in_actor_space':False,'socket_scale_resolved':True})
    report.append({'id':str(entry.get_editor_property('id')),'bone':bone,'actor_reference':str(desired),'socket_local':str(entry.get_editor_property('relative_transform'))})
def tr(p,s,yaw=-90):return unreal.Transform(location=unreal.Vector(*p),rotation=unreal.Rotator(yaw=yaw),scale=unreal.Vector(s,s,s))
# The police visor is the bounding-box minimum, not the cap's supporting base.
# Its actual central underside is -7.39 mesh cm. Seat that at head Z=82 cm.
# Other hats have distinct openings and retain their independently fitted sizes.
head_fits={'Head.PoliceCap':((0,0,88.30),.85226),'Head.Beanie':((0,0,88.5),.7665),'Head.Crown':((0,0,98.5),.64),'Head.Cowboy':((0,0,92.0),.92),'Head.Chicken':((0,0,95.5),.9359)}
face_fits={'Face.Glasses':((3.2,0,55.2),.61),'Face.Mustache':((33.0,0,44.5),.34),'Face.Sunglasses':((1.0,0,54.1),.70)}
for entry in entries:
    id=str(entry.get_editor_property('id'))
    if id in head_fits:fit(entry,tr(*head_fits[id]))
    if id in face_fits:fit(entry,tr(*face_fits[id]))
    if entry.get_editor_property('category')==unreal.CosmeticCategory.PROP:
        # Keep original per-object orientation/scale, place the grip within the
        # mitten rather than measuring from the mesh bounding-box center.
        offsets={'Prop.Banana':(8,-6),'Prop.Donut':(8,-6),'Prop.MoneyBag':(8,-20),'Prop.Microphone':(8,9),'Prop.Megaphone':(10,4),'Prop.RubberDuck':(9,-4)}
        if id not in offsets:continue
        x,z=offsets[id];s=entry.get_editor_property('actor_relative_transform').scale3d.x
        if s<=0:s=entry.get_editor_property('relative_transform').scale3d.x*105
        fits=[]
        for bone in ('RightHand','LeftHand'):
            p=ref[bone].translation
            fits.append(tr((p.x+x,p.y,p.z+z),s))
        fit(entry,fits[0],'RightHand')
        entry.set_editor_properties({'use_left_hand_transform':True,'left_hand_relative_transform':unreal.MathLibrary.make_relative_transform(fits[1],ref['LeftHand'])})
legacy=next(e for e in entries if str(e.get_editor_property('id'))=='Head.RobberMask')
if not any(str(e.get_editor_property('id'))=='Face.RobberMask' for e in entries):
    alias=unreal.CosmeticDefinition()
    for key in ('display_name','icon','static_mesh','materials','socket_name','relative_transform','actor_relative_transform','fit_in_actor_space','socket_scale_resolved'):
        alias.set_editor_property(key,legacy.get_editor_property(key))
    alias.set_editor_properties({'id':'Face.RobberMask','category':unreal.CosmeticCategory.FACE})
    entries.append(alias)
catalog.set_editor_property('cosmetics',entries);assets.save_loaded_asset(catalog,False)
levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
by_id={str(e.get_editor_property('id')):e for e in entries}
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='MovieManager':
        original=list(actor.get_editor_property('cosmetic_catalog'))
        merged=[by_id.get(str(e.get_editor_property('id')),e) for e in original]
        if not any(str(e.get_editor_property('id'))=='Face.RobberMask' for e in merged):merged.append(by_id['Face.RobberMask'])
        actor.set_editor_property('cosmetic_catalog',merged)
    if actor.get_class().get_name() in ('MoneyBagProp','DonutProp','BananaProp'):
        actor.set_editor_property('equipped_visual',by_id[str(actor.get_editor_property('prop_id'))])
levels.save_current_level()
(A/'AttachmentFits.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('Individual cosmetic fits saved against actual Base3 reference bones')
