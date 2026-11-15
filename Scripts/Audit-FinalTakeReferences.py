"""Conservative UE Asset Registry audit. Candidates are evidence, not deletion approval."""
import json,pathlib,re,unreal
project=pathlib.Path(unreal.Paths.project_dir())
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game'],force_rescan=False)
options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
assets=registry.get_assets_by_path('/Game',recursive=True,include_only_on_disk_assets=True)
packages={str(asset.package_name):asset for asset in assets}
roots={'/Game/OneManStudio/Maps/Lvl_Bank','/Game/OneManStudio/Maps/Lvl_Cinema'}
dynamic=set()
for folder in ('Source','Scripts','Config'):
    for file in (project/folder).rglob('*'):
        if file.suffix.lower() not in ('.h','.cpp','.py','.ps1','.ini'):continue
        text=file.read_text(encoding='utf-8-sig',errors='replace')
        for path in re.findall(r'/Game/[A-Za-z0-9_/]+',text):
            if path in packages:dynamic.add(path)
            else:dynamic.update(p for p in packages if p.startswith(path.rstrip('/')+'/'))
roots|=dynamic
# Approved art and upcoming artwork stay protected even when not currently equipped.
protected={p for p in packages if p.startswith('/Game/OneManStudio/')}
all_dependencies={p:[str(d) for d in registry.get_dependencies(p,options)] for p in packages}
dependencies={p:[d for d in ds if d.startswith('/Game/')] for p,ds in all_dependencies.items()}
reachable=set();queue=list(roots)
while queue:
    path=queue.pop()
    if path in reachable:continue
    reachable.add(path);queue.extend(d for d in dependencies.get(path,[]) if d not in reachable)
rows=[]
for path,asset in sorted(packages.items()):
    relative=path.removeprefix('/Game/')
    files=[project/'Content'/(relative+extension) for extension in ('.uasset','.umap')]
    file=next((p for p in files if p.exists()),None)
    candidate=path not in reachable and path not in protected
    imports=[]
    if candidate:
        obj=asset.get_asset()
        try:
            data=obj.get_editor_property('asset_import_data')
            if data:imports=list(data.extract_filenames())
        except Exception:pass
    rows.append({'package':path,'class':str(asset.asset_class_path),'bytes':file.stat().st_size if file else 0,
      'dependencies':dependencies[path],'referencers':[str(p) for p in registry.get_referencers(path,options)],
      'external_dependencies':[d for d in all_dependencies[path] if not d.startswith('/Game/')],
      'import_sources':imports,
      'runtime_or_code_reachable':path in reachable,'approved_art_protected':path in protected,
      'candidate_only':path not in reachable and path not in protected})
result={'asset_count':len(rows),'roots':sorted(roots),'dynamic_code_script_config_roots':sorted(dynamic),
 'assets':rows,'candidate_count':sum(r['candidate_only'] for r in rows),'deleted':[],
 'rules':'Hard and soft package/management references plus native/script/config roots. Approved art protected; absence from this graph alone never proves unused. Source import dependencies and localization require separate checks before deletion.'}
phase=re.search(r'-AuditPhase=([A-Za-z0-9]+)',unreal.SystemLibrary.get_command_line())
output=project/('GameJam 2026/Optimization/AssetReferences'+(phase.group(1) if phase else 'Current')+'.json');output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(result,indent=2),encoding='utf8')
unreal.log('FINAL TAKE reference audit: %d assets, %d candidates, no deletions'%(len(rows),result['candidate_count']))
