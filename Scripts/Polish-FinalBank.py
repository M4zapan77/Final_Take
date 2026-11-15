"""Director-authorized final arrangement of the EXISTING bank; no map replacement.
Run once after the native door/bounds classes have compiled. Saves a map backup first.
"""
import hashlib,json,pathlib,shutil,math,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve()
W=pathlib.Path(r'C:\Users\diego\OneDrive\Documentos\ChatGPT\gamejam')
MAP='/Game/OneManStudio/Maps/Lvl_Bank'
mapfile=P/'Content/OneManStudio/Maps/Lvl_Bank.umap'
before=hashlib.sha256(mapfile.read_bytes()).hexdigest()
expected='96831951115fffe24f6a689a7c1f3486d180e7e09b74b88c01467b1f1423a62f'
if before!=expected:raise RuntimeError('Bank changed since inventory; refusing to overwrite: '+before)
backup=W/'output/recovery/BeforeFinalBank20261115';backup.mkdir(parents=True,exist_ok=True)
shutil.copy2(mapfile,backup/'Lvl_Bank.umap')
(backup/'sha256.txt').write_text(before,encoding='utf8')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level(MAP)
allactors=[a for a in actors.get_all_level_actors() if '/Lvl_Bank.' in a.get_path_name()]
by={a.get_actor_label():a for a in allactors}
report={'before':before,'changes':[],'markers':{},'backup':str(backup)}
def xyz(v):return [v.x,v.y,v.z]
def tag(a,t):a.tags=list(a.tags)+[unreal.Name(t)]
def spawn(cls,label,loc=(0,0,0),yaw=0):
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),unreal.Rotator(yaw=yaw))
    if not a:raise RuntimeError('Spawn failed '+label)
    a.set_actor_label(label);tag(a,'FinalBankRelease');return a
def bounds(a):return a.get_component_by_class(unreal.StaticMeshComponent).static_mesh.get_bounding_box()
def fit(a,dims):
    b=bounds(a);s=xyz(b.max-b.min);a.set_actor_scale3d(unreal.Vector(*[dims[i]/max(.001,s[i]) for i in range(3)]))
def place(a,xy,dims=None,yaw=0,support=None):
    if isinstance(a,str):a=by[a]
    if dims:fit(a,dims)
    a.set_actor_rotation(unreal.Rotator(yaw=yaw),False)
    a.set_actor_location(unreal.Vector(xy[0],xy[1],0),False,False)
    c,e=a.get_actor_bounds(False,False);p=a.get_actor_location()
    a.set_actor_location(unreal.Vector(p.x,p.y,(F if support is None else support)-(c.z-e.z)),False,False)
    c,e=a.get_actor_bounds(False,False)
    report['changes'].append({'label':a.get_actor_label(),'location':xyz(a.get_actor_location()),'size':xyz(e*2)})
    return c.z+e.z
palette='/Game/OneManStudio/Environment/Bank/FinalRelease'
unreal.EditorAssetLibrary.make_directory(palette)
def material(name,color):
    path=palette+'/'+name;m=unreal.load_asset(path)
    if not m:
        m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,palette,unreal.Material,unreal.MaterialFactoryNew())
        c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector)
        c.set_editor_property('constant',unreal.LinearColor(*color,1))
        unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant)
        rough.set_editor_property('r',.78)
        unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(m)
        unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m
cream=material('M_FinishedCream',(.72,.64,.46));navy=material('M_StudioNavy',(.035,.085,.14))
grass=material('M_ExteriorGreen',(.13,.23,.09));gold=material('M_Brass',(.62,.37,.075))
floor_mat=material('M_WarmFloor',(.63,.52,.35));ceilingmat=material('M_Ceiling',(.82,.77,.65))
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
def box(label,loc,dims,mat=cream,collision=True):
    a=spawn(unreal.StaticMeshActor,label,loc);c=a.static_mesh_component;c.set_static_mesh(cube)
    c.set_material(0,mat);a.set_actor_scale3d(unreal.Vector(*[d/100 for d in dims]))
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    return a
# Fix the oversized imported shell with its existing orientation correction.
shell=by['BANK - Meshy Diorama'];s=1980/(9509.0703125+9521.2197265625)
shell.set_actor_scale3d(unreal.Vector(s,s,-s));shell.set_actor_rotation(unreal.Rotator(roll=180),False)
shell.set_actor_location(unreal.Vector(0,0,0),False,False)
c,e=shell.get_actor_bounds(False,False);shell.set_actor_location(unreal.Vector(-c.x,-c.y,e.z-c.z),False,False)
surface=[]
for x,y in [(0,0),(0,-300),(200,0),(-200,0),(0,250)]:
    hit=unreal.StudioAssetCorrectionTools.trace_surface(shell.static_mesh_component,unreal.Vector(x,y,400),unreal.Vector(x,y,-100))
    if -20<hit.z<180:surface.append(hit.z)
