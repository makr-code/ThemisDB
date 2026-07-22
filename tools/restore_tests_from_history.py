import subprocess
from pathlib import Path
root = Path('C:/Projects/ThemisDB')
report_in = root / 'build' / 'search_missing_tests_git_history.txt'
report_out = root / 'build' / 'restore_from_history_report.txt'
if not report_in.exists():
    print('Input report not found:', report_in)
    raise SystemExit(1)
lines = report_in.read_text(encoding='utf-8').splitlines()
# parse lines like: tests\path.cpp -> <hash>|author|date
entries = {}
for ln in lines:
    if '->' in ln:
        left,right = ln.split('->',1)
        path = left.strip()
        right = right.strip()
        if '|' in right:
            commit = right.split('|',1)[0]
        else:
            commit = right
        entries[path]=commit
# perform restore
restored=[]
errors=[]
notfound=[]
for path,commit in entries.items():
    if path.strip()=='' or path.startswith('Search report'):
        continue
    rel = path.replace('\\','/')
    full = root / rel
    # skip if already exists
    if full.exists():
        restored.append((path,'already_exists'))
        continue
    cmd = ['git','restore','--source=%s'%commit,'--',rel]
    cp = subprocess.run(cmd, cwd=str(root), capture_output=True, text=True)
    if cp.returncode==0:
        restored.append((path,commit))
    else:
        errors.append((path,commit,cp.stderr.strip() or cp.stdout.strip()))

with report_out.open('w', encoding='utf-8') as f:
    f.write('Restore from history report\n')
    f.write('==========================\n\n')
    f.write('Restored or already present:\n')
    for r in restored:
        f.write(f'{r[0]} -> {r[1]}\n')
    f.write('\nErrors:\n')
    for e in errors:
        f.write(f'{e[0]} -> {e[1]} : {e[2]}\n')

print('Report written to', report_out)
print('Restored:', len(restored), 'Errors:', len(errors))
