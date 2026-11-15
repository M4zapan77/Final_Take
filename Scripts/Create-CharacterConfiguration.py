"""Create the editable locomotion graph and model profile. No character/map Blueprint is overwritten."""
import json
import pathlib
import unreal

project=pathlib.Path(unreal.Paths.project_dir()).resolve()
root="/Game/OneManStudio/Characters/Base3/Customization"
library=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools=unreal.AssetToolsHelpers.get_asset_tools()
mesh=unreal.load_asset("/Game/OneManStudio/Characters/Base3/SK_Base3")
animations={name:unreal.load_asset(root+"/Animations/B3_"+name) for name in ("MM_Idle","MF_Unarmed_Walk_Fwd","MF_Unarmed_Jog_Fwd","MM_Jump","MM_Fall_Loop","MM_Death_Front_01")}
abp_path=root+"/Animations/ABP_Base3"
abp=library.load_asset(abp_path) if library.does_asset_exist(abp_path) else unreal.StudioCharacterAssetTools.create_locomotion_blueprint(mesh,animations["MM_Idle"],animations["MF_Unarmed_Walk_Fwd"],animations["MF_Unarmed_Jog_Fwd"],animations["MM_Jump"],animations["MM_Fall_Loop"],abp_path)
if not abp:
    raise RuntimeError("The locomotion graph did not compile")
library.save_loaded_asset(abp)
gestures=[]
for identifier in ("Dance","Point","Scared","Faint","Arrest"):
    if identifier=="Faint":
        clip=animations["MM_Death_Front_01"]
    else:
        clip=unreal.StudioCharacterAssetTools.create_placeholder_gesture(mesh,animations["MM_Idle"],identifier,root+"/Animations/B3_"+identifier)
        if not clip:
            raise RuntimeError("Could not create gesture " + identifier)
        library.save_loaded_asset(clip)
    gesture=unreal.ActingGestureDefinition()
    gesture.set_editor_properties({"identifier":identifier,"animation":clip,"loop":identifier=="Dance","duration_seconds":3.0})
    gestures.append(gesture)
factory=unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class",unreal.StudioCharacterProfile)
profile=tools.create_asset("DA_Base3Character",root,unreal.StudioCharacterProfile,factory)
if not profile:
    raise RuntimeError("Could not create the character profile")
face=unreal.FacialSurfaceSettings()
surface=unreal.load_asset(root+"/Meshes/SM_Base3_FaceFront") if library.does_asset_exist(root+"/Meshes/SM_Base3_FaceFront") else unreal.StudioCharacterAssetTools.build_face_surface(mesh,root+"/Meshes/SM_Base3_FaceFront")
if not surface:
    raise RuntimeError("Could not create the forward-facing head surface")
library.save_loaded_asset(surface)
face.set_editor_properties({"surface_mesh":surface,"material":unreal.load_asset(root+"/Materials/M_Base3Face"),"none_texture":unreal.load_asset("/Game/OneManStudio/Cosmetics/T_Face_None"),"head_bone":"Head"})
retargeted={}
rig_report=json.loads((project/"Saved/UnifiedCustomization/RigAssets.json").read_text(encoding="utf8"))
original_paths=["/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle","/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd","/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd","/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump","/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop","/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land","/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01","/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS"]
for original in original_paths:
    retargeted[unreal.load_asset(original)]=unreal.load_asset(root+"/Animations/B3_"+original.split("/")[-1])
profile.set_editor_properties({"body_mesh":mesh,"animation_class":abp.generated_class(),"skin_material":unreal.load_asset(root+"/Materials/M_Base3Skin"),"face":face,"default_gestures":gestures,"retargeted_animations":retargeted})
library.save_loaded_asset(profile)
(project/"Saved/UnifiedCustomization/CharacterConfiguration.json").write_text(json.dumps({"profile":profile.get_path_name(),"animation_blueprint":abp.get_path_name(),"gestures":[str(g.identifier) for g in gestures]},indent=2),encoding="utf8")
unreal.log("Base3 character profile, locomotion graph and five in-place gestures created")
