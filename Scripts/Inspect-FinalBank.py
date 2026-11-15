"""Read-only inventory before the director-authorized final bank changes."""
import hashlib,json,pathlib,unreal
p=pathlib.Path(unreal.Paths.project_dir()).resolve();out=p/'Saved/FinalRelease';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
def v(x):return [x.x,x.y,x.z]
rows=[]
for a in actors.get_all_level_actors():
    if '/Lvl_Bank.' not in a.get_path_name():continue
    c,e=a.get_actor_bounds(False,False);m=a.get_component_by_class(unreal.StaticMeshComponent)
    row={'name':a.get_name(),'label':a.get_actor_label(),'class':a.get_class().get_name(),'tags':[str(t) for t in a.tags],
        'location':v(a.get_actor_location()),'rotation':str(a.get_actor_rotation()),'scale':v(a.get_actor_scale3d()),'center':v(c),'extent':v(e)}
    if m and m.static_mesh:
        b=m.static_mesh.get_bounding_box();row.update(mesh=m.static_mesh.get_path_name(),local_min=v(b.min),local_max=v(b.max),
            materials=[m.get_material(i).get_path_name() if m.get_material(i) else None for i in range(m.get_num_materials())])
    rows.append(row)
path=p/'Content/OneManStudio/Maps/Lvl_Bank.umap'
report={'map_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'actors':rows}
(out/'BankBefore.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('FINAL BANK read-only inventory: %d actors, map not saved'%len(rows))
