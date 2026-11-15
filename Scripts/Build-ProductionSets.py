"""Place supplied art in the existing bank and a streamed theater. Preserve gameplay actor identities."""
import hashlib,json,pathlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/ProductionArt';assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);tools=unreal.AssetToolsHelpers.get_asset_tools();mel=unreal.MaterialEditingLibrary
receipt=json.loads((A/'ImportReceipt.json').read_text(encoding='utf8'));catalog=unreal.load_asset('/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics')
if not catalog:raise RuntimeError('Configure the production cosmetic catalog first')
level_receipt=A/'LevelReceipt.json';previous=json.loads(level_receipt.read_text()) if level_receipt.exists() else {}
report={'placed':[],'preserved_gameplay':[],'modified_pickups':[],'npc_markers':[]}
cinema_channels=unreal.LightingChannels();cinema_channels.set_editor_properties({'channel0':False,'channel1':True,'channel2':False})
BANK='/Game/OneManStudio/Maps/Lvl_Bank';CINEMA='/Game/OneManStudio/Maps/Lvl_Cinema';ROOT='/Game/OneManStudio/Environment/Cinema/ProductionArt/Shared'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def file_for(path):return P/'Content'/(path.removeprefix('/Game/')+'.umap')
baseline={e['path']:e['sha256'] for e in json.loads((A/'ProtectedBefore.json').read_text())}
bank_hash=digest(file_for(BANK));allowed=previous.get('bank_sha256',baseline['Content\\OneManStudio\\Maps\\Lvl_Bank.umap'])
if bank_hash!=allowed:raise RuntimeError('Bank changed independently; refusing to overwrite it')
if assets.does_asset_exist(CINEMA) and digest(file_for(CINEMA))!=previous.get('cinema_sha256'):raise RuntimeError('Cinema is not an unchanged generated map')

def spawn(cls,label,point,rotation=None,cinema=False):
    actor=actors.spawn_actor_from_class(cls,unreal.Vector(*point),rotation or unreal.Rotator());actor.set_actor_label(label)
    actor.set_editor_property('tags',['ProductionArtSet']+(['CinemaSet'] if cinema else []));return actor
def mesh_actor(mesh,material,label,point,scale,yaw=0,block=False,cinema=False,rotation=None):
    actor=spawn(unreal.StaticMeshActor,label,point,rotation or unreal.Rotator(yaw=yaw),cinema)
    component=actor.static_mesh_component;component.set_static_mesh(mesh)
    if cinema:component.set_editor_property('lighting_channels',cinema_channels)
    if material:component.set_material(0,material)
    actor.set_actor_scale3d(unreal.Vector(*scale));component.set_collision_profile_name('BlockAll' if block else 'NoCollision');component.set_editor_property('generate_overlap_events',False)
    report['placed'].append({'label':label,'mesh':mesh.get_path_name(),'location':list(point),'scale':list(scale),'blocking':block,'scene':'Cinema' if cinema else 'Bank'});return actor
def art(key,label,point,size=None,largest=None,yaw=0,block=False,cinema=False):
    r=receipt[key];mesh=unreal.load_asset(r['mesh']);s=[r['bounds_max'][i]-r['bounds_min'][i] for i in range(3)]
    scale=[size[i]/max(.01,s[i]) for i in range(3)] if size else [largest/max(s)]*3 if largest else [1]*3
    return mesh_actor(mesh,None,label,point,scale,yaw,block,cinema)

