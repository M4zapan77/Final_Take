"""Small final set dressing, guarded against concurrent map edits."""
import pathlib,json,hashlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();out=P/'Saved/FinalRelease'
receipt=json.loads((out/'BankPolish.json').read_text(encoding='utf8'))
path=P/'Content/OneManStudio/Maps/Lvl_Bank.umap'
if hashlib.sha256(path.read_bytes()).hexdigest()!=receipt['after']:raise RuntimeError('Concurrent bank edit preserved')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);F=receipt['floor_z'];rows=[]
for a in sub.get_all_level_actors():
    label=a.get_actor_label()
    if label in ('Studio light stand','Studio light panel'):
        c=a.get_component_by_class(unreal.StaticMeshComponent);b=c.static_mesh.get_bounding_box();v=b.max-b.min
        d=(8,8,160) if label.endswith('stand') else (85,18,65)
        a.set_actor_scale3d(unreal.Vector(d[0]/v.x,d[1]/v.y,d[2]/v.z))
        center,extent=a.get_actor_bounds(False,False);p=a.get_actor_location()
        support=F if label.endswith('stand') else F+156
        a.set_actor_location(unreal.Vector(p.x,p.y,p.z+support-(center.z-extent.z)),False,False)
        rows.append({'label':label,'size_cm':d})
if not levels.save_current_level():raise RuntimeError('Save failed')
receipt['after']=hashlib.sha256(path.read_bytes()).hexdigest();receipt['final_dressing']=rows
(out/'BankPolish.json').write_text(json.dumps(receipt,indent=2),encoding='utf8')
unreal.log('FINAL BANK light stands corrected to actual slender supports')
