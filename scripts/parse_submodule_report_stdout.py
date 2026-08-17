#!/usr/bin/env python3
import re
from pathlib import Path
p=Path('submodule_report.txt')
if not p.exists():
    print('No submodule_report.txt found')
    raise SystemExit(1)
text=p.read_text(encoding='utf-8')
blocks=[b.strip() for b in re.split(r'\n---\s+', text) if b.strip()]
issues=[]
for b in blocks:
    first_line=b.splitlines()[0] if b.splitlines() else ''
    path=first_line.strip()
    if 'MISSING PATH' in b:
        issues.append((path,'MISSING PATH'))
        continue
    if 'commit-on-origin: NO' in b:
        issues.append((path,'commit not present on origin'))
    m=re.search(r'porcelain:\s*\n([\s\S]*?)(?:\nbranch:|\nremote:|$)', b)
    if m:
        porcelain=m.group(1).strip()
        if porcelain:
            issues.append((path,'uncommitted changes'))
if not issues:
    print('No issues detected in submodules.')
else:
    print('Submodule issues detected:')
    for path,reason in issues:
        print(f'- {path or "<unknown path>":40}: {reason}')
