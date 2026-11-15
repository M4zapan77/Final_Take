"""Create only new media sources and the UI material for the director's three supplied videos."""
import json
import pathlib
import unreal

PROJECT = pathlib.Path(unreal.Paths.project_dir()).resolve()
ROOT = "/Game/OneManStudio/UI/StartupVideos"
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
assets.make_directory(ROOT)
created = []
def create(name, cls, factory):
    if assets.does_asset_exist(ROOT + "/" + name):
        raise RuntimeError("Refusing to overwrite " + ROOT + "/" + name)
    asset = tools.create_asset(name, ROOT, cls, factory)
    if not asset:
        raise RuntimeError("Could not create " + name)
    created.append(asset)
    return asset

for name, filename in [("MS_StudioIntro", "StudioIntro.mp4"), ("MS_GameIntro", "GameIntro.mp4"), ("MS_HubLoop", "HubLoop.mp4")]:
    source = create(name, unreal.FileMediaSource, unreal.FileMediaSourceFactoryNew())
    source.set_file_path(str(PROJECT / "Content" / "Movies" / "Startup" / filename))
    source.set_editor_property("precache_file", True)
    assets.save_loaded_asset(source, False)

texture = create("MT_StartupVideo", unreal.MediaTexture, unreal.MediaTextureFactoryNew())
texture.set_editor_properties({"auto_clear": True, "clear_color": unreal.LinearColor(0,0,0,1), "new_style_output": False})
assets.save_loaded_asset(texture, False)
material = create("M_StartupVideoUI", unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_properties({"material_domain": unreal.MaterialDomain.MD_UI, "blend_mode": unreal.BlendMode.BLEND_OPAQUE})
sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -300, 0)
sample.set_editor_properties({"parameter_name": "VideoTexture", "texture": texture, "sampler_type": unreal.MaterialSamplerType.SAMPLERTYPE_EXTERNAL})
unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
assets.save_loaded_asset(material, False)
audit = PROJECT / "Saved" / "StartupVideos"
audit.mkdir(parents=True, exist_ok=True)
audit.joinpath("Assets.json").write_text(json.dumps([item.get_path_name() for item in created], indent=2), encoding="utf-8")
unreal.log("StartupVideos: created three media sources, a media texture and UI material; no existing assets/maps saved")
