from pathlib import Path
import re

root=Path(r'C:\Emerald\pokeemerald-vita-ptbr')
pairs={}
for line in (root/'tools/ui_ptbr_strings.txt').read_text('utf8').splitlines():
    if '|' in line:
        a,b=line.split('|',1)
        pairs[a]=b
rx=re.compile(r'(\bsnprintf\s*\(\s*[^,]+,\s*[^,]+,\s*)("(?:\\.|[^"\\])*")')
fmt_rx=re.compile(r'%(?:[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z|t|j)?[dioxXufFeEgGaAcspn%])')

def compatible(a,b):
    return fmt_rx.findall(a)==fmt_rx.findall(b)
changed=[]
for p in (root/'src/platform/ui').glob('*.c'):
    if p.name=='ui_locale.c': continue
    s=p.read_text('utf8')
    n=[0]
    def translate(m):
        key=m.group(2)[1:-1]
        if key not in pairs or not compatible(key,pairs[key]):
            return m.group(0)
        n[0]+=1
        return m.group(1)+'UiLocale_Translate('+m.group(2)+')'
    updated=rx.sub(translate,s)
    if updated!=s:
        p.write_text(updated,'utf8',newline='\n')
        changed.append((p.name,n[0]))
# One common declaration provided by ui_internal.h for the UI files.
h=root/'src/platform/ui/ui_internal.h'
s=h.read_text('utf8')
if '#include "ui_locale.h"' not in s:
    assert '#include <stdint.h>' in s
    s=s.replace('#include <stdint.h>','#include <stdint.h>\n#include "ui_locale.h"',1)
    h.write_text(s,'utf8',newline='\n')
print('Localized format strings before snprintf:',sum(n for _,n in changed),'in',len(changed),'files')
print('By file:',changed)