def material(name,color):
    path=ROOT+'/MI_'+name
    if assets.does_asset_exist(path):return assets.load_asset(path)
    m=tools.create_asset('MI_'+name,ROOT,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(m,assets.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
    mel.set_material_instance_vector_parameter_value(m,'Color',unreal.LinearColor(*color,1));assets.save_loaded_asset(m,False);return m
colors={k:material(k,v) for k,v in {'Cream':(.72,.60,.40),'Navy':(.025,.055,.12),'Mint':(.10,.43,.32),'Red':(.42,.012,.026),'Gold':(.85,.48,.045),'Steel':(.14,.17,.19),'Dark':(.012,.012,.018)}.items()}
cube=assets.load_asset('/Engine/BasicShapes/Cube');cylinder=assets.load_asset('/Engine/BasicShapes/Cylinder')
def box(label,point,size,color='Cream',block=False,cinema=False,rotation=None):return mesh_actor(cube,colors[color],label,point,[v/100 for v in size],block=block,cinema=cinema,rotation=rotation)
def cyl(label,point,size,color='Steel',cinema=False,rotation=None):return mesh_actor(cylinder,colors[color],label,point,[v/100 for v in size],cinema=cinema,rotation=rotation)
def marker(label,point,yaw,tags,cinema=False):
    actor=spawn(unreal.TargetPoint,label,point,unreal.Rotator(yaw=yaw),cinema);actor.set_editor_property('tags',list(actor.get_editor_property('tags'))+tags)
    report['npc_markers'].append({'label':label,'location':list(point),'tags':tags});return actor
def text(label,content,point,yaw,size=25,cinema=False):
    t=spawn(unreal.TextRenderActor,label,point,unreal.Rotator(yaw=yaw),cinema);c=t.get_component_by_class(unreal.TextRenderComponent);c.set_text(content);c.set_world_size(size);c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER);c.set_text_render_color(unreal.Color(r=245,g=196,b=80,a=255));return t

# New map uses a different offset in the same world, so the original performances stay alive.
if assets.does_asset_exist(CINEMA):levels.load_level(CINEMA)
else:
    if not levels.new_level(CINEMA,False):raise RuntimeError('Could not create cinema')
for actor in list(actors.get_all_level_actors()):
    if actor.actor_has_tag('ProductionArtSet'):actors.destroy_actor(actor)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_properties({'default_game_mode':unreal.GameModeBase,'force_no_precomputed_lighting':True})
Y=5000
box('Cinema floor',(-50,Y,-15),(1800,1400,30),'Dark',True,True)
box('Cinema screen wall',(800,Y,285),(30,1400,600),'Red',True,True)
box('Cinema left wall',(-50,Y-700,285),(1800,30,600),'Red',True,True)
box('Cinema right wall',(-50,Y+700,285),(1800,30,600),'Red',True,True)
box('Cinema rear wall',(-950,Y,285),(30,1400,600),'Red',True,True)
box('Cinema ceiling',(-50,Y,590),(1800,1400,20),'Dark',True,True)
for side in (-1,1):
    box('Golden theater wall trim',(-50,Y+side*680,100),(1750,8,12),'Gold',False,True)
    for x in (-750,-350,50,450):
        cyl('Theater warm wall column',(x,Y+side*665,260),(24,24,520),'Cream',True)
box('Screen gold frame',(728,Y,335),(22,940,545),'Gold',False,True)
box('Screen black border',(710,Y,335),(12,900,505),'Dark',False,True)
for side in (-1,1):
    for i in range(7):cyl('Red velvet curtain fold',(705-i%2*4,Y+side*(470+i*24),330),(45,45,530),'Red',True)

screen_mat_path=ROOT+'/M_MovieScreen'
if not assets.does_asset_exist(screen_mat_path):
    m=tools.create_asset('M_MovieScreen',ROOT,unreal.Material,unreal.MaterialFactoryNew());m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT);m.set_editor_property('two_sided',True)
    target=tools.create_asset('RT_ScreenDefault',ROOT,unreal.TextureRenderTarget2D,unreal.TextureRenderTargetFactoryNew());assets.save_loaded_asset(target,False)
    uv=mel.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate)
    flipu=mel.create_material_expression(m,unreal.MaterialExpressionScalarParameter);flipu.set_editor_properties({'parameter_name':'FlipU','default_value':0})
    flipv=mel.create_material_expression(m,unreal.MaterialExpressionScalarParameter);flipv.set_editor_properties({'parameter_name':'FlipV','default_value':0})
    tex=mel.create_material_expression(m,unreal.MaterialExpressionTextureObjectParameter);tex.set_editor_properties({'parameter_name':'MovieTexture','texture':target})
    n=mel.create_material_expression(m,unreal.MaterialExpressionCustom);n.set_editor_properties({'code':'float2 p=float2(lerp(UV.x,1-UV.x,U),lerp(UV.y,1-UV.y,V));return Texture2DSample(T,TSampler,p).rgb;','output_type':unreal.CustomMaterialOutputType.CMOT_FLOAT3})
    inputs=[]
    for k in ('UV','U','V','T'):
        item=unreal.CustomInput();item.set_editor_property('input_name',k);inputs.append(item)
    n.set_editor_property('inputs',inputs)
    for k,v in {'UV':uv,'U':flipu,'V':flipv,'T':tex}.items():assert mel.connect_material_expressions(v,'',n,k)
    inv=mel.create_material_expression(m,unreal.MaterialExpressionEyeAdaptationInverse);names=mel.get_material_expression_input_names(inv);assert mel.connect_material_expressions(n,'',inv,names[0])
    one=mel.create_material_expression(m,unreal.MaterialExpressionConstant);one.set_editor_property('r',1);assert mel.connect_material_expressions(one,'',inv,names[1])
    assert mel.connect_material_property(inv,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);mel.recompile_material(m);assets.save_loaded_asset(m,False)
