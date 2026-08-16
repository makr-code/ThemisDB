#!/usr/bin/env python3
import re
from pathlib import Path
p=Path('submodule_report.txt')
text=p.read_text(encoding='utf-8') if p.exists() else ''
blocks=[b.strip() for b in re.split(r'\n---\s+', text) if b.strip()]
issues=[]
for b in blocks:
    path_line=b.splitlines()[0] if b.splitlines() else ''
    path=path_line.strip()
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

# write summary
out=Path('submodule_summary.txt')
if not issues:
    out.write_text('No issues detected in submodules.\n',encoding='utf-8')
else:
    lines=['Submodule issues detected:']
    for path,reason in issues:
        lines.append(f'- {path or "<unknown path>"}: {reason}')
    out.write_text('\n'.join(lines)+"\n",encoding='utf-8')
print(out.read_text(encoding='utf-8'))
