#!/usr/bin/env python3
import re
from pathlib import Path
root = Path('.github/workflows')
third_party = re.compile(r'^[0-9a-f]{40}$')
allowed_unpinned = ("actions/","github/")
issues=[]
for p in sorted(root.glob('*.yml')):
    text = p.read_text(errors='replace')
    for i,line in enumerate(text.splitlines(),1):
        s=line.strip()
        if s.startswith('uses:'):
            spec=s.split('uses:',1)[1].strip()
            if '@' not in spec:
                issues.append((p,i,'missing-ref',spec))
                continue
            action,ref=spec.split('@',1)
            action=action.strip(); ref=ref.strip()
            if action.startswith('./') or any(action.startswith(a) for a in allowed_unpinned):
                continue
            if not third_party.fullmatch(ref):
                issues.append((p,i,'unpinned',spec))
        if 'continue-on-error:' in s:
            if 'true' in s:
                issues.append((p,i,'continue-on-error',s.strip()))
print('Found',len(issues),'issues')
for p,i,typ,txt in issues:
    print(f"{p}:{i}:{typ}:{txt}")