# Negative-scale collision can remain stale during a commandlet transform edit.
# The previous import receipt measured this SAME floor at world Z=25.708449;
# carry that measured source plane through the exact scale/translation change.
F=(max(surface) if surface else ((25.70844924533765-844.1976958299979)/.25+3638.989990234375)*s)+2
report['floor_z']=F;report['measured_floor']=surface;report['floor_fallback']='Transformed existing measured source plane' if not surface else None
# Finished floor is continuous; very shallow decorative seams cannot catch physics props.
box('Finished bank floor',(0,0,F-8),(1430,1900,16),floor_mat)
for x in range(-660,661,110):box('Floor brass seam',(x,0,F+.08),(1.1,1880,.12),cream,False)
for y in range(-880,881,110):box('Floor brass seam',(0,y,F+.08),(1420,1.1,.12),cream,False)
ex=by['Bank exterior ground'];ex.static_mesh_component.set_material(0,grass);fit(ex,(4600,4600,30))
ex.set_actor_location(unreal.Vector(0,0,F-28),False,False)
box('Entrance exterior path',(-165,-1110,F-9),(310,430,16),cream)
roof=box('Finished cream ceiling',(0,0,F+620),(1460,1980,28),ceilingmat)
tag(roof,'FinalBankCeiling')
for x in (-650,650):
    a=box('Ceiling edge molding',(x,0,F+590),(30,1920,34),cream);tag(a,'FinalBankCeiling')
for y in (-930,930):
    a=box('Ceiling edge molding',(0,y,F+590),(1320,30,34),cream);tag(a,'FinalBankCeiling')
# Deliberate left-hand teller row. Preserve all original actor identities and imported art.
tops=[]
for i,y in enumerate((-500,-170,160),1):
    top=place(f'Teller desk {i}',(-470,y),(245,115,110),90);tops.append(top)
    place(f'Teller blue front {i}',(-407,y),(235,10,96),90)
    place(f'Teller cream lip {i}',(-401,y),(253,22,10),90,top-4)
    place(f'Teller laptop {i}',(-480,y+55),(55,38,40),90,top+.7)
    place(f'Teller stool {i}',(-620,y),(62,62,62))
    for label in (f'Teller reserve {i}',):by[label].set_actor_location(unreal.Vector(-620,y,F+90),False,False)
    sign=by[f'Counter sign {i}'];sign.set_actor_location(unreal.Vector(-394,y,F+65),False,False);sign.set_actor_rotation(unreal.Rotator(),False)
    sign.get_component_by_class(unreal.TextRenderComponent).set_text('CAJA '+str(i))
