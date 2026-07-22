from pathlib import Path
p=Path('build/missing_workspace_paths_internal_pruned.txt')
out=Path('build/missing_tests.txt')
if not p.exists():
    print('Source missing:', p)
    raise SystemExit(1)
lines=[l for l in p.read_text(encoding='utf-8').splitlines() if l.strip() and (l.startswith('tests\\') or l.startswith('tests/'))]
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text('\n'.join(lines), encoding='utf-8')
print('Wrote', out, 'entries:', len(lines))
