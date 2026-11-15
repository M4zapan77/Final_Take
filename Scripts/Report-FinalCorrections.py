"""Audit files and supplied originals; never modifies Content."""
import pathlib,json,hashlib,csv
P=pathlib.Path(__file__).resolve().parent.parent;A=P/'Saved/FinalCorrections';before=json.loads((A/'Before.json').read_text(encoding='utf8'));baseline={e['path']:e['sha256'] for e in before}
def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()
changed=[];missing=[];new=[]
for rel,old in baseline.items():
    path=P/rel
    if not path.exists():missing.append(rel)
    elif sha(path)!=old:changed.append(rel)
for folder in ('Source','Scripts','Docs','Config','Content'):
    for path in (P/folder).rglob('*'):
        if path.is_file() and str(path.relative_to(P)) not in baseline:new.append(str(path.relative_to(P)))
allowed={r'Content\OneManStudio\Maps\Lvl_Bank.umap',r'Content\OneManStudio\Maps\Lvl_Cinema.umap',r'Content\OneManStudio\Cosmetics\ProductionArt\DA_ProductionCosmetics.uasset'}
unexpected=[p for p in changed if p.startswith('Content\\') and p not in allowed]
audio=json.loads((A/'AudioInventory.json').read_text(encoding='utf8'));bad_audio=[]
for item in audio:
    if sha(P/'SourceArt/FinalAudio20261114/Original'/item['source'])!=item['sha256']:bad_audio.append(item['source'])
receipt=json.loads((A/'AudioImportReceipt.json').read_text(encoding='utf8'));rows=[{'name':key,**value} for key,value in receipt.items() if not key.startswith('_')]
vault=json.loads((A/'VaultSourceInventory.json').read_text(encoding='utf8'));bad_vault=[]
for item in vault['files']:
    if sha(P/'SourceArt/FinalCorrections20261114/Vault'/item['path'])!=item['sha256']:bad_vault.append(item['path'])
with (A/'AudioAssignments.csv').open('w',newline='',encoding='utf-8-sig') as f:
    w=csv.DictWriter(f,fieldnames=rows[0].keys());w.writeheader();w.writerows(rows)
result={'changed_original_files':changed,'missing_original_files':missing,'new_files':new,'unexpected_original_content_changes':unexpected,'changed_source_audio':bad_audio,'changed_source_vault':bad_vault,'audio_assets':len(rows),'protection_passed':not missing and not unexpected and not bad_audio and not bad_vault}
(A/'FilesChanged.json').write_text(json.dumps(result,indent=2),encoding='utf8')
print(json.dumps({k:v for k,v in result.items() if k not in ('changed_original_files','new_files')},indent=2))
if not result['protection_passed']:raise RuntimeError('Original-file audit failed')
