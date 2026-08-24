import json
import re
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
before_dir = repo / 'ai_working' / 'fp_tuning_before'
after_dir = repo / 'ai_working' / 'fp_tuning_after'

pairs = [
    ('concurrency', 'gap_scan_v3_concurrency_aggregate.json', 'data_race'),
    ('reliability', 'gap_scan_v3_reliability_aggregate.json', 'no_timeout'),
    ('memory', 'gap_scan_v3_memory_aggregate.json', 'pointer_arithmetic'),
    ('security', 'gap_scan_v3_security_aggregate.json', 'null_dereference'),
    ('container', 'gap_scan_v3_container_misuse_aggregate.json', 'iterator_invalidation'),
]

def load_json(path: Path):
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding='utf-8', errors='ignore'))


def type_count(agg: dict, gap_type: str) -> int:
    total = 0
    for payload in agg.values():
        bt = payload.get('by_type', {})
        gt = payload.get('gap_types', {})
        total += int(bt.get(gap_type, 0)) + int(gt.get(gap_type, 0))
    return total


def iter_findings(agg: dict, gap_type: str):
    for payload in agg.values():
        by_file = payload.get('gaps_by_file', {})
        for rel, items in by_file.items():
            for it in items:
                if it.get('type') == gap_type:
                    yield rel, int(it.get('line', 0)), it.get('snippet', '')


def read_context(rel: str, line: int, radius: int = 16):
    p = repo / rel.replace('\\', '/')
    if not p.exists():
        return '', '', ''
    lines = p.read_text(encoding='utf-8', errors='ignore').splitlines()
    i = max(0, line - 1)
    s = max(0, i - radius)
    e = min(len(lines), i + radius + 1)
    prev = '\n'.join(lines[s:i])
    cur = lines[i] if i < len(lines) else ''
    nxt = '\n'.join(lines[i+1:e])
    return prev, cur, nxt


def maybe_fp_reason(gap_type: str, rel: str, line: int):
    prev, cur, nxt = read_context(rel, line)
    ctx = (prev + '\n' + cur + '\n' + nxt)
    lctx = ctx.lower()

    if gap_type == 'data_race':
        if re.search(r'std::(lock_guard|unique_lock|scoped_lock|shared_lock)\b', ctx):
            return 'lock_scope_present'

    if gap_type == 'no_timeout':
        if ('fopen' in lctx or 'open(' in lctx) and any(ext in lctx for ext in ['.pem', '.crt', '.key']):
            return 'local_pem_file_io'
        if any(t in lctx for t in ['expires_after', 'expires_at', 'wait_for', 'timeout', 'deadline']):
            return 'timeout_already_present'

    if gap_type == 'pointer_arithmetic':
        if re.search(r'if\s*\([^)]*(size\s*\(\)|length\s*\(\)|data\.size\s*\(\))[^)]*[<>]=?[^)]*\)', prev):
            return 'entry_size_guard_present'

    if gap_type == 'null_dereference':
        m = re.search(r'\b([A-Za-z_]\w*)\s*->', cur)
        if m:
            var = re.escape(m.group(1))
            checks = [
                rf'if\s*\(\s*{var}\s*!=\s*nullptr\s*\)',
                rf'if\s*\(\s*!\s*{var}\s*\)',
                rf'assert\s*\(\s*{var}\s*\)',
                rf'CHECK\s*\(\s*{var}\s*\)',
            ]
            if any(re.search(p, prev) for p in checks):
                return 'null_check_present'

    if gap_type == 'iterator_invalidation':
        m = re.search(r'auto\s+(\w+)\s*=.*\.(find|begin)\s*\(', cur)
        if m:
            it = re.escape(m.group(1))
            tail = nxt
            if re.search(rf'\b{it}\s*=\s*\w+\.erase\s*\(\s*{it}\s*\)', tail):
                return 'erase_return_rebind'
            if re.search(rf'\w+\.erase\s*\(\s*{it}\s*\)', tail):
                return 'erase_after_find_pattern'

    return None


rows = []
all_candidates = []
for scanner, file_name, gtype in pairs:
    b = load_json(before_dir / file_name)
    a = load_json(after_dir / file_name)
    bc = type_count(b, gtype)
    ac = type_count(a, gtype)
    delta = ac - bc
    pct = 0.0 if bc == 0 else (delta / bc) * 100.0
    rows.append({
        'scanner': scanner,
        'gap_type': gtype,
        'before': bc,
        'after': ac,
        'delta': delta,
        'pct': round(pct, 2),
    })

    for rel, line, snippet in iter_findings(a, gtype):
        reason = maybe_fp_reason(gtype, rel, line)
        if reason:
            all_candidates.append({
                'gap_type': gtype,
                'reason': reason,
                'file': rel,
                'line': line,
                'snippet': snippet,
            })

all_candidates.sort(key=lambda x: (x['gap_type'], x['file'], x['line']))

data = {
    'rows': rows,
    'fp_candidate_count': len(all_candidates),
    'fp_candidates_top30': all_candidates[:30],
}

out_json = repo / 'ai_working' / 'fp_tuning_delta_report.json'
out_md = repo / 'ai_working' / 'fp_tuning_delta_report.md'
out_json.write_text(json.dumps(data, indent=2), encoding='utf-8')

md = []
md.append('# Gap Scanner FP Tuning Delta Report')
md.append('')
md.append('| Scanner | Gap Type | Before | After | Delta | Delta % |')
md.append('|---|---:|---:|---:|---:|---:|')
for r in rows:
    md.append(f"| {r['scanner']} | {r['gap_type']} | {r['before']} | {r['after']} | {r['delta']} | {r['pct']}% |")
md.append('')
md.append(f"False-positive candidates (heuristic, top 30): {len(all_candidates)}")
md.append('')
for c in all_candidates[:30]:
    md.append(f"- [{c['gap_type']}] {c['file']}:{c['line']} ({c['reason']})")
out_md.write_text('\n'.join(md) + '\n', encoding='utf-8')

print(out_json)
print(out_md)
print(f'rows={len(rows)} fp_candidates={len(all_candidates)}')
