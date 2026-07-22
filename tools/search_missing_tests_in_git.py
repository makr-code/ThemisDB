import subprocess
from pathlib import Path
root = Path('C:/Projects/ThemisDB')
missing = (root / 'build' / 'missing_tests.txt')
report = (root / 'build' / 'search_missing_tests_git_history.txt')
if not missing.exists():
    print('Missing source list not found:', missing)
    raise SystemExit(1)
lines = [l.strip() for l in missing.read_text(encoding='utf-8').splitlines() if l.strip()]
files = [l for l in lines if '*' not in l and '?' not in l]
found = {}
notfound = []
for rel in files:
    rel_norm = rel.replace('\\','/').lstrip('./')
    try:
        # git log --all --pretty=format:%H -- path
        cp = subprocess.run(['git','log','--all','--pretty=format:%H','--', rel_norm], cwd=str(root), capture_output=True, text=True)
    except Exception as e:
        found[rel] = ('error', str(e))
        continue
    if cp.returncode != 0 or not cp.stdout.strip():
        notfound.append(rel)
        continue
    # take first (most recent) commit hash
    commit = cp.stdout.splitlines()[0]
    # get commit info
    cp2 = subprocess.run(['git','show','-s','--format=%H|%an|%ad','--date=iso', commit], cwd=str(root), capture_output=True, text=True)
    info = cp2.stdout.strip() if cp2.returncode==0 else commit
    found[rel] = info

with report.open('w', encoding='utf-8') as f:
    f.write('Search report for missing tests\n')
    f.write('================================\n\n')
    f.write(f'Total files checked: {len(files)}\n')
    f.write(f'Found in history: {len(found)}\n')
    f.write(f'Not found: {len(notfound)}\n\n')
    if found:
        f.write('Found entries:\n')
        for k,v in found.items():
            f.write(f'{k} -> {v}\n')
        f.write('\n')
    if notfound:
        f.write('Not found in any ref:\n')
        for n in notfound:
            f.write(n+'\n')

print('Report written to', report)
print('Found:', len(found), 'Not found:', len(notfound))
