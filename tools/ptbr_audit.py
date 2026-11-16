from pathlib import Path
import importlib.util
root=Path(r'C:\Emerald\pokeemerald-vita-ptbr')
ref=Path(r'C:\Emerald\emerald-3ds-ptbr-reference')
mod=ref/'tools/localize_portuguese.py'
spec=importlib.util.spec_from_file_location('ptlocalize',str(mod))
pt=importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name]=pt
spec.loader.exec_module(pt)
total=matched=missingfiles=notext=bad=0
byclass={}
examples=[]
for relative in pt.catalogs():
 totalentries=pt.read_catalog(pt.CATALOG/(relative+'.txt'))
 total+=len(totalentries)
 source=root/relative
 if not source.exists():
  missingfiles+=1
  continue
 txt=source.read_text(encoding='utf-8')
 try:
  texts=(pt.inc_texts(txt) if pt.is_script(relative) else pt.c_texts(txt))
 except Exception as exc:
  print('PARSER_ERR',relative,str(exc)[:100])
  continue
 m=0; mismatches=0; absent=0
 for ent in totalentries:
  if ent.label not in texts:
   absent+=1
  elif pt.text_hash(texts[ent.label][2]) == ent.digest:
   m+=1
  else:
   mismatches+=1
 if len(examples)<30 and (mismatches or absent):examples.append((relative,m,mismatches,absent))
 matched+=m;bad+=mismatches;notext+=absent
 cls=relative.split('/')[0]
 old=byclass.get(cls,[0,0])
 old[0]+=m;old[1]+=len(totalentries)
 byclass[cls]=old
print('FILES',len(pt.catalogs()),'ENTRIES',total,'MATCHED',matched,'BAD_HASH',bad,'MISSING_LABELS',notext,'MISSING_FILES',missingfiles,'MATCH_RATIO',round(matched/total*100,2))
print('BY CLASS',byclass)
print('EXAMPLES',examples)
print('PATCH_0057')
p=ref/'patches/pokeemerald/0057-portuguese-tilde-letters.patch'
print(p.read_text(encoding='utf-8').encode('ascii','backslashreplace').decode()[:7000])
