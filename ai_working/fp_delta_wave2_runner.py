import json
import importlib.util
from pathlib import Path

repo = Path(__file__).resolve().parents[1]


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, str(path))
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def collect_cpp_files(root: Path):
    exts = {'.cpp', '.cc', '.c', '.h', '.hpp'}
    files = []
    for base in (root / 'src', root / 'include'):
        if not base.exists():
            continue
        for p in base.rglob('*'):
            if p.is_file() and p.suffix.lower() in exts:
                files.append(p)
    return files


def run_concurrency(mod, files):
    scanner = mod.ConcurrencyGapScanner(str(repo))
    out = []
    for f in files:
        out.extend(scanner.scan_file(f))
    return [g for g in out if g.gap_type.value == 'deadlock_risk']


def run_reliability(mod, files):
    scanner = mod.ReliabilityGapScanner(str(repo))
    out = []
    for f in files:
        out.extend(scanner.scan_file(f))
    return [g for g in out if g.gap_type.value == 'uncaught_exception']


def run_security(mod, files):
    scanner = mod.SecurityGapScanner(str(repo))
    out = []
    for f in files:
        out.extend(scanner.scan_file(f))
    return [g for g in out if g.gap_type.value == 'null_dereference']


def run_phase8(mod, files):
    scanner = mod.PerformanceAntiPatternsScan(str(repo))
    gaps = scanner.scan_files(files)
    return [g for g in gaps if g.get('pattern') == 'lock_in_loop']


def as_rows(items, style):
    rows = []
    if style == 'obj':
        for g in items:
            rows.append({
                'file': g.file_path,
                'line': g.line_num,
                'type': g.gap_type.value,
                'snippet': g.snippet,
            })
    else:
        for g in items:
            rows.append({
                'file': g.get('file', ''),
                'line': int(g.get('line', 0) or 0),
                'type': g.get('pattern', ''),
                'snippet': g.get('context', ''),
            })
    return rows


def keyset(rows):
    return {f"{r['file']}:{r['line']}:{r['type']}" for r in rows}


files = collect_cpp_files(repo)

specs = [
    {
        'name': 'deadlock_risk',
        'old': repo / 'ai_working' / '_tmp_old_gap_scanner_v3_concurrency.py',
        'new': repo / 'tools' / 'gap_scanner_v3_concurrency.py',
        'runner': run_concurrency,
        'style': 'obj',
    },
    {
        'name': 'lock_in_loop',
        'old': repo / 'ai_working' / '_tmp_old_gap_scanner_v3_phase8_performance_patterns.py',
        'new': repo / 'tools' / 'gap_scanner_v3_phase8_performance_patterns.py',
        'runner': run_phase8,
        'style': 'dict',
    },
    {
        'name': 'null_dereference',
        'old': repo / 'ai_working' / '_tmp_old_gap_scanner_v3_security.py',
        'new': repo / 'tools' / 'gap_scanner_v3_security.py',
        'runner': run_security,
        'style': 'obj',
    },
    {
        'name': 'uncaught_exception',
        'old': repo / 'ai_working' / '_tmp_old_gap_scanner_v3_reliability.py',
        'new': repo / 'tools' / 'gap_scanner_v3_reliability.py',
        'runner': run_reliability,
        'style': 'obj',
    },
]

summary = []
remaining = {}
for s in specs:
    old_mod = load_module(s['old'], f"old_{s['name']}")
    new_mod = load_module(s['new'], f"new_{s['name']}")

    old_rows = as_rows(s['runner'](old_mod, files), s['style'])
    new_rows = as_rows(s['runner'](new_mod, files), s['style'])

    old_n = len(old_rows)
    new_n = len(new_rows)
    delta = new_n - old_n
    pct = 0.0 if old_n == 0 else round((delta / old_n) * 100.0, 2)

    old_keys = keyset(old_rows)
    new_keys = keyset(new_rows)

    removed = sorted(list(old_keys - new_keys))
    added = sorted(list(new_keys - old_keys))

    summary.append({
        'gap_type': s['name'],
        'before': old_n,
        'after': new_n,
        'delta': delta,
        'delta_pct': pct,
        'removed_count': len(removed),
        'added_count': len(added),
    })

    remaining[s['name']] = new_rows[:25]

report = {
    'scope': 'full src+include scan',
    'file_count': len(files),
    'summary': summary,
    'remaining_top25': remaining,
}

json_out = repo / 'ai_working' / 'fp_delta_wave2_report.json'
md_out = repo / 'ai_working' / 'fp_delta_wave2_report.md'
json_out.write_text(json.dumps(report, indent=2), encoding='utf-8')

lines = []
lines.append('# FP Delta Report (Wave 2)')
lines.append('')
lines.append(f"Scope: full src+include scan ({len(files)} files)")
lines.append('')
lines.append('| Gap Type | Before | After | Delta | Delta % | Removed | Added |')
lines.append('|---|---:|---:|---:|---:|---:|---:|')
for r in summary:
    lines.append(f"| {r['gap_type']} | {r['before']} | {r['after']} | {r['delta']} | {r['delta_pct']}% | {r['removed_count']} | {r['added_count']} |")
lines.append('')
for gtype, items in remaining.items():
    lines.append(f"## Remaining {gtype} (Top 25)")
    for it in items:
        lines.append(f"- {it['file']}:{it['line']}")
    lines.append('')
md_out.write_text('\n'.join(lines), encoding='utf-8')

print(json_out)
print(md_out)
print(f"files={len(files)}")
for r in summary:
    print(f"{r['gap_type']}: before={r['before']} after={r['after']} delta={r['delta']}")