presenter=spawn(unreal.load_class(None,'/Script/MyProject.StudioCinemaPresenter'),'ONE MAN STUDIO - Theater screen',(695,Y,335),cinema=True)
presenter.set_editor_property('screen_material',assets.load_asset(screen_mat_path))
for row,x in enumerate((-600,-300,0)):
    box('Audience riser',(x,Y,6+row*6),(260,1020,12+row*12),'Dark',True,True)
    for seat,y in enumerate((-420,-220,220,420)):
        art('Sillon_SalaDeCine',f'Cinema seat {row+1}-{seat+1}',(x,Y+y,70+row*12),size=(92,100,140),yaw=-90,cinema=True)
        marker(f'Audience NPC {row+1}-{seat+1}',(x,Y+y,75+row*12),0,['NPC.Audience','CharacterProfile.Base3'],True)
        if row==1 and seat in (0,3):art('Palomitas','Cinema popcorn',(x+15,Y+y+54,100+row*12),largest=28,cinema=True)
for x in (-600,0,500):
    for side in (-1,1):
        art('Lampara_Colgante1','Theater ceiling fixture',(x,Y+side*560,500),size=(130,35,160),cinema=True)
    light=spawn(unreal.PointLight,'Warm cinema light',(x,Y,430),cinema=True);c=light.get_component_by_class(unreal.PointLightComponent);c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_editor_properties({'intensity_units':unreal.LightUnits.LUMENS,'intensity':8000.0,'attenuation_radius':800.0,'cast_shadows':False,'light_color':unreal.Color(r=255,g=211,b=161,a=255),'lighting_channels':cinema_channels})
text('Cinema title','ONE MAN CINEMA',(680,Y,600),180,40,True)
art('Palomitas','Lobby popcorn',(-770,Y-560,100),largest=45,cinema=True)
levels.save_current_level();cinema_hash=digest(file_for(CINEMA))
level_receipt.write_text(json.dumps({'bank_sha256':bank_hash,'cinema_sha256':cinema_hash,'construction_in_progress':True},indent=2),encoding='utf8')

# Reuse the bank shell and every gameplay manager/trigger/player start/camera already saved there.
if not levels.load_level(BANK):raise RuntimeError('Could not open existing bank')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not unreal.GameplayStatics.get_streaming_level(world,CINEMA):
    streaming=unreal.EditorLevelUtils.add_level_to_world(world,CINEMA,unreal.LevelStreamingAlwaysLoaded)
    if not streaming:unreal.log_warning('Cinema streaming level may already be present')
if not levels.set_current_level_by_name('Lvl_Bank'):raise RuntimeError('Could not make bank the current level')
for actor in list(actors.get_all_level_actors()):
    if actor.get_outer().get_path_name().startswith(CINEMA):continue
    label=actor.get_actor_label()
    if actor.actor_has_tag('ProductionArtSet') and not actor.actor_has_tag('CinemaSet'):actors.destroy_actor(actor)
    elif label in ('Bank counter','Counter top') and isinstance(actor,unreal.StaticMeshActor) and actor.static_mesh_component.static_mesh.get_path_name().startswith('/Engine/BasicShapes/'):
        report.setdefault('replaced_placeholders',[]).append(label);actors.destroy_actor(actor)
    elif isinstance(actor,(unreal.PlayerStart,unreal.CameraActor)) or actor.get_class().get_name() in ('MovieManager','ObjectiveZone'):
        t=actor.get_actor_transform();report['preserved_gameplay'].append({'name':actor.get_name(),'class':actor.get_class().get_name(),'location':[t.translation.x,t.translation.y,t.translation.z]})