place('Bank notebook',(-455,-550),None,90,tops[0]+.5)
place('Bank ledger',(-455,210),None,90,tops[2]+.5)
place('Teller coffee',(-470,-230),None,0,tops[1]+.5)
place('Teller coffee2',(-490,90),None,0,tops[2]+.5)
place('Bank shelf',(-610,500),(155,60,265),90)
place('Bank shelf books',(-610,500),(125,31,72),90,F+62)
place('Bank shelf archive',(-610,500),(120,30,66),90,F+162)
# Waiting lounge on the right, with a walkable approach to both witnesses.
place('Front-right waiting bench',(555,-450),(245,110,135),90)
place('Waiting table',(365,-420),(85,65,58))
place('Waiting stool',(415,-650),(62,62,62))
for label,xy,d in [('Entrance planter left',(-570,-830),(85,85,160)),('Waiting planter',(570,-780),(80,80,145)),('Vault planter',(-510,790),(75,75,145)),('Waiting corner plant',(590,-40),(75,75,145))]:place(label,xy,d)
# The existing glass entrance stays open and framed, never blocking the exit trigger.
place('Open bank entrance door',(-300,-945),(225,26,265),65)
box('Entrance threshold',(-165,-935,F+3),(300,50,6),navy)
for x in (-325,-5):box('Entrance cream frame',(x,-948,F+145),(24,32,290),cream)
box('Entrance upper frame',(-165,-948,F+300),(344,32,28),cream)
# Actual vault room, with one guarded, recorded moving leaf and no decorative fake opening.
VX=-65;Y=480;VH=330
box('Vault rear wall',(VX,910,F+VH/2),(710,24,VH),navy)
for x in (-420,290):box('Vault side wall',(x,700,F+VH/2),(24,445,VH),navy)
box('Vault front left',(-320,Y,F+VH/2),(200,28,VH),navy)
box('Vault front right',(205,Y,F+VH/2),(170,28,VH),navy)
box('Vault entrance lintel',(VX,Y,F+315),(710,32,35),gold)
vaultroof=box('Vault ceiling',(VX,700,F+345),(730,450,24),ceilingmat);tag(vaultroof,'FinalBankCeiling')
door=spawn(unreal.load_class(None,'/Script/MyProject.StudioSetDoor'),'Vault - interactive recorded door',(-205,Y,F),-90)
leaf=door.get_editor_property('leaf');mesh=by['Vault door open steel leaf'].static_mesh_component.static_mesh
leaf.set_static_mesh(mesh);leaf.set_relative_rotation(unreal.Rotator(yaw=90),False,False)
leaf.set_relative_scale3d(unreal.Vector(280/95.37950134277344,1,3))
leaf.set_relative_location(unreal.Vector(0,140,150),False,False)
door.set_editor_properties({'door_id':'Scene.VaultDoor','open_angle':-100.0,'open_seconds':.65})
# Remove only the now-replaced static leaf/handle from this map; assets remain untouched.
for label in ('Vault door open steel leaf','Vault golden handle'):actors.destroy_actor(by[label])
lockers=[a for a in allactors if a.get_actor_label()=='Vault security locker']
for i,a in enumerate(lockers):place(a,(-310+i*155,830),(130,65,210),0)
# Keep the original locker detail grouping, moving each group with its original cabinet.
for a in allactors:
    if a.get_actor_label() in ('Locker door','Locker golden lock'):
        old=a.get_actor_location();index=(0 if old.y<0 else 2)+(1 if old.x>700 else 0)
        a.set_actor_location(unreal.Vector(-310+index*155,790,F+old.z),False,False)
        a.set_actor_rotation(unreal.Rotator(yaw=90),False)
