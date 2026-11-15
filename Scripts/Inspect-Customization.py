"""Read-only survey: final mesh, rig APIs, source animation assets and face pack."""
import json
import pathlib
import unreal

root = pathlib.Path(unreal.Paths.project_dir()).resolve()
out = root / "Saved" / "UnifiedCustomization"
out.mkdir(parents=True, exist_ok=True)
report = {}
for name in ("IKRigController", "IKRetargeterController", "IKRetargetBatchOperation", "IKRigDefinitionFactory", "IKRetargetFactory", "SkeletonModifier", "GeometryScript_AssetUtils", "GeometryScript_MeshQueryFunctions", "GeometryScript_MeshUVFunctions", "GeometryScript_MeshBasicEditFunctions", "GeometryScript_ListUtilityFunctions", "GeometryScript_MeshTransformFunctions", "AnimSequence", "AnimationLibrary", "AnimSequenceFactory", "IKRetargetGlobalSettings"):
    cls = getattr(unreal, name, None)
    report[name] = {"doc": cls.__doc__ if cls else None, "methods": {n: getattr(cls, n).__doc__ for n in dir(cls) if not n.startswith("_") and any(s in n.lower() for s in ("retarget", "chain", "skeletal", "mesh", "bone", "pose", "batch", "uv", "vertex", "animation", "root", "rig", "map", "setting"))}} if cls else None
mesh = unreal.load_asset("/Game/OneManStudio/Characters/Base3/SK_Base3")
component = unreal.new_object(unreal.SkeletalMeshComponent)
component.set_skeletal_mesh_asset(mesh)
info = {"mesh": mesh.get_path_name(), "bones": [str(component.get_bone_name(i)) for i in range(component.get_num_bones())], "materials": [str(x) for x in mesh.get_editor_property("materials")]}
try:
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(mesh)
    info["reference_pose"] = {n: str(modifier.get_bone_transform(n, True)) for n in info["bones"]}
except Exception as e:
    info["ref_error"] = str(e)
report["character"] = info
report["animations"] = list(unreal.EditorAssetLibrary.list_assets("/Game/Characters/Mannequins", True, False))
cls = unreal.load_class(None, "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")
player = unreal.get_default_object(cls)
body = player.get_component_by_class(unreal.SkeletalMeshComponent)
report["player"] = {"mesh": body.get_skeletal_mesh_asset().get_path_name(), "relative_transform": str(body.get_relative_transform()), "anim_class": str(body.get_editor_property("anim_class"))}
(out / "Inspection.json").write_text(json.dumps(report, indent=2, default=str), encoding="utf8")
unreal.log("Customization read-only inspection complete")
