"""Keep the default third-person camera inside the entrance lintel."""
import pathlib,json,hashlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();out=P/'Saved/FinalRelease'
r=json.loads((out/'BankPolish.json').read_text(encoding='utf8'));path=P/'Content/OneManStudio/Maps/Lvl_Bank.umap'
if hashlib.sha256(path.read_bytes()).hexdigest()!=r['after']:raise RuntimeError('Concurrent bank edit preserved')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
a=next(a for a in sub.get_all_level_actors() if a.actor_has_tag('StorySpawn.PoliceOfficer'))
p=a.get_actor_location();a.set_actor_location(unreal.Vector(-100,-510,p.z),False,False)
if not levels.save_current_level():raise RuntimeError('Save failed')
r['after']=hashlib.sha256(path.read_bytes()).hexdigest();r['police_camera_clearance_spawn']=[-100,-510,p.z]
(out/'BankPolish.json').write_text(json.dumps(r,indent=2),encoding='utf8')
unreal.log('FINAL BANK police spawn advanced 220cm to keep camera inside entrance frame')