for i,x in enumerate((-350,-50,250)):
    art('Escritorio',f'Teller desk {i+1}',(x,-420,50),size=(225,125,100),block=True)
    box(f'Teller blue front {i+1}',(x,-353,43),(215,10,80),'Navy',True)
    box(f'Teller cream lip {i+1}',(x,-350,102),(235,22,12),'Cream')
    art('Laptop',f'Teller laptop {i+1}',(x,-420,120),largest=65,yaw=180)
    art('Banquito_Amarillo',f'Teller stool {i+1}',(x,-550,30),size=(60,60,60))
    marker(f'Teller reserve {i+1}',(x,-545,96),90,['NPC.Teller','CharacterProfile.Base3'])
    text(f'Counter sign {i+1}',f'CAJA 0{i+1}',(x,-341,72),90,21)
art('Libreta','Bank notebook',(-290,-400,113),largest=35,yaw=90)
art('Libro','Bank ledger',(300,-420,112),largest=32)
art('Tasa','Teller coffee',(-15,-430,115),largest=20)
art('Estanteria','Bank shelf',(-550,-585,145),size=(150,65,290),block=True)
art('Libros_Estanteria','Bank shelf books',(-550,-580,115),largest=110)
art('Libros_Estanteria2','Bank shelf archive',(-550,-580,220),largest=105)
counter_zone=spawn(unreal.load_class(None,'/Script/MyProject.ObjectiveZone'),'Teller frontage objective extension',(-50,-225,110))
counter_zone.set_editor_property('zone_tag','Bank.Counter');counter_zone.get_editor_property('bounds').set_box_extent(unreal.Vector(470,100,180),False);counter_zone.set_editor_property('show_floor_marker',False);counter_zone.get_editor_property('floor_marker').set_visibility(False)

art('Sillon_Azul','Front-right waiting bench',(-500,475,60),size=(240,100,120),yaw=180,block=True)
art('Sillon_Amarillo','Waiting table',(-280,350,30),size=(90,65,60),block=True)
art('Banquito_Amarillo','Waiting stool',(-160,460,30),size=(60,60,60),block=True)
art('Planta_1','Entrance planter left',(-780,-550,90),largest=180)
art('Planta_2','Waiting planter',(-710,540,80),largest=160)
art('Planta_3','Vault planter',(730,-560,90),largest=180)
art('Planta_4','Waiting corner plant',(-80,565,85),largest=170)
art('Puerta_Vidrio','Open bank entrance door',(-875,155,125),size=(190,22,250),yaw=70)
for i,p in enumerate(((-600,375,96),(-420,430,96),(-180,375,96))):marker(f'Customer reserve {i+1}',p,-90,['NPC.Customer','CharacterProfile.Base3'])
marker('Security guard reserve',(500,-220,96),180,['NPC.Security','CharacterProfile.Base3'])

# The archive has no separate vault door or rail. Keep these small configurable stage constructions.
cyl('Vault door open steel leaf',(565,-190,125),(225,225,16),'Steel',rotation=unreal.Rotator(pitch=90,yaw=55))
cyl('Vault golden handle',(580,-185,125),(58,58,12),'Gold',rotation=unreal.Rotator(pitch=90,yaw=55))
for side in (-1,1):
    for x in (700,865):
        box('Vault security locker',(x,side*490,92),(130,95,170),'Steel',True)
        for z in (45,95,145):
            box('Locker door',(x-69,side*490,z),(5,80,40),'Navy')
            cyl('Locker golden lock',(x-74,side*490, z),(10,10,8),'Gold',rotation=unreal.Rotator(pitch=90))
