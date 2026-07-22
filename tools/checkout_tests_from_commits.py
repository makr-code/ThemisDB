import subprocess
from pathlib import Path
root = Path('C:/Projects/ThemisDB')
report_in = root / 'build' / 'search_missing_tests_git_history.txt'
report_out = root / 'build' / 'checkout_from_history_report.txt'
if not report_in.exists():
    print('Input report not found:', report_in)
    raise SystemExit(1)
lines = report_in.read_text(encoding='utf-8').splitlines()
entries = []
for ln in lines:
    if '->' in ln:
        left,right = ln.split('->',1)
        path = left.strip()
        right = right.strip()
        commit = right.split('|',1)[0] if '|' in right else right
        entries.append((path,commit))
restored=[]
errors=[]
for path,commit in entries:
    rel = path.replace('\\','/').lstrip('./')
    # skip the Not found entry
    if rel.startswith('Not found') or rel.strip()=='' or rel=='tests/test.cpp':
        errors.append((path,commit,'skipped'))
        continue
    try:
        cp = subprocess.run(['git','show','%s:%s'%(commit,rel)], cwd=str(root), capture_output=True)
    except Exception as e:
        errors.append((path,commit,str(e)))
        continue
    if cp.returncode != 0:
        errors.append((path,commit,cp.stderr.decode('utf-8',errors='ignore').strip() or 'no output'))
        continue
    # write blob
    outp = root / rel
    outp.parent.mkdir(parents=True, exist_ok=True)
    try:
        with outp.open('wb') as f:
            f.write(cp.stdout)
        restored.append((path,commit))
    except Exception as e:
        errors.append((path,commit,str(e)))

with report_out.open('w', encoding='utf-8') as f:
    f.write('Checkout from history report\n')
    f.write('==========================\n\n')
    f.write('Restored:\n')
    for r in restored:
        f.write(f'{r[0]} -> {r[1]}\n')
    f.write('\nErrors or skipped:\n')
    for e in errors:
        f.write(f'{e[0]} -> {e[1]} : {e[2]}\n')

print('Report written to', report_out)
print('Restored:', len(restored), 'Errors/Skipped:', len(errors))
