import unreal,json,pathlib
mel=unreal.MaterialEditingLibrary
m=unreal.load_asset('/Game/OneManStudio/Environment/Cinema/ProductionArt/Corrections/M_WarmCinemaRed')
n=mel.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
data={'base_node':str(n),'output':mel.get_material_property_input_node_output_name(m,unreal.MaterialProperty.MP_BASE_COLOR),'color':str(mel.get_material_default_vector_parameter_value(m,'Color'))}
pathlib.Path(unreal.Paths.project_saved_dir(),'FinalCorrections/CinemaMaterialInspection.json').write_text(json.dumps(data,indent=2));unreal.log(str(data))
