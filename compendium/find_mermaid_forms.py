#!/usr/bin/env python3
import re
from pathlib import Path
forms = {}
for f in sorted(Path('.').glob('chapter_*.md')) + sorted(Path('.').glob('appendix_*.md')):
    text = Path(f).read_text(encoding='utf-8', errors='ignore')
    for m in re.finditer(r'```.*mermaid.*', text, flags=re.IGNORECASE):
        forms[m.group(0).strip()] = forms.get(m.group(0).strip(), 0) + 1
print('Gefundene Fence-Zeilen:')
for k,v in sorted(forms.items()):
    print(f'{v}x: {k}')
