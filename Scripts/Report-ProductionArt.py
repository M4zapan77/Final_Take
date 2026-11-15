"""Filesystem audit/inventory. Read-only for Content; reports are written to Saved."""
import csv,hashlib,json,pathlib
P=pathlib.Path(__file__).resolve().parent.parent;A=P/'Saved/ProductionArt'
receipt=json.loads((A/'ImportReceipt.json').read_text(encoding='utf8'))
optimization={e['key']:e for e in json.loads((A/'PropOptimization.json').read_text(encoding='utf8'))}
rows=[]
for key,e in receipt.items():
    if key.startswith('_'):continue
    row={'key':key,'category':e['kind'],'mesh':e.get('mesh') or 'Existing SK_Base3 / material variant',
         'material':e['material'],'textures':len(e['textures']),'runtime_triangles':e['runtime_triangles'],
         'source_triangles':optimization.get(key,{}).get('source_triangles',e.get('source_triangles')),
         'source_fbx':e['source'],'imported_fbx':e.get('imported_source',e['source'])}
    rows.append(row)
rows.sort(key=lambda r:(r['category'],r['key']))
with (A/'AssetInventory.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=rows[0].keys());writer.writeheader();writer.writerows(rows)
before=json.loads((A/'ProtectedBefore.json').read_text(encoding='utf8'))
baseline={e['path']:e for e in before};changed=[];missing=[];new=[]
for rel,e in baseline.items():
    path=P/rel
    if not path.exists():missing.append(rel);continue
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    if digest!=e['sha256']:changed.append(rel)
for root in ('Source','Config','Content','Scripts','Docs'):
    for file in (P/root).rglob('*'):
        if file.is_file():
            rel=str(file.relative_to(P))
            if rel not in baseline:new.append(rel)
allowed={'Content\\OneManStudio\\Maps\\Lvl_Bank.umap',
         'Content\\OneManStudio\\Characters\\Base3\\Customization\\DA_Base3Character.uasset'}
unexpected=[p for p in changed if p.startswith('Content\\') and p not in allowed]
result={'entries':len(rows),'new_static_meshes':sum(bool(e.get('mesh')) for k,e in receipt.items() if not k.startswith('_')),
        'imported_source_textures':sum(r['textures'] for r in rows),'changed_original_files':changed,
        'missing_original_files':missing,'new_files':new,'unexpected_original_content_changes':unexpected,
        'protection_passed':not missing and not unexpected}
(A/'FilesChanged.json').write_text(json.dumps(result,indent=2),encoding='utf8')
if missing or unexpected:raise RuntimeError('Protected content audit failed: '+str(missing+unexpected))
print(json.dumps({k:v for k,v in result.items() if k not in ('new_files','changed_original_files')},indent=2))