box('Vault money plinth',(700,0,30),(130,120,60),'Mint',True)
for i in range(6):box('Gold bar',(740+(i%2)*35,250+(i//2)*35,15),(30,24,20),'Gold')
for i in range(3):art('MoneyBag','Decorative vault money',(800,260+i*90,38),largest=65)

rail=spawn(unreal.load_class(None,'/Script/MyProject.StudioRailRig'),'Production camera rail',(280,520,0))
rail.get_editor_property('camera_model').set_static_mesh(unreal.load_asset(receipt['Camara2_Baja']['mesh']))
for component in rail.get_components_by_class(unreal.StaticMeshComponent):
    if component!=rail.get_editor_property('camera_model'):component.set_material(0,colors['Steel'])
art('Camara1_Alta','Secondary studio tripod',(510,350,90),size=(95,85,180),yaw=180)
art('Silla_Director','Director chair',(0,450,72),size=(100,105,144),yaw=180)
for i,x in enumerate((80,550)):
    box('Equipment case',(x,590,30),(120,70,60),'Navy');box('Equipment case latch',(x,551,35),(35,5,18),'Gold')
    box('Studio light panel',(x,320,180),(90,12,65),'Cream',rotation=unreal.Rotator(yaw=-20 if i else 20));cyl('Studio light stand',(x,320,85),(8,8,170),'Steel')
art('Clapperboard','Director clapperboard',(0,375,100),largest=52,yaw=180)
text('Film set notice','SILENCIO - RODANDO',(400,615,215),180,26)

# Soft, shadow-free practical fill keeps the same recorded actors legible in
# both the gameplay view and the theater SceneCapture (which has its own GI).
for index,point in enumerate(((-500,-80,650),(350,-80,650),(0,700,550))):
    light=spawn(unreal.RectLight,'Bank soft production fill '+str(index+1),point)
    light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*point),unreal.Vector(0,0,110)),False)
    component=light.get_component_by_class(unreal.RectLightComponent);component.set_mobility(unreal.ComponentMobility.MOVABLE)
    component.set_editor_properties({'intensity_units':unreal.LightUnits.LUMENS,'intensity':12000.0,'attenuation_radius':1500.0,'source_width':300.0,'source_height':200.0,'cast_shadows':False,'light_color':unreal.Color(r=255,g=228,b=196,a=255)})

by_id={str(item.get_editor_property('id')):item for item in catalog.get_editor_property('cosmetics')}
for actor in actors.get_all_level_actors():
    if actor.get_class().get_name()=='MovieManager':
        # Merge through the component's current native defaults, preserving all face entries and role data.
        defaults=list(unreal.get_default_object(unreal.WardrobeComponent).get_editor_property('catalog'))
        for override in catalog.get_editor_property('cosmetics'):
            matches=[i for i,item in enumerate(defaults) if item.get_editor_property('id')==override.get_editor_property('id')]
            if matches:defaults[matches[0]]=override
            else:defaults.append(override)
        actor.set_editor_property('cosmetic_catalog',defaults)
    if actor.get_class().get_name() in ('MoneyBagProp','DonutProp','BananaProp'):
        prop_id=str(actor.get_editor_property('prop_id'));definition=by_id[prop_id]
        actor.set_editor_property('equipped_visual',definition);component=actor.get_editor_property('pickup_mesh');component.set_static_mesh(definition.get_editor_property('static_mesh'))
        for i,mat in enumerate(definition.get_editor_property('materials')):component.set_material(i,mat)
        actor.set_actor_scale3d(unreal.Vector(.38 if prop_id=='Prop.MoneyBag' else .24,.38 if prop_id=='Prop.MoneyBag' else .24,.38 if prop_id=='Prop.MoneyBag' else .24))
        if prop_id=='Prop.MoneyBag':actor.set_actor_location(unreal.Vector(700,0,82),False,False)
        elif prop_id=='Prop.Donut':actor.set_actor_location(unreal.Vector(10,-400,124),False,False)
        else:actor.set_actor_location(unreal.Vector(250,-400,124),False,False)
        report['modified_pickups'].append({'name':actor.get_name(),'prop_id':prop_id,'pickup_id':str(actor.get_editor_property('pickup_id'))})

if not levels.save_current_level():raise RuntimeError('Could not save bank')
level_receipt.write_text(json.dumps({'bank_sha256':digest(file_for(BANK)),'cinema_sha256':cinema_hash,'bank':BANK,'cinema':CINEMA},indent=2),encoding='utf8')
(A/'PlacementReport.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('Production sets saved; existing managers, spawn transforms and objective actors retained')
