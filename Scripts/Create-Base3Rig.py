"""Create new editable IK rigs/retargeter and retarget copies; never save the source mesh/skeleton."""
import json
import pathlib
import unreal

project = pathlib.Path(unreal.Paths.project_dir()).resolve()
audit = project / "Saved" / "UnifiedCustomization"
audit.mkdir(parents=True, exist_ok=True)
root = "/Game/OneManStudio/Characters/Base3/Customization"
library = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
source = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
target = unreal.load_asset("/Game/OneManStudio/Characters/Base3/SK_Base3")
(audit / "MeshSurvey.json").write_text(unreal.StudioCharacterAssetTools.describe_mesh(target), encoding="utf8")

def create(name, cls, factory, folder=root + "/Rigs"):
    path = folder + "/" + name
    existing = library.load_asset(path) if library.does_asset_exist(path) else None
    if existing:
        # These three assets were created by this integration before its first retarget attempt.
        allowed = ("IK_Quinn_Source", "IK_Base3_Target", "RTG_Quinn_To_Base3")
        if name not in allowed or not (audit / "AllowRigResume.json").exists():
            raise RuntimeError("Refusing to overwrite existing rig: " + path)
        return existing
    return tools.create_asset(name, folder, cls, factory)

chains = [
    ("Spine", "spine_01", "spine_05", "Spine", "Spine1"),
    ("Neck", "neck_01", "neck_02", "Neck", "Neck"),
    ("Head", "head", "head", "Head", "Head"),
    ("LeftClavicle", "clavicle_l", "clavicle_l", "LeftShoulder", "LeftShoulder"),
    ("RightClavicle", "clavicle_r", "clavicle_r", "RightShoulder", "RightShoulder"),
    ("LeftArm", "upperarm_l", "hand_l", "LeftArm", "LeftHand"),
    ("RightArm", "upperarm_r", "hand_r", "RightArm", "RightHand"),
    ("LeftLeg", "thigh_l", "foot_l", "LeftUpLeg", "LeftFoot"),
    ("RightLeg", "thigh_r", "foot_r", "RightUpLeg", "RightFoot"),
]
rigs = []
for name, mesh, pelvis, offset in (("IK_Quinn_Source", source, "pelvis", 1), ("IK_Base3_Target", target, "Hips", 3)):
    rig = create(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    assert controller.set_skeletal_mesh(mesh)
    assert controller.set_retarget_root(pelvis)
    for old in controller.get_retarget_chains():
        controller.remove_retarget_chain(old.chain_name)
    for chain in chains:
        controller.add_retarget_chain(chain[0], chain[offset], chain[offset + 1], "None")
    library.save_loaded_asset(rig)
    rigs.append(rig)

retargeter = create("RTG_Quinn_To_Base3", unreal.IKRetargeter, unreal.IKRetargetFactory())
controller = unreal.IKRetargeterController.get_controller(retargeter)
controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rigs[0])
controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rigs[1])
unreal.log("Retargeter initial op count: " + str(controller.get_num_retarget_ops()))
if controller.get_num_retarget_ops() == 0:
    for op in ("IKRetargetCopyBasePoseOp", "IKRetargetPelvisMotionOp", "IKRetargetFKChainsOp"):
        index = controller.add_retarget_op("/Script/IKRig." + op)
        unreal.log("Adding retarget op: " + op + " => " + str(index))
        if index < 0:
            raise RuntimeError("Required retarget operation could not be added")
controller.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
controller.auto_align_bones(["Spine", "Spine1", "Neck", "Head", "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "LeftUpLeg", "LeftLeg", "LeftFoot", "RightUpLeg", "RightLeg", "RightFoot"], unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN, unreal.RetargetSourceOrTarget.TARGET)
library.save_loaded_asset(retargeter)
paths = [
    "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle",
    "/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Fall_Loop",
    "/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land",
    "/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01",
    "/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS",
]
assets = [unreal.load_asset(p) for p in paths]
if not all(assets):
    raise RuntimeError("A required source animation is missing")
existing_paths = [root + "/Animations/B3_" + a.get_name() for a in assets]
copies = [library.load_asset(p) for p in existing_paths] if all(library.does_asset_exist(p) for p in existing_paths) else unreal.StudioCharacterAssetTools.retarget_animations(assets, source, target, retargeter, root + "/Animations")
if len(copies) != len(assets):
    raise RuntimeError("Retargeting did not produce all requested animation copies")
for asset in copies:
    asset.set_editor_property("enable_root_motion", False)
    asset.set_editor_property("force_root_lock", True)
    unreal.AnimationLibrary.remove_all_animation_notify_tracks(asset)
    library.save_loaded_asset(asset)
surface_path=root+"/Meshes/SM_Base3_FaceFront"
surface = library.load_asset(surface_path) if library.does_asset_exist(surface_path) else unreal.StudioCharacterAssetTools.build_face_surface(target, surface_path)
if not surface:
    raise RuntimeError("Unable to construct a conforming head surface")
library.save_loaded_asset(surface)
(audit / "RigAssets.json").write_text(json.dumps({"rigs": [r.get_path_name() for r in rigs], "retargeter": retargeter.get_path_name(), "animations": [a.get_path_name() for a in copies], "face_surface": surface.get_path_name()}, indent=2), encoding="utf8")
unreal.log("Base3 IK retargeting and head surface completed")
