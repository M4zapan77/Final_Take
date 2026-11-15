"""Fill Unreal-exported PO files from reviewed ES translations. No third-party packages."""
from pathlib import Path
import json,ast,re
root=Path(__file__).resolve().parents[1]
data=json.loads((root/'Config/Localization/FinalTakeTranslations.json').read_text(encoding='utf-8-sig'))
directory=root/'Content/Localization/FinalTake'
report={'cultures':['en','es'],'native':'en','translated':0,'missing':[]}
def parse(block,field):
    lines=block.splitlines();found=False;parts=[]
    for line in lines:
        if line.startswith(field+' '):found=True;parts=[line[len(field)+1:]]
        elif found and line.startswith('"'):parts.append(line)
        elif found:break
    return ''.join(ast.literal_eval(x) for x in parts) if parts else ''
neutral=lambda s: not any(c.isalpha() for c in re.sub(r'\{[^}]*\}','',s)) or s in ('FINAL TAKE','English','Español','Language / Idioma')
for culture in ('en','es'):
    file=directory/culture/'FinalTake.po';blocks=file.read_text(encoding='utf-8-sig').split('\n\n');updated=[]
    for block in blocks:
        source=parse(block,'msgid');context=parse(block,'msgctxt')
        if not source:updated.append(block);continue
        value=source if culture=='en' else data.get(context,data.get(source,source))
        if culture=='es':
            if context not in data and source not in data and not neutral(source):report['missing'].append({'context':context,'source':source})
            else:report['translated']+=1
        quoted=json.dumps(value,ensure_ascii=False)
        block=re.sub(r'^msgstr .*?(?=\n[^"\n]|\Z)',lambda _: 'msgstr '+quoted,block,flags=re.M|re.S)
        updated.append(block)
    file.write_text('\n\n'.join(updated),encoding='utf8',newline='\n')
(root/'Saved/Automation/Localization').mkdir(parents=True,exist_ok=True)
(root/'Saved/Automation/Localization/coverage.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
print(json.dumps(report,ensure_ascii=False,indent=2))
