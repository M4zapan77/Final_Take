"""Correct existing set actors in place; preserve gameplay identities, zones and spawns."""
import json,pathlib,math,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report={'bank':[],'cinema':[],'floor_samples':[],'protected':[]}
def vec(v):return [v.x,v.y,v.z]
def snapshot(actor):return {'name':actor.get_name(),'label':actor.get_actor_label(),'class':actor.get_class().get_name(),'transform':str(actor.get_actor_transform())}
def local_bounds(actor):
    c=actor.get_component_by_class(unreal.StaticMeshComponent)
    if not c or not c.static_mesh:raise RuntimeError('Missing mesh '+actor.get_actor_label())
    return c.static_mesh.get_bounding_box()
def size(actor,dimensions):
    b=local_bounds(actor);s=b.max-b.min
    actor.set_actor_scale3d(unreal.Vector(*[dimensions[i]/max(.01,vec(s)[i]) for i in range(3)]))
def ground(actor,z):
    center,extent=actor.get_actor_bounds(False,False);p=actor.get_actor_location()
    actor.set_actor_location(unreal.Vector(p.x,p.y,p.z+z-(center.z-extent.z)),False,False)
def place(actor,xy,dimensions=None,yaw=None,support=0):
    before=snapshot(actor)
    if dimensions:size(actor,dimensions)
    if yaw is not None:actor.set_actor_rotation(unreal.Rotator(yaw=yaw),False)
    p=actor.get_actor_location();actor.set_actor_location(unreal.Vector(xy[0],xy[1],p.z),False,False);ground(actor,support)
    center,extent=actor.get_actor_bounds(False,False)
    return {'before':before,'after':snapshot(actor),'bottom':center.z-extent.z,'top':center.z+extent.z,'dimensions':vec(extent*2),'support':support}
levels.load_level('/Game/OneManStudio/Maps/Lvl_Bank')
all_actors=[a for a in actors.get_all_level_actors() if '/Lvl_Bank.' in a.get_path_name()]
by_label={a.get_actor_label():a for a in all_actors}
shell=by_label['BANK - Meshy Diorama'].static_mesh_component
def floor(x,y):
    hits=[]
    for dx,dy in ((0,0),(-3,0),(3,0),(0,-3),(0,3)):
        p=unreal.StudioAssetCorrectionTools.trace_surface(shell,unreal.Vector(x+dx,y+dy,180),unreal.Vector(x+dx,y+dy,-100))
        if p.z>-1000:hits.append(p.z)
    source='Bank shell top surface'
    if not hits:
        # Tile grout can leave a tiny visual-mesh gap. Measure the actual
        # existing collision floor below it, never invent a universal height.
        c=by_label['Bank exterior ground'].static_mesh_component
        p=unreal.StudioAssetCorrectionTools.trace_surface(c,unreal.Vector(x,y,180),unreal.Vector(x,y,-100))
        if p.z < -1000:raise RuntimeError(f'No supporting floor at {x},{y}; aborting placement')
        hits=[p.z];source='Existing underlying collision floor'
    z=sorted(hits)[len(hits)//2]
    report['floor_samples'].append({'xy':[x,y],'z':z,'source':source});return z
protected=[a for a in all_actors if isinstance(a,unreal.PlayerStart) or a.get_class().get_name() in ('MovieManager','ObjectiveZone')]
saved={a.get_path_name():str(a.get_actor_transform()) for a in protected}
def bank(label,xy,dims=None,yaw=None,support=None):
    a=by_label[label];r=place(a,xy,dims,yaw,floor(*xy) if support is None else support);report['bank'].append(r);return r
tops={}
for i,x in enumerate((-350,-50,250),1):
    r=bank(f'Teller desk {i}',(x,-430),(245,115,110));tops[i]=r['top']
    bank(f'Teller blue front {i}',(x,-366),(235,10,96))
    bank(f'Teller cream lip {i}',(x,-360),(253,22,10),support=r['top']-4)
    bank(f'Teller laptop {i}',(x,-443),(55,38,40),180,r['top']+.7)
    bank(f'Teller stool {i}',(x,-565),(62,62,62))
bank('Bank notebook',(-285,-420),None,90,tops[1]+.5)
bank('Bank ledger',(310,-440),None,0,tops[3]+.5)
bank('Teller coffee',(-10,-438),None,0,tops[2]+.5)
bank('Bank shelf',(-550,-615),(155,60,265))
shelf=by_label['Bank shelf'];base=floor(-550,-615)
bank('Bank shelf books',(-550,-608),(125,31,72),0,base+62)
bank('Bank shelf archive',(-550,-608),(120,30,66),0,base+162)
bank('Front-right waiting bench',(-500,500),(245,110,135),180)
bank('Waiting table',(-280,350),(85,65,58))
bank('Waiting stool',(-145,480),(62,62,62))
for label,xy,dims in [('Entrance planter left',(-800,-580),(95,95,175)),('Waiting planter',(-730,560),(90,90,150)),('Vault planter',(760,-600),(90,90,165)),('Waiting corner plant',(-100,590),(80,80,155))]:bank(label,xy,dims)
bank('Open bank entrance door',(-900,165),(225,26,265),70)
# Existing open vault leaf remains an open entrance to the same robbery prop.
for label in ('Vault door open steel leaf','Vault golden handle'):
    a=by_label[label];p=a.get_actor_location();base=floor(p.x,p.y)
    if label.endswith('leaf'):
        a.set_actor_scale3d(unreal.Vector(2.55,2.55,.20));ground(a,base+2)
    else:a.set_actor_location(unreal.Vector(p.x,p.y,base+129.5),False,False)
    report['bank'].append({'after':snapshot(a)})
for a in all_actors:
    if a.get_actor_label() in ('Vault security locker','Gold bar','Decorative vault money','Vault money plinth'):
        label=a.get_actor_label();p=a.get_actor_location()
        if label=='Gold bar':size(a,(35,25,17))
        if label=='Decorative vault money':size(a,(49,35,61))
        ground(a,floor(p.x,p.y)+.3);report['bank'].append({'after':snapshot(a)})
for a in all_actors:
    if a.get_class().get_name() in ('MoneyBagProp','DonutProp','BananaProp'):
        prop=str(a.get_editor_property('prop_id'));xy=(700,0) if prop=='Prop.MoneyBag' else (10,-405) if prop=='Prop.Donut' else (250,-405)
        # Retain actor, PickupId, PropId and objective references. Change visuals only.
        support=by_label['Vault money plinth'].get_actor_bounds(False,False)
        z=support[0].z+support[1].z+.5 if prop=='Prop.MoneyBag' else tops[2 if prop=='Prop.Donut' else 3]+.5
        report['bank'].append(place(a,xy,None,None,z))
rail=by_label['Production camera rail'];rail.set_actor_location(unreal.Vector(300,620,floor(300,620)),False,False)
rail.set_editor_properties({'start_offset':unreal.Vector(-235,0,16),'end_offset':unreal.Vector(235,0,16),'look_at_world':unreal.Vector(0,-80,105)})
rail.get_editor_property('camera_model').set_relative_location(unreal.Vector(0,0,88),False,False)
rail.rerun_construction_scripts() if hasattr(rail,'rerun_construction_scripts') else None
bank('Secondary studio tripod',(670,390),(95,85,180),math.degrees(math.atan2(-470,-670))-90)
bank('Director chair',(70,700),(100,105,144),-90)
bank('Director clapperboard',(70,660),None,180,floor(70,660)+90)
for a in all_actors:
    label=a.get_actor_label();p=a.get_actor_location()
    if label=='Equipment case':ground(a,floor(p.x,p.y))
    if label=='Equipment case latch':a.set_actor_location(unreal.Vector(p.x,p.y,floor(p.x,p.y)+35),False,False)
    if label=='Studio light stand':ground(a,floor(p.x,p.y))
    if label=='Studio light panel':a.set_actor_location(unreal.Vector(p.x,p.y,floor(p.x,p.y)+185),False,False)
    if isinstance(a,unreal.CameraActor) and a.get_actor_label()=='Movie premiere camera':
        # Same camera actor/reference, now framing actors and the open floor instead of the tall rear wall.
        location=unreal.Vector(250,1140,850);target=unreal.Vector(250,-70,80)
        a.set_actor_location(location,False,False);a.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False)
        a.get_component_by_class(unreal.CameraComponent).set_field_of_view(90)
        report['film_camera']=snapshot(a)
