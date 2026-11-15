"""Offline authoring step: retain source FBX/UVs and export lightweight derivatives.

Run with portable Blender --background --python this_file. Blender is not shipped
with the game and the original supplied files are never edited.
"""
import bpy
import json
from pathlib import Path

PROJECT = Path(r"C:\Games\OneManStudio\MyProject")
SOURCE = PROJECT / "SourceArt/ProductionArt20261113"
DEST = SOURCE / "Derived"
DEST.mkdir(exist_ok=True)
report = []
for original in sorted((SOURCE / "objetos").rglob("*.fbx")):
    color = next(original.parent.rglob("*basecolor.jpg"))
    key = "".join(w.title() for w in color.stem.split("_3d_model")[0].split("_"))
    target = DEST / (key + ".fbx")
    if target.exists():
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(original), use_custom_normals=True)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    before = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in meshes)
    for obj in meshes:
        bpy.context.view_layer.objects.active = obj
        count = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        if count > 24000:
            mod = obj.modifiers.new("Runtime budget", "DECIMATE")
            mod.ratio = max(0.0001, 24000 / max(before, 1))
            mod.use_collapse_triangulate = True
            bpy.ops.object.modifier_apply(modifier=mod.name)
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
    bpy.ops.object.select_all(action="DESELECT")
    for obj in meshes:
        obj.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(target), use_selection=True,
                            object_types={"MESH"}, use_mesh_modifiers=True,
                            bake_anim=False, add_leaf_bones=False,
                            axis_forward="-Z", axis_up="Y", path_mode="AUTO")
    after = sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in meshes)
    report.append({"key": key, "original": str(original), "derived": str(target),
                   "source_triangles": before, "derived_triangles": after,
                   "uv_layers": [len(o.data.uv_layers) for o in meshes]})
    (PROJECT / "Saved/ProductionArt/PropOptimization.json").write_text(
        json.dumps(report, indent=2), encoding="utf8")
    print("PRODUCTION_PROP", key, before, after, flush=True)
