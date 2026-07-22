import re
from pathlib import Path
root = Path(r"C:\Projects\ThemisDB")
cmake_files = list(root.rglob('CMakeLists.txt')) + list(root.rglob('*.cmake'))
pattern = re.compile(r'(["\'])(?P<path>[^"\']*(?:src|tests)[^"\']*\.(?:c|cc|cpp|cxx|cu|h|hpp))\1', re.IGNORECASE)
missing = set()
seen = set()
for cm in cmake_files:
    try:
        text = cm.read_text(encoding='utf-8', errors='ignore')
    except Exception:
        continue
    for m in pattern.finditer(text):
        raw = m.group('path')
        # resolve variables
        p = raw
        if '${THEMIS_ROOT_DIR}' in p:
            p = p.replace('${THEMIS_ROOT_DIR}', str(root))
        if '${CMAKE_SOURCE_DIR}' in p:
            p = p.replace('${CMAKE_SOURCE_DIR}', str(root))
        if '${CMAKE_CURRENT_SOURCE_DIR}' in p or '${CMAKE_CURRENT_LIST_DIR}' in p:
            p = p.replace('${CMAKE_CURRENT_SOURCE_DIR}', str(cm.parent))
            p = p.replace('${CMAKE_CURRENT_LIST_DIR}', str(cm.parent))
        # normalize ../ occurrences relative to cm
        if p.startswith('..') or p.startswith('./') or p.startswith('\..'):
            p = str((cm.parent / p).resolve())
        # if becomes relative path like src/..., prefix root
        if p.startswith('src') or p.startswith('tests'):
            p = str((root / p).resolve())
        # if still contains ${...}, skip
        if '${' in p:
            continue
        try:
            pth = Path(p)
        except Exception:
            continue
        # only care about project-owned paths
        try:
            rel = pth.relative_to(root)
        except Exception:
            continue
        if ('src' in rel.parts) or ('tests' in rel.parts):
            if not pth.exists():
                missing.add(str(rel))

out = root / 'build' / 'missing_workspace_paths_relative.txt'
out.parent.mkdir(parents=True, exist_ok=True)
with out.open('w', encoding='utf-8') as f:
    for p in sorted(missing):
        f.write(p + '\n')

print('Wrote', out, 'entries:', len(missing))