for a in protected:
    if saved[a.get_path_name()]!=str(a.get_actor_transform()):raise RuntimeError('Protected gameplay transform changed '+a.get_name())
    report['protected'].append(snapshot(a))
if not levels.save_current_level():raise RuntimeError('Bank save failed')
levels.load_level('/Game/OneManStudio/Maps/Lvl_Cinema')
cinema=list(actors.get_all_level_actors());Y=5000
for a in cinema:
    label=a.get_actor_label();p=a.get_actor_location()
    if label.startswith('Audience riser'):
        elevation=48 if p.x<-450 else 24 if p.x<-150 else 0
        size(a,(250,1040,max(2,elevation)));ground(a,0);report['cinema'].append({'after':snapshot(a)})
    elif label.startswith('Cinema seat '):
        row=int(label.split()[-1].split('-')[0]);elevation=(3-row)*24
        report['cinema'].append(place(a,(p.x,p.y),(96,104,146),-90,elevation+2))
    elif label.startswith('Audience NPC '):
        row=int(label.split()[-1].split('-')[0]);a.set_actor_location(unreal.Vector(p.x,p.y,98+(3-row)*24),False,False);a.set_actor_rotation(unreal.Rotator(),False)
    elif label.startswith('Cinema popcorn'):
        ground(a,24+2+90);report['cinema'].append({'after':snapshot(a)})
    elif label=='Lobby popcorn':
        # Keep the lobby/circulation route open; the popcorn rests on a small pedestal.
        label_table='Popcorn lobby pedestal';existing=next((x for x in cinema if x.get_actor_label()==label_table),None)
        table=existing or actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-770,Y-560,35))
        table.set_actor_label(label_table);table.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
        table.static_mesh_component.set_material(0,unreal.load_asset('/Game/OneManStudio/Environment/Cinema/ProductionArt/Shared/MI_Gold'))
        table.static_mesh_component.set_collision_profile_name('NoCollision');table.set_actor_scale3d(unreal.Vector(.55,.55,.7));ground(a,70)
    elif label.startswith('Theater ceiling fixture'):
        # Lights stay on side aisles; no hanging model crosses the projected film.
        a.set_actor_location(unreal.Vector(p.x,Y+(-570 if p.y<Y else 570),510),False,False)
if not levels.save_current_level():raise RuntimeError('Cinema save failed')
(A/'EnvironmentCorrections.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('Bank/cinema corrections saved; protected gameplay actors unchanged')
