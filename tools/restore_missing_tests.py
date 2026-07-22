import subprocess
from pathlib import Path
root = Path('C:/Projects/ThemisDB')
missing = (root / 'build' / 'missing_tests.txt')
if not missing.exists():
    print('Missing source list not found:', missing)
    raise SystemExit(1)
lines = [l.strip() for l in missing.read_text(encoding='utf-8').splitlines() if l.strip()]
# keep only concrete files (no glob *)
files = [l for l in lines if '*' not in l and '?' not in l]
restored = []
not_found = []
errors = []
for rel in files:
    rel_norm = rel.replace('\\','/').lstrip('./')
    p = root / rel.replace('\\','/')
    if p.exists():
        restored.append(rel)
        continue
    # check git ls-tree
    try:
        cp = subprocess.run(['git','ls-tree','-r','--name-only','HEAD','--', rel_norm], cwd=str(root), capture_output=True, text=True)
    except Exception as e:
        errors.append((rel, str(e)))
        continue
    if cp.returncode != 0 or not cp.stdout.strip():
        not_found.append(rel)
        continue
    # file exists in HEAD, restore
    try:
        cp2 = subprocess.run(['git','restore','--source=HEAD','--', rel_norm], cwd=str(root), capture_output=True, text=True)
    except Exception as e:
        errors.append((rel, str(e)))
        continue
    if cp2.returncode == 0:
        restored.append(rel)
    else:
        errors.append((rel, cp2.stderr.strip() or cp2.stdout.strip()))

out = root / 'build' / 'restore_missing_tests_report.txt'
with out.open('w', encoding='utf-8') as f:
    f.write('Restored files:\n')
    for r in restored:
        f.write(r+'\n')
    f.write('\nNot found in HEAD:\n')
    for n in not_found:
        f.write(n+'\n')
    f.write('\nErrors:\n')
    for e in errors:
        f.write(f"{e[0]} -> {e[1]}\n")

print('Report written to', out)
print('Restored:', len(restored), 'Not found:', len(not_found), 'Errors:', len(errors))
