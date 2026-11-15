"""Import the supplied PNGs unchanged and create configurable skin, face and preview materials."""
import hashlib
import json
import pathlib
import unreal

project = pathlib.Path(unreal.Paths.project_dir()).resolve()
source = project / "SourceArt" / "FacialAssets"
audit = project / "Saved" / "UnifiedCustomization"
library = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
textures = []
for png in sorted(source.rglob("*.png")):
    folder = "/Game/OneManStudio/Cosmetics" + ("/" + png.parent.name if png.parent != source else "")
    path = folder + "/" + png.stem
    exists = library.does_asset_exist(path)
    if exists and not (audit / "AllowFaceResume.json").exists():
        raise RuntimeError("Refusing to overwrite a facial texture: " + path)
    if exists:
        texture = library.load_asset(path)
    else:
        task = unreal.AssetImportTask()
        task.set_editor_properties({"filename": str(png), "destination_path": folder, "destination_name": png.stem, "automated": True, "save": False, "replace_existing": False, "factory": unreal.TextureFactory()})
        tools.import_asset_tasks([task])
        texture = task.get_objects()[0]
        texture.set_editor_properties({"srgb": True, "compression_settings": unreal.TextureCompressionSettings.TC_EDITOR_ICON, "lod_group": unreal.TextureGroup.TEXTUREGROUP_UI, "address_x": unreal.TextureAddress.TA_CLAMP, "address_y": unreal.TextureAddress.TA_CLAMP})
        library.save_loaded_asset(texture)
    textures.append({"source": str(png), "sha256": hashlib.sha256(png.read_bytes()).hexdigest(), "asset": texture.get_path_name()})
if len(textures) != 22:
    raise RuntimeError("Expected exactly 22 supplied facial textures")
root = "/Game/OneManStudio/Characters/Base3/Customization/Materials"
mel = unreal.MaterialEditingLibrary

def material(name, ui=False, translucent=False):
    if library.does_asset_exist(root + "/" + name):
        if not (audit / "AllowMaterialRebuild.json").exists():
            raise RuntimeError("Refusing to overwrite an existing material: " + name)
        mat=library.load_asset(root+"/"+name)
        # Unroot this integration's startup-loaded expression subobjects before rebuilding.
        assert unreal.StudioCharacterAssetTools.reset_customization_material(mat)
    else:
        mat = tools.create_asset(name, root, unreal.Material, unreal.MaterialFactoryNew())
    if ui:
        mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    if translucent:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    return mat

def node(mat, cls, **props):
    n = mel.create_material_expression(mat, cls)
    n.set_editor_properties(props)
    return n

