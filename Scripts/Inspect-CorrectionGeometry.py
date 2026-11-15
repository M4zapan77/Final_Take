"""Read current mesh geometry and bone/material configuration without editing Content."""
import json,pathlib,unreal
P=pathlib.Path(unreal.Paths.project_dir()).resolve();A=P/'Saved/FinalCorrections';A.mkdir(exist_ok=True)
receipt=json.loads((P/'Saved/ProductionArt/ImportReceipt.json').read_text(encoding='utf8'))
for key in ('RobberMask','Glasses','Sunglasses','Mustache','PoliceCap','Beanie','Crown','Cowboy','Chicken','Banana','HandheldMicrophone','MoneyBag','Donut','Megaphone','RubberDuck'):
    mesh=unreal.load_asset(receipt[key]['mesh'])
    (A/(key+'.json')).write_text(unreal.StudioAssetCorrectionTools.describe_static_mesh(mesh),encoding='utf8')
profile=unreal.load_asset('/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character')
(A/'Body.json').write_text(unreal.StudioCharacterAssetTools.describe_mesh(profile.get_editor_property('body_mesh')),encoding='utf8')
face=profile.get_editor_property('face')
(A/'CharacterReference.json').write_text(json.dumps({'mesh_transform':str(profile.get_editor_property('mesh_transform')),'face_transform':str(face.get_editor_property('relative_transform')),'eyes':str(face.get_editor_property('eyes')),'mouth':str(face.get_editor_property('mouth'))},indent=2),encoding='utf8')
unreal.log('Correction geometry exported; no Content changes')
