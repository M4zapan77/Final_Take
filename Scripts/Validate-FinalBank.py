"""Validate authored capsule space and the entrance in the actual final bank."""
import pathlib,json,hashlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();out=P/'Saved/FinalRelease'
receipt=json.loads((out/'BankPolish.json').read_text(encoding='utf8'))
path=P/'Content/OneManStudio/Maps/Lvl_Bank.umap'
if hashlib.sha256(path.read_bytes()).hexdigest()!=receipt['after']:raise RuntimeError('Concurrent bank edit preserved')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
allactors=list(sub.get_all_level_actors());F=receipt['floor_z'];result={'capsules':[],'entrance':[]}
unreal.log(str(unreal.SystemLibrary.capsule_trace_single_by_profile.__doc__))
def probe(x,y,x2=None,y2=None):
    # The entrance has a 6 cm threshold; sweep 20 cm higher to allow the
    # character's existing step-up rather than misclassifying it as a wall.
    z=F+(118 if y<-800 else 98)
    start=unreal.Vector(x,y,z);end=unreal.Vector(x if x2 is None else x2,y+.01 if y2 is None else y2,z)
    h=unreal.SystemLibrary.capsule_trace_single_by_profile(world,start,end,42,96,'Pawn',False,[],unreal.DrawDebugTrace.NONE)
    return {'blocked':bool(h and h.to_tuple()[0]),'hit':str(h)}
teller=next(a for a in allactors if a.actor_has_tag('StorySpawn.NPC.Teller'))
for x,y in [(-595,-385),(-590,-700),(-590,-60),(-570,-385)]:
    r=probe(x,y)
    if not r['blocked']:
        teller.set_actor_location(unreal.Vector(x,y,F+110),False,False);result['teller']=[x,y,F+110];break
else:raise RuntimeError('No safe teller position')
for a in allactors:
    tags=[str(t) for t in a.tags]
    if any(t.startswith('StorySpawn.') for t in tags):
        p=a.get_actor_location();r=probe(p.x,p.y);r['marker']=a.get_actor_label();result['capsules'].append(r)
# Align the added entrance furniture with the measured clear opening of the shell.
by={a.get_actor_label():a for a in allactors}
for label in ('Entrance exterior path','Entrance threshold','Entrance upper frame'):
    a=by[label];p=a.get_actor_location();a.set_actor_location(unreal.Vector(100,p.y,p.z),False,False)
frames=[a for a in allactors if a.get_actor_label()=='Entrance cream frame']
for a,x in zip(frames,(-60,260)):
    p=a.get_actor_location();a.set_actor_location(unreal.Vector(x,p.y,p.z),False,False)
a=by['Open bank entrance door'];p=a.get_actor_location();a.set_actor_location(unreal.Vector(-40,p.y,p.z),False,False)
for a in allactors:
    if a.actor_has_tag('StoryExit') or a.get_actor_label()=='Bank escape objective':
        p=a.get_actor_location();a.set_actor_location(unreal.Vector(100,p.y,p.z),False,False)
for x in [50,100,150,200]:
    r=probe(x,-820,x,-1120);r['x']=x;result['entrance'].append(r)
door=next(a for a in allactors if a.get_class().get_name()=='StudioSetDoor')
blocker=door.get_editor_property('door_blocker');blocker.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
result['open_vault_passage']=probe(-65,300,-65,580)
blocker.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
(out/'BankClearance.json').write_text(json.dumps(result,indent=2),encoding='utf8')
if any(r['blocked'] for r in result['capsules']):raise RuntimeError('A cast marker is obstructed; see BankClearance.json')
if next(r for r in result['entrance'] if r['x']==100)['blocked']:raise RuntimeError('Authored entrance is obstructed')
if result['open_vault_passage']['blocked']:raise RuntimeError('Open vault passage is obstructed')
if not levels.save_current_level():raise RuntimeError('Save failed')
receipt['after']=hashlib.sha256(path.read_bytes()).hexdigest();(out/'BankPolish.json').write_text(json.dumps(receipt,indent=2),encoding='utf8')
unreal.log('FINAL BANK clearance saved: '+json.dumps(result))
