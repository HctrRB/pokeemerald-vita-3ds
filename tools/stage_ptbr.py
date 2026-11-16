from __future__ import annotations
from pathlib import Path
import shutil,re,importlib.util,sys

root=Path(r'C:\Emerald\pokeemerald-vita-ptbr')
reference=Path(r'C:\Emerald\emerald-3ds-ptbr-reference')
target_catalog=root/'tools/locales/pt_br'
target_catalog.mkdir(parents=True,exist_ok=True)
for filename in ['localize_portuguese.py','easy_chat_order.py']:
    shutil.copy2(reference/'tools'/filename,root/'tools'/filename)
sys.path.insert(0,str(root/'tools'))
import localize_portuguese as pt
pt.ROOT=root
pt.CATALOG=target_catalog
total=applied=skipped=0
miss=[]
for catalog in sorted((reference/'tools/locales/pt_br').rglob('*.txt')):
    relcat=catalog.relative_to(reference/'tools/locales/pt_br')
    relsrc=str(relcat.with_suffix('')).replace('\\','/')
    if relcat.parts[0]=='referencia':
        continue
    src=root/relsrc
    if not src.is_file():
        miss.append(f'{relsrc}: missing file')
        continue
    contents=src.read_text(encoding='utf-8')
    index=(pt.inc_texts(contents) if pt.is_script(relsrc) else pt.c_texts(contents))
    entries=[]
    for entry in pt.read_catalog(catalog):
        total+=1
        existing=index.get(entry.label)
        if existing is None or pt.text_hash(existing[2]) != entry.digest:
            skipped+=1
            miss.append(f'{relsrc}: {entry.label}: missing or differing source')
            continue
        header=f'[{entry.label}] {entry.digest}'
        if entry.width is not None:
            header+=f' width={entry.width}'
        entries.append(header+'\n'+entry.text+'\n')
        applied+=1
    if entries:
        dest=target_catalog/relcat
        dest.parent.mkdir(parents=True,exist_ok=True)
        dest.write_text('\n'.join(entries),encoding='utf-8')
(target_catalog/'ADAPTATION_REPORT.txt').write_text(
    'Portuguese translation data adapted from ZallaxDev/emerald-3ds tools/locales/pt_br.\n'
    'This data is not extracted from the PK EMR (PT-BR).gba fan translation; wording may differ.\n'
    f'Catalog entries: {applied} matched out of {total}; {skipped} unmapped.\n'
    + '\n'.join(miss)+'\n',encoding='utf-8')
print('CATALOG',applied,'/',total,'SKIPPED',skipped,'FILES',len(list(target_catalog.rglob('*.txt'))))

p=root/'charmap.txt'
s=p.read_text(encoding='utf-8')
if "'ã'         = 31" not in s:
    anchor="'+'         = 2E"
    assert s.count(anchor)==1
    s=s.replace(anchor,anchor+"\n@ Extra Portuguese letters (ZallaxDev localization)\n'Ã'         = 2F\n'Õ'         = 30\n'ã'         = 31\n'õ'         = 32")
    p.write_text(s,encoding='utf-8')

p=root/'include/constants/characters.h'
s=p.read_text(encoding='utf-8')
if 'CHAR_A_TILDE' not in s:
    needle='#define CHAR_PLUS              0x2E'
    assert s.count(needle)==1
    s=s.replace(needle,needle+'\n#define CHAR_A_TILDE           0x2F\n#define CHAR_O_TILDE           0x30\n#define CHAR_a_TILDE           0x31\n#define CHAR_o_TILDE           0x32')
    p.write_text(s,encoding='utf-8')

p=root/'src/fonts.c'
s=p.read_text(encoding='utf-8')
for name in ('SmallNarrow','Small','Narrow','Short','Normal'):
    pattern=r'(const u8 gFont'+name+r'LatinGlyphWidths\[\]\s*=\s*\{)(.*?)(\};)'
    m=re.search(pattern,s,re.S)
    assert m
    body=m.group(2)
    matches=list(re.finditer(r'\b\d+\b',body))
    assert len(matches)==512, (name,len(matches))
    vals=[int(t.group()) for t in matches]
    destvalues=dict(zip((47,48,49,50),(vals[0xBB],vals[0xC9],vals[0xD5],vals[0xE3])))
    for idx in (50,49,48,47):
        tok=matches[idx]
        body=body[:tok.start()]+str(destvalues[idx])+body[tok.end():]
    s=s[:m.start(2)]+body+s[m.end(2):]
    print('WIDTHS',name,destvalues)
p.write_text(s,encoding='utf-8')
print('STAGED')