plinth=place('Vault money plinth',(VX,700),(140,110,55))
goldbars=[a for a in allactors if a.get_actor_label()=='Gold bar']
for i,a in enumerate(goldbars):place(a,(-210+(i%3)*45,575+(i//3)*40),(35,25,17))
for i,a in enumerate(a for a in allactors if a.get_actor_label()=='Decorative vault money'):place(a,(180,570+i*75),(49,35,61))
for a in allactors:
    typ=a.get_class().get_name()
    if typ=='MoneyBagProp':place(a,(VX,700),support=plinth+.5)
    elif typ=='DonutProp':place(a,(340,180),support=F+70)
    elif typ=='BananaProp':place(a,(365,-420),support=F+58)
box('Guard snack pedestal',(340,180,F+35),(60,60,70),navy)
# Production corner: rail and camera point toward action; no equipment in the central route.
rail=by['Production camera rail'];rail.set_actor_location(unreal.Vector(505,280,F),False,False)
rail.set_actor_rotation(unreal.Rotator(yaw=90),False)
rail.set_editor_properties({'start_offset':unreal.Vector(-160,0,16),'end_offset':unreal.Vector(160,0,16),'look_at_world':unreal.Vector(0,-50,F+110)})
place('Secondary studio tripod',(570,510),(95,85,180),110)
place('Director chair',(430,620),(100,105,144),110)
place('Director clapperboard',(430,590),None,90,F+90)
for i,a in enumerate(a for a in allactors if a.get_actor_label()=='Equipment case'):place(a,(430+i*145,760),(100,70,60))
for i,a in enumerate(a for a in allactors if a.get_actor_label()=='Equipment case latch'):place(a,(430+i*145,722),support=F+30)
for i,a in enumerate(a for a in allactors if a.get_actor_label()=='Studio light stand'):place(a,(365+i*235,60),(60,60,185))
for i,a in enumerate(a for a in allactors if a.get_actor_label()=='Studio light panel'):place(a,(365+i*235,60),support=F+180)
notice=by['Film set notice'];notice.set_actor_location(unreal.Vector(610,370,F+220),False,False);notice.set_actor_rotation(unreal.Rotator(yaw=180),False)
notice.get_component_by_class(unreal.TextRenderComponent).set_text('FINAL TAKE / SET')
sign=by['Bank sign'];sign.set_actor_location(unreal.Vector(0,900,F+470),False,False);sign.set_actor_rotation(unreal.Rotator(yaw=-90),False)
sign.get_component_by_class(unreal.TextRenderComponent).set_text('FINAL BANK')
# Stable initial role/NPC points are consumed only once by the existing story session.
def marker(name,loc,yaw=0):
    a=spawn(unreal.TargetPoint,name,loc,yaw);tag(a,name);report['markers'][name]=loc;return a
marker('StorySceneAnchor',(0,0,F+100))
for role,xy,yaw in [('Robber',(-210,-400),180),('PoliceOfficer',(-100,-730),90),('Customer',(280,-520),180)]:marker('StorySpawn.'+role,(*xy,F+110),yaw)
for role,xy,yaw in [('Teller',(-620,-500),0),('Guard',(230,250),-90),('Witness1',(470,-220),180),('Witness2',(490,-640),180)]:marker('StorySpawn.NPC.'+role,(*xy,F+110),yaw)
marker('StoryExit',(-165,-1070,F+60));marker('StorySecure',(220,300,F+60));marker('StoryMoneySurface',(VX,700,plinth+.5))
safe=spawn(unreal.load_class(None,'/Script/MyProject.StudioBankBounds'),'Bank invisible safety perimeter',(0,0,F-2))
safe.set_editor_property('half_size',unreal.Vector2D(940,1260))
safe.set_actor_transform(safe.get_actor_transform(),False,False)
start=by['Movie player start'];start.set_actor_location(unreal.Vector(-210,-400,F+110),False,False);start.set_actor_rotation(unreal.Rotator(yaw=180),False)
cam=by['Movie premiere camera'];cam.set_actor_location(unreal.Vector(0,-800,F+550),False,False)
cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(cam.get_actor_location(),unreal.Vector(0,100,F+100)),False)
cam.get_component_by_class(unreal.CameraComponent).set_field_of_view(90)
# Original daylight rig retained. Shadow-free fill reaches the cream-ceiling interior.
by['Soft interior fill'].get_component_by_class(unreal.DirectionalLightComponent).set_editor_properties({'intensity':18000.0,'cast_shadows':False})
by['Bank daylight'].get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('intensity',45000.0)
for i,(x,y) in enumerate([(-300,-550),(300,-550),(-300,150),(300,150),(VX,690)],1):
    light=spawn(unreal.RectLight,'Warm ceiling fill '+str(i),(x,y,F+460));light.set_actor_rotation(unreal.Rotator(pitch=-90),False)
    c=light.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_editor_properties({'intensity':6500.0,'source_width':220.0,'source_height':140.0,'attenuation_radius':950.0,'cast_shadows':False,'use_temperature':True,'temperature':4600.0})
for a in allactors:
    if a.get_actor_label().startswith('Bank soft production fill'):
        c=a.get_component_by_class(unreal.RectLightComponent);c.set_editor_property('intensity',1500.0)
settings=by['Bank exposure and clear motion'].get_editor_property('settings')
settings.set_editor_properties({'auto_exposure_min_brightness':13.0,'auto_exposure_max_brightness':13.0,'override_motion_blur_amount':True,'motion_blur_amount':0.0})
by['Bank exposure and clear motion'].set_editor_property('settings',settings)
# Existing score zones remain, aligned with the final route.
for label,loc in [('Bank entry objective',(-165,-800,F+100)),('Bank counter objective',(-280,-170,F+100)),('Teller frontage objective extension',(-280,-400,F+100)),('Bank escape objective',(-165,-1070,F+100))]:by[label].set_actor_location(unreal.Vector(*loc),False,False)
if not levels.save_current_level():raise RuntimeError('Bank map save failed')
report['after']=hashlib.sha256(mapfile.read_bytes()).hexdigest()
out=P/'Saved/FinalRelease';out.mkdir(parents=True,exist_ok=True)
(out/'BankPolish.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('FINAL BANK saved existing map. Floor %.3f; %d arranged actors; stable markers and vault installed'%(F,len(report['changes'])))
