"""Run after importing all 45 pack entries, in a rendering-enabled editor."""
import pathlib,runpy,unreal
scripts=pathlib.Path(unreal.Paths.project_dir()).resolve()/'Scripts'
for filename in ('Import-ProductionArt.py','Configure-ProductionCosmetics.py','Build-ProductionSets.py','Inspect-ProductionScene.py'):
    unreal.log('Production art finalization: '+filename)
    runpy.run_path(str(scripts/filename),run_name='__main__')
