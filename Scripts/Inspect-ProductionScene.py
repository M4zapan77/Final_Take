"""Read-only transform and collision audit for the integrated sets."""
import json,pathlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank');result=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    t=actor.get_actor_transform();origin,extent=actor.get_actor_bounds(False)
    item={'name':actor.get_name(),'label':actor.get_actor_label(),'class':actor.get_class().get_name(),'location':str(t.translation),'rotation':str(t.rotation),'scale':str(t.scale3d),'bounds_center':str(origin),'bounds_extent':str(extent)}
    if isinstance(actor,unreal.CameraActor):item['camera_component']=str(actor.get_component_by_class(unreal.CameraComponent).get_world_transform())
    result.append(item)
(P/'Saved/ProductionArt/SceneAudit.json').write_text(json.dumps(result,indent=2),encoding='utf8')
