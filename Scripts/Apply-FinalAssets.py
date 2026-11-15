"""Run authored corrections/imports sequentially using the existing project."""
import pathlib,runpy,unreal
folder=pathlib.Path(unreal.Paths.project_dir()).resolve()/'Scripts'
for name in ('Correct-CosmeticAttachments.py','Correct-ClearGlasses.py','Correct-ProductionEnvironments.py','Import-FinalAudio.py'):
    unreal.log('Final corrections: '+name)
    runpy.run_path(str(folder/name),run_name='__main__')