def custom(mat, code, inputs, output=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    n = node(mat, unreal.MaterialExpressionCustom, code=code, output_type=output)
    custom_inputs=[]
    for name in inputs:
        item=unreal.CustomInput()
        item.set_editor_property("input_name",name)
        custom_inputs.append(item)
    n.set_editor_property("inputs", custom_inputs)
    for name, expr in inputs.items():
        if not mel.connect_material_expressions(expr, "", n, name):
            raise RuntimeError("Could not connect custom material input: "+name)
    return n

def save(mat):
    mel.layout_material_expressions(mat)
    mel.recompile_material(mat)
    library.save_loaded_asset(mat)

skin = material("M_Base3Skin")
p = node(skin, unreal.MaterialExpressionPreSkinnedPosition)
interpolator=node(skin,unreal.MaterialExpressionVertexInterpolator)
assert mel.connect_material_expressions(p,"",interpolator,"")
mel.set_material_usage(skin, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
skin_color = node(skin, unreal.MaterialExpressionVectorParameter, parameter_name="SkinColor", default_value=unreal.LinearColor(1,1,1,1))
cloth = node(skin, unreal.MaterialExpressionVectorParameter, parameter_name="Color", default_value=unreal.LinearColor(.12,.18,.34,1))
style = node(skin, unreal.MaterialExpressionScalarParameter, parameter_name="OutfitStyle", default_value=0)
body_uv=node(skin,unreal.MaterialExpressionTextureCoordinate)
clothing_mask=node(skin,unreal.MaterialExpressionTextureObjectParameter,parameter_name="ClothingMask",texture=unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
use_mask=node(skin,unreal.MaterialExpressionScalarParameter,parameter_name="UseClothingMask",default_value=0)
rgb = custom(skin, """
float torso=step(40,P.z)*(1-step(102,P.z))*(1-step(28,abs(P.x)));
float legs=step(14,P.z)*(1-step(58,P.z));
float hands=step(28,abs(P.x))*(1-step(79,P.z));
float suit=Style>=1.5 ? max(torso,legs) : torso;
float dress=step(26,P.z)*(1-step(96,P.z))*(1-step(28,abs(P.x)));
float mask=Style>=3.5 ? dress : suit;
mask*=step(.5,Style)*(1-hands)*(1-step(104,P.z));
mask=lerp(mask,Texture2DSample(MaskTex,MaskTexSampler,UV).r*step(.5,Style),saturate(UseMask));
float collar=step(88,P.z)*torso*(1-step(6,abs(P.x))) * (Style>=2.5 && Style<3.5);
return lerp(lerp(Skin.rgb,Cloth.rgb,mask),float3(.96,.93,.85),collar);
""", {"P": interpolator, "Skin": skin_color, "Cloth": cloth, "Style": style,"UV":body_uv,"MaskTex":clothing_mask,"UseMask":use_mask})
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(skin, unreal.MaterialExpressionScalarParameter, parameter_name="Roughness", default_value=.65)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
save(skin)

face = material("M_Base3Face", translucent=True)
face.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
uv = node(face, unreal.MaterialExpressionTextureCoordinate)
none = unreal.load_asset("/Game/OneManStudio/Cosmetics/T_Face_None")
inputs = {"UV": uv}
defaults = {"Eyes": (.5,.42,.9,.7), "Mouth": (.5,.72,.62,.65), "Eyebrows": (.5,.22,.85,.8)}
for name, layout in defaults.items():
    inputs[name + "Tex"] = node(face, unreal.MaterialExpressionTextureObjectParameter, parameter_name=name + "Texture", texture=none)
    layout_parameter = node(face, unreal.MaterialExpressionVectorParameter, parameter_name=name + "Layout", default_value=unreal.LinearColor(*layout))
    layout_rgba=node(face,unreal.MaterialExpressionAppendVector)
    assert mel.connect_material_expressions(layout_parameter,"",layout_rgba,"A")
    assert mel.connect_material_expressions(layout_parameter,"A",layout_rgba,"B")
    inputs[name + "Layout"] = layout_rgba
    inputs[name + "Visible"] = node(face, unreal.MaterialExpressionScalarParameter, parameter_name=name + "Visible", default_value=1)
code = """
float2 e=(UV-EyesLayout.xy)/EyesLayout.zw+.5;
float2 m=(UV-MouthLayout.xy)/MouthLayout.zw+.5;
float2 b=(UV-EyebrowsLayout.xy)/EyebrowsLayout.zw+.5;
float4 E=Texture2DSample(EyesTex,EyesTexSampler,e);
float4 M=Texture2DSample(MouthTex,MouthTexSampler,m);
float4 B=Texture2DSample(EyebrowsTex,EyebrowsTexSampler,b);
E.a*=all(e>=0)&&all(e<=1) ? EyesVisible : 0;
M.a*=all(m>=0)&&all(m<=1) ? MouthVisible : 0;
B.a*=all(b>=0)&&all(b<=1) ? EyebrowsVisible : 0;
float A=1-(1-E.a)*(1-M.a)*(1-B.a);
float3 RGB=(E.rgb*E.a*(1-M.a)*(1-B.a)+M.rgb*M.a*(1-B.a)+B.rgb*B.a)/max(A,.0001);
return float4(RGB,A);
"""
composite = custom(face, code, inputs, unreal.CustomMaterialOutputType.CMOT_FLOAT4)
color = node(face, unreal.MaterialExpressionComponentMask, r=True, g=True, b=True, a=False)
alpha = node(face, unreal.MaterialExpressionComponentMask, r=False, g=False, b=False, a=True)
assert mel.connect_material_expressions(composite, "", color, "")
assert mel.connect_material_expressions(composite, "", alpha, "")
eye_inverse=node(face,unreal.MaterialExpressionEyeAdaptationInverse)
inverse_names=mel.get_material_expression_input_names(eye_inverse)
assert mel.connect_material_expressions(color,"",eye_inverse,inverse_names[0])
full_inverse=node(face,unreal.MaterialExpressionConstant,r=1)
assert mel.connect_material_expressions(full_inverse,"",eye_inverse,inverse_names[1])
mel.connect_material_property(eye_inverse, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
save(face)

preview = material("M_CharacterPreviewUI", ui=True, translucent=True)
target=library.load_asset(root+"/RT_CharacterPreview") if library.does_asset_exist(root+"/RT_CharacterPreview") else tools.create_asset("RT_CharacterPreview",root,unreal.TextureRenderTarget2D,unreal.TextureRenderTargetFactoryNew())
target.set_editor_properties({"size_x":1024,"size_y":1024,"render_target_format":unreal.TextureRenderTargetFormat.RTF_RGBA8})
library.save_loaded_asset(target)
texture = node(preview, unreal.MaterialExpressionTextureSampleParameter2D, parameter_name="PreviewTexture", texture=target,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
assert mel.connect_material_property(texture, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
opacity=node(preview,unreal.MaterialExpressionConstant,r=1)
assert mel.connect_material_property(opacity,"",unreal.MaterialProperty.MP_OPACITY)
save(preview)
(audit / "FacialAssets.json").write_text(json.dumps({"textures": textures, "materials": [skin.get_path_name(),face.get_path_name(),preview.get_path_name()]}, indent=2), encoding="utf8")
unreal.log("22 facial textures and independent skin/face/preview materials created")
